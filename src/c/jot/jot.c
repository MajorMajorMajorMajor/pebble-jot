#include <pebble.h>
#include "jot/jot.h"
#include "jot/store.h"
#include "jot/list_focus.h"
#include "jot/list_touch.h"
#include "jot/backup.h"
#include "jot/reader.h"
#include "ui/alert.h"

#define AUTOSAVE_DELAY_MS 1800
#define LIST_ROWS JOT_LIST_TOUCH_ROWS
#define LIST_HEADER_H JOT_LIST_TOUCH_HEADER_H
#define LIST_FOOTER_H JOT_LIST_TOUCH_FOOTER_H
#define LIST_NEW_PILL_X_RIGHT 99
#define LIST_NEW_PILL_W 91
#define LIST_NEW_PILL_Y 8
#define LIST_NEW_PILL_H 25

static Window *s_list_window;
static Layer *s_list_layer;
static Layer *s_cards_layer;
// Pixel position of the clipped list viewport. Only touched while list open.
static int s_list_scroll_px;
static AppTimer *s_save_timer;
static int s_selected_rank;
// A finger drag puts the list in touch-browsing mode: no note or NEW pill
// is button-selected until an explicit hardware navigation action.
static bool s_list_focus_valid = true;
static bool s_confirm_delete;
static bool s_storage_error;
static bool s_input_full;
// -1 means quick capture; otherwise the saved note opened from the reader.
static int s_editing_slot = -1;

static void prv_cancel_autosave(void) {
  if (s_save_timer) {
    app_timer_cancel(s_save_timer);
    s_save_timer = NULL;
  }
}

static bool prv_flush(void) {
  prv_cancel_autosave();
  bool ok = jot_store_flush();
  s_storage_error = !ok;
  return ok;
}

// Render the actual note in the 40px editing viewport.
void jot_refresh_editor(void) {
  alert_set_editor(jot_store_text(), jot_store_cursor(),
                   jot_store_dirty(), s_storage_error, s_input_full);
}

static void prv_autosave(void *context) {
  s_save_timer = NULL;
  s_storage_error = !jot_store_flush();
  jot_refresh_editor();
}

static void prv_schedule_autosave(void) {
  prv_cancel_autosave();
  s_save_timer = app_timer_register(AUTOSAVE_DELAY_MS, prv_autosave, NULL);
}

void jot_input(const char *binding) {
  if ((s_list_window && window_stack_get_top_window() == s_list_window) ||
      jot_reader_is_open()) return;
  if (!binding || !*binding) return;
  bool ok = jot_store_input(binding);
  s_input_full = !ok;
  if (ok && jot_store_dirty()) prv_schedule_autosave();
  jot_refresh_editor();
}

void jot_delete_forward(void) {
  if ((s_list_window && window_stack_get_top_window() == s_list_window) ||
      jot_reader_is_open()) return;
  const bool changed = jot_store_delete_forward();
  // Deletion is never a failed insertion due to a full buffer.
  s_input_full = false;
  if (changed) prv_schedule_autosave();
  jot_refresh_editor();
}

void jot_save_now(void) {
  prv_flush();
  jot_refresh_editor();
}

void jot_new_note(void) {
  if (prv_flush() && jot_store_new()) {
    s_input_full = false;
    s_storage_error = false;
  }
  jot_refresh_editor();
}

static void prv_format_time(time_t when, char *out, size_t cap) {
  // Show today's notes by time, older notes by month/day. Copy today
  // before the second localtime() call; both calls share a static tm.
  const time_t now = time(NULL);
  const struct tm *current = localtime(&now);
  if (!current) { snprintf(out, cap, "--:--"); return; }
  const int today_year = current->tm_year;
  const int today_yday = current->tm_yday;
  const struct tm *note = localtime(&when);
  if (!note) { snprintf(out, cap, "--:--"); return; }
  if (note->tm_year == today_year && note->tm_yday == today_yday)
    snprintf(out, cap, "%02d:%02d", note->tm_hour, note->tm_min);
  else
    snprintf(out, cap, "%02d/%02d", note->tm_mon + 1, note->tm_mday);
}

// The notes list uses the same paper / yellow / charcoal visual language as
// the phone backup viewer. The editor and keyboard retain their existing sizes.
static void prv_draw_pencil(GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorFromRGB(41, 40, 43));
  graphics_context_set_stroke_width(ctx, 3);
  graphics_draw_line(ctx, GPoint(10, 29), GPoint(24, 15));
  graphics_draw_line(ctx, GPoint(14, 33), GPoint(28, 19));
  graphics_draw_line(ctx, GPoint(10, 29), GPoint(14, 33));
  graphics_draw_line(ctx, GPoint(24, 15), GPoint(28, 19));
  graphics_draw_line(ctx, GPoint(8, 35), GPoint(14, 33));
}

// The cards have their own clipped viewport so drag scrolling can move them
// a pixel at a time without covering the fixed notebook header and footer.
static void prv_cards_draw(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  const int width = bounds.size.w;
  const int height = bounds.size.h;
  const int row_h = height / LIST_ROWS;
  const int count = jot_store_count();
  const GColor ink = GColorFromRGB(41, 40, 43);
  const GColor yellow = GColorFromRGB(255, 209, 88);
  const GColor paper = GColorFromRGB(249, 242, 219);
  const GColor white_paper = GColorFromRGB(255, 250, 239);

  graphics_context_set_fill_color(ctx, paper);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, GColorFromRGB(226, 216, 190));
  graphics_context_set_stroke_width(ctx, 1);
  for (int y = 12; y < height; y += 24) {
    graphics_draw_line(ctx, GPoint(0, y), GPoint(width, y));
  }

  if (!count) {
    // An unused sheet of paper should look inviting, not like an error.
    const int card_y = 17;
    const int card_h = height - card_y - 13;
    graphics_context_set_fill_color(ctx, ink);
    graphics_fill_rect(ctx, GRect(14, card_y + 3, width - 26, card_h),
                       5, GCornersAll);
    graphics_context_set_fill_color(ctx, white_paper);
    graphics_fill_rect(ctx, GRect(12, card_y, width - 26, card_h),
                       5, GCornersAll);
    graphics_context_set_text_color(ctx, ink);
    graphics_draw_text(ctx, "A fresh page.",
                       fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(22, card_y + 18, width - 44, 30),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
    graphics_draw_text(ctx, "Select NEW above.",
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(22, card_y + 47, width - 44, 20),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
  }

  // Only draw the visible note cards; layer clipping prevents cards from
  // painting over the fixed yellow header and the backup/status footer.
  const int first = row_h ? s_list_scroll_px / row_h : 0;
  for (int rank = first; rank < count && rank <= first + LIST_ROWS + 1; ++rank) {
    const int slot = jot_store_slot_at(rank);
    char preview[64];
    time_t created = 0;
    if (slot < 0 ||
        !jot_store_preview(slot, preview, sizeof preview, &created)) continue;

    const int x = 7;
    const int y = rank * row_h - s_list_scroll_px + JOT_LIST_CARD_Y_PAD;
    const int w = width - 14;
    const int h = row_h - 6;
    const bool selected = s_list_focus_valid && rank == s_selected_rank;
    if (selected) {
      graphics_context_set_fill_color(ctx, ink);
      graphics_fill_rect(ctx, GRect(x + 2, y + 2, w, h),
                         5, GCornersAll);
    }

    graphics_context_set_fill_color(ctx, selected ? ink :
                                     GColorFromRGB(170, 159, 136));
    graphics_fill_rect(ctx, GRect(x, y, w, h), 5, GCornersAll);
    graphics_context_set_fill_color(ctx, selected ? yellow : white_paper);
    graphics_fill_rect(ctx, GRect(x + 2, y + 2, w - 4, h - 4),
                       4, GCornersAll);

    graphics_context_set_text_color(ctx, ink);
    if (!*preview) strcpy(preview, "(empty)");
    graphics_draw_text(ctx, preview,
                       fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(x + 10, y + 1, w - 70, h - 2),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);

    graphics_context_set_stroke_color(ctx, selected ?
                                     GColorFromRGB(143, 111, 45) :
                                     GColorFromRGB(213, 202, 180));
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(x + w - 56, y + 8),
                       GPoint(x + w - 56, y + h - 8));

    char clock[8];
    prv_format_time(created, clock, sizeof clock);
    graphics_context_set_text_color(ctx, ink);
    graphics_draw_text(ctx, clock,
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(x + w - 54, y + (h - 20) / 2, 49, 20),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
  }

}

static void prv_list_draw(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  const int width = bounds.size.w;
  const int height = bounds.size.h;
  const int count = jot_store_count();
  const GColor ink = GColorFromRGB(41, 40, 43);
  const GColor yellow = GColorFromRGB(255, 209, 88);
  const GColor paper = GColorFromRGB(249, 242, 219);
  const GColor white_paper = GColorFromRGB(255, 250, 239);

  // The clipped cards child paints the ruled-paper middle section.
  graphics_context_set_fill_color(ctx, paper);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Yellow notebook band with ink edge, pencil mark and + NEW action.
  graphics_context_set_fill_color(ctx, yellow);
  graphics_fill_rect(ctx, GRect(0, 0, width, LIST_HEADER_H - 3),
                     0, GCornerNone);
  graphics_context_set_fill_color(ctx, ink);
  graphics_fill_rect(ctx, GRect(0, LIST_HEADER_H - 3, width, 3),
                     0, GCornerNone);
  prv_draw_pencil(ctx);
  graphics_context_set_text_color(ctx, ink);
  graphics_draw_text(ctx, "Jot",
                     fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(37, 3, width - 141, 33),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  // NEW is a button-focusable action just before the newest note.
  // A selected dark pill is unmistakable; inactive is cream with ink text.
  // The previous capacity badge has been removed to give the action room.
  const bool new_selected =
      s_list_focus_valid && s_selected_rank == JOT_LIST_FOCUS_NEW;
  const GRect new_pill = GRect(width - LIST_NEW_PILL_X_RIGHT,
                               LIST_NEW_PILL_Y,
                               LIST_NEW_PILL_W, LIST_NEW_PILL_H);
  graphics_context_set_fill_color(ctx, new_selected ? ink : white_paper);
  graphics_fill_rect(ctx, new_pill, 8, GCornersAll);
  graphics_context_set_stroke_color(ctx, ink);
  graphics_context_set_stroke_width(ctx, new_selected ? 3 : 1);
  graphics_draw_round_rect(ctx, new_pill, 8);
  graphics_context_set_text_color(ctx, new_selected ? yellow : ink);
  graphics_draw_text(ctx, "+ NEW",
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(new_pill.origin.x + 3, new_pill.origin.y,
                           new_pill.size.w - 6, new_pill.size.h),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // A permanent ink footer also carries backup progress and delete prompts.
  graphics_context_set_fill_color(ctx, yellow);
  graphics_fill_rect(ctx, GRect(0, height - LIST_FOOTER_H, width, 3),
                     0, GCornerNone);
  graphics_context_set_fill_color(ctx, ink);
  graphics_fill_rect(ctx, GRect(0, height - LIST_FOOTER_H + 3,
                                width, LIST_FOOTER_H - 3),
                     0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);
  const char *status =
      s_confirm_delete ? "DELETE?  SEL yes  BACK no" :
      s_storage_error ? "SAVE ERROR" :
      *jot_backup_status() ? jot_backup_status() :
      !s_list_focus_valid ? "UP/DOWN select  TAP open" :
      s_selected_rank == JOT_LIST_FOCUS_NEW ?
        (count ? "SEL new  DOWN notes" : "SEL new note") :
      s_selected_rank == 0 ? "UP new  SEL open" :
      "HOLD UP backup  SEL open";
  graphics_draw_text(ctx, status,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(5, height - LIST_FOOTER_H + 5,
                           width - 10, LIST_FOOTER_H - 8),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
}

static void prv_redraw_list(void) {
  if (s_list_layer) layer_mark_dirty(s_list_layer);
  if (s_cards_layer) layer_mark_dirty(s_cards_layer);
}

// Keep button navigation familiar: one card at a time with the focused note
// near the middle. Dragging, by contrast, retains a fractional pixel offset.
static int prv_list_screen_height(void) {
  return s_list_window ?
      layer_get_bounds(window_get_root_layer(s_list_window)).size.h : 0;
}

static void prv_follow_list_focus(void) {
  s_list_scroll_px = jot_list_touch_scroll_for_focus(
      s_selected_rank, jot_store_count(), prv_list_screen_height());
  prv_redraw_list();
}

// First physical Up *or* Down after touching resumes at the first
// completely visible card. Do not move the scroll position on that first
// press; after this, ordinary one-card-at-a-time button navigation resumes.
static bool prv_restore_list_focus(void) {
  if (s_list_focus_valid) return false;
  const int count = jot_store_count();
  const int first = jot_list_touch_first_full_card(
      s_list_scroll_px, count, prv_list_screen_height());
  s_selected_rank = jot_list_focus_initial(first, count);
  s_list_focus_valid = true;
  prv_redraw_list();
  return true;
}

void jot_backup_ui_changed(void) {
  prv_redraw_list();
}

// One new-note path shared by Select on focused NEW and a tap on its pill.
static void prv_list_new_note(void) {
  if (s_confirm_delete || jot_backup_active()) return;
  // Do not clear a pending quick-capture draft unless it was saved.
  if (!prv_flush()) { prv_redraw_list(); return; }
  if (!jot_store_new()) {
    s_storage_error = true;
    prv_redraw_list();
    return;
  }
  s_editing_slot = -1;
  s_input_full = false;
  s_storage_error = false;
  jot_refresh_editor();
  window_stack_pop(true);
}

static void prv_list_up(ClickRecognizerRef ref, void *context) {
  if (s_confirm_delete || jot_backup_active()) return;
  if (prv_restore_list_focus()) return;
  s_selected_rank = jot_list_focus_up(s_selected_rank);
  prv_follow_list_focus();
}
static void prv_list_down(ClickRecognizerRef ref, void *context) {
  if (s_confirm_delete || jot_backup_active()) return;
  if (prv_restore_list_focus()) return;
  s_selected_rank = jot_list_focus_down(s_selected_rank, jot_store_count());
  prv_follow_list_focus();
}
// Select and touch taps use the same guarded reader-opening path.
static void prv_open_list_note(int rank) {
  if (s_confirm_delete || jot_backup_active()) return;
  const int count = jot_store_count();
  const int slot = jot_store_slot_at(rank);
  if (slot < 0) return;
  s_selected_rank = rank;
  s_list_focus_valid = true;
  prv_redraw_list();
  if (!jot_reader_show(slot, rank, count)) {
    s_storage_error = true;
    prv_redraw_list();
  }
}

static void prv_list_select(ClickRecognizerRef ref, void *context) {
  if (jot_backup_active()) return;
  // An unhighlighted card must not open unexpectedly based on a stale rank.
  // First Select restores visible focus, just like first Up or Down.
  if (prv_restore_list_focus()) return;
  if (s_selected_rank == JOT_LIST_FOCUS_NEW) {
    prv_list_new_note();
    return;
  }
  int slot = jot_store_slot_at(s_selected_rank);
  if (slot < 0) return;
  if (s_confirm_delete) {
    if (!jot_store_delete(slot)) s_storage_error = true;
    else {
      s_storage_error = false;
      s_input_full = false;
      s_selected_rank = jot_list_focus_initial(
          s_selected_rank, jot_store_count());
      s_list_scroll_px = jot_list_touch_clamp_scroll(
          s_list_scroll_px, jot_store_count(), prv_list_screen_height());
    }
    s_confirm_delete = false;
    // Deleting the active note clears its editor buffer. Paint that change
    // before returning from the list; never show a phantom deleted draft.
    jot_refresh_editor();
    prv_redraw_list();
    return;
  }
  // Browsing is read-only. The reader makes its own copy of this note.
  prv_open_list_note(s_selected_rank);
}
static void prv_list_hold(ClickRecognizerRef ref, void *context) {
  if (jot_backup_active()) return;
  // Never request deletion of a stale, unhighlighted note after dragging.
  if (prv_restore_list_focus()) return;
  // NEW is an action, not a note; it must never enter delete confirmation.
  if (s_selected_rank >= 0 && jot_store_count() > 0) {
    s_confirm_delete = true;
    prv_redraw_list();
  }
}
static void prv_list_back(ClickRecognizerRef ref, void *context) {
  if (jot_backup_active()) jot_backup_cancel();
  if (s_confirm_delete) {
    s_confirm_delete = false;
    prv_redraw_list();
    return;
  }
  // From touch-browsing mode, Back re-establishes newest-note focus at
  // the top instead of using a stale rank or exiting unexpectedly.
  if (!s_list_focus_valid) {
    s_selected_rank = jot_list_focus_initial(0, jot_store_count());
    s_list_focus_valid = true;
    prv_follow_list_focus();
    return;
  }
  const int focus = jot_list_focus_back(s_selected_rank, jot_store_count());
  if (focus != JOT_LIST_FOCUS_EXIT) {
    s_selected_rank = focus;
    prv_follow_list_focus();
    return;
  }
  // At the newest note, or NEW on an empty list, leave Jot as before.
  if (prv_flush()) window_stack_pop_all(true);
  else prv_redraw_list();
}
static void prv_list_export(ClickRecognizerRef ref, void *context) {
  if (s_confirm_delete || jot_backup_active()) return;
  if (!prv_flush()) { prv_redraw_list(); return; }
  jot_backup_start();
  prv_redraw_list();
}
static void prv_list_clicks(void *context) {
  window_long_click_subscribe(BUTTON_ID_UP, 900, prv_list_export, NULL);
  window_single_click_subscribe(BUTTON_ID_UP, prv_list_up);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_list_down);
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_list_select);
  window_long_click_subscribe(BUTTON_ID_SELECT, 800, prv_list_hold, NULL);
  window_single_click_subscribe(BUTTON_ID_BACK, prv_list_back);
}
static void prv_list_load(Window *window) {
  const GRect bounds = layer_get_bounds(window_get_root_layer(window));
  s_list_layer = layer_create(bounds);
  if (!s_list_layer) return;
  layer_set_update_proc(s_list_layer, prv_list_draw);
  layer_add_child(window_get_root_layer(window), s_list_layer);

  // Cards cannot paint past the notebook header or footer, even if the
  // finger drags them partially outside the viewport.
  s_cards_layer = layer_create(GRect(
      0, LIST_HEADER_H, bounds.size.w,
      bounds.size.h - LIST_HEADER_H - LIST_FOOTER_H));
  if (s_cards_layer) {
    layer_set_clips(s_cards_layer, true);
    layer_set_update_proc(s_cards_layer, prv_cards_draw);
    layer_add_child(s_list_layer, s_cards_layer);
  }
  s_list_scroll_px = jot_list_touch_scroll_for_focus(
      s_selected_rank, jot_store_count(), bounds.size.h);
}
static void prv_list_unload(Window *window) {
  if (s_cards_layer) layer_destroy(s_cards_layer);
  s_cards_layer = NULL;
  if (s_list_layer) layer_destroy(s_list_layer);
  s_list_layer = NULL;
  s_list_scroll_px = 0;
  s_confirm_delete = false;
}

bool jot_reader_edit(int slot) {
  // Only Select from a reader commits the selected note to the editor.
  if (!jot_reader_is_open() || !jot_store_open(slot)) {
    s_storage_error = true;
    return false;
  }
  s_input_full = false;
  s_storage_error = false;
  s_editing_slot = slot;
  jot_refresh_editor();

  // The reader remains on top while we remove the list beneath it.
  // Popping the reader reveals the keyboard, not another list.
  if (s_list_window && window_stack_contains_window(s_list_window))
    window_stack_remove(s_list_window, false);
  return true;
}

// Resolve the selected note by persistent slot rather than assuming it is
// always the newest. An existing note may be reopened for editing.
static int prv_rank_of_slot(int slot) {
  if (slot < 0) return -1;
  for (int rank = 0; rank < jot_store_count(); ++rank) {
    if (jot_store_slot_at(rank) == slot) return rank;
  }
  return -1;
}

static void prv_push_list_at(int rank) {
  if (!s_list_window) return;
  s_selected_rank = jot_list_focus_initial(rank, jot_store_count());
  s_list_focus_valid = true;
  s_confirm_delete = false;
  s_list_scroll_px = jot_list_touch_scroll_for_focus(
      s_selected_rank, jot_store_count(), prv_list_screen_height());
  window_stack_push(s_list_window, true);
}

void jot_show_list(void) {
  if (jot_reader_is_open()) return;
  if (s_list_window && window_stack_get_top_window() == s_list_window) return;
  if (!prv_flush()) { jot_refresh_editor(); return; }
  prv_push_list_at(prv_rank_of_slot(jot_store_active_slot()));
}

// Rebuild the exact reader page from the committed record. Do not reuse the
// reader's old text snapshot: it predates edits in the keyboard.
static void prv_finish_saved_edit(void) {
  if (!s_list_window || s_editing_slot < 0) return;
  if (!prv_flush()) { jot_refresh_editor(); return; }

  const int slot = s_editing_slot;
  const int rank = prv_rank_of_slot(slot);
  if (rank < 0) {
    s_storage_error = true;
    jot_refresh_editor();
    return;
  }
  // Switch the underlying keyboard back to a fresh quick-capture buffer.
  // The edited text is already safely committed; the reader opens from it.
  if (!jot_store_new()) {
    s_storage_error = true;
    jot_refresh_editor();
    return;
  }
  s_editing_slot = -1;
  s_input_full = false;
  jot_refresh_editor();
  prv_push_list_at(rank);
  if (!jot_reader_show(slot, rank, jot_store_count())) {
    s_storage_error = true;
    prv_redraw_list();
  }
}

void jot_editor_touch_save(void) {
  // Unlike physical Select on a new note, the keyboard SAVE key lands
  // directly on the *committed note's reader*. Keep the established
  // saved-note edit path identical for both completion controls.
  if (s_editing_slot >= 0) {
    prv_finish_saved_edit();
    return;
  }
  if (!s_list_window || jot_reader_is_open() || jot_list_is_open()) return;

  // No phantom empty records, nor a reader accidentally showing the last
  // real note when SAVE is tapped before the first character.
  if (!jot_store_length()) {
    s_input_full = false;
    jot_refresh_editor();
    return;
  }

  // Commit BEFORE any navigation or editor reset. This is also essential
  // for an auto-saved draft: the active record slot is reused, not copied.
  if (!prv_flush()) {
    jot_refresh_editor();
    return;
  }
  const int slot = jot_store_active_slot();
  const int rank = prv_rank_of_slot(slot);
  if (slot < 0 || rank < 0) {
    s_storage_error = true;
    jot_refresh_editor();
    return;
  }

  // The reader's contextual NOTES Back must reveal the real notes list.
  // Keep the original editor intact while setting up both windows: if
  // the list/reader fails to open, unwind to the keyboard with the
  // saved note still in its buffer.
  prv_push_list_at(rank);
  if (!jot_list_is_open() || !s_list_layer || !s_cards_layer ||
      !jot_reader_show(slot, rank, jot_store_count())) {
    if (jot_list_is_open()) window_stack_pop(false);
    s_storage_error = true;
    jot_refresh_editor();
    return;
  }

  // Now a fresh full-screen reader already owns its committed snapshot.
  // The underlying keyboard is reset only after that transition succeeds.
  // Because the flush above succeeded, jot_store_new() cannot fail under
  // normal single-threaded execution (it has no dirty record to write).
  if (!jot_store_new()) {
    s_storage_error = true;
    jot_refresh_editor();
    return;
  }
  s_editing_slot = -1;
  s_input_full = false;
  s_storage_error = false;
  jot_refresh_editor();
}

void jot_editor_select(void) {
  // Select is "finish", never "create a new blank note" while editing an
  // existing saved note. Back has the same safe return path in this mode.
  if (s_editing_slot >= 0) {
    prv_finish_saved_edit();
    return;
  }
  if (!s_list_window) return;
  if (!prv_flush()) { jot_refresh_editor(); return; }
  const int active = jot_store_active_slot();
  const int rank = prv_rank_of_slot(active);

  // Quick capture: a finished note is in storage, then the editor resets for
  // the next capture. Blank drafts simply open history without allocation.
  if (active >= 0 && !jot_store_new()) {
    s_storage_error = true;
    jot_refresh_editor();
    return;
  }
  s_input_full = false;
  jot_refresh_editor();
  prv_push_list_at(rank);
}

void jot_editor_back(void) {
  if (s_editing_slot >= 0) {
    prv_finish_saved_edit();
  } else {
    // Unlike the old unhandled hardware Back, this never unexpectedly exits
    // PebbleOS. If persistence fails, jot_show_list() stays in the editor.
    jot_show_list();
  }
}

bool jot_list_is_open(void) {
  return s_list_window && window_stack_get_top_window() == s_list_window;
}

void jot_list_tap(int16_t x, int16_t y) {
  if (!jot_list_is_open() || !s_list_layer ||
      s_confirm_delete || jot_backup_active()) return;
  const GRect screen = layer_get_bounds(s_list_layer);
  // Header NEW remains a distinct tappable action.
  if (x >= screen.size.w - LIST_NEW_PILL_X_RIGHT &&
      x < screen.size.w - LIST_NEW_PILL_X_RIGHT + LIST_NEW_PILL_W &&
      y >= LIST_NEW_PILL_Y && y < LIST_NEW_PILL_Y + LIST_NEW_PILL_H) {
    prv_list_new_note();
    return;
  }

  // Use precisely the same row height, scroll pixels and card margins as
  // the clipped drawing layer. Neither the paper gaps nor footer are links.
  const int rank = jot_list_touch_hit_card(
      x, y, screen.size.w, screen.size.h, jot_store_count(), s_list_scroll_px);
  if (rank >= 0) prv_open_list_note(rank);
}

bool jot_list_can_drag(int16_t y) {
  if (!jot_list_is_open() || !s_list_layer || !s_cards_layer ||
      s_confirm_delete || jot_backup_active()) return false;
  const int height = layer_get_bounds(s_list_layer).size.h;
  return y >= LIST_HEADER_H && y < height - LIST_FOOTER_H;
}

void jot_list_drag(int16_t delta_y) {
  if (!jot_list_is_open() || !s_cards_layer || s_confirm_delete ||
      jot_backup_active() || delta_y == 0) return;
  const int count = jot_store_count();
  const int height = prv_list_screen_height();
  const int next = jot_list_touch_clamp_scroll(
      s_list_scroll_px - delta_y, count, height);
  // Clear the yellow highlight as soon as touch movement becomes a drag,
  // even at a scroll boundary. Keep s_selected_rank as dormant metadata,
  // NOT as NEW (-1), which is a genuine keyboard focus target.
  const bool had_focus = s_list_focus_valid;
  s_list_focus_valid = false;
  if (next == s_list_scroll_px) {
    if (had_focus) prv_redraw_list();
    return;
  }
  s_list_scroll_px = next;
  prv_redraw_list();
}

void jot_init(void) {
  jot_store_init();
  s_editing_slot = -1;
  s_storage_error = false;
  s_input_full = false;
  s_list_window = window_create();
  if (s_list_window) {
    window_set_background_color(s_list_window, GColorWhite);
    window_set_window_handlers(s_list_window,
                               (WindowHandlers) {.load = prv_list_load,
                                                 .unload = prv_list_unload});
    window_set_click_config_provider(s_list_window, prv_list_clicks);
  }
  jot_reader_init();
  jot_backup_init();
  jot_refresh_editor();
}

void jot_deinit(void) {
  prv_flush();
  jot_backup_deinit();
  jot_reader_deinit();
  if (s_list_window) { window_destroy(s_list_window); s_list_window = NULL; }
}

#include <pebble.h>
#include "jot/jot.h"
#include "jot/store.h"
#include "jot/backup.h"
#include "jot/reader.h"
#include "ui/alert.h"

#define AUTOSAVE_DELAY_MS 1800
#define LIST_ROWS 3
#define LIST_HEADER_H 43
#define LIST_FOOTER_H 29

static Window *s_list_window;
static Layer *s_list_layer;
static AppTimer *s_save_timer;
static int s_selected_rank;
static bool s_confirm_delete;
static bool s_storage_error;
static bool s_input_full;

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
  alert_set_editor(jot_store_text(), jot_store_dirty(),
                   s_storage_error, s_input_full);
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

static void prv_list_draw(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  const int width = bounds.size.w;
  const int height = bounds.size.h;
  const int row_h = (height - LIST_HEADER_H - LIST_FOOTER_H) / LIST_ROWS;
  const int count = jot_store_count();
  // Keep the selected note near the middle and advance by one card,
  // rather than jumping an entire page every third button press.
  int first = s_selected_rank - 1;
  if (first < 0) first = 0;
  int max_first = count - LIST_ROWS;
  if (max_first < 0) max_first = 0;
  if (first > max_first) first = max_first;
  const GColor ink = GColorFromRGB(41, 40, 43);
  const GColor yellow = GColorFromRGB(255, 209, 88);
  const GColor paper = GColorFromRGB(249, 242, 219);
  const GColor white_paper = GColorFromRGB(255, 250, 239);

  // Cream paper, with subtle ruled lines in the empty area.
  graphics_context_set_fill_color(ctx, paper);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, GColorFromRGB(226, 216, 190));
  graphics_context_set_stroke_width(ctx, 1);
  for (int y = LIST_HEADER_H + 12; y < height - LIST_FOOTER_H; y += 24) {
    graphics_draw_line(ctx, GPoint(0, y), GPoint(width, y));
  }

  // Yellow notebook band with an ink edge, pencil mark and count badge.
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
                     GRect(37, 3, width - 112, 33),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  char usage[16];
  snprintf(usage, sizeof usage, "%d / %d", count, JOT_NOTE_CAPACITY);
  graphics_context_set_fill_color(ctx, ink);
  graphics_fill_rect(ctx, GRect(width - 67, 8, 59, 25),
                     5, GCornersAll);
  graphics_context_set_text_color(ctx, yellow);
  graphics_draw_text(ctx, usage,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(width - 65, 11, 55, 20),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  if (!count) {
    // An unused sheet of paper should look inviting, not like an error.
    const int card_y = LIST_HEADER_H + 17;
    const int card_h = height - LIST_FOOTER_H - card_y - 13;
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
    graphics_draw_text(ctx, "Back to write.",
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(22, card_y + 47, width - 44, 20),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
  }

  // Three generous note cards per page: two-line previews, clock at right.
  // Keep all hit-testing/button behavior unchanged; only the paint changes.
  for (int row = 0; row < LIST_ROWS; ++row) {
    const int rank = first + row;
    if (rank >= count) break;
    const int slot = jot_store_slot_at(rank);
    char preview[64];
    time_t created = 0;
    if (slot < 0 ||
        !jot_store_preview(slot, preview, sizeof preview, &created)) continue;

    const int x = 7;
    const int y = LIST_HEADER_H + row * row_h + 3;
    const int w = width - 14;
    const int h = row_h - 6;
    const bool selected = rank == s_selected_rank;
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
}

void jot_backup_ui_changed(void) {
  prv_redraw_list();
}

static void prv_list_up(ClickRecognizerRef ref, void *context) {
  if (s_confirm_delete || jot_backup_active()) return;
  if (s_selected_rank > 0) --s_selected_rank;
  prv_redraw_list();
}
static void prv_list_down(ClickRecognizerRef ref, void *context) {
  if (s_confirm_delete || jot_backup_active()) return;
  if (s_selected_rank + 1 < jot_store_count()) ++s_selected_rank;
  prv_redraw_list();
}
static void prv_list_select(ClickRecognizerRef ref, void *context) {
  if (jot_backup_active()) return;
  int slot = jot_store_slot_at(s_selected_rank);
  if (slot < 0) return;
  if (s_confirm_delete) {
    if (!jot_store_delete(slot)) s_storage_error = true;
    else {
      s_storage_error = false;
      s_input_full = false;
      if (s_selected_rank >= jot_store_count() && s_selected_rank)
        --s_selected_rank;
    }
    s_confirm_delete = false;
    // Deleting the active note clears its editor buffer. Paint that change
    // before returning from the list; never show a phantom deleted draft.
    jot_refresh_editor();
    prv_redraw_list();
    return;
  }
  // Browsing is read-only. The reader makes its own copy of this note.
  if (!jot_reader_show(slot, s_selected_rank, jot_store_count())) {
    s_storage_error = true;
    prv_redraw_list();
  }
}
static void prv_list_hold(ClickRecognizerRef ref, void *context) {
  if (jot_backup_active()) return;
  if (jot_store_count() > 0) {
    s_confirm_delete = true;
    prv_redraw_list();
  }
}
static void prv_list_back(ClickRecognizerRef ref, void *context) {
  if (jot_backup_active()) jot_backup_cancel();
  if (s_confirm_delete) {
    s_confirm_delete = false;
    prv_redraw_list();
  } else {
    window_stack_pop(true);
  }
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
  GRect bounds = layer_get_bounds(window_get_root_layer(window));
  s_list_layer = layer_create(bounds);
  if (!s_list_layer) return;
  layer_set_update_proc(s_list_layer, prv_list_draw);
  layer_add_child(window_get_root_layer(window), s_list_layer);
}
static void prv_list_unload(Window *window) {
  if (s_list_layer) layer_destroy(s_list_layer);
  s_list_layer = NULL;
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
  jot_refresh_editor();

  // The reader remains on top while we remove the list beneath it.
  // Popping the reader reveals the keyboard, not another list.
  if (s_list_window && window_stack_contains_window(s_list_window))
    window_stack_remove(s_list_window, false);
  return true;
}

void jot_show_list(void) {
  if (jot_reader_is_open()) return;
  if (s_list_window && window_stack_get_top_window() == s_list_window) return;
  if (!prv_flush()) { jot_refresh_editor(); return; }
  if (!s_list_window) return;
  s_selected_rank = 0;
  s_confirm_delete = false;
  window_stack_push(s_list_window, true);
}

bool jot_list_is_open(void) {
  return s_list_window && window_stack_get_top_window() == s_list_window;
}

void jot_init(void) {
  jot_store_init();
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

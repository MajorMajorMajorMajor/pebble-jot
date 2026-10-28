#include <pebble.h>
#include "jot/jot.h"
#include "jot/store.h"
#include "jot/backup.h"
#include "ui/alert.h"

#define AUTOSAVE_DELAY_MS 1800
#define LIST_ROWS 4

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

static const char *prv_tail(const char *text, size_t bytes) {
  // Render the end of the note. Start at a UTF-8 boundary.
  const char *tail = bytes > 36 ? text + bytes - 36 : text;
  while (((unsigned char)*tail & 0xC0) == 0x80) ++tail;
  return tail;
}

void jot_refresh_editor(void) {
  char message[200];
  const char *heading = s_storage_error ? "JOT  SAVE ERROR" :
                        s_input_full ? "JOT  FULL" :
                        jot_store_dirty() ? "JOT  *" : "JOT";
  const char *text = jot_store_text();
  snprintf(message, sizeof message, "%s\n%s", heading,
           *text ? prv_tail(text, jot_store_length()) : "Write something...");
  alert_set_text(message);
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
  if (s_list_window && window_stack_get_top_window() == s_list_window) return;
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
  struct tm *tm = localtime(&when);
  if (!tm) { snprintf(out, cap, "--:--"); return; }
  snprintf(out, cap, "%02d:%02d", tm->tm_hour, tm->tm_min);
}

static void prv_list_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  const int width = bounds.size.w;
  const int height = bounds.size.h;
  const int header_h = 34;
  const int footer_h = 19;
  const int row_h = (height - header_h - footer_h) / LIST_ROWS;
  const int count = jot_store_count();
  const int first = (s_selected_rank / LIST_ROWS) * LIST_ROWS;

  graphics_context_set_fill_color(ctx, GColorFromRGB(249, 242, 219));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorFromRGB(255, 209, 88));
  graphics_fill_rect(ctx, GRect(0, 0, width, header_h), 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorBlack);
  char title[36];
  snprintf(title, sizeof title, "JOT   %d / %d", count, JOT_NOTE_CAPACITY);
  graphics_draw_text(ctx, title, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(8, 4, width - 16, 28), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  if (!count) {
    graphics_draw_text(ctx, "No notes yet.\nBack to write!",
                       fonts_get_system_font(FONT_KEY_GOTHIC_18),
                       GRect(10, 52, width - 20, 60), GTextOverflowModeWordWrap,
                       GTextAlignmentLeft, NULL);
  }
  for (int row = 0; row < LIST_ROWS; ++row) {
    const int rank = first + row;
    if (rank >= count) break;
    const int slot = jot_store_slot_at(rank);
    const int y = header_h + row * row_h;
    const bool selected = rank == s_selected_rank;
    if (selected) {
      graphics_context_set_fill_color(ctx, GColorFromRGB(255, 209, 88));
      graphics_fill_rect(ctx, GRect(4, y + 1, width - 8, row_h - 2),
                         3, GCornersAll);
    }
    char preview[64];
    time_t created = 0;
    if (!jot_store_preview(slot, preview, sizeof preview, &created)) continue;
    char clock[8];
    prv_format_time(created, clock, sizeof clock);
    if (!*preview) strcpy(preview, "(empty)");
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, preview, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(10, y + 1, width - 75, row_h - 2),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, clock, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(width - 68, y + 5, 62, row_h - 4),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
  }
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, GRect(0, height - footer_h, width, footer_h),
                     0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx,
                     s_confirm_delete ? "Delete? Select=yes Back=no" :
                     *jot_backup_status() ? jot_backup_status() :
                     "Hold UP:backup  Sel:edit",
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(5, height - footer_h - 2, width - 10, footer_h + 2),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
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
    prv_redraw_list();
    return;
  }
  if (!jot_store_open(slot)) { s_storage_error = true; prv_redraw_list(); return; }
  s_input_full = false;
  s_storage_error = false;
  jot_refresh_editor();
  window_stack_pop(true);
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

void jot_show_list(void) {
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
  jot_backup_init();
  jot_refresh_editor();
}

void jot_deinit(void) {
  prv_flush();
  jot_backup_deinit();
  if (s_list_window) { window_destroy(s_list_window); s_list_window = NULL; }
}

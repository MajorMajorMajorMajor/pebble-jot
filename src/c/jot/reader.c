#include <pebble.h>
#include "jot/reader.h"
#include "jot/jot.h"
#include "jot/store.h"

enum {
  READER_HEADER_H = 42,
  READER_FOOTER_H = 30,
  READER_META_H = 33,
  READER_TEXT_PADDING = 12,
  READER_SCROLL_STEP = 60
};

static Window *s_window;
static Layer *s_chrome;
static Layer *s_paper;
static Layer *s_viewport;
static TextLayer *s_text_layer;

static char s_note[JOT_NOTE_TEXT_BYTES];
static char s_timestamp[32];
static int s_slot, s_rank, s_total;
static int s_scroll, s_scroll_max;
static int s_text_height;
static bool s_open_error;
static bool s_is_peek;

static GColor prv_ink(void) { return GColorFromRGB(41, 40, 43); }
static GColor prv_yellow(void) { return GColorFromRGB(255, 209, 88); }
static GColor prv_paper(void) { return GColorFromRGB(249, 242, 219); }

bool jot_reader_is_open(void) {
  return s_window && window_stack_get_top_window() == s_window;
}

bool jot_reader_is_peek(void) {
  return s_is_peek && jot_reader_is_open();
}

void jot_reader_dismiss_peek(void) {
  if (jot_reader_is_peek()) window_stack_pop(true);
}

static void prv_draw_pencil(GContext *ctx) {
  graphics_context_set_stroke_color(ctx, prv_ink());
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, GPoint(10, 27), GPoint(22, 15));
  graphics_draw_line(ctx, GPoint(14, 31), GPoint(26, 19));
  graphics_draw_line(ctx, GPoint(10, 27), GPoint(14, 31));
  graphics_draw_line(ctx, GPoint(22, 15), GPoint(26, 19));
  graphics_draw_line(ctx, GPoint(8, 33), GPoint(14, 31));
}

static void prv_draw_chrome(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  const int width = bounds.size.w;
  const int footer_y = bounds.size.h - READER_FOOTER_H;

  graphics_context_set_fill_color(ctx, prv_paper());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_fill_color(ctx, prv_yellow());
  graphics_fill_rect(ctx, GRect(0, 0, width, READER_HEADER_H - 3),
                     0, GCornerNone);
  graphics_context_set_fill_color(ctx, prv_ink());
  graphics_fill_rect(ctx, GRect(0, READER_HEADER_H - 3, width, 3),
                     0, GCornerNone);
  prv_draw_pencil(ctx);

  graphics_context_set_text_color(ctx, prv_ink());
  graphics_draw_text(ctx, "Jot",
                     fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(37, 3, width - 112, 33),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  char count[20];
  if (s_is_peek) snprintf(count, sizeof count, "DRAFT");
  else snprintf(count, sizeof count, "%d / %d", s_rank + 1, s_total);
  graphics_context_set_fill_color(ctx, prv_ink());
  graphics_fill_rect(ctx, GRect(width - 67, 8, 59, 25), 5, GCornersAll);
  graphics_context_set_text_color(ctx, prv_yellow());
  graphics_draw_text(ctx, count,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(width - 65, 11, 55, 20),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // A fixed, readable footer. The text region above it has its own clip.
  graphics_context_set_fill_color(ctx, prv_yellow());
  graphics_fill_rect(ctx, GRect(0, footer_y, width, 3), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, prv_ink());
  graphics_fill_rect(ctx, GRect(0, footer_y + 3, width, READER_FOOTER_H - 3),
                     0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);
  const char *hint = s_open_error ? "COULD NOT OPEN NOTE" :
                     s_is_peek ? (s_scroll_max ? "UP/DN scroll  TAP return" :
                                                 "TAP to return to typing") :
                     s_scroll_max ? "UP/DN scroll   SEL edit" :
                                    "SEL edit    BACK list";
  graphics_draw_text(ctx, hint,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(5, footer_y + 5, width - 10, READER_FOOTER_H - 8),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
}

static void prv_draw_paper(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  const int width = bounds.size.w;
  const int height = bounds.size.h;
  graphics_context_set_fill_color(ctx, prv_paper());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, GColorFromRGB(226, 216, 190));
  graphics_context_set_stroke_width(ctx, 1);
  for (int y = READER_META_H + 18; y < height; y += 30) {
    graphics_draw_line(ctx, GPoint(0, y), GPoint(width, y));
  }
  graphics_context_set_text_color(ctx, GColorFromRGB(106, 95, 80));
  graphics_draw_text(ctx, s_timestamp,
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(12, 0, width - 24, 28),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  graphics_context_set_stroke_color(ctx, GColorFromRGB(181, 164, 137));
  graphics_draw_line(ctx, GPoint(12, 29), GPoint(width - 12, 29));
}

static void prv_update_scroll(void) {
  if (!s_text_layer || !s_viewport) return;
  const int width = layer_get_bounds(s_viewport).size.w;
  layer_set_frame(text_layer_get_layer(s_text_layer),
                  GRect(READER_TEXT_PADDING, 2 - s_scroll,
                        width - 2 * READER_TEXT_PADDING, s_text_height));
  if (s_chrome) layer_mark_dirty(s_chrome);
}

static void prv_measure_text(void) {
  if (!s_text_layer || !s_viewport) return;
  const GRect area = layer_get_bounds(s_viewport);
  const int width = area.size.w - 2 * READER_TEXT_PADDING;
  const GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  const char *text = *s_note ? s_note : "(empty note)";
  const GSize size = graphics_text_layout_get_content_size(
      text, font, GRect(0, 0, width, 2000),
      GTextOverflowModeWordWrap, GTextAlignmentLeft);
  s_text_height = size.h + 8;
  if (s_text_height < 34) s_text_height = 34;
  s_scroll_max = s_text_height + 4 - area.size.h;
  if (s_scroll_max < 0) s_scroll_max = 0;
  if (s_scroll > s_scroll_max) s_scroll = s_scroll_max;
  text_layer_set_text(s_text_layer, text);
  prv_update_scroll();
}

static void prv_reader_up(ClickRecognizerRef ref, void *context) {
  s_scroll -= READER_SCROLL_STEP;
  if (s_scroll < 0) s_scroll = 0;
  prv_update_scroll();
}

static void prv_reader_down(ClickRecognizerRef ref, void *context) {
  s_scroll += READER_SCROLL_STEP;
  if (s_scroll > s_scroll_max) s_scroll = s_scroll_max;
  prv_update_scroll();
}

static void prv_reader_select(ClickRecognizerRef ref, void *context) {
  if (s_is_peek) {
    window_stack_pop(true);
    return;
  }
  if (!jot_reader_edit(s_slot)) {
    s_open_error = true;
    if (s_chrome) layer_mark_dirty(s_chrome);
    return;
  }
  // The notes list was removed beneath this window by jot_reader_edit().
  // Popping the reader now reveals the existing keyboard/editor.
  window_stack_pop(true);
}

static void prv_reader_back(ClickRecognizerRef ref, void *context) {
  window_stack_pop(true);  // Return to notes list without touching note data.
}

static void prv_reader_clicks(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, prv_reader_up);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_reader_down);
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_reader_select);
  window_single_click_subscribe(BUTTON_ID_BACK, prv_reader_back);
}

static void prv_reader_load(Window *window) {
  const GRect bounds = layer_get_bounds(window_get_root_layer(window));
  const int width = bounds.size.w;
  const int paper_h = bounds.size.h - READER_HEADER_H - READER_FOOTER_H;
  const int view_h = paper_h - READER_META_H;

  s_chrome = layer_create(bounds);
  if (s_chrome) {
    layer_set_update_proc(s_chrome, prv_draw_chrome);
    layer_add_child(window_get_root_layer(window), s_chrome);
  }

  s_paper = layer_create(GRect(0, READER_HEADER_H, width, paper_h));
  if (s_paper) {
    layer_set_update_proc(s_paper, prv_draw_paper);
    layer_add_child(window_get_root_layer(window), s_paper);
  }

  if (s_paper) {
    s_viewport = layer_create(GRect(0, READER_META_H, width, view_h));
    if (s_viewport) layer_add_child(s_paper, s_viewport);
  }

  if (s_viewport) {
    s_text_layer = text_layer_create(
        GRect(READER_TEXT_PADDING, 2, width - 2 * READER_TEXT_PADDING, 30));
    if (s_text_layer) {
      text_layer_set_background_color(s_text_layer, GColorClear);
      text_layer_set_text_color(s_text_layer, prv_ink());
      text_layer_set_font(s_text_layer,
                          fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
      text_layer_set_overflow_mode(s_text_layer, GTextOverflowModeWordWrap);
      text_layer_set_text_alignment(s_text_layer, GTextAlignmentLeft);
      layer_add_child(s_viewport, text_layer_get_layer(s_text_layer));
      prv_measure_text();
    }
  }
}

static void prv_reader_unload(Window *window) {
  if (s_text_layer) text_layer_destroy(s_text_layer);
  s_text_layer = NULL;
  if (s_viewport) layer_destroy(s_viewport);
  s_viewport = NULL;
  if (s_paper) layer_destroy(s_paper);
  s_paper = NULL;
  if (s_chrome) layer_destroy(s_chrome);
  s_chrome = NULL;
}

bool jot_reader_init(void) {
  s_window = window_create();
  if (!s_window) return false;
  window_set_background_color(s_window, prv_paper());
  window_set_window_handlers(s_window,
                             (WindowHandlers) {.load = prv_reader_load,
                                               .unload = prv_reader_unload});
  window_set_click_config_provider(s_window, prv_reader_clicks);
  return true;
}

void jot_reader_deinit(void) {
  if (!s_window) return;
  if (window_stack_contains_window(s_window))
    window_stack_remove(s_window, false);
  window_destroy(s_window);
  s_window = NULL;
}

bool jot_reader_show(int slot, int rank, int total) {
  if (!s_window || jot_reader_is_open() || slot < 0 ||
      rank < 0 || rank >= total) return false;
  time_t created = 0;
  if (!jot_store_export(slot, s_note, sizeof s_note, &created)) return false;
  const struct tm *tm = localtime(&created);
  if (tm) {
    strftime(s_timestamp, sizeof s_timestamp, "%b %d  %H:%M", tm);
  } else {
    snprintf(s_timestamp, sizeof s_timestamp, "Saved note");
  }
  s_is_peek = false;
  s_slot = slot;
  s_rank = rank;
  s_total = total;
  s_scroll = 0;
  s_scroll_max = 0;
  s_open_error = false;

  window_stack_push(s_window, true);
  // If any layer allocation failed, avoid trapping users in a blank reader.
  if (!s_chrome || !s_paper || !s_viewport || !s_text_layer) {
    window_stack_pop(false);
    return false;
  }
  return true;
}

bool jot_reader_peek(void) {
  if (!s_window || jot_reader_is_open()) return false;
  // The editor may be dirty or completely blank. This is only a snapshot:
  // no flush, no note allocation, no switching the active note.
  const size_t length = jot_store_length();
  if (length >= sizeof s_note) return false;
  memcpy(s_note, jot_store_text(), length + 1);
  snprintf(s_timestamp, sizeof s_timestamp, "CURRENT NOTE");
  s_slot = -1;
  s_rank = 0;
  s_total = 0;
  s_scroll = 0;
  s_scroll_max = 0;
  s_open_error = false;
  s_is_peek = true;
  window_stack_push(s_window, true);
  if (!s_chrome || !s_paper || !s_viewport || !s_text_layer) {
    window_stack_pop(false);
    return false;
  }
  return true;
}

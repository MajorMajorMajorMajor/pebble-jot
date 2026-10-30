#include <pebble.h>
#include <stdlib.h>
#include "jot/reader.h"
#include "jot/reader_touch.h"
#include "jot/reader_review.h"
#include "jot/jot.h"
#include "jot/store.h"

enum {
  READER_HEADER_H = JOT_READER_HEADER_H,
  READER_FOOTER_H = JOT_READER_FOOTER_H,
  READER_META_H = JOT_READER_META_H,
  READER_TEXT_PADDING = JOT_READER_TEXT_PADDING,
  READER_SCROLL_STEP = 60
};

static Window *s_window;
static Layer *s_chrome;
static ActionBarLayer *s_action_bar; // Native PebbleOS control.
static bool s_action_bar_attached;
static Layer *s_nub; // Solid collapsed-side affordance.
enum { ICON_UP, ICON_DOWN, ICON_PENCIL, ICON_TRASH, ICON_CHECK, ICON_X, ICON_COUNT };
static GBitmap *s_icons[ICON_COUNT];
static const uint32_t s_icon_ids[ICON_COUNT] = {
  RESOURCE_ID_JOT_ACTION_UP, RESOURCE_ID_JOT_ACTION_DOWN,
  RESOURCE_ID_JOT_ACTION_PENCIL, RESOURCE_ID_JOT_ACTION_TRASH,
  RESOURCE_ID_JOT_ACTION_CHECK, RESOURCE_ID_JOT_ACTION_X
};
static void prv_reader_clicks(void *context);
static Layer *s_paper;
static Layer *s_viewport;
static TextLayer *s_text_layer;

static char *s_note; // Allocated only while Reader/Peek is open.
static char s_timestamp[32];
static int s_slot, s_rank, s_total;
static int s_scroll, s_scroll_max;
static int s_text_height;
static bool s_open_error;
static bool s_is_peek;
static JotReaderReview s_review;
static const char *s_review_message;

static GColor prv_ink(void) { return GColorFromRGB(41, 40, 43); }
static GColor prv_yellow(void) { return GColorFromRGB(255, 209, 88); }
static GColor prv_paper(void) { return GColorFromRGB(249, 242, 219); }

bool jot_reader_is_open(void) {
  return s_window && window_stack_get_top_window() == s_window;
}

bool jot_reader_is_peek(void) {
  return s_is_peek && jot_reader_is_open();
}

static void prv_mark_review(void) {
  if (s_nub) layer_set_hidden(s_nub, s_review.open);
  if (s_action_bar) {
    if (s_review.open) {
      // SDK handles alignment, button highlighting and window geometry.
      action_bar_layer_set_background_color(s_action_bar,
          s_review.delete_mode ? GColorFromRGB(202, 39, 47) : prv_ink());
      if (s_review.armed)
        action_bar_layer_clear_icon(s_action_bar, BUTTON_ID_UP);
      else
        action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icons[ICON_UP]);
      action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT,
          s_review.armed ? s_icons[ICON_CHECK] :
          s_review.delete_mode ? s_icons[ICON_TRASH] : s_icons[ICON_PENCIL]);
      action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN,
          s_review.armed ? s_icons[ICON_X] : s_icons[ICON_DOWN]);
      if (!s_action_bar_attached) {
        action_bar_layer_add_to_window(s_action_bar, s_window);
        s_action_bar_attached = true;
      }
    } else if (s_action_bar_attached) {
      // SDK removes the window's click provider with the action bar.
      action_bar_layer_remove_from_window(s_action_bar);
      s_action_bar_attached = false;
      window_set_click_config_provider(s_window, prv_reader_clicks);
    }
  }
  if (s_chrome) layer_mark_dirty(s_chrome);
}

void jot_reader_back(void) {
  if (!jot_reader_is_open()) return;
  // First Back disarms deletion; next Back closes the action bar.
  // Once the bar is closed, Back returns to the list (or draft editor).
  if (!s_is_peek && jot_reader_review_back(&s_review)) {
    s_review_message = NULL;
    prv_mark_review();
    return;
  }
  window_stack_pop(true);
}

void jot_reader_dismiss_peek(void) {
  if (jot_reader_is_peek()) jot_reader_back();
}

static void prv_draw_back(GContext *ctx) {
  // The actual touch rectangle is 0..41 x 0..41 (reader_touch.h).
  // A small paper key makes the arrow obviously actionable rather than
  // decorative. Both reader states share this exact button.
  const GRect button = GRect(5, 7, 34, 27);
  graphics_context_set_fill_color(ctx, GColorFromRGB(255, 250, 239));
  graphics_fill_rect(ctx, button, 5, GCornersAll);
  graphics_context_set_stroke_color(ctx, prv_ink());
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_round_rect(ctx, button, 5);
  graphics_context_set_stroke_width(ctx, 3);
  graphics_draw_line(ctx, GPoint(30, 20), GPoint(14, 20));
  graphics_draw_line(ctx, GPoint(14, 20), GPoint(22, 12));
  graphics_draw_line(ctx, GPoint(14, 20), GPoint(22, 28));
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
  prv_draw_back(ctx);

  graphics_context_set_text_color(ctx, prv_ink());
  // Context label tells the user where the header Back action goes.
  // A normal saved note returns to NOTES; a temporary draft peek to EDIT.
  graphics_draw_text(ctx, s_is_peek ? "EDIT" :
                             s_review.open ? "CLOSE" : "NOTES",
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(44, 7, width - 44 - 73, 27),
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
                     s_review.armed ? "Confirm delete?" :
                     s_review_message ? s_review_message :
                     s_is_peek ? (s_scroll_max ? "DRAG scroll  TAP return" :
                                                 "TAP to return to typing") :
                     s_review.armed ? "SEL confirm  DN cancel" :
                     s_review.open ? "UP/DN notes  SEL action" :
                     s_scroll_max ? "DRAG scroll  SEL menu" :
                                    "TAP text  SEL menu";
  graphics_draw_text(ctx, hint,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(5, footer_y + 5, width - 10, READER_FOOTER_H - 8),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
}


// Solid closed-state nub. The open-source PebbleOS ActionBar uses the
// same graphics_fill_oval primitive for circular background drawing.
// There is no standalone collapsed ActionBar widget in the SDK.
static void prv_draw_nub(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  const GRect oval = GRect(bounds.size.w - 11,
      bounds.size.h / 2 - 20, 40, 40);
  graphics_context_set_fill_color(ctx, prv_ink());
  graphics_fill_oval(ctx, oval, GOvalScaleModeFitCircle);
}

bool jot_reader_action_overlay_hit(int16_t x, int16_t y) {
  if (!jot_reader_is_open() || s_is_peek) return false;
  if (s_review.open) {
    const GRect bounds = layer_get_bounds(window_get_root_layer(s_window));
    return x >= bounds.size.w - ACTION_BAR_WIDTH && x < bounds.size.w &&
           y >= 0 && y < bounds.size.h;
  }
  if (!s_nub) return false;
  const GRect bounds = layer_get_bounds(s_nub);
  return x >= bounds.size.w - 11 && x < bounds.size.w &&
         y >= bounds.size.h / 2 - 20 && y < bounds.size.h / 2 + 20;
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
                  GRect(READER_TEXT_PADDING, JOT_READER_TEXT_TOP_INSET - s_scroll,
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
      text, font, GRect(0, 0, width, 16384),
      GTextOverflowModeWordWrap, GTextAlignmentLeft);
  s_text_height = size.h + 8;
  if (s_text_height < 34) s_text_height = 34;
  s_scroll_max = s_text_height + 4 - area.size.h;
  if (s_scroll_max < 0) s_scroll_max = 0;
  if (s_scroll > s_scroll_max) s_scroll = s_scroll_max;
  text_layer_set_text(s_text_layer, text);
  prv_update_scroll();
}


static void prv_update_rank(int rank) {
  // Changing notes is a read-only preview. Load into a temporary buffer
  // first so an I/O error never destroys the current reader snapshot.
  if (!jot_reader_is_open() || s_is_peek || rank == s_rank) return;
  const int count = jot_store_count();
  const int slot = jot_store_slot_at(rank);
  if (slot < 0 || rank < 0 || rank >= count) return;
  char *next = malloc(JOT_NOTE_TEXT_BYTES);
  time_t created = 0;
  if (!next) {
    s_review_message = "NOT ENOUGH MEMORY";
    prv_mark_review();
    return;
  }
  if (!jot_store_export(slot, next, JOT_NOTE_TEXT_BYTES, &created)) {
    free(next);
    s_review_message = "COULD NOT OPEN NOTE";
    prv_mark_review();
    return;
  }

  free(s_note);
  s_note = next;
  s_slot = slot;
  s_rank = rank;
  s_total = count;
  s_scroll = 0; // Start each new note at the beginning.
  s_scroll_max = 0;
  s_open_error = false;
  s_review_message = NULL;
  const struct tm *tm = localtime(&created);
  if (tm) strftime(s_timestamp, sizeof s_timestamp, "%b %d  %H:%M", tm);
  else snprintf(s_timestamp, sizeof s_timestamp, "Saved note");
  prv_measure_text();
  // When the reader closes, the list already focuses this exact note.
  jot_reader_focus_rank(rank);
  prv_mark_review();
}

static void prv_browse(int direction) {
  if (jot_reader_review_cancel_armed(&s_review)) {
    s_review_message = NULL; // Navigation cancels an armed deletion.
    prv_mark_review();
    return;
  }
  const int rank = jot_reader_review_next_rank(
      s_rank, jot_store_count(), direction);
  if (rank < 0 || rank == s_rank) {
    s_review_message = direction < 0 ? "FIRST NOTE" : "LAST NOTE";
    prv_mark_review();
    return;
  }
  prv_update_rank(rank);
}

static void prv_confirm_delete(void) {
  if (!jot_reader_is_open() || s_is_peek || s_slot < 0) return;
  const int rank = s_rank;
  if (!jot_store_delete(s_slot)) {
    s_review_message = "DELETE FAILED";
    prv_mark_review();
    return;
  }
  jot_refresh_editor();
  const int count = jot_store_count();
  if (!count) {
    jot_reader_focus_rank(-1); // Empty list: focus + NEW.
    window_stack_pop(true);
    return;
  }
  const int next_rank = rank < count ? rank : count - 1;
  // The just-deleted page must never be shown again. If the next note
  // cannot be loaded, return to the list instead of showing stale text.
  jot_reader_focus_rank(next_rank);
  const int next_slot = jot_store_slot_at(next_rank);
  char *next = malloc(JOT_NOTE_TEXT_BYTES);
  time_t created = 0;
  if (!next || next_slot < 0 ||
      !jot_store_export(next_slot, next, JOT_NOTE_TEXT_BYTES, &created)) {
    free(next);
    window_stack_pop(true);
    return;
  }
  free(s_note);
  s_note = next;
  s_slot = next_slot;
  s_rank = next_rank;
  s_total = count;
  s_scroll = 0;
  s_scroll_max = 0;
  s_open_error = false;
  s_review_message = NULL;
  const struct tm *tm = localtime(&created);
  if (tm) strftime(s_timestamp, sizeof s_timestamp, "%b %d  %H:%M", tm);
  else snprintf(s_timestamp, sizeof s_timestamp, "Saved note");
  prv_measure_text();
  prv_mark_review(); // Keep Delete mode open, ready for next triage action.
}

static void prv_reader_up(ClickRecognizerRef ref, void *context) {
  if (s_review.open && !s_is_peek) { prv_browse(-1); return; }
  s_scroll -= READER_SCROLL_STEP;
  if (s_scroll < 0) s_scroll = 0;
  prv_update_scroll();
}

static void prv_reader_down(ClickRecognizerRef ref, void *context) {
  if (s_review.open && !s_is_peek) { prv_browse(1); return; }
  s_scroll += READER_SCROLL_STEP;
  if (s_scroll > s_scroll_max) s_scroll = s_scroll_max;
  prv_update_scroll();
}

JotReaderTouchLayout jot_reader_touch_layout(void) {
  JotReaderTouchLayout layout = {0};
  if (!jot_reader_is_open() || !s_chrome || !s_viewport || !s_text_layer)
    return layout;
  const GRect bounds = layer_get_bounds(s_chrome);
  layout.screen_width = bounds.size.w;
  layout.screen_height = bounds.size.h;
  layout.text_height = s_text_height;
  layout.scroll_y = s_scroll;
  layout.scroll_max = s_scroll_max;
  return layout;
}

void jot_reader_drag(int16_t finger_delta_y) {
  if (!jot_reader_is_open() || !s_viewport || !s_text_layer) return;
  const JotReaderTouchLayout layout = jot_reader_touch_layout();
  const int next = jot_reader_touch_scroll_after_drag(
      s_scroll, finger_delta_y, &layout);
  if (next == s_scroll) return;
  s_scroll = next;
  prv_update_scroll();
}

bool jot_reader_edit_current(void) {
  // Keep physical Select and touch text-edit on the exact same route.
  // A Peek is not an existing saved note and cannot become one here.
  if (!jot_reader_is_open() || s_is_peek || s_slot < 0) return false;
  if (!jot_reader_edit(s_slot)) {
    s_open_error = true;
    if (s_chrome) layer_mark_dirty(s_chrome);
    return false; // Leave the reader visible on a failed open.
  }
  // The edit origin and slot are retained by jot_reader_edit(). Popping
  // the reader reveals the keyboard, and completing an edit later
  // reconstructs a fresh reader using the committed note.
  window_stack_pop(true);
  return true;
}

static void prv_reader_select(ClickRecognizerRef ref, void *context) {
  if (s_is_peek) {
    jot_reader_back();
    return;
  }
  if (!s_action_bar) { jot_reader_edit_current(); return; }
  const JotReaderReviewAction action = jot_reader_review_select(&s_review);
  s_review_message = NULL;
  prv_mark_review();
  if (action == JOT_REVIEW_EDIT) jot_reader_edit_current();
  else if (action == JOT_REVIEW_CONFIRM_DELETE) prv_confirm_delete();
}

static void prv_reader_select_hold(ClickRecognizerRef ref, void *context) {
  if (s_is_peek || !s_action_bar) return; // Drafts are never deletable.
  jot_reader_review_hold(&s_review);
  s_review_message = NULL;
  prv_mark_review();
}

static void prv_reader_back(ClickRecognizerRef ref, void *context) {
  jot_reader_back();
}

static void prv_reader_clicks(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, prv_reader_up);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_reader_down);
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_reader_select);
  window_long_click_subscribe(BUTTON_ID_SELECT, 800,
                              prv_reader_select_hold, NULL);
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

  // The closed nub is opaque and unmarked. The expanded bar is native.
  if (!s_is_peek) {
    s_nub = layer_create(bounds);
    if (s_nub) {
      layer_set_update_proc(s_nub, prv_draw_nub);
      layer_add_child(window_get_root_layer(window), s_nub);
    }
    for (int i = 0; i < ICON_COUNT; ++i)
      s_icons[i] = gbitmap_create_with_resource(s_icon_ids[i]);
    s_action_bar = action_bar_layer_create();
    if (s_action_bar)
      action_bar_layer_set_click_config_provider(s_action_bar,
                                                  prv_reader_clicks);
  }
}

static void prv_reader_unload(Window *window) {
  if (s_action_bar_attached && s_action_bar)
    action_bar_layer_remove_from_window(s_action_bar);
  s_action_bar_attached = false;
  if (s_action_bar) action_bar_layer_destroy(s_action_bar);
  s_action_bar = NULL;
  for (int i = 0; i < ICON_COUNT; ++i) {
    if (s_icons[i]) gbitmap_destroy(s_icons[i]);
    s_icons[i] = NULL;
  }
  if (s_nub) layer_destroy(s_nub);
  s_nub = NULL;
  jot_reader_review_reset(&s_review);
  s_review_message = NULL;
  if (s_text_layer) text_layer_destroy(s_text_layer);
  s_text_layer = NULL;
  if (s_viewport) layer_destroy(s_viewport);
  s_viewport = NULL;
  if (s_paper) layer_destroy(s_paper);
  s_paper = NULL;
  if (s_chrome) layer_destroy(s_chrome);
  s_chrome = NULL;
  free(s_note);
  s_note = NULL;
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
  free(s_note);
  s_note = NULL;
}

bool jot_reader_show(int slot, int rank, int total) {
  if (!s_window || jot_reader_is_open() || slot < 0 ||
      rank < 0 || rank >= total) return false;
  s_note = malloc(JOT_NOTE_TEXT_BYTES);
  if (!s_note) return false;
  time_t created = 0;
  if (!jot_store_export(slot, s_note, JOT_NOTE_TEXT_BYTES, &created)) {
    free(s_note);
    s_note = NULL;
    return false;
  }
  const struct tm *tm = localtime(&created);
  if (tm) {
    strftime(s_timestamp, sizeof s_timestamp, "%b %d  %H:%M", tm);
  } else {
    snprintf(s_timestamp, sizeof s_timestamp, "Saved note");
  }
  s_is_peek = false;
  jot_reader_review_reset(&s_review);
  s_review_message = NULL;
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
  if (length >= JOT_NOTE_TEXT_BYTES) return false;
  s_note = malloc(JOT_NOTE_TEXT_BYTES);
  if (!s_note) return false;
  memcpy(s_note, jot_store_text(), length + 1);
  snprintf(s_timestamp, sizeof s_timestamp, "CURRENT NOTE");
  s_slot = -1;
  s_rank = 0;
  s_total = 0;
  s_scroll = 0;
  s_scroll_max = 0;
  s_open_error = false;
  s_is_peek = true;
  jot_reader_review_reset(&s_review);
  s_review_message = NULL;
  window_stack_push(s_window, true);
  if (!s_chrome || !s_paper || !s_viewport || !s_text_layer) {
    window_stack_pop(false);
    return false;
  }
  return true;
}

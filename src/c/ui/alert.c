#include <pebble.h>
#include "ui/alert.h"
#include "ui/caret_layout.h"

// The editor strip is a clipped viewport containing a TextLayer for the
// actual note and a separate, transparent 1px insertion-caret overlay.
// Diagnostics still use the same viewport without a caret.
static Layer *s_root;
static TextLayer *s_textbox;
static Layer *s_caret_layer;
static TextLayer *s_statusbox;
static char s_text[ALERT_MAX_LENGTH];
static bool s_editor_mode;
static bool s_placeholder;
static size_t s_caret_index;
static int s_caret_x, s_caret_y, s_caret_height;

enum {
  // Leave a visible notebook margin: 2px ink frame, 2px red rule at x=10,
  // then 6px of paper before the first glyph or insertion caret.
  EDITOR_PADDING_LEFT = 18,
  EDITOR_PADDING_RIGHT = 5,
  EDITOR_MARGIN_RULE_X = 10,
  LAYOUT_MAX_H = 4096,
  // Reserve a few real pixels under the font and caret: the text-layout
  // bounds can stop before the bottom of letters such as g, j, p and y.
  EDITOR_DESCENDER_GUARD = 5
};

// The editor uses the same bold 18pt family as our note cards.
// Caret widths, wrapped rows and visible scrolling all measure this
// exact font through prv_font(), never through an approximate size.
static GFont prv_font(void) {
  return fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
}

static GSize prv_measure(const char *text, GFont font, int width) {
  return graphics_text_layout_get_content_size(
      text, font, GRect(0, 0, width, LAYOUT_MAX_H),
      GTextOverflowModeWordWrap, GTextAlignmentLeft);
}

static void prv_background(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  const GColor ink = GColorFromRGB(41, 40, 43);
  // The ink frame is a filled 2px rim, stronger than a one-pixel
  // outline and visible on Emery. The inner paper face preserves the
  // entire 44px viewport; text and cursor are drawn on top unchanged.
  graphics_context_set_fill_color(ctx, ink);
  graphics_fill_rect(ctx, bounds, 5, GCornersAll);
  graphics_context_set_fill_color(ctx, GColorFromRGB(255, 245, 220));
  graphics_fill_rect(ctx, GRect(2, 1, bounds.size.w - 4,
                               bounds.size.h - 3), 4, GCornersAll);

  // Medium blue-gray ruled lines remain visible after Pebble's color
  // quantization. The red margin has 2px of saturated ink for contrast.
  graphics_context_set_stroke_color(ctx, GColorFromRGB(150, 172, 181));
  graphics_context_set_stroke_width(ctx, 1);
  for (int y = 19; y < bounds.size.h - 3; y += 18) {
    graphics_draw_line(ctx, GPoint(3, y),
                       GPoint(bounds.size.w - 4, y));
  }
  graphics_context_set_stroke_color(ctx, GColorFromRGB(231, 72, 58));
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, GPoint(EDITOR_MARGIN_RULE_X, 3),
                     GPoint(EDITOR_MARGIN_RULE_X, bounds.size.h - 5));

  // Subtle inner bottom edge gives the paper a little thickness while
  // keeping the caret and text region free of decorative overlays.
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(ctx, GColorFromRGB(188, 161, 124));
  graphics_draw_line(ctx, GPoint(4, bounds.size.h - 4),
                     GPoint(bounds.size.w - 5, bounds.size.h - 4));
}

static void prv_draw_caret(Layer *layer, GContext *ctx) {
  if (!s_editor_mode) return;
  const int height = layer_get_bounds(layer).size.h;
  if (s_caret_y >= height || s_caret_y + s_caret_height <= 0) return;
  graphics_context_set_stroke_color(ctx, GColorFromRGB(41, 40, 43));
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx,
                     GPoint(s_caret_x, s_caret_y),
                     GPoint(s_caret_x, s_caret_y + s_caret_height - 1));
}

// Use the very same Pebble font as the actual TextLayer for unwrapped word
// widths; the pure C helper handles line breaks and UTF-8 boundaries.
static int prv_caret_measure_width(const char *text, size_t first,
                                  size_t last, void *context) {
  if (last <= first) return 0;
  char piece[ALERT_MAX_LENGTH];
  size_t count = last - first;
  if (count >= sizeof piece) count = sizeof piece - 1;
  memcpy(piece, text + first, count);
  piece[count] = '\0';
  const GFont font = *(const GFont *)context;
  return prv_measure(piece, font, LAYOUT_MAX_H).w;
}

static void prv_reflow(void) {
  if (!s_root || !s_textbox) return;
  const GRect bounds = layer_get_bounds(s_root);
  const int width = bounds.size.w - EDITOR_PADDING_LEFT - EDITOR_PADDING_RIGHT;
  const GFont font = prv_font();

  text_layer_set_font(s_textbox, font);
  text_layer_set_overflow_mode(s_textbox, GTextOverflowModeWordWrap);
  text_layer_set_text(s_textbox, s_text);

  int height = bounds.size.h;
  int scroll_y = 0;
  s_caret_x = EDITOR_PADDING_LEFT;
  s_caret_y = 1;
  s_caret_height = 12;

  if (s_editor_mode) {
    const GSize size = prv_measure(s_text, font, width);
    if (size.h + 2 > height) height = size.h + 2;

    const int one_line_h = prv_measure("i", font, width).h;
    const int two_line_h = prv_measure("i\ni", font, width).h;
    int step = two_line_h - one_line_h;
    if (step < 8) step = 14; // Defensive fallback for missing font metrics.

    int caret_top = 1;
    if (!s_placeholder) {
      // Walk forward through whole words, explicit LF and overlong words,
      // tracking the actual displayed row containing the cursor. The old
      // reverse-suffix search could place 'ar' at the right of the screen
      // even when Pebble had wrapped it to a new row on the left.
      const JotCaretLayout caret = jot_caret_layout(
          s_text, s_caret_index, width, prv_caret_measure_width,
          (void *)&font);
      caret_top += caret.row * step;
      s_caret_x = EDITOR_PADDING_LEFT + caret.x;
    }
    if (s_caret_x >= bounds.size.w - EDITOR_PADDING_RIGHT)
      s_caret_x = bounds.size.w - EDITOR_PADDING_RIGHT - 1;
    // Descenders reach below the ink's ordinary baseline. The original
    // step-2 caret appeared to float above the tails of g/j/p/y on Emery.
    s_caret_height = step + 2;
    if (s_caret_height < 10) s_caret_height = 10;
    if (s_caret_height > 22) s_caret_height = 22;

    // The visual bottom must leave room for both the caret's full descent
    // and the font's descenders, which Pebble's measured height can clip
    // against the shared editor/keyboard boundary.
    const int caret_bottom =
        caret_top + s_caret_height + EDITOR_DESCENDER_GUARD;
    if (caret_bottom > height) height = caret_bottom;

    scroll_y = caret_bottom - bounds.size.h;
    if (scroll_y < 0) scroll_y = 0;
    if (scroll_y > height - bounds.size.h)
      scroll_y = height - bounds.size.h;
    s_caret_y = caret_top - scroll_y;
  }

  layer_set_frame(text_layer_get_layer(s_textbox),
                  GRect(EDITOR_PADDING_LEFT, -scroll_y, width, height));
  if (s_caret_layer) {
    layer_set_hidden(s_caret_layer, !s_editor_mode);
    layer_mark_dirty(s_caret_layer);
  }
}

static void prv_status(bool dirty, bool save_error, bool full) {
  if (!s_statusbox || !s_root) return;
  const char *label = save_error ? "SAVE ERROR" :
                      full ? "FULL" : dirty ? "*" : "";
  if (!*label) {
    layer_set_hidden(text_layer_get_layer(s_statusbox), true);
    return;
  }
  const GRect bounds = layer_get_bounds(s_root);
  const int width = save_error ? 84 : full ? 42 : 14;
  layer_set_frame(text_layer_get_layer(s_statusbox),
                  GRect(bounds.size.w - width - 2, 0, width, 19));
  text_layer_set_text(s_statusbox, label);
  text_layer_set_background_color(s_statusbox,
      save_error ? GColorFromRGB(132, 38, 39) :
      full ? GColorFromRGB(41, 40, 43) :
             GColorFromRGB(255, 250, 239));
  text_layer_set_text_color(s_statusbox,
      save_error || full ? GColorWhite : GColorFromRGB(41, 40, 43));
  layer_set_hidden(text_layer_get_layer(s_statusbox), false);
}

void alert_set_text(const char *new_text) {
  s_editor_mode = false;
  snprintf(s_text, sizeof s_text, "%s", new_text ? new_text : "");
  prv_status(false, false, false);
  prv_reflow();
}

void alert_set_editor(const char *note, size_t cursor,
                      bool dirty, bool save_error, bool full) {
  s_editor_mode = true;
  if (!note) note = "";
  const size_t length = strlen(note);
  if (cursor > length) cursor = length;

  // Copy the *unchanged* note. The caret is drawn over it in its own layer,
  // never inserted as a printable character or saved with the note.
  s_placeholder = !*note;
  if (s_placeholder) {
    snprintf(s_text, sizeof s_text, "Write something...");
    s_caret_index = 0;
  } else {
    snprintf(s_text, sizeof s_text, "%s", note);
    s_caret_index = cursor;
  }
  prv_reflow();
  prv_status(dirty, save_error, full);
}

Layer *alert_ui_init(GRect frame) {
  s_root = layer_create(frame);
  if (!s_root) return NULL;
  layer_set_update_proc(s_root, prv_background);
  layer_set_clips(s_root, true);  // Never paint over the gesture keyboard.

  s_textbox = text_layer_create(GRect(EDITOR_PADDING_LEFT, 0,
                                     frame.size.w - EDITOR_PADDING_LEFT -
                                     EDITOR_PADDING_RIGHT,
                                     frame.size.h));
  if (!s_textbox) {
    layer_destroy(s_root);
    s_root = NULL;
    return NULL;
  }
  text_layer_set_text_alignment(s_textbox, GTextAlignmentLeft);
  text_layer_set_text_color(s_textbox, GColorFromRGB(41, 40, 43));
  text_layer_set_background_color(s_textbox, GColorClear);
  layer_add_child(s_root, text_layer_get_layer(s_textbox));

  // The transparent overlay stays at the viewport origin while the TextLayer
  // scrolls under it; caret coordinates are already viewport-relative.
  s_caret_layer = layer_create(GRect(0, 0, frame.size.w, frame.size.h));
  if (s_caret_layer) {
    layer_set_update_proc(s_caret_layer, prv_draw_caret);
    layer_add_child(s_root, s_caret_layer);
  }

  s_statusbox = text_layer_create(GRect(frame.size.w - 16, 0, 14, 19));
  if (s_statusbox) {
    text_layer_set_font(s_statusbox,
                        fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_text_alignment(s_statusbox, GTextAlignmentCenter);
    layer_add_child(s_root, text_layer_get_layer(s_statusbox));
    layer_set_hidden(text_layer_get_layer(s_statusbox), true);
  }
  alert_set_text("Input text");
  return s_root;
}

void alert_ui_deinit(void) {
  if (s_statusbox) {
    text_layer_destroy(s_statusbox);
    s_statusbox = NULL;
  }
  if (s_caret_layer) {
    layer_destroy(s_caret_layer);
    s_caret_layer = NULL;
  }
  if (s_textbox) {
    text_layer_destroy(s_textbox);
    s_textbox = NULL;
  }
  if (s_root) {
    layer_destroy(s_root);
    s_root = NULL;
  }
}

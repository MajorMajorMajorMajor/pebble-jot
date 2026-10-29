#include <pebble.h>
#include "ui/alert.h"

// The 40px strip is a clipped viewport containing a TextLayer for the
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
  EDITOR_PADDING_X = 3,
  LAYOUT_MAX_H = 4096,
  // Reserve a few real pixels under the font and caret: the text-layout
  // bounds can stop before the bottom of letters such as g, j, p and y.
  EDITOR_DESCENDER_GUARD = 5
};

static GFont prv_font(void) {
  return fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
}

static GSize prv_measure(const char *text, GFont font, int width) {
  return graphics_text_layout_get_content_size(
      text, font, GRect(0, 0, width, LAYOUT_MAX_H),
      GTextOverflowModeWordWrap, GTextAlignmentLeft);
}

static void prv_background(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorFromRGB(255, 214, 109));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorFromRGB(41, 40, 43));
  graphics_fill_rect(ctx, GRect(0, bounds.size.h - 1, bounds.size.w, 1),
                     0, GCornerNone);
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

// Only navigate UTF-8 codepoint boundaries when finding visual line starts.
// Measure a portion of the note without altering the actual TextLayer data.
static GSize prv_measure_range(const char *text, size_t first, size_t last,
                               GFont font, int width) {
  char piece[ALERT_MAX_LENGTH];
  size_t length = last - first;
  if (length >= sizeof piece) length = sizeof piece - 1;
  memcpy(piece, text + first, length);
  piece[length] = '\0';
  return prv_measure(piece, font, width);
}

// Use the same Pebble font metrics as the word-wrapped TextLayer. Try
// whole-word boundaries first, then character boundaries when a single
// very long word requires wrapping. Notes are bounded at 159 bytes, so
// this search is small, and it runs only when an explicit line overflows.
static size_t prv_visual_line_start(const char *text, size_t cursor,
                                     GFont font, int width, int caret_h) {
  size_t logical_start = cursor;
  while (logical_start > 0 && text[logical_start - 1] != '\n')
    --logical_start;

  // Common case: the whole explicit line fits without soft wrapping.
  if (prv_measure_range(text, logical_start, cursor, font, LAYOUT_MAX_H).w
      <= width) return logical_start;

  for (size_t at = logical_start; at < cursor; ++at) {
    if (text[at] != ' ' && text[at] != '\t') continue;
    const size_t candidate = at + 1;
    if (prv_measure_range(text, candidate, cursor, font, LAYOUT_MAX_H).w
        > width) continue;
    if (prv_measure_range(text, 0, candidate, font, width).h < caret_h)
      return candidate;
  }

  size_t candidate = logical_start;
  while (candidate < cursor) {
    if (prv_measure_range(text, candidate, cursor, font, LAYOUT_MAX_H).w
        <= width &&
        prv_measure_range(text, 0, candidate, font, width).h < caret_h)
      return candidate;
    // Advance one codepoint, not one byte.
    size_t next = candidate + 1;
    while (next < cursor && ((unsigned char)text[next] & 0xC0) == 0x80)
      ++next;
    candidate = next;
  }
  return cursor;
}

static void prv_reflow(void) {
  if (!s_root || !s_textbox) return;
  const GRect bounds = layer_get_bounds(s_root);
  const int width = bounds.size.w - 2 * EDITOR_PADDING_X;
  const GFont font = prv_font();

  text_layer_set_font(s_textbox, font);
  text_layer_set_overflow_mode(s_textbox, GTextOverflowModeWordWrap);
  text_layer_set_text(s_textbox, s_text);

  int height = bounds.size.h;
  int scroll_y = 0;
  s_caret_x = EDITOR_PADDING_X;
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
      // When moving left across an explicit newline, the cursor lands
      // *before* that LF: it belongs at the end of the preceding line,
      // not on an extra row created by the measurement-only 'i' probe.
      const bool before_newline = s_text[s_caret_index] == '\n';
      size_t visual_end = s_caret_index;
      if (before_newline) {
        // Pebble's word wrapping can collapse trailing horizontal whitespace
        // at a hard break. Anchor the caret beside the last visible glyph,
        // rather than out in blank space to the right.
        while (visual_end > 0 &&
               (s_text[visual_end - 1] == ' ' ||
                s_text[visual_end - 1] == '\t')) --visual_end;
      }

      char through_caret[ALERT_MAX_LENGTH];
      memcpy(through_caret, s_text, visual_end);
      // An empty line before an LF still needs a probe to make the blank
      // line measurable; otherwise a preceding LF can vanish from sizing.
      const bool blank_before_lf =
          before_newline && visual_end > 0 &&
          s_text[visual_end - 1] == '\n';
      if (before_newline && !blank_before_lf) {
        // No probe here: one more glyph can falsely trigger a soft wrap
        // immediately before the explicit newline.
        through_caret[visual_end] = '\0';
      } else {
        // Probe fresh/empty lines without drawing an actual character.
        through_caret[visual_end] = 'i';
        through_caret[visual_end + 1] = '\0';
      }
      int measured_h = prv_measure(through_caret, font, width).h;
      if (measured_h < one_line_h) measured_h = one_line_h;
      caret_top = measured_h - one_line_h + 1;
      if (caret_top < 1) caret_top = 1;

      const size_t line_start = prv_visual_line_start(
          s_text, visual_end, font, width, measured_h);
      const GSize line_prefix = prv_measure_range(
          s_text, line_start, visual_end, font, LAYOUT_MAX_H);
      s_caret_x = EDITOR_PADDING_X + line_prefix.w;
    }
    if (s_caret_x >= bounds.size.w - 1) s_caret_x = bounds.size.w - 2;
    // Descenders reach below the ink's ordinary baseline. The original
    // step-2 caret appeared to float above the tails of g/j/p/y on Emery.
    s_caret_height = step + 2;
    if (s_caret_height < 10) s_caret_height = 10;
    if (s_caret_height > 22) s_caret_height = 22;

    // The visual bottom must leave room for both the caret's full descent
    // and the font's descenders, which Pebble's measured height can clip
    // against the fixed 40px keyboard boundary.
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
                  GRect(EDITOR_PADDING_X, -scroll_y, width, height));
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
             GColorFromRGB(255, 214, 109));
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
    snprintf(s_text, sizeof s_text, "  Write something...");
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

  s_textbox = text_layer_create(GRect(3, 0, frame.size.w - 6,
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

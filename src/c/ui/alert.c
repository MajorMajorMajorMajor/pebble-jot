#include <pebble.h>
#include "ui/alert.h"

// This 40-pixel strip is shared by the Jot editor and the older diagnostic
// harness. Use a clipping parent so a long note can scroll behind its edges,
// with the actual newlines and wrapping handled by Pebble's own text layout.
static Layer *s_root;
static TextLayer *s_textbox;
static TextLayer *s_statusbox;
static char s_text[ALERT_MAX_LENGTH];
static bool s_editor_mode;
static size_t s_caret_index;

static void prv_background(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorFromRGB(255, 214, 109));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorFromRGB(41, 40, 43));
  graphics_fill_rect(ctx, GRect(0, bounds.size.h - 1, bounds.size.w, 1),
                     0, GCornerNone);
}

static void prv_reflow(void) {
  if (!s_root || !s_textbox) return;
  const GRect bounds = layer_get_bounds(s_root);
  const int width = bounds.size.w - 6;
  const GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);

  text_layer_set_font(s_textbox, font);
  text_layer_set_overflow_mode(s_textbox, GTextOverflowModeWordWrap);
  text_layer_set_text(s_textbox, s_text);

  // Compute height for the entire note, but scroll to the insertion caret,
  // not invariably to the end of the note. A prefix ending in the drawn '|'
  // includes the caret's current line even after a trailing newline.
  int height = bounds.size.h;
  int offset_y = 0;
  if (s_editor_mode) {
    const GSize text_size = graphics_text_layout_get_content_size(
        s_text, font, GRect(0, 0, width, 4096),
        GTextOverflowModeWordWrap, GTextAlignmentLeft);
    if (text_size.h + 2 > height) height = text_size.h + 2;

    char through_caret[ALERT_MAX_LENGTH];
    size_t prefix_bytes = s_caret_index + 1;
    if (prefix_bytes >= sizeof through_caret)
      prefix_bytes = sizeof through_caret - 1;
    memcpy(through_caret, s_text, prefix_bytes);
    through_caret[prefix_bytes] = '\0';
    const GSize caret_size = graphics_text_layout_get_content_size(
        through_caret, font, GRect(0, 0, width, 4096),
        GTextOverflowModeWordWrap, GTextAlignmentLeft);
    offset_y = caret_size.h + 2 - bounds.size.h;
    if (offset_y < 0) offset_y = 0;
    const int max_offset = height - bounds.size.h;
    if (offset_y > max_offset) offset_y = max_offset;
  }
  layer_set_frame(text_layer_get_layer(s_textbox),
                  GRect(3, -offset_y, width, height));
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
  // Jot's note data is untouched. Insert the visual caret into a private
  // display buffer at a UTF-8 boundary. Both halves of the text stay visible
  // and editable, including explicit newlines in either half.
  const size_t length = strlen(note);
  if (cursor > length) cursor = length;
  if (!*note) {
    snprintf(s_text, sizeof s_text, "Write something...|");
    s_caret_index = strlen(s_text) - 1;
  } else if (length + 2 <= sizeof s_text) {
    memcpy(s_text, note, cursor);
    s_text[cursor] = '|';
    memcpy(s_text + cursor + 1, note + cursor, length - cursor + 1);
    s_caret_index = cursor;
  } else {
    // Defensive fallback; 159-byte Jot notes always fit.
    snprintf(s_text, sizeof s_text, "%s", note);
    s_caret_index = strlen(s_text);
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
  if (s_textbox) {
    text_layer_destroy(s_textbox);
    s_textbox = NULL;
  }
  if (s_root) {
    layer_destroy(s_root);
    s_root = NULL;
  }
}

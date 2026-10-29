#pragma once
#include <stdbool.h>
#include <stdint.h>

// Reader touch coordinates are measured in full-screen Pebble pixels.
// The header, timestamp, footer and blank paper are NOT editing targets.
enum {
  JOT_READER_HEADER_H = 42,
  JOT_READER_BACK_W = 42,
  JOT_READER_META_H = 33,
  JOT_READER_FOOTER_H = 30,
  JOT_READER_TEXT_PADDING = 12,
  JOT_READER_TEXT_TOP_INSET = 2,
  JOT_READER_TAP_SLOP = 10,
  JOT_READER_TAP_MAX_MS = 700
};

typedef struct {
  int screen_width;
  int screen_height;
  int text_height;
  int scroll_y;
} JotReaderTouchLayout;

typedef enum {
  JOT_READER_TOUCH_NONE = 0,
  JOT_READER_TOUCH_BACK,
  JOT_READER_TOUCH_DISMISS_PEEK,
  JOT_READER_TOUCH_EDIT
} JotReaderTouchAction;

// For a saved-note reader: short taps inside the Back rectangle exit;
// short taps that START AND END inside the visible text area edit.
// In a draft peek, ordinary stationary taps simply return to the draft.
// No content-region edit action is ever emitted for a peek.
//
// 'moved' is sticky across the whole gesture: moving away then back is
// never mistaken for a tap. layout may be NULL, in which case content
// taps do nothing but Back and Peek still work.
JotReaderTouchAction jot_reader_touch_action(bool peek, int start_x,
    int start_y, int end_x, int end_y, bool moved, uint32_t duration_ms,
    const JotReaderTouchLayout *layout);

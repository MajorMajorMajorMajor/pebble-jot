#pragma once
#include <stdbool.h>
#include <stdint.h>

// Reader touch coordinates are measured in full-screen Pebble pixels.
// The whole 42px header height is a Back region only at the left edge;
// tapping the note or date must not exit or start editing accidentally.
enum {
  JOT_READER_HEADER_H = 42,
  JOT_READER_BACK_W = 42,
  JOT_READER_TAP_SLOP = 10,
  JOT_READER_TAP_MAX_MS = 700
};

typedef enum {
  JOT_READER_TOUCH_NONE = 0,
  JOT_READER_TOUCH_BACK,
  JOT_READER_TOUCH_DISMISS_PEEK
} JotReaderTouchAction;

// For a saved-note reader, only a short stationary tap starting AND ending
// within the Back target may exit. In an editor draft peek, any stationary
// tap may dismiss the preview back to the same unsaved draft.
// 'moved' is sticky across the entire gesture: moving away then back is
// never mistaken for a tap.
JotReaderTouchAction jot_reader_touch_action(bool peek, int start_x,
    int start_y, int end_x, int end_y, bool moved, uint32_t duration_ms);

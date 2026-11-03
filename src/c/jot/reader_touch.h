#pragma once
#include <stdbool.h>
#include <stdint.h>

// Reader touch coordinates are measured in full-screen Pebble pixels.
// The entire visible notebook paper is an edit target, including timestamp,
// margins and blank space; the header and footer are separate regions.
enum {
  JOT_READER_HEADER_H = 42,
  JOT_READER_BACK_W = 42, // Fallback only when layout is unavailable.
  JOT_READER_HEADER_BADGE_SPACE = 67, // Reserved count/DRAFT badge at right.
  JOT_READER_META_H = 33,
  JOT_READER_FOOTER_H = 30,
  JOT_READER_TEXT_PADDING = 12,
  JOT_READER_TEXT_TOP_INSET = 2,
  JOT_READER_TAP_SLOP = 10,
  JOT_READER_TAP_MAX_MS = 700
};

typedef struct {
  int screen_width; // Visible paper width: excludes native action bar if open.
  int screen_height;
  int text_height;
  int scroll_y;
  int scroll_max; // Measured scroll limit from the live reader viewport.
} JotReaderTouchLayout;

typedef enum {
  JOT_READER_TOUCH_NONE = 0,
  JOT_READER_TOUCH_BACK,
  JOT_READER_TOUCH_DISMISS_PEEK,
  JOT_READER_TOUCH_EDIT
} JotReaderTouchAction;

// For a saved-note reader: short taps across the full-height left HEADER
// (arrow + NOTES/EDIT label) go Back. The badge at right remains inactive.
// Taps that START AND END anywhere on the visible paper edit, including
// timestamp, empty paper and side margins, but not header/footer/action bar.
// In a draft Peek, ordinary stationary taps simply return to the draft.
// No content-region edit action is ever emitted for a peek.
//
// 'moved' is sticky across the whole gesture: moving away then back is
// never mistaken for a tap. layout may be NULL, in which case content
// taps do nothing but Back and Peek still work.
JotReaderTouchAction jot_reader_touch_action(bool peek, int start_x,
    int start_y, int end_x, int end_y, bool moved, uint32_t duration_ms,
    const JotReaderTouchLayout *layout);

// Only gestures starting in the clipped body viewport may scroll. The
// header, timestamp and footer remain fixed. Blank paper in the viewport
// is both a drag surface and (for short stationary taps) an edit target.
bool jot_reader_touch_can_drag(int start_x, int start_y,
                               const JotReaderTouchLayout *layout);

// Pixel-for-pixel finger tracking (finger up -> content down the page).
// Clamps both ends, including overscroll and notes shorter than the screen.
int jot_reader_touch_scroll_after_drag(int scroll_y, int finger_delta_y,
                                       const JotReaderTouchLayout *layout);

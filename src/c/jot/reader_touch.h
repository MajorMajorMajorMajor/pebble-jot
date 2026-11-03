#pragma once
#include <stdbool.h>
#include <stdint.h>

// Full-screen Pebble touch coordinates. The header and timestamp scroll
// out of view with the note; the footer stays fixed. Entire visible paper
// is an edit target, except where the moving yellow header is still shown.
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
  int scroll_y; // Current offset of the ENTIRE document, including header.
  int scroll_max; // Measured limit against full-height paper viewport.
} JotReaderTouchLayout;

typedef enum {
  JOT_READER_TOUCH_NONE = 0,
  JOT_READER_TOUCH_BACK,
  JOT_READER_TOUCH_DISMISS_PEEK,
  JOT_READER_TOUCH_EDIT
} JotReaderTouchAction;

// Back hitbox moves up with the header and disappears when it does.
// The badge remains inactive while visible. Start/end taps on the exposed
// paper, including the vacated top area, edit the saved note; never footer/bar.
// In a draft Peek, ordinary stationary taps simply return to the draft.
// No content-region edit action is ever emitted for a peek.
//
// 'moved' is sticky across the whole gesture: moving away then back is
// never mistaken for a tap. layout may be NULL, in which case content
// taps do nothing but Back and Peek still work.
JotReaderTouchAction jot_reader_touch_action(bool peek, int start_x,
    int start_y, int end_x, int end_y, bool moved, uint32_t duration_ms,
    const JotReaderTouchLayout *layout);

// Drag anywhere on the paper (including the visible header/timestamp)
// to scroll. Footer and native action bar cannot start a paper drag.
bool jot_reader_touch_can_drag(int start_x, int start_y,
                               const JotReaderTouchLayout *layout);

// Pixel-for-pixel finger tracking (finger up -> content down the page).
// Clamps both ends, including overscroll and notes shorter than the screen.
int jot_reader_touch_scroll_after_drag(int scroll_y, int finger_delta_y,
                                       const JotReaderTouchLayout *layout);

#include "jot/reader_touch.h"

static int prv_abs(int n) { return n < 0 ? -n : n; }

static bool prv_back_hit(int x, int y) {
  return x >= 0 && x < JOT_READER_BACK_W &&
         y >= 0 && y < JOT_READER_HEADER_H;
}

JotReaderTouchAction jot_reader_touch_action(bool peek, int start_x,
    int start_y, int end_x, int end_y, bool moved, uint32_t duration_ms) {
  if (moved || duration_ms >= JOT_READER_TAP_MAX_MS ||
      prv_abs(end_x - start_x) > JOT_READER_TAP_SLOP ||
      prv_abs(end_y - start_y) > JOT_READER_TAP_SLOP)
    return JOT_READER_TOUCH_NONE;

  if (prv_back_hit(start_x, start_y) &&
      prv_back_hit(end_x, end_y)) return JOT_READER_TOUCH_BACK;
  if (peek) return JOT_READER_TOUCH_DISMISS_PEEK;
  return JOT_READER_TOUCH_NONE;
}

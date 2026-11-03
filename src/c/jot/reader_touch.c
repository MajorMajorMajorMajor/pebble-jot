#include "jot/reader_touch.h"

static int prv_abs(int n) { return n < 0 ? -n : n; }

// The painted NOTES/EDIT context label belongs to Back too. Reserve the
// right-hand count/DRAFT badge, and respect the reduced paper width when the
// native action bar is open. Missing layout retains the historical arrow hit.
static bool prv_back_hit(int x, int y, const JotReaderTouchLayout *layout) {
  const int reach = layout && layout->screen_width > 0 ?
                    layout->screen_width - JOT_READER_HEADER_BADGE_SPACE :
                    JOT_READER_BACK_W;
  return x >= 0 && x < reach &&
         y >= 0 && y < JOT_READER_HEADER_H;
}

// Whole-paper hit: unlike the old TextLayer-only area, this includes
// timestamp, margin and unused blank notebook space. Header, footer and
// action-bar column are intentionally excluded.
static bool prv_paper_hit(int x, int y,
                          const JotReaderTouchLayout *layout) {
  return layout && layout->screen_width > 0 &&
         layout->screen_height > JOT_READER_HEADER_H + JOT_READER_FOOTER_H &&
         x >= 0 && x < layout->screen_width &&
         y >= JOT_READER_HEADER_H &&
         y < layout->screen_height - JOT_READER_FOOTER_H;
}

JotReaderTouchAction jot_reader_touch_action(bool peek, int start_x,
    int start_y, int end_x, int end_y, bool moved, uint32_t duration_ms,
    const JotReaderTouchLayout *layout) {
  if (moved || duration_ms >= JOT_READER_TAP_MAX_MS ||
      prv_abs(end_x - start_x) > JOT_READER_TAP_SLOP ||
      prv_abs(end_y - start_y) > JOT_READER_TAP_SLOP)
    return JOT_READER_TOUCH_NONE;

  if (prv_back_hit(start_x, start_y, layout) &&
      prv_back_hit(end_x, end_y, layout)) return JOT_READER_TOUCH_BACK;
  if (peek) return JOT_READER_TOUCH_DISMISS_PEEK;
  if (prv_paper_hit(start_x, start_y, layout) &&
      prv_paper_hit(end_x, end_y, layout))
    return JOT_READER_TOUCH_EDIT;
  return JOT_READER_TOUCH_NONE;
}

bool jot_reader_touch_can_drag(int x, int y,
                                const JotReaderTouchLayout *layout) {
  if (!layout || layout->scroll_max <= 0 || layout->screen_width <= 0 ||
      layout->screen_height <= JOT_READER_HEADER_H +
                               JOT_READER_META_H +
                               JOT_READER_FOOTER_H) return false;
  const int viewport_top = JOT_READER_HEADER_H + JOT_READER_META_H;
  const int viewport_bottom = layout->screen_height - JOT_READER_FOOTER_H;
  return x >= 0 && x < layout->screen_width &&
         y >= viewport_top && y < viewport_bottom;
}

int jot_reader_touch_scroll_after_drag(int scroll_y, int finger_delta_y,
                                       const JotReaderTouchLayout *layout) {
  if (!layout || layout->scroll_max <= 0) return 0;
  // During an active drag the content follows the finger exactly.
  // A separate shared kinetic model takes over only after liftoff.
  const int proposed = scroll_y - finger_delta_y;
  if (proposed < 0) return 0;
  if (proposed > layout->scroll_max) return layout->scroll_max;
  return proposed;
}

// List Back follows the visual scroll offset, never the selected note.
#include <assert.h>
#include <stdio.h>
#include "jot/list_focus.h"
#include "jot/list_touch.h"

static void top_without_scroll(void) {
  const int screen_height = 228;
  // Note #2 remains visible in the initial 3-card viewport.
  assert(jot_list_touch_scroll_for_focus(1, 8, screen_height) == 0);
  assert(!jot_list_back_should_scroll(0));
  assert(!jot_list_back_should_scroll(-1)); // Defensive invalid offset.
  assert(jot_list_focus_initial(-1, 8) == 0);
  assert(jot_list_focus_initial(-1, 0) == JOT_LIST_FOCUS_NEW);
  // Previously these non-first selections or NEW required extra Back.
  assert(jot_list_focus_down(0, 8) == 1);
  assert(!jot_list_back_should_scroll(
      jot_list_touch_scroll_for_focus(1, 8, screen_height)));
  assert(!jot_list_back_should_scroll(
      jot_list_touch_scroll_for_focus(JOT_LIST_FOCUS_NEW, 8, screen_height)));
  assert(!jot_list_back_should_scroll(
      jot_list_touch_scroll_for_focus(0, 1, screen_height)));
  assert(!jot_list_back_should_scroll(
      jot_list_touch_scroll_for_focus(JOT_LIST_FOCUS_NEW, 0, screen_height)));
}

static void scrolled_list_returns_to_top(void) {
  const int screen_height = 228;
  const int count = 8;
  const int row_height = jot_list_touch_row_height(screen_height);
  assert(row_height > 0);
  // The selected item, whether by buttons or touch, is irrelevant.
  const int focused_scroll =
      jot_list_touch_scroll_for_focus(3, count, screen_height);
  assert(focused_scroll > 0);
  assert(jot_list_back_should_scroll(focused_scroll));
  // A fractional touch drag is also a real scroll, even if the newest
  // card still looks mostly visible (no rank-based shortcut).
  assert(jot_list_back_should_scroll(
      jot_list_touch_clamp_scroll(1, count, screen_height)));
  assert(jot_list_back_should_scroll(
      jot_list_touch_clamp_scroll(row_height / 2, count, screen_height)));
  // After the first Back the viewport is at the true top, and the next
  // Back exits. Re-focus on newest only on that first scroll-home step.
  const int at_top = 0;
  assert(jot_list_focus_initial(0, count) == 0);
  assert(!jot_list_back_should_scroll(at_top));
  // On short/empty lists, scrolling can't be active in the first place.
  assert(jot_list_touch_clamp_scroll(80, 2, screen_height) == 0);
  assert(jot_list_touch_clamp_scroll(80, 0, screen_height) == 0);
}

int main(void) {
  top_without_scroll();
  scrolled_list_returns_to_top();
  puts("Note list Back: visual-top exit, scrolled return, touch/button focus passed");
  return 0;
}

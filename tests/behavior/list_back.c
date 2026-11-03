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

static void expanded_new_hit_targets(void) {
  // Header action extends beyond the drawn +NEW pill while staying clear
  // of the Jot brand and all card rows. Edges are exact half-open bounds.
  assert(jot_list_touch_new_hit(96, 0, 200));
  assert(jot_list_touch_new_hit(199, 42, 200));
  assert(jot_list_touch_new_hit(105, 39, 200));
  assert(!jot_list_touch_new_hit(95, 20, 200));
  assert(!jot_list_touch_new_hit(200, 20, 200));
  assert(!jot_list_touch_new_hit(100, 43, 200));
  assert(!jot_list_touch_new_hit(100, -1, 200));
  assert(!jot_list_touch_new_hit(100, 228, 200));
  // Originating inside a card's 43+ region is never a NEW hit.
  assert(!jot_list_touch_new_hit(120, 46, 200));
  assert(jot_list_touch_hit_card(120, 46, 200, 228, 5, 0) == 0);
}

int main(void) {
  expanded_new_hit_targets();
  top_without_scroll();
  scrolled_list_returns_to_top();
  puts("Note list Back: visual-top exit, scrolled return, touch/button focus passed");
  return 0;
}

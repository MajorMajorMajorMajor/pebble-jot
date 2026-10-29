// Host-only geometry tests for clipped, touch-scrolling note cards.
// Compile:
// cc -std=c11 -Wall -Wextra -Werror -I src/c
//   tests/list-touch-test.c src/c/jot/list_touch.c -o /tmp/jot-list-touch-test
// /tmp/jot-list-touch-test
#include <assert.h>
#include <stdio.h>
#include "jot/list_touch.h"

static void test_layout(void) {
  const int H = 228; // Pebble Time 2
  assert(jot_list_touch_row_height(H) == 52);
  assert(jot_list_touch_max_scroll(0, H) == 0);
  assert(jot_list_touch_max_scroll(1, H) == 0);
  assert(jot_list_touch_max_scroll(3, H) == 0);
  assert(jot_list_touch_max_scroll(4, H) == 52);
  assert(jot_list_touch_max_scroll(16, H) == 676);
  assert(jot_list_touch_clamp_scroll(-100, 16, H) == 0);
  assert(jot_list_touch_clamp_scroll(123, 16, H) == 123);
  assert(jot_list_touch_clamp_scroll(9999, 16, H) == 676);

  // Buttons keep the focus near the middle, one note at a time.
  assert(jot_list_touch_scroll_for_focus(-1, 16, H) == 0);
  assert(jot_list_touch_scroll_for_focus(0, 16, H) == 0);
  assert(jot_list_touch_scroll_for_focus(1, 16, H) == 0);
  assert(jot_list_touch_scroll_for_focus(2, 16, H) == 52);
  assert(jot_list_touch_scroll_for_focus(15, 16, H) == 676);
  assert(jot_list_touch_scroll_for_focus(2, 2, H) == 0);

  // When touch browsing ends, *either* Up or Down must focus the
  // topmost fully visible card, not the middle or a clipped top card.
  // The card itself begins 3px into its row, so the first 3 pixels of
  // scrolling still leave the first card fully visible.
  assert(jot_list_touch_first_full_card(0, 16, H) == 0);
  assert(jot_list_touch_first_full_card(1, 16, H) == 0);
  assert(jot_list_touch_first_full_card(3, 16, H) == 0);
  assert(jot_list_touch_first_full_card(4, 16, H) == 1);
  assert(jot_list_touch_first_full_card(26, 16, H) == 1);
  assert(jot_list_touch_first_full_card(52, 16, H) == 1);
  assert(jot_list_touch_first_full_card(55, 16, H) == 1);
  assert(jot_list_touch_first_full_card(56, 16, H) == 2);
  assert(jot_list_touch_first_full_card(75, 16, H) == 2);
  assert(jot_list_touch_first_full_card(676, 16, H) == 13);
  assert(jot_list_touch_first_full_card(9999, 16, H) == 13);

  // Empty and short lists still return appropriate button focus.
  assert(jot_list_touch_first_full_card(30, 0, H) == -1);
  assert(jot_list_touch_first_full_card(0, 1, H) == 0);
  assert(jot_list_touch_first_full_card(300, 2, H) == 0);
  assert(jot_list_touch_first_full_card(5, 3, H) == 1);
  assert(jot_list_touch_first_full_card(30, 4, H) == 1);
  assert(jot_list_touch_first_full_card(52, 4, H) == 1);
}

static void test_hits(void) {
  const int W = 200, H = 228, N = 6;
  // The first drawn card occupies x 7..192, y 46..91.
  assert(jot_list_touch_hit_card(7, 46, W, H, N, 0) == 0);
  assert(jot_list_touch_hit_card(192, 91, W, H, N, 0) == 0);
  assert(jot_list_touch_hit_card(6, 60, W, H, N, 0) == -1);
  assert(jot_list_touch_hit_card(193, 60, W, H, N, 0) == -1);
  assert(jot_list_touch_hit_card(40, 43, W, H, N, 0) == -1);
  assert(jot_list_touch_hit_card(40, 45, W, H, N, 0) == -1);
  assert(jot_list_touch_hit_card(40, 92, W, H, N, 0) == -1);
  assert(jot_list_touch_hit_card(40, 93, W, H, N, 0) == -1);
  assert(jot_list_touch_hit_card(40, 98, W, H, N, 0) == 1);
  assert(jot_list_touch_hit_card(40, 150, W, H, N, 0) == 2);
  assert(jot_list_touch_hit_card(40, 198, W, H, N, 0) == -1);
  assert(jot_list_touch_hit_card(40, 199, W, H, N, 0) == -1);
  assert(jot_list_touch_hit_card(150, 20, W, H, N, 0) == -1);

  // A partial-pixel scroll changes which card is under the finger.
  assert(jot_list_touch_hit_card(40, 50, W, H, N, 26) == 0);
  assert(jot_list_touch_hit_card(40, 68, W, H, N, 26) == -1);
  assert(jot_list_touch_hit_card(40, 66, W, H, N, 26) == -1);
  assert(jot_list_touch_hit_card(40, 73, W, H, N, 26) == 1);
  // At the bottom of a six-note list, last card is tappable.
  assert(jot_list_touch_hit_card(40, 180, W, H, N, 156) == 5);

  // Empty lists, one-note lists and unfilled paper must not open a note.
  assert(jot_list_touch_hit_card(40, 60, W, H, 0, 0) == -1);
  assert(jot_list_touch_hit_card(40, 160, W, H, 1, 0) == -1);
  assert(jot_list_touch_hit_card(40, 110, W, H, 1, 0) == -1);
}

static void test_short_viewport(void) {
  assert(jot_list_touch_row_height(74) == 0);
  assert(jot_list_touch_first_full_card(0, 10, 74) == -1);
  assert(jot_list_touch_max_scroll(10, 74) == 0);
  assert(jot_list_touch_hit_card(50, 44, 200, 74, 10, 0) == -1);
}

int main(void) {
  test_layout();
  test_hits();
  test_short_viewport();
  puts("Touch list geometry tests passed");
  return 0;
}

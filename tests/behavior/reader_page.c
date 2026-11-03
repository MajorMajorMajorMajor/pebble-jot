// Continuous-paper Reader geometry, scroll limits and back-navigation.
#include <assert.h>
#include <stdio.h>
#include "jot/reader_page.h"

static void continuous_paper_geometry(void) {
  assert(jot_reader_page_view_height(228) == 198);
  assert(jot_reader_page_view_height(30) == 0);
  assert(jot_reader_page_text_top(0) == 77);
  assert(jot_reader_page_text_top(42) == 35);
  assert(jot_reader_page_text_top(75) == 2);
  assert(jot_reader_page_text_top(100) == -23);
  assert(jot_reader_page_visible_header(0) == 42);
  assert(jot_reader_page_visible_header(20) == 22);
  assert(jot_reader_page_visible_header(41) == 1);
  assert(jot_reader_page_visible_header(42) == 0);
  assert(jot_reader_page_visible_header(100) == 0);

  // The ruled lines travel with the text, never with the screen.
  assert(jot_reader_page_first_rule(0) == 93);
  assert(jot_reader_page_first_rule(20) == 73);
  assert(jot_reader_page_first_rule(75) == 18);
  assert(jot_reader_page_first_rule(92) == 1);
  assert(jot_reader_page_first_rule(93) == 0);
  assert(jot_reader_page_first_rule(94) == 29);
  assert(jot_reader_page_first_rule(123) == 0);
  assert(jot_reader_page_first_rule(124) == 29);
}

static void scroll_limits(void) {
  // In the older fixed-header reader this note ended at 181px scroll:
  // 300 + 4 - 123 = 181. Now the 75px chrome and 75px more usable area
  // cancel, so the end of the note stays at exactly the same bottom.
  assert(jot_reader_page_scroll_limit(300, 198) == 181);
  assert(jot_reader_page_scroll_limit(60, 198) == 0);
  assert(jot_reader_page_scroll_limit(300, 90) == 289);
  assert(jot_reader_page_scroll_limit(30, 198) == 0);
  assert(jot_reader_page_scroll_limit(0, 198) == 0);
  assert(!jot_reader_page_back_to_top(0));
  assert(!jot_reader_page_back_to_top(-1));
  assert(jot_reader_page_back_to_top(1));
  assert(jot_reader_page_back_to_top(181));
}
int main(void) {
  continuous_paper_geometry();
  scroll_limits();
  puts("Reader page: 75px liberated, synchronized paper rules, scroll limits, Back-to-top passed");
  return 0;
}

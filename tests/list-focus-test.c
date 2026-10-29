// Run with:
// cc -std=c11 -Wall -Wextra -Werror -I src/c \
//   tests/list-focus-test.c src/c/jot/list_focus.c -o /tmp/jot-list-focus-test
// /tmp/jot-list-focus-test
#include <assert.h>
#include <stdio.h>
#include "jot/list_focus.h"

int main(void) {
  // New notes list always opens on the saved/newest note, not NEW.
  assert(jot_list_focus_initial(0, 15) == 0);
  assert(jot_list_focus_initial(4, 15) == 4);
  assert(jot_list_focus_initial(99, 15) == 0);
  assert(jot_list_focus_initial(-1, 15) == 0);
  assert(jot_list_focus_initial(0, 0) == JOT_LIST_FOCUS_NEW);

  // The top note has an actual selectable action immediately above it.
  assert(jot_list_focus_up(4) == 3);
  assert(jot_list_focus_up(1) == 0);
  assert(jot_list_focus_up(0) == JOT_LIST_FOCUS_NEW);
  assert(jot_list_focus_up(JOT_LIST_FOCUS_NEW) == JOT_LIST_FOCUS_NEW);
  assert(jot_list_focus_down(JOT_LIST_FOCUS_NEW, 15) == 0);
  assert(jot_list_focus_down(0, 15) == 1);
  assert(jot_list_focus_down(14, 15) == 14);
  assert(jot_list_focus_down(JOT_LIST_FOCUS_NEW, 0) ==
         JOT_LIST_FOCUS_NEW);

  // Back from older notes returns to top; at top exits. Back from NEW
  // returns to the top note when notes exist, or exits when empty.
  assert(jot_list_focus_back(8, 15) == 0);
  assert(jot_list_focus_back(0, 15) == JOT_LIST_FOCUS_EXIT);
  assert(jot_list_focus_back(JOT_LIST_FOCUS_NEW, 15) == 0);
  assert(jot_list_focus_back(JOT_LIST_FOCUS_NEW, 0) ==
         JOT_LIST_FOCUS_EXIT);

  // After deleting the final note, NEW becomes the only focusable item.
  assert(jot_list_focus_initial(0, 0) == JOT_LIST_FOCUS_NEW);
  assert(jot_list_focus_down(JOT_LIST_FOCUS_NEW, 0) ==
         JOT_LIST_FOCUS_NEW);
  // After deleting a middle item, stay on that new current rank.
  assert(jot_list_focus_initial(5, 7) == 5);
  assert(jot_list_focus_initial(7, 7) == 0);

  puts("List NEW-focus navigation tests passed");
  return 0;
}

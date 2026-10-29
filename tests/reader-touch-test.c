// Pebble-independent tests for contextual reader Back touch behavior.
// Build: cc -std=c11 -Wall -Wextra -Werror -I src/c
//   tests/reader-touch-test.c src/c/jot/reader_touch.c -o /tmp/jot-reader-touch-test
// /tmp/jot-reader-touch-test
#include <assert.h>
#include <stdio.h>
#include "jot/reader_touch.h"

static JotReaderTouchAction action(bool peek,
    int sx, int sy, int ex, int ey, bool moved, unsigned elapsed) {
  return jot_reader_touch_action(peek, sx, sy, ex, ey, moved, elapsed);
}

int main(void) {
  // Normal saved-note readers only accept Back at the upper-left header.
  assert(action(false, 20, 20, 20, 20, false, 100)
         == JOT_READER_TOUCH_BACK);
  assert(action(false, 0, 0, 2, 4, false, 80)
         == JOT_READER_TOUCH_BACK);
  assert(action(false, 41, 41, 40, 39, false, 120)
         == JOT_READER_TOUCH_BACK);
  assert(action(false, 42, 20, 42, 20, false, 100)
         == JOT_READER_TOUCH_NONE);
  assert(action(false, 20, 42, 20, 42, false, 100)
         == JOT_READER_TOUCH_NONE);
  assert(action(false, 80, 20, 80, 20, false, 100)
         == JOT_READER_TOUCH_NONE);
  assert(action(false, 20, 85, 20, 85, false, 100)
         == JOT_READER_TOUCH_NONE); // Reader text reserved for #23.
  assert(action(false, 20, 0, 20, 42, false, 100)
         == JOT_READER_TOUCH_NONE); // End outside Back target.
  assert(action(false, 45, 20, 20, 20, false, 100)
         == JOT_READER_TOUCH_NONE); // Start outside Back target.

  // Touch can tolerate a small stationary movement, but never a swipe.
  assert(action(false, 10, 10, 20, 20, false, 699)
         == JOT_READER_TOUCH_BACK);
  assert(action(false, 10, 10, 21, 10, false, 200)
         == JOT_READER_TOUCH_NONE);
  assert(action(false, 10, 10, 10, 21, false, 200)
         == JOT_READER_TOUCH_NONE);
  assert(action(false, 10, 10, 10, 10, true, 100)
         == JOT_READER_TOUCH_NONE); // Sticky movement: left then returned.
  assert(action(false, 10, 10, 10, 10, false, 700)
         == JOT_READER_TOUCH_NONE); // Long hold cannot activate Back.
  assert(action(false, 10, 10, 10, 10, false, 1500)
         == JOT_READER_TOUCH_NONE);

  // Peek always returns to its *unsaved editor draft*, never the note list.
  // A deliberate tap anywhere in Peek still works, including the content.
  assert(action(true, 20, 20, 20, 20, false, 100)
         == JOT_READER_TOUCH_BACK);
  assert(action(true, 80, 20, 80, 20, false, 100)
         == JOT_READER_TOUCH_DISMISS_PEEK);
  assert(action(true, 160, 100, 160, 100, false, 100)
         == JOT_READER_TOUCH_DISMISS_PEEK);
  assert(action(true, 160, 100, 160, 100, true, 100)
         == JOT_READER_TOUCH_NONE);
  assert(action(true, 160, 100, 160, 100, false, 900)
         == JOT_READER_TOUCH_NONE);
  puts("Contextual reader Back touch tests passed");
  return 0;
}

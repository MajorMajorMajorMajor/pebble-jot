// Pebble-independent touch tests for contextual reader Back and tap-to-edit.
// Build with cc -std=c11 -Wall -Wextra -Werror -I src/c, linking reader_touch.c
#include <assert.h>
#include <stdio.h>
#include "jot/reader_touch.h"

// Pebble Time 2 is 200x228. The 60px measured text occupies y=77..136;
// the text viewport occupies y=75..197, leaving blank paper below.
static const JotReaderTouchLayout short_note = {200, 228, 60, 0, 0};
static const JotReaderTouchLayout long_scrolled = {200, 228, 300, 160, 181};
static const JotReaderTouchLayout empty_layout = {0, 0, 0, 0, 0};

static JotReaderTouchAction action_with_layout(
    bool peek, int sx, int sy, int ex, int ey, bool moved,
    unsigned elapsed, const JotReaderTouchLayout *layout) {
  return jot_reader_touch_action(
      peek, sx, sy, ex, ey, moved, elapsed, layout);
}

static JotReaderTouchAction action(bool peek, int sx, int sy,
    int ex, int ey, bool moved, unsigned elapsed) {
  return action_with_layout(
      peek, sx, sy, ex, ey, moved, elapsed, &short_note);
}

static void test_back(void) {
  // Saved readers only accept Back within the 42x42 header target.
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
  assert(action(false, 20, 0, 20, 42, false, 100)
         == JOT_READER_TOUCH_NONE); // End outside Back target.
  assert(action(false, 45, 20, 20, 20, false, 100)
         == JOT_READER_TOUCH_NONE); // Start outside Back target.

  // Small stationary movement is fine, but never a swipe or long hold.
  assert(action(false, 10, 10, 20, 20, false, 699)
         == JOT_READER_TOUCH_BACK);
  assert(action(false, 10, 10, 21, 10, false, 200)
         == JOT_READER_TOUCH_NONE);
  assert(action(false, 10, 10, 10, 21, false, 200)
         == JOT_READER_TOUCH_NONE);
  assert(action(false, 10, 10, 10, 10, true, 100)
         == JOT_READER_TOUCH_NONE); // Swipe away then back.
  assert(action(false, 10, 10, 10, 10, false, 700)
         == JOT_READER_TOUCH_NONE);
}

static void test_saved_text(void) {
  // Touch within a visible line of note text opens editing directly.
  assert(action(false, 12, 77, 12, 77, false, 100)
         == JOT_READER_TOUCH_EDIT);
  assert(action(false, 187, 136, 187, 136, false, 100)
         == JOT_READER_TOUCH_EDIT);
  assert(action(false, 30, 105, 35, 109, false, 699)
         == JOT_READER_TOUCH_EDIT);

  // Date, header, footer, gutters, and unused paper cannot enter editing.
  assert(action(false, 80, 41, 80, 41, false, 100)
         == JOT_READER_TOUCH_NONE);
  assert(action(false, 80, 65, 80, 65, false, 100)
         == JOT_READER_TOUCH_NONE); // Timestamp/metadata.
  assert(action(false, 80, 75, 80, 75, false, 100)
         == JOT_READER_TOUCH_NONE); // 2px text top inset.
  assert(action(false, 11, 100, 11, 100, false, 100)
         == JOT_READER_TOUCH_NONE); // Left gutter.
  assert(action(false, 188, 100, 188, 100, false, 100)
         == JOT_READER_TOUCH_NONE); // Right gutter.
  assert(action(false, 100, 137, 100, 137, false, 100)
         == JOT_READER_TOUCH_NONE); // Just below short note.
  assert(action(false, 100, 185, 100, 185, false, 100)
         == JOT_READER_TOUCH_NONE); // Blank page.
  assert(action(false, 100, 205, 100, 205, false, 100)
         == JOT_READER_TOUCH_NONE); // Footer.

  // A stroke must both begin and finish inside the visible text region.
  assert(action(false, 45, 65, 45, 80, false, 100)
         == JOT_READER_TOUCH_NONE);
  assert(action(false, 45, 135, 45, 140, false, 100)
         == JOT_READER_TOUCH_NONE);
  assert(action(false, 45, 100, 45, 100, true, 100)
         == JOT_READER_TOUCH_NONE); // Sticky movement after a drag.
  assert(action(false, 45, 100, 45, 100, false, 700)
         == JOT_READER_TOUCH_NONE);
  assert(action(false, 45, 100, 56, 100, false, 100)
         == JOT_READER_TOUCH_NONE); // >10px slop.
  assert(action(false, 45, 100, 45, 111, false, 100)
         == JOT_READER_TOUCH_NONE);

  // Reader content can be scrolled with physical buttons: text hitbox
  // remains clipped to the viewport even if the TextLayer extends beyond it.
  assert(action_with_layout(false, 40, 76, 40, 76, false, 100,
                            &long_scrolled) == JOT_READER_TOUCH_EDIT);
  assert(action_with_layout(false, 40, 197, 40, 197, false, 100,
                            &long_scrolled) == JOT_READER_TOUCH_EDIT);
  assert(action_with_layout(false, 40, 198, 40, 198, false, 100,
                            &long_scrolled) == JOT_READER_TOUCH_NONE);
  assert(action_with_layout(false, 40, 60, 40, 60, false, 100,
                            &long_scrolled) == JOT_READER_TOUCH_NONE);

  // No viewport allocation / invalid layout must never open an editor.
  assert(action_with_layout(false, 40, 100, 40, 100, false, 100, NULL)
         == JOT_READER_TOUCH_NONE);
  assert(action_with_layout(false, 40, 100, 40, 100, false, 100,
                            &empty_layout) == JOT_READER_TOUCH_NONE);
  assert(action_with_layout(false, 20, 20, 20, 20, false, 100, NULL)
         == JOT_READER_TOUCH_BACK); // Back remains available.
}

static void test_peek(void) {
  // Draft peek taps always return to the SAME unsaved draft, not editing
  // of a saved note. The Back key takes precedence, regardless of layout.
  assert(action(true, 20, 20, 20, 20, false, 100)
         == JOT_READER_TOUCH_BACK);
  assert(action(true, 80, 20, 80, 20, false, 100)
         == JOT_READER_TOUCH_DISMISS_PEEK);
  assert(action(true, 45, 100, 45, 100, false, 100)
         == JOT_READER_TOUCH_DISMISS_PEEK);
  assert(action(true, 160, 190, 160, 190, false, 100)
         == JOT_READER_TOUCH_DISMISS_PEEK);
  assert(action(true, 160, 100, 160, 100, true, 100)
         == JOT_READER_TOUCH_NONE);
  assert(action(true, 160, 100, 160, 100, false, 900)
         == JOT_READER_TOUCH_NONE);
  assert(action_with_layout(true, 160, 100, 160, 100, false, 100, NULL)
         == JOT_READER_TOUCH_DISMISS_PEEK);
}

static void test_touch_drag(void) {
  // The reader body is y=75..197 inclusive. The header, metadata and
  // footer are fixed, even for very long notes.
  assert(jot_reader_touch_can_drag(100, 75, &long_scrolled));
  assert(jot_reader_touch_can_drag(100, 197, &long_scrolled));
  assert(jot_reader_touch_can_drag(0, 110, &long_scrolled));
  assert(jot_reader_touch_can_drag(199, 110, &long_scrolled));
  assert(!jot_reader_touch_can_drag(100, 74, &long_scrolled));
  assert(!jot_reader_touch_can_drag(100, 198, &long_scrolled));
  assert(!jot_reader_touch_can_drag(100, 41, &long_scrolled));
  assert(!jot_reader_touch_can_drag(100, 65, &long_scrolled));
  assert(!jot_reader_touch_can_drag(100, 220, &long_scrolled));
  assert(!jot_reader_touch_can_drag(-1, 100, &long_scrolled));
  assert(!jot_reader_touch_can_drag(200, 100, &long_scrolled));
  assert(!jot_reader_touch_can_drag(100, 100, &short_note));
  assert(!jot_reader_touch_can_drag(100, 100, NULL));
  assert(!jot_reader_touch_can_drag(100, 100, &empty_layout));

  // Each 1px finger movement changes scroll by exactly 1px in the
  // opposite direction. No inertia or full-page jumps on release.
  assert(jot_reader_touch_scroll_after_drag(100, -1, &long_scrolled) == 101);
  assert(jot_reader_touch_scroll_after_drag(100, 1, &long_scrolled) == 99);
  assert(jot_reader_touch_scroll_after_drag(100, -47, &long_scrolled) == 147);
  assert(jot_reader_touch_scroll_after_drag(10, 70, &long_scrolled) == 0);
  assert(jot_reader_touch_scroll_after_drag(170, -50, &long_scrolled) == 181);
  assert(jot_reader_touch_scroll_after_drag(181, -20, &long_scrolled) == 181);
  assert(jot_reader_touch_scroll_after_drag(0, 20, &long_scrolled) == 0);
  assert(jot_reader_touch_scroll_after_drag(0, -100, &short_note) == 0);
  assert(jot_reader_touch_scroll_after_drag(0, -100, NULL) == 0);

  // Release after a swipe must NEVER edit a saved note or dismiss Peek,
  // even when the finger returns to where it started.
  assert(action_with_layout(false, 100, 100, 100, 100, true, 180,
                            &long_scrolled) == JOT_READER_TOUCH_NONE);
  assert(action_with_layout(true, 100, 100, 100, 100, true, 180,
                            &long_scrolled) == JOT_READER_TOUCH_NONE);
}

int main(void) {
  test_touch_drag();
  test_back();
  test_saved_text();
  test_peek();
  puts("Reader touch scrolling, Back, tap-to-edit and draft-peek tests passed");
  return 0;
}

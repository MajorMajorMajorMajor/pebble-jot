// Reading behavior: navigation, deliberate deletion, and touch scrolling.
// This is a Pebble-independent contract test; on-watch touch QA is separate.
#include <assert.h>
#include <stdio.h>
#include "jot/reader_review.h"
#include "jot/reader_touch.h"

static const JotReaderTouchLayout short_note = {200, 228, 60, 0, 0};
static const JotReaderTouchLayout long_note = {200, 228, 300, 160, 181};

static JotReaderTouchAction tap(bool peek, int sx, int sy,
                                int ex, int ey, bool moved, unsigned ms,
                                const JotReaderTouchLayout *layout) {
  return jot_reader_touch_action(peek, sx, sy, ex, ey, moved, ms, layout);
}

static void navigation_and_delete_confirmation(void) {
  JotReaderReview r = {0};
  assert(jot_reader_review_select(&r) == JOT_REVIEW_NONE && r.open);
  assert(jot_reader_review_select(&r) == JOT_REVIEW_EDIT && r.open);
  assert(jot_reader_review_back(&r) && !r.open);

  // Holding enters Delete, but never deletes by itself. Two short presses
  // are required. Any cancellation must safely disarm the first press.
  jot_reader_review_hold(&r);
  assert(r.open && r.delete_mode && !r.armed);
  assert(jot_reader_review_select(&r) == JOT_REVIEW_NONE && r.armed);
  assert(jot_reader_review_cancel_armed(&r) && !r.armed);
  assert(jot_reader_review_select(&r) == JOT_REVIEW_NONE && r.armed);
  assert(jot_reader_review_back(&r) && !r.armed && r.delete_mode);
  assert(jot_reader_review_select(&r) == JOT_REVIEW_NONE);
  assert(jot_reader_review_select(&r) == JOT_REVIEW_CONFIRM_DELETE);
  assert(!r.armed && r.delete_mode);
  jot_reader_review_hold(&r);
  assert(!r.armed); // Long hold must not confirm another deletion.
  assert(jot_reader_review_back(&r) && !r.open);
}

static void page_and_note_navigation(void) {
  assert(jot_reader_review_paper_width(200, false, 34) == 200);
  assert(jot_reader_review_paper_width(200, true, 34) == 166);
  assert(jot_reader_review_paper_width(20, true, 34) == 1);
  assert(jot_reader_review_paper_width(200, true, 0) == 200);

  assert(jot_reader_review_next_rank(4, 20, -1) == 3);
  assert(jot_reader_review_next_rank(4, 20, 1) == 5);
  assert(jot_reader_review_next_rank(0, 20, -1) == 0);
  assert(jot_reader_review_next_rank(19, 20, 1) == 19);
  assert(jot_reader_review_next_rank(0, 0, 1) == -1);
}

static void tap_to_edit_or_return(void) {
  // Back: both endpoints must land inside the top-left header target.
  assert(tap(false, 20, 20, 20, 20, false, 100, &short_note)
         == JOT_READER_TOUCH_BACK);
  assert(tap(false, 42, 20, 42, 20, false, 100, &short_note)
         == JOT_READER_TOUCH_NONE);
  assert(tap(false, 20, 20, 45, 20, false, 100, &short_note)
         == JOT_READER_TOUCH_NONE);

  // Saved notes: text can be edited, blank paper and metadata cannot.
  assert(tap(false, 45, 100, 45, 100, false, 100, &short_note)
         == JOT_READER_TOUCH_EDIT);
  assert(tap(false, 45, 137, 45, 137, false, 100, &short_note)
         == JOT_READER_TOUCH_NONE);
  assert(tap(false, 45, 65, 45, 65, false, 100, &short_note)
         == JOT_READER_TOUCH_NONE);
  assert(tap(false, 11, 100, 11, 100, false, 100, &short_note)
         == JOT_READER_TOUCH_NONE);
  assert(tap(false, 45, 65, 45, 100, false, 100, &short_note)
         == JOT_READER_TOUCH_NONE);
  assert(tap(false, 45, 100, 45, 100, false, 100, NULL)
         == JOT_READER_TOUCH_NONE);

  // Scrolled TextLayer is still clipped to the visible paper viewport.
  assert(tap(false, 40, 197, 40, 197, false, 100, &long_note)
         == JOT_READER_TOUCH_EDIT);
  assert(tap(false, 40, 198, 40, 198, false, 100, &long_note)
         == JOT_READER_TOUCH_NONE);

  // Never interpret a drag, long press, or wide tap as an edit.
  assert(tap(false, 45, 100, 45, 100, true, 100, &short_note)
         == JOT_READER_TOUCH_NONE);
  assert(tap(false, 45, 100, 45, 100, false, 700, &short_note)
         == JOT_READER_TOUCH_NONE);
  assert(tap(false, 45, 100, 56, 100, false, 100, &short_note)
         == JOT_READER_TOUCH_NONE);

  // Draft peek is dismissible without editing a saved note; Back wins.
  assert(tap(true, 80, 20, 80, 20, false, 100, &short_note)
         == JOT_READER_TOUCH_DISMISS_PEEK);
  assert(tap(true, 20, 20, 20, 20, false, 100, &short_note)
         == JOT_READER_TOUCH_BACK);
  assert(tap(true, 100, 100, 100, 100, true, 100, &short_note)
         == JOT_READER_TOUCH_NONE);
}

static void drag_and_clamp(void) {
  assert(jot_reader_touch_can_drag(100, 75, &long_note));
  assert(jot_reader_touch_can_drag(100, 197, &long_note));
  assert(!jot_reader_touch_can_drag(100, 74, &long_note));
  assert(!jot_reader_touch_can_drag(100, 198, &long_note));
  assert(!jot_reader_touch_can_drag(100, 100, &short_note));
  assert(!jot_reader_touch_can_drag(100, 100, NULL));

  assert(jot_reader_touch_scroll_after_drag(100, -1, &long_note) == 101);
  assert(jot_reader_touch_scroll_after_drag(100, 1, &long_note) == 99);
  assert(jot_reader_touch_scroll_after_drag(10, 70, &long_note) == 0);
  assert(jot_reader_touch_scroll_after_drag(170, -50, &long_note) == 181);
  assert(jot_reader_touch_scroll_after_drag(0, -10, NULL) == 0);
}

int main(void) {
  navigation_and_delete_confirmation();
  page_and_note_navigation();
  tap_to_edit_or_return();
  drag_and_clamp();
  puts("Reading behavior: navigation, delete safety, touch, and drag passed");
  return 0;
}

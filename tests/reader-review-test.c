// Host-only state machine tests: no Pebble SDK required.
#include <assert.h>
#include <stdio.h>
#include "jot/reader_review.h"

int main(void) {
  JotReaderReview r = {0};
  assert(jot_reader_review_select(&r) == JOT_REVIEW_NONE);
  assert(r.open && !r.delete_mode && !r.armed);
  assert(jot_reader_review_select(&r) == JOT_REVIEW_EDIT);
  assert(r.open);  // Editing cannot collapse the persistent bar by itself.
  assert(jot_reader_review_back(&r));
  assert(!r.open && !r.delete_mode && !r.armed);
  assert(!jot_reader_review_back(&r));

  // Long hold enters persistent Delete mode, with NO deletion.
  jot_reader_review_hold(&r);
  assert(r.open && r.delete_mode && !r.armed);
  assert(jot_reader_review_select(&r) == JOT_REVIEW_NONE);
  assert(r.armed);
  // Down/Up before confirmation cancels without deleting or navigating.
  assert(jot_reader_review_cancel_armed(&r));
  assert(!r.armed && r.delete_mode && r.open);
  assert(!jot_reader_review_cancel_armed(&r));
  assert(jot_reader_review_select(&r) == JOT_REVIEW_NONE);
  assert(r.armed);
  assert(jot_reader_review_back(&r));  // First Back disarms.
  assert(r.open && r.delete_mode && !r.armed);
  assert(jot_reader_review_select(&r) == JOT_REVIEW_NONE);
  assert(jot_reader_review_select(&r) == JOT_REVIEW_CONFIRM_DELETE);
  assert(r.open && r.delete_mode && !r.armed);
  assert(jot_reader_review_select(&r) == JOT_REVIEW_NONE);
  assert(r.armed);  // Each successive delete requires two presses.
  jot_reader_review_hold(&r); // Holding Select again never confirms.
  assert(!r.armed);
  assert(jot_reader_review_back(&r)); // Then Back closes.
  assert(!r.open);
  jot_reader_review_reset(&r);
  assert(jot_reader_review_select(NULL) == JOT_REVIEW_NONE);
  assert(!jot_reader_review_back(NULL));

  // In newest-first order Up = newer; Down = older. Stop at both ends.
  assert(jot_reader_review_next_rank(4, 20, -1) == 3);
  assert(jot_reader_review_next_rank(4, 20, 1) == 5);
  assert(jot_reader_review_next_rank(0, 20, -1) == 0);
  assert(jot_reader_review_next_rank(19, 20, 1) == 19);
  assert(jot_reader_review_next_rank(3, 20, 0) == 3);
  assert(jot_reader_review_next_rank(0, 1, -1) == 0);
  assert(jot_reader_review_next_rank(0, 1, 1) == 0);
  assert(jot_reader_review_next_rank(0, 0, 1) == -1);
  assert(jot_reader_review_next_rank(-1, 20, 1) == -1);
  assert(jot_reader_review_next_rank(20, 20, -1) == -1);
  puts("Persistent action-bar and two-press delete state tests passed");
  return 0;
}

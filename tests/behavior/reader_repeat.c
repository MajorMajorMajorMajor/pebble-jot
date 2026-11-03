// Reader hardware scrolling should not change the saved note.
#include <assert.h>
#include <stdio.h>
#include "jot/reader_repeat.h"

static void immediate_click_and_delayed_repeat(void) {
  assert((int)JOT_READER_REPEAT_DELAY_MS == (int)JOT_REPEAT_DELAY_MS);
  assert(JOT_READER_REPEAT_DELAY_MS > JOT_READER_REPEAT_INTERVAL_MS);
  assert(JOT_READER_REPEAT_STEP_PX < JOT_READER_REPEAT_CLICK_PX);

  JotReaderRepeat state = {0};
  int y = 90;
  jot_reader_repeat_begin(&state, 1);
  assert(jot_reader_repeat_advance(&state, &y, 230,
                                   JOT_READER_REPEAT_CLICK_PX));
  assert(y == 150 && state.direction == 1);
  // A short click ends here. Delayed repeat has its own small steps.
  jot_reader_repeat_cancel(&state);
  assert(!jot_reader_repeat_advance(&state, &y, 230,
                                    JOT_READER_REPEAT_STEP_PX));
  assert(y == 150);

  jot_reader_repeat_begin(&state, 1);
  assert(jot_reader_repeat_advance(&state, &y, 230,
                                   JOT_READER_REPEAT_STEP_PX));
  assert(y == 168 && state.direction == 1);
  assert(jot_reader_repeat_advance(&state, &y, 230, 500));
  assert(y == 230 && state.direction == 0); // Boundary stops timer.
  assert(!jot_reader_repeat_advance(&state, &y, 230,
                                    JOT_READER_REPEAT_STEP_PX));

  jot_reader_repeat_begin(&state, -1);
  assert(jot_reader_repeat_advance(&state, &y, 230,
                                   JOT_READER_REPEAT_CLICK_PX));
  assert(y == 170 && state.direction == -1);
  assert(jot_reader_repeat_advance(&state, &y, 230, 300));
  assert(y == 0 && state.direction == 0);
}
static void transitions_and_short_notes(void) {
  JotReaderRepeat state = {0};
  int pos = 0;
  jot_reader_repeat_begin(&state, -1);
  assert(!jot_reader_repeat_advance(&state, &pos, 100, 60));
  assert(!state.direction && pos == 0);
  jot_reader_repeat_begin(&state, 1);
  assert(!jot_reader_repeat_advance(&state, &pos, 0, 60));
  assert(!state.direction && pos == 0); // Short note doesn't schedule timer.
  jot_reader_repeat_begin(&state, 1);
  assert(jot_reader_repeat_advance(&state, &pos, 65, 60));
  assert(pos == 60);
  jot_reader_repeat_cancel(&state); // Pop/edit/overlay/release.
  assert(!jot_reader_repeat_advance(&state, &pos, 65, 18));
  assert(pos == 60);
  jot_reader_repeat_begin(&state, 1);
  assert(jot_reader_repeat_advance(&state, &pos, 65, 18));
  assert(pos == 65 && state.direction == 0);
  jot_reader_repeat_begin(&state, 4); // Invalid buttons don't scroll.
  assert(!state.direction);
  assert(!jot_reader_repeat_advance(&state, &pos, 65, 18));
  jot_reader_repeat_begin(&state, -1);
  assert(!jot_reader_repeat_advance(&state, &pos, 65, 0));
  assert(pos == 65);
  jot_reader_repeat_cancel(&state);
  jot_reader_repeat_cancel(&state); // Cleanup can be repeated safely.
}
int main(void) {
  immediate_click_and_delayed_repeat();
  transitions_and_short_notes();
  puts("Reader button repeat: immediate click, delayed steps, boundaries and cancellation passed");
  return 0;
}

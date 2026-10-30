#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "ui/key_repeat.h"

static void bindings(void) {
  for (int face = 0; face <= 1; ++face) {
    assert(jot_key_repeat_action(10, 0, face) == JOT_REPEAT_BACKSPACE);
    for (int gesture = 2; gesture <= 5; ++gesture)
      assert(jot_key_repeat_action(10, gesture, face) ==
             JOT_REPEAT_FORWARD_DELETE);
    const int backward[] = {0, 1, 6, 7, 8};
    for (unsigned i = 0; i < sizeof backward / sizeof backward[0]; ++i)
      assert(jot_key_repeat_action(10, backward[i], face) ==
             JOT_REPEAT_BACKSPACE);
    assert(jot_key_repeat_action(2, 1, face) == JOT_REPEAT_CURSOR_UP);
    assert(jot_key_repeat_action(8, 5, face) == JOT_REPEAT_CURSOR_DOWN);
    assert(jot_key_repeat_action(6, 6, face) == JOT_REPEAT_BACKSPACE);
  }
  assert(jot_key_repeat_action(9, 3, false) == JOT_REPEAT_CURSOR_RIGHT);
  assert(jot_key_repeat_action(9, 7, false) == JOT_REPEAT_CURSOR_LEFT);
  assert(jot_key_repeat_action(14, 3, true) == JOT_REPEAT_CURSOR_RIGHT);
  assert(jot_key_repeat_action(14, 7, true) == JOT_REPEAT_CURSOR_LEFT);
  assert(jot_key_repeat_action(9, 3, true) == JOT_REPEAT_NONE);
  assert(jot_key_repeat_action(14, 3, false) == JOT_REPEAT_NONE);
  // Ordinary typing, Save, mode changes, Return, digits, and return/circle
  // shortcuts must never be repeated.
  const int excluded[] = {0,1,3,4,5,7,11,12,13};
  for (unsigned i = 0; i < sizeof excluded / sizeof excluded[0]; ++i)
    assert(jot_key_repeat_action(excluded[i], 0, false) ==
           JOT_REPEAT_NONE);
  for (int g = 9; g <= 18; ++g)
    for (int key = 0; key <= 14; ++key)
      assert(jot_key_repeat_action(key, g, false) == JOT_REPEAT_NONE);
  assert(jot_key_repeat_action(-1, 0, false) == JOT_REPEAT_NONE);
  assert(jot_key_repeat_action(10, -1, false) == JOT_REPEAT_NONE);
}

static void hold_and_release(void) {
  JotRepeatState state = {0};
  jot_key_repeat_begin(&state, JOT_REPEAT_BACKSPACE, 1000, 7, 80);
  assert(!jot_key_repeat_due(&state, 1549));
  assert(jot_key_repeat_due(&state, 1550));
  assert(!jot_key_repeat_due(&state, 1684));
  assert(jot_key_repeat_due(&state, 1685));
  assert(jot_key_repeat_end(&state));  // Suppress ordinary liftoff dispatch.
  assert(!jot_key_repeat_due(&state, 2000));
  assert(!jot_key_repeat_end(&state));
  jot_key_repeat_begin(&state, JOT_REPEAT_BACKSPACE, 2000, 7, 80);
  assert(!jot_key_repeat_end(&state)); // Short tap follows original liftoff.
}

static void movement_and_safety(void) {
  JotRepeatState state = {0};
  jot_key_repeat_begin(&state, JOT_REPEAT_NONE, 0, 100, 100);
  assert(!jot_key_repeat_observe(&state, JOT_REPEAT_NONE, 10, 101, 100));
  assert(jot_key_repeat_observe(&state, JOT_REPEAT_CURSOR_LEFT,
                                150, 72, 100));
  assert(!jot_key_repeat_due(&state, 699));
  assert(!jot_key_repeat_observe(&state, JOT_REPEAT_CURSOR_LEFT,
                                 690, 76, 104));
  assert(jot_key_repeat_due(&state, 700));
  // Once active, neither reversal, return to origin, nor a long slide
  // into an unmapped gesture interrupts the latched LEFT movement.
  assert(!jot_key_repeat_observe(&state, JOT_REPEAT_CURSOR_RIGHT,
                                 710, 105, 100));
  assert(!jot_key_repeat_observe(&state, JOT_REPEAT_NONE,
                                 735, 190, 220));
  assert(state.fired && state.action == JOT_REPEAT_CURSOR_LEFT);
  assert(!jot_key_repeat_due(&state, 834));
  assert(jot_key_repeat_due(&state, 835));
  assert(jot_key_repeat_due(&state, 970));
  assert(jot_key_repeat_end(&state));
  assert(!jot_key_repeat_due(&state, 2000));
  jot_key_repeat_begin(&state, JOT_REPEAT_BACKSPACE, 1000, 10, 100);
  assert(jot_key_repeat_observe(&state, JOT_REPEAT_FORWARD_DELETE,
                                1100, 40, 110));
  assert(!jot_key_repeat_due(&state, 1649));
  assert(jot_key_repeat_due(&state, 1650));
  // Forward deletion stays forward deletion even if the finger slides
  // into the Backspace gesture or off every repeat target.
  assert(!jot_key_repeat_observe(&state, JOT_REPEAT_BACKSPACE,
                                 1660, 5, 120));
  assert(!jot_key_repeat_observe(&state, JOT_REPEAT_NONE,
                                 1690, 2, 5));
  assert(state.action == JOT_REPEAT_FORWARD_DELETE);
  assert(jot_key_repeat_due(&state, 1785));
  assert(jot_key_repeat_end(&state));
  jot_key_repeat_begin(&state, JOT_REPEAT_CURSOR_DOWN, 100, 30, 40);
  assert(jot_key_repeat_observe(&state, JOT_REPEAT_CURSOR_DOWN,
                                400, 30, 49)); // Actual motion resets hold.
  assert(!jot_key_repeat_due(&state, 949));
  assert(jot_key_repeat_due(&state, 950));
  assert(!jot_key_repeat_observe(&state, JOT_REPEAT_CURSOR_UP,
                                 955, 90, 145));
  assert(state.action == JOT_REPEAT_CURSOR_DOWN);
  assert(jot_key_repeat_due(&state, 1085));
  assert(jot_key_repeat_end(&state));
  // Timer arithmetic must survive the natural 32-bit millisecond rollover.
  jot_key_repeat_begin(&state, JOT_REPEAT_CURSOR_UP,
                       UINT32_MAX - 200u, 20, 50);
  assert(!jot_key_repeat_due(&state, 300));
  assert(jot_key_repeat_due(&state, 349));
  assert(jot_key_repeat_end(&state));
  assert(!jot_key_repeat_due(NULL, 0));
}

int main(void) {
  bindings();
  hold_and_release();
  movement_and_safety();
  puts("Gesture-held repeat mapping, timing and release safety passed");
}

#include "ui/key_repeat.h"

static int prv_abs(int n) { return n < 0 ? -n : n; }

JotRepeatAction jot_key_repeat_action(int key, int gesture, bool numeric) {
  if (gesture < 0 || gesture > 8) return JOT_REPEAT_NONE;
  // The bottom row's separate numeric Space is index 14. The two-column
  // numeric zero (index 9) is NOT a Space cursor gesture.
  if ((key == 9 && !numeric) || (key == 14 && numeric)) {
    if (gesture == 3) return JOT_REPEAT_CURSOR_RIGHT;
    if (gesture == 7) return JOT_REPEAT_CURSOR_LEFT;
    return JOT_REPEAT_NONE;
  }
  if (key == 2 && gesture == 1) return JOT_REPEAT_CURSOR_UP;
  if (key == 8 && gesture == 5) return JOT_REPEAT_CURSOR_DOWN;
  if (key == 10) {
    if (gesture >= 2 && gesture <= 5) return JOT_REPEAT_FORWARD_DELETE;
    if (gesture == 0 || gesture == 1 || gesture == 6 ||
        gesture == 7 || gesture == 8) return JOT_REPEAT_BACKSPACE;
  }
  // The workbook also reserves T/1 SW for backward deletion.
  if (key == 6 && gesture == 6) return JOT_REPEAT_BACKSPACE;
  return JOT_REPEAT_NONE;
}

void jot_key_repeat_begin(JotRepeatState *state, JotRepeatAction action,
                          uint32_t now_ms, int16_t x, int16_t y) {
  if (!state) return;
  *state = (JotRepeatState) {
    .action = action, .next_ms = now_ms + JOT_REPEAT_DELAY_MS,
    .anchor_x = x, .anchor_y = y
  };
}

bool jot_key_repeat_observe(JotRepeatState *state, JotRepeatAction action,
                            uint32_t now_ms, int16_t x, int16_t y) {
  if (!state) return false;
  // Once a repeat fires, commit to that exact editing command until
  // liftoff: further sliding cannot stop, reset or change its direction.
  if (state->fired) return false;
  if (state->action == JOT_REPEAT_NONE && action == JOT_REPEAT_NONE)
    return false;
  const bool moved = prv_abs((int)x - state->anchor_x) >
                         JOT_REPEAT_DRIFT_PX ||
                     prv_abs((int)y - state->anchor_y) >
                         JOT_REPEAT_DRIFT_PX;
  if (!moved && state->action == action) return false;
  state->action = action;
  state->next_ms = now_ms + JOT_REPEAT_DELAY_MS;
  state->anchor_x = x;
  state->anchor_y = y;
  return true;
}

bool jot_key_repeat_due(JotRepeatState *state, uint32_t now_ms) {
  if (!state || state->action == JOT_REPEAT_NONE ||
      (int32_t)(now_ms - state->next_ms) < 0) return false;
  state->fired = true;
  state->next_ms = now_ms + JOT_REPEAT_INTERVAL_MS;
  return true;
}

bool jot_key_repeat_end(JotRepeatState *state) {
  if (!state) return false;
  const bool fired = state->fired;
  *state = (JotRepeatState){0};
  return fired;
}

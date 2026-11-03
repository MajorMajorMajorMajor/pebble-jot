#include "jot/reader_repeat.h"

void jot_reader_repeat_begin(JotReaderRepeat *state, int direction) {
  if (state) state->direction =
      direction == -1 || direction == 1 ? direction : 0;
}

void jot_reader_repeat_cancel(JotReaderRepeat *state) {
  if (state) state->direction = 0;
}

bool jot_reader_repeat_advance(JotReaderRepeat *state, int *position,
                               int scroll_max, int step) {
  if (!state || !position || !state->direction || step <= 0) return false;
  if (scroll_max < 0) scroll_max = 0;
  int next = *position;
  if (state->direction < 0) {
    next = next > step ? next - step : 0;
  } else {
    next = scroll_max - next > step ? next + step : scroll_max;
  }
  if (next < 0) next = 0;
  if (next > scroll_max) next = scroll_max;
  const bool moved = next != *position;
  *position = next;
  if (!moved || (state->direction < 0 && next == 0) ||
                (state->direction > 0 && next == scroll_max)) {
    jot_reader_repeat_cancel(state);
  }
  return moved;
}

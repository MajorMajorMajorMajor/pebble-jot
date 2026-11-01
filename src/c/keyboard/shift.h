#pragma once
#include <stdbool.h>

// This is temporary editor input state, not a persisted preference.
typedef enum {
  JOT_SHIFT_OFF = 0,
  JOT_SHIFT_ONCE = 1,
  JOT_SHIFT_LOCKED = 2
} JotShiftMode;

// R-up: normal -> one character -> locked -> normal.
// R-down: immediately return to normal from either shifted state.
JotShiftMode jot_shift_cycle(JotShiftMode mode);
JotShiftMode jot_shift_cancel(JotShiftMode mode);
bool jot_shift_active(JotShiftMode mode);
char jot_shift_upper(char ascii);

// Only an inserted *letter* consumes one-shot Shift. Cursor movement,
// Space, punctuation, Backspace, Return, Save and numeric-face switching
// do not. Capitalizes a one-byte ASCII letter without altering UTF-8.
const char *jot_shift_prepare(JotShiftMode *mode,
                              const char *binding, char output[2]);

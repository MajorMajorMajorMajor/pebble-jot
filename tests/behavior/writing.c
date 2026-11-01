// Writing behavior: the visible caret and editing gestures.
// Focus on reported bugs and safety invariants, not every adjacent pixel.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ui/caret_layout.h"
#include "ui/key_repeat.h"
#include "keyboard/shift.h"

static int measure(const char *text, size_t first, size_t last, void *unused) {
  (void)unused;
  int width = 0;
  for (size_t i = first; i < last; ++i) {
    unsigned char c = (unsigned char)text[i];
    if ((c & 0xC0) == 0x80) continue; // UTF-8 continuation
    width += (c == ' ' || c == '\t') ? 3 : 6;
  }
  return width;
}

static void caret(const char *text, size_t cursor, int width,
                  int row, int x, size_t line_start) {
  const JotCaretLayout actual =
      jot_caret_layout(text, cursor, width, measure, NULL);
  if (actual.row != row || actual.x != x ||
      actual.line_start != line_start) {
    fprintf(stderr, "caret mismatch: row %d x %d start %zu"
                    " (expected row %d x %d start %zu)\n",
            actual.row, actual.x, actual.line_start, row, x, line_start);
    assert(0);
  }
}

static void text_wrapping_and_cursor(void) {
  caret("", 0, 30, 0, 0, 0);
  caret("abcdef", 6, 60, 0, 36, 0);
  caret("abc de", 4, 30, 1, 0, 4); // Word-wrap boundary.
  caret("abcdef", 6, 30, 1, 6, 5); // Long word splits.
  caret("abc   \ndef", 6, 30, 0, 18, 0); // Hidden trailing spaces.
  caret("abc de\nf", 7, 30, 2, 0, 7); // Explicit line break.
  caret("a\n\nb", 3, 30, 2, 0, 3); // Empty explicit line.

  // Exact wrist regression: a hyphen is a legal *preceding* wrap point.
  const char *compound = "one looking-glass";
  const size_t hyphen = (size_t)(strchr(compound, '-') - compound);
  caret(compound, hyphen, 80, 1, 0, hyphen);
  caret(compound, strlen(compound), 80, 1, 36, hyphen);
  caret(compound, strlen(compound), 110, 0, 99, 0);
  caret("-", 1, 30, 0, 6, 0);
  caret("abcd\xC3\xA9\xC3\xA9", 8, 30, 1, 6, 6);

  // Wrapped UTF-8 caret positions stay in bounds and move only forward.
  const char *sample = "one two three \xf0\x9f\x98\x8a\nverylongwordwithoutspaces";
  int previous_row = 0;
  for (size_t pos = 0; pos <= strlen(sample); ++pos) {
    if (sample[pos] && ((unsigned char)sample[pos] & 0xC0) == 0x80)
      continue;
    const JotCaretLayout p =
        jot_caret_layout(sample, pos, 40, measure, NULL);
    assert(p.row >= previous_row);
    assert(p.x >= 0 && p.x <= 40);
    assert(p.line_start <= pos);
    previous_row = p.row;
  }
}

static void editing_gesture_bindings(void) {
  for (int numeric = 0; numeric <= 1; ++numeric) {
    assert(jot_key_repeat_action(10, 0, numeric) ==
           JOT_REPEAT_BACKSPACE);
    assert(jot_key_repeat_action(10, 3, numeric) ==
           JOT_REPEAT_FORWARD_DELETE);
    assert(jot_key_repeat_action(2, 1, numeric) ==
           JOT_REPEAT_CURSOR_UP);
    assert(jot_key_repeat_action(8, 5, numeric) ==
           JOT_REPEAT_CURSOR_DOWN);
    assert(jot_key_repeat_action(6, 6, numeric) ==
           JOT_REPEAT_BACKSPACE);
  }
  assert(jot_key_repeat_action(9, 7, false) ==
         JOT_REPEAT_CURSOR_LEFT);
  assert(jot_key_repeat_action(9, 3, false) ==
         JOT_REPEAT_CURSOR_RIGHT);
  assert(jot_key_repeat_action(14, 3, true) ==
         JOT_REPEAT_CURSOR_RIGHT);
  assert(jot_key_repeat_action(9, 3, true) == JOT_REPEAT_NONE);
  assert(jot_key_repeat_action(14, 3, false) == JOT_REPEAT_NONE);
  assert(jot_key_repeat_action(0, 0, false) == JOT_REPEAT_NONE);
  assert(jot_key_repeat_action(10, -1, false) == JOT_REPEAT_NONE);
  // Non-editing return/circle gestures must never start autorepeat.
  for (int gesture = 9; gesture <= 18; ++gesture)
    assert(jot_key_repeat_action(10, gesture, false) ==
           JOT_REPEAT_NONE);
}

static void hold_latches_until_release(void) {
  JotRepeatState state = {0};
  jot_key_repeat_begin(&state, JOT_REPEAT_BACKSPACE, 1000, 7, 80);
  assert(!jot_key_repeat_due(&state, 1549));
  assert(jot_key_repeat_due(&state, 1550));
  assert(!jot_key_repeat_due(&state, 1684));
  assert(jot_key_repeat_due(&state, 1685));
  assert(jot_key_repeat_end(&state));
  assert(!jot_key_repeat_due(&state, 2000));

  // A press released before its repeat delay is still an ordinary tap.
  jot_key_repeat_begin(&state, JOT_REPEAT_BACKSPACE, 2000, 7, 80);
  assert(!jot_key_repeat_end(&state));

  // Important wrist regression: reversing / leaving the key AFTER the
  // repeat fires must not change the original deletion direction.
  jot_key_repeat_begin(&state, JOT_REPEAT_FORWARD_DELETE, 100, 50, 50);
  assert(jot_key_repeat_due(&state, 650));
  assert(!jot_key_repeat_observe(&state, JOT_REPEAT_BACKSPACE,
                                 660, 2, 10));
  assert(!jot_key_repeat_observe(&state, JOT_REPEAT_NONE,
                                 680, 199, 220));
  assert(state.action == JOT_REPEAT_FORWARD_DELETE);
  assert(jot_key_repeat_due(&state, 785));
  assert(jot_key_repeat_end(&state));

  // Motion before activation moves the hold deadline.
  jot_key_repeat_begin(&state, JOT_REPEAT_CURSOR_DOWN, 100, 30, 40);
  assert(jot_key_repeat_observe(&state, JOT_REPEAT_CURSOR_DOWN,
                                400, 30, 49));
  assert(!jot_key_repeat_due(&state, 949));
  assert(jot_key_repeat_due(&state, 950));
  assert(jot_key_repeat_end(&state));

  // Pebble's 32-bit millisecond clock can wrap during a hold.
  jot_key_repeat_begin(&state, JOT_REPEAT_CURSOR_UP,
                       UINT32_MAX - 200u, 20, 50);
  assert(!jot_key_repeat_due(&state, 300));
  assert(jot_key_repeat_due(&state, 349));
  assert(jot_key_repeat_end(&state));
}


static void shift_case_and_state(void) {
  JotShiftMode mode = JOT_SHIFT_OFF;
  char out[2] = {0};
  assert(!jot_shift_active(mode));
  assert(jot_shift_cycle(mode) == JOT_SHIFT_ONCE);
  mode = jot_shift_cycle(mode);
  assert(jot_shift_active(mode));
  assert(jot_shift_upper('q') == 'Q');
  assert(jot_shift_upper('w') == 'W');
  assert(jot_shift_upper('r') == 'R');
  assert(jot_shift_upper('a') == 'A');
  assert(jot_shift_upper('z') == 'Z');
  assert(jot_shift_upper(':') == ':');

  // Cancel is always available from either mode, and does not type R.
  assert(jot_shift_cancel(mode) == JOT_SHIFT_OFF);
  mode = jot_shift_cycle(mode);
  assert(mode == JOT_SHIFT_LOCKED);
  assert(jot_shift_cancel(mode) == JOT_SHIFT_OFF);
  mode = JOT_SHIFT_OFF;

  // One-shot survives Space/punctuation/cursor/delete until a letter.
  mode = jot_shift_cycle(mode);
  assert(jot_shift_prepare(&mode, " ", out)[0] == ' ');
  assert(mode == JOT_SHIFT_ONCE);
  assert(jot_shift_prepare(&mode, ".", out)[0] == '.');
  assert(mode == JOT_SHIFT_ONCE);
  assert(jot_shift_prepare(&mode, "\b", out)[0] == '\b');
  assert(mode == JOT_SHIFT_ONCE);
  assert(jot_shift_prepare(&mode, "\n", out)[0] == '\n');
  assert(mode == JOT_SHIFT_ONCE);
  assert(jot_shift_prepare(&mode, "é", out)[0] != '\0');
  assert(mode == JOT_SHIFT_ONCE);
  assert(strcmp(jot_shift_prepare(&mode, "f", out), "F") == 0);
  assert(mode == JOT_SHIFT_OFF);
  assert(strcmp(jot_shift_prepare(&mode, "o", out), "o") == 0);

  // Lock remains through multiple uppercase and lowercase ASCII gestures.
  mode = jot_shift_cycle(jot_shift_cycle(JOT_SHIFT_OFF));
  assert(mode == JOT_SHIFT_LOCKED);
  assert(strcmp(jot_shift_prepare(&mode, "j", out), "J") == 0);
  assert(strcmp(jot_shift_prepare(&mode, "O", out), "O") == 0);
  assert(strcmp(jot_shift_prepare(&mode, "x", out), "X") == 0);
  assert(mode == JOT_SHIFT_LOCKED);
  assert(jot_shift_cycle(mode) == JOT_SHIFT_OFF);
  assert(jot_shift_prepare(NULL, "q", out)[0] == 'q');
}
int main(void) {
  shift_case_and_state();
  text_wrapping_and_cursor();
  editing_gesture_bindings();
  hold_latches_until_release();
  puts("Writing behavior: caret layout, editing gestures, and repeat passed");
  return 0;
}

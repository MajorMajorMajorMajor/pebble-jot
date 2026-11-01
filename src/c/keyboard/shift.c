#include "keyboard/shift.h"

JotShiftMode jot_shift_cycle(JotShiftMode mode) {
  return mode == JOT_SHIFT_OFF ? JOT_SHIFT_ONCE :
         mode == JOT_SHIFT_ONCE ? JOT_SHIFT_LOCKED : JOT_SHIFT_OFF;
}

JotShiftMode jot_shift_cancel(JotShiftMode mode) {
  (void)mode;
  return JOT_SHIFT_OFF;
}

bool jot_shift_active(JotShiftMode mode) {
  return mode == JOT_SHIFT_ONCE || mode == JOT_SHIFT_LOCKED;
}

char jot_shift_upper(char ascii) {
  return ascii >= 'a' && ascii <= 'z' ? (char)(ascii - 'a' + 'A') :
                                       ascii;
}

const char *jot_shift_prepare(JotShiftMode *mode,
                              const char *binding, char output[2]) {
  if (!binding || !output) return binding;
  if (!mode || !jot_shift_active(*mode) ||
      !binding[0] || binding[1]) return binding;
  const char c = binding[0];
  if (!((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z'))) return binding;
  output[0] = jot_shift_upper(c);
  output[1] = '\0';
  if (*mode == JOT_SHIFT_ONCE) *mode = JOT_SHIFT_OFF;
  return output;
}

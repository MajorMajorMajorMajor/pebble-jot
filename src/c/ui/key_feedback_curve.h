#pragma once
#include <stdint.h>

// Each letter row compares a different visual treatment. Columns vary the
// release duration so styles and timings can be assessed independently.
// Touch indices come from the EXISTING recognizer; hitboxes are unchanged.
uint16_t key_feedback_release_ms(int touch_index);

// Smoothstep interpolation, from 0 to 255, with bounded integer arithmetic.
// The animation timer uses real elapsed time so a late frame never extends
// the total duration. Used for both a 50ms touch-in and the release curve.
uint8_t key_feedback_ease(uint32_t elapsed_ms, uint16_t duration_ms);

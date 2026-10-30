#pragma once
#include <stdint.h>

// Palette-aware feedback: integer smoothstep schedules a handful of *actual*
// Pebble 64-colour palette states. Continuous RGB interpolation on a 2-bit
// per-channel display can render as no visible colour change at all.
enum {
  KEY_FEEDBACK_FRAME_MS = 25,
  KEY_FEEDBACK_PRESS_MS = 70,
  KEY_FEEDBACK_RELEASE_MS = 180,
  KEY_FEEDBACK_MIN_VISIBLE_MS = 60,
  KEY_FEEDBACK_INITIAL_LEVEL = 128
};

// All keys use the same legible release timing; no hidden row-specific fades.
uint16_t key_feedback_release_ms(int touch_index);

// 3t² - 2t³, for pacing, not for interpolating arbitrary RGB colours.
uint8_t key_feedback_ease(uint32_t elapsed_ms, uint16_t duration_ms);

// Quantized, guaranteed distinct frames: 0=rest, 1=soft, 2=warm, 3=pressed.
// Boundaries are intentionally below our initial intensity of 128, so quick
// taps have a visible colour on their very first frame.
uint8_t key_feedback_palette_step(uint8_t intensity);

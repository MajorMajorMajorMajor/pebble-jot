#include "ui/key_feedback_curve.h"

uint16_t key_feedback_release_ms(int touch_index) {
  (void)touch_index;
  return KEY_FEEDBACK_RELEASE_MS;
}

uint8_t key_feedback_ease(uint32_t elapsed_ms, uint16_t duration_ms) {
  if (!duration_ms || elapsed_ms >= duration_ms) return 255;
  const uint32_t p = elapsed_ms * 255u / duration_ms;
  // Bounded integer smoothstep.
  return (uint8_t)((p * p * (765u - 2u * p) + 32512u) / 65025u);
}

uint8_t key_feedback_palette_step(uint8_t intensity) {
  if (!intensity) return 0;
  if (intensity < 96) return 1;
  if (intensity < 192) return 2;
  return 3;
}

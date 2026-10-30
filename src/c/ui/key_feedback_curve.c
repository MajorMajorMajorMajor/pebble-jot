#include "ui/key_feedback_curve.h"

uint16_t key_feedback_release_ms(int touch_index) {
  if (touch_index >= 0 && touch_index < 9) {
    static const uint16_t durations[3] = {100, 150, 200};
    return durations[touch_index % 3];
  }
  return 250; // Dark controls and wide gold Spacebar.
}

uint8_t key_feedback_ease(uint32_t elapsed_ms, uint16_t duration_ms) {
  if (!duration_ms || elapsed_ms >= duration_ms) return 255;
  const uint32_t p = elapsed_ms * 255u / duration_ms;
  // 3t^2 - 2t^3; at p=255, output is exactly 255.
  return (uint8_t)((p * p * (765u - 2u * p) + 32512u) / 65025u);
}

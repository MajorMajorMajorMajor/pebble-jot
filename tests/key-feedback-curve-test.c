#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "ui/key_feedback_curve.h"

int main(void) {
  // Column durations; each of the three letter styles gets all three.
  for (int row = 0; row < 3; ++row) {
    assert(key_feedback_release_ms(row * 3) == 100);
    assert(key_feedback_release_ms(row * 3 + 1) == 150);
    assert(key_feedback_release_ms(row * 3 + 2) == 200);
  }
  for (int key = 9; key < 14; ++key)
    assert(key_feedback_release_ms(key) == 250);

  assert(key_feedback_ease(0, 150) == 0);
  assert(key_feedback_ease(150, 150) == 255);
  assert(key_feedback_ease(500, 150) == 255);
  assert(key_feedback_ease(0, 0) == 255);
  for (uint16_t ms = 100; ms <= 250; ms += 50) {
    uint8_t before = 0;
    for (uint32_t t = 0; t <= ms; ++t) {
      uint8_t current = key_feedback_ease(t, ms);
      assert(current >= before);
      before = current;
    }
    assert(key_feedback_ease(ms / 2, ms) >= 126);
    assert(key_feedback_ease(ms / 2, ms) <= 129);
  }
  puts("Keyboard A/B feedback timing and smoothstep curves passed");
  return 0;
}

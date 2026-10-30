#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "ui/key_feedback_curve.h"

static uint8_t release_step(uint8_t start, uint32_t ms) {
  const uint8_t ease = key_feedback_ease(ms, KEY_FEEDBACK_RELEASE_MS);
  return key_feedback_palette_step((uint16_t)start * (255 - ease) / 255);
}

int main(void) {
  assert(KEY_FEEDBACK_FRAME_MS == 25);
  assert(KEY_FEEDBACK_PRESS_MS == 80);
  assert(KEY_FEEDBACK_RELEASE_MS == 190);
  assert(KEY_FEEDBACK_MIN_VISIBLE_MS == 60);
  assert(KEY_FEEDBACK_INITIAL_LEVEL == 128);
  // Only charcoal controls get the softer on-touch ramp.
  for (int key = 0; key <= 9; ++key)
    assert(key_feedback_initial_level(key) == KEY_FEEDBACK_INITIAL_LEVEL);
  for (int key = 10; key < 14; ++key) {
    assert(key_feedback_initial_level(key) == 80);
    assert(key_feedback_palette_step(key_feedback_initial_level(key)) == 1);
  }
  assert(key_feedback_initial_level(-1) == KEY_FEEDBACK_INITIAL_LEVEL);
  for (int i = 0; i < 14; ++i)
    assert(key_feedback_release_ms(i) == 190);

  assert(key_feedback_palette_step(0) == 0);
  assert(key_feedback_palette_step(1) == 1);
  assert(key_feedback_palette_step(95) == 1);
  assert(key_feedback_palette_step(96) == 2);
  assert(key_feedback_palette_step(191) == 2);
  assert(key_feedback_palette_step(192) == 3);
  assert(key_feedback_palette_step(255) == 3);
  // A tap's *first* painted colour must NOT be the resting palette entry.
  assert(key_feedback_palette_step(KEY_FEEDBACK_INITIAL_LEVEL) == 2);

  assert(key_feedback_ease(0, 190) == 0);
  assert(key_feedback_ease(95, 190) >= 126);
  assert(key_feedback_ease(95, 190) <= 129);
  assert(key_feedback_ease(190, 190) == 255);
  assert(key_feedback_ease(210, 190) == 255);
  assert(key_feedback_ease(0, 0) == 255);
  for (uint16_t ms = 80; ms <= 190; ms += 55) {
    uint8_t previous = 0;
    for (uint32_t t = 0; t <= ms; ++t) {
      uint8_t step = key_feedback_ease(t, ms);
      assert(step >= previous);
      previous = step;
    }
  }
  // Long press: strong -> warm -> soft -> resting, no brightening on release.
  assert(release_step(255, 0) == 3);
  assert(release_step(255, 75) == 2);
  assert(release_step(255, 145) == 1);
  assert(release_step(255, 190) == 0);
  // Short press: warm -> soft -> resting, still distinguishable.
  assert(release_step(128, 0) == 2);
  assert(release_step(128, 90) == 1);
  assert(release_step(128, 190) == 0);
  puts("Palette-aware 80ms ease-in and 190ms ease-out regression tests passed");
  return 0;
}

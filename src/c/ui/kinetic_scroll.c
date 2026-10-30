#include "ui/kinetic_scroll.h"

static int32_t prv_clamp(int32_t v, int32_t min, int32_t max) {
  if (v < min) return min;
  if (v > max) return max;
  return v;
}

static int32_t prv_abs(int32_t v) { return v < 0 ? -v : v; }

void jot_kinetic_cancel(JotKinetic *motion) {
  if (motion) *motion = (JotKinetic){0};
}

void jot_kinetic_begin(JotKinetic *motion, uint32_t now_ms) {
  if (!motion) return;
  jot_kinetic_cancel(motion);
  motion->tracking = true;
  motion->sample_ms = now_ms;
  motion->last_motion_ms = now_ms;
}

void jot_kinetic_sample(JotKinetic *motion, int delta_scroll,
                        uint32_t now_ms) {
  if (!motion || !motion->tracking || !delta_scroll) return;
  const uint32_t elapsed = now_ms - motion->sample_ms;
  if (elapsed > JOT_KINETIC_MOTION_GAP_MS) {
    // A long pause is not a flick; ignore earlier movement and the
    // freshly resumed sample until enough new motion has been recorded.
    motion->velocity = 0;
    motion->pending_pixels = 0;
    motion->sample_ms = now_ms;
    motion->last_motion_ms = now_ms;
    return;
  }
  motion->pending_pixels += delta_scroll;
  motion->last_motion_ms = now_ms;
  if (elapsed < JOT_KINETIC_DRAG_SLOP_MS) return;
  const int32_t instant = prv_clamp(
      motion->pending_pixels * 1000 / (int32_t)elapsed,
      -JOT_KINETIC_MAX_SPEED, JOT_KINETIC_MAX_SPEED);
  // Recent finger motion matters more than earlier, slower dragging.
  motion->velocity = motion->velocity ?
      (motion->velocity * 2 + instant * 3) / 5 : instant;
  motion->pending_pixels = 0;
  motion->sample_ms = now_ms;
}

bool jot_kinetic_release(JotKinetic *motion, uint32_t now_ms) {
  if (!motion || !motion->tracking) return false;
  motion->tracking = false;
  if (now_ms - motion->last_motion_ms > JOT_KINETIC_MOTION_GAP_MS ||
      prv_abs(motion->velocity) < JOT_KINETIC_MIN_SPEED) {
    motion->active = false;
    return false;
  }
  motion->active = true;
  motion->frame_ms = now_ms;
  motion->remainder = 0;
  return true;
}

bool jot_kinetic_frame(JotKinetic *motion, uint32_t now_ms,
                       int *delta_scroll) {
  if (delta_scroll) *delta_scroll = 0;
  if (!motion || !delta_scroll || !motion->active) return false;
  uint32_t elapsed = now_ms - motion->frame_ms;
  if (!elapsed) return true;
  // Large timer stalls must not fling the reader a full screen at once.
  if (elapsed > 50) elapsed = 50;
  motion->frame_ms = now_ms;
  const int32_t pixels1000 =
      motion->velocity * (int32_t)elapsed + motion->remainder;
  *delta_scroll = pixels1000 / 1000;
  motion->remainder = pixels1000 % 1000;
  // Exponential damping: approximately 92% retained each 20ms frame.
  motion->velocity = motion->velocity *
      (1000 - 4 * (int32_t)elapsed) / 1000;
  if (prv_abs(motion->velocity) < JOT_KINETIC_STOP_SPEED) {
    motion->active = false;
    motion->remainder = 0;
  }
  return true;
}

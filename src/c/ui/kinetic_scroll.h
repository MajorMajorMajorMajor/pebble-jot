#pragma once
#include <stdbool.h>
#include <stdint.h>

// Small, integer-only kinetic scrolling model for Emery. The framework
// provides touch coordinates but no native inertial-scrolling parameters
// for a custom reader/list. Tune these together after on-wrist testing.
enum {
  JOT_KINETIC_FRAME_MS = 20,
  JOT_KINETIC_MOTION_GAP_MS = 90,
  JOT_KINETIC_MIN_SPEED = 120,   // px/s necessary to launch a flick
  JOT_KINETIC_MAX_SPEED = 1600,  // cap for very fast/noisy touch samples
  JOT_KINETIC_STOP_SPEED = 40,   // stop instead of an endless subpixel tail
  JOT_KINETIC_DRAG_SLOP_MS = 5
};

typedef struct {
  int32_t velocity;       // signed *scroll* pixels per second
  int32_t remainder;      // subpixel accumulation in 1/1000 px
  int32_t pending_pixels;
  uint32_t sample_ms;
  uint32_t last_motion_ms;
  uint32_t frame_ms;
  bool active;
  bool tracking;
} JotKinetic;

void jot_kinetic_begin(JotKinetic *motion, uint32_t now_ms);
// Supply the same scroll delta as rendered (negative finger movement).
void jot_kinetic_sample(JotKinetic *motion, int delta_scroll, uint32_t now_ms);
bool jot_kinetic_release(JotKinetic *motion, uint32_t now_ms);
// Returns whether a frame was advanced; delta_scroll may be 0 for a
// subpixel frame. Caller must stop at a clamped content boundary.
bool jot_kinetic_frame(JotKinetic *motion, uint32_t now_ms,
                       int *delta_scroll);
void jot_kinetic_cancel(JotKinetic *motion);

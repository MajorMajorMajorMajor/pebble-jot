#pragma once
#include <stdbool.h>
#include <stdint.h>

// Repeat is available only for editing gestures, never printable letters,
// SPACE, SAVE, face-switch, Return, circles or the diagnostics harness.
typedef enum {
  JOT_REPEAT_NONE = 0,
  JOT_REPEAT_CURSOR_LEFT,
  JOT_REPEAT_CURSOR_RIGHT,
  JOT_REPEAT_CURSOR_UP,
  JOT_REPEAT_CURSOR_DOWN,
  JOT_REPEAT_BACKSPACE,
  JOT_REPEAT_FORWARD_DELETE
} JotRepeatAction;

enum {
  JOT_REPEAT_DELAY_MS = 550,
  JOT_REPEAT_INTERVAL_MS = 135,
  JOT_REPEAT_SWIPE_MIN_PX = 14,
  JOT_REPEAT_DRIFT_PX = 6
};

typedef struct {
  JotRepeatAction action;
  uint32_t next_ms;
  int16_t anchor_x, anchor_y;
  bool fired; // Locks the original repeat command until liftoff and
              // suppresses the ordinary gesture on release.
} JotRepeatState;

// Gesture indices match the existing canonical recognizer:
// TAP=0, N=1, NE=2, E=3, SE=4, S=5, SW=6, W=7, NW=8.
JotRepeatAction jot_key_repeat_action(int key, int gesture, bool numeric);
void jot_key_repeat_begin(JotRepeatState *state, JotRepeatAction action,
                          uint32_t now_ms, int16_t x, int16_t y);
// Returns true if the hold timer should be cancelled and rearmed (or stopped).
bool jot_key_repeat_observe(JotRepeatState *state, JotRepeatAction action,
                            uint32_t now_ms, int16_t x, int16_t y);
// Called by the timer: returns true only when a repeat should be emitted.
bool jot_key_repeat_due(JotRepeatState *state, uint32_t now_ms);
// Ends a stroke and reports whether it already issued repeat commands.
bool jot_key_repeat_end(JotRepeatState *state);

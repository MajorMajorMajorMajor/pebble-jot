#pragma once
#include <stdbool.h>

// Pure, allocation-free scrolling state for Reader's physical Up/Down hold.
// Button taps remain 60px, but a held button takes smaller animation steps.
// Use Jot's existing 550ms keyboard hold threshold.
#include "ui/key_repeat.h"
enum {
  JOT_READER_REPEAT_DELAY_MS = JOT_REPEAT_DELAY_MS,
  JOT_READER_REPEAT_INTERVAL_MS = 65,
  JOT_READER_REPEAT_CLICK_PX = 60,
  JOT_READER_REPEAT_STEP_PX = 18
};

typedef struct {
  int direction; // -1 = Up, +1 = Down, 0 = idle.
} JotReaderRepeat;

void jot_reader_repeat_begin(JotReaderRepeat *state, int direction);
// Move and clamp. Returns true only if an actual step was applied.
// Reaching either end automatically disarms repeat.
bool jot_reader_repeat_advance(JotReaderRepeat *state, int *position,
                               int scroll_max, int step);
void jot_reader_repeat_cancel(JotReaderRepeat *state);

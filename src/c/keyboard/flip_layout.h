#pragma once
#include <stdbool.h>

// Saved editor orientations in the precise #36 circle-cycle order.
typedef enum {
  JOT_FLIP_LH_TOP = 0,
  JOT_FLIP_RH_TOP = 1,
  JOT_FLIP_RH_BOTTOM = 2,
  JOT_FLIP_LH_BOTTOM = 3,
  JOT_FLIP_COUNT = 4
} JotFlipLayout;

JotFlipLayout jot_flip_validated(int saved);
JotFlipLayout jot_flip_next(JotFlipLayout current, int direction);
bool jot_flip_right(JotFlipLayout mode);
bool jot_flip_bottom(JotFlipLayout mode);
// Only the physical CONTROL COLUMN moves; all three alphabet columns
// retain their reading order and every gesture still maps to one key.
int jot_flip_visual_column(bool right, int logical_col);
int jot_flip_logical_column(bool right, int physical_col);

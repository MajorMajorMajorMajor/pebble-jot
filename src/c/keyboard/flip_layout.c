#include "keyboard/flip_layout.h"

JotFlipLayout jot_flip_validated(int saved) {
  return saved >= 0 && saved < JOT_FLIP_COUNT ?
      (JotFlipLayout)saved : JOT_FLIP_LH_TOP;
}

JotFlipLayout jot_flip_next(JotFlipLayout current, int direction) {
  const int index = jot_flip_validated((int)current);
  if (direction > 0) return (JotFlipLayout)((index + 1) % JOT_FLIP_COUNT);
  if (direction < 0)
    return (JotFlipLayout)((index + JOT_FLIP_COUNT - 1) % JOT_FLIP_COUNT);
  return (JotFlipLayout)index;
}

bool jot_flip_right(JotFlipLayout mode) {
  return mode == JOT_FLIP_RH_TOP || mode == JOT_FLIP_RH_BOTTOM;
}

bool jot_flip_bottom(JotFlipLayout mode) {
  return mode == JOT_FLIP_RH_BOTTOM || mode == JOT_FLIP_LH_BOTTOM;
}

int jot_flip_visual_column(bool right, int logical_col) {
  if (logical_col < 0 || logical_col > 3) return -1;
  return right ? (logical_col == 0 ? 3 : logical_col - 1) :
                 logical_col;
}

int jot_flip_logical_column(bool right, int physical_col) {
  if (physical_col < 0 || physical_col > 3) return -1;
  return right ? (physical_col == 3 ? 0 : physical_col + 1) :
                 physical_col;
}

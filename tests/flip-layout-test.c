#include <assert.h>
#include <stdio.h>
#include "keyboard/flip_layout.h"

int main(void) {
  const JotFlipLayout expected[4] = {
    JOT_FLIP_LH_TOP, JOT_FLIP_RH_TOP,
    JOT_FLIP_RH_BOTTOM, JOT_FLIP_LH_BOTTOM
  };
  for (int i = 0; i < 4; ++i) {
    const JotFlipLayout m = expected[i];
    assert(jot_flip_validated(i) == m);
    assert(jot_flip_right(m) == (i == 1 || i == 2));
    assert(jot_flip_bottom(m) == (i >= 2));
    assert(jot_flip_next(m, 1) == expected[(i + 1) % 4]);
    assert(jot_flip_next(m, -1) == expected[(i + 3) % 4]);
    for (int c = 0; c < 4; ++c) {
      const int v = jot_flip_visual_column(jot_flip_right(m), c);
      assert(v >= 0 && v < 4);
      assert(jot_flip_logical_column(jot_flip_right(m), v) == c);
    }
  }
  assert(jot_flip_validated(-5) == JOT_FLIP_LH_TOP);
  assert(jot_flip_validated(88) == JOT_FLIP_LH_TOP);
  assert(jot_flip_next(JOT_FLIP_LH_TOP, 0) == JOT_FLIP_LH_TOP);
  assert(jot_flip_visual_column(true, -1) == -1);
  assert(jot_flip_logical_column(false, 4) == -1);
  // Right hand function column is visual right; alphabet order unmirrored.
  assert(jot_flip_visual_column(true, 0) == 3);
  assert(jot_flip_visual_column(true, 1) == 0);
  assert(jot_flip_visual_column(true, 2) == 1);
  assert(jot_flip_visual_column(true, 3) == 2);
  puts("Four handedness/editor layouts, wrap and inverse hitboxes passed");
  return 0;
}

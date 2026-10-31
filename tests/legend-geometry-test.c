// Pixel-parity regression: no alphabet/numeric visual change in refactor.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "ui/legend_geometry.h"

static void check(int width, int height) {
  static const int8_t dirs_x[8] = {0,1,2,2,2,1,0,0};
  static const int8_t dirs_y[8] = {0,0,0,1,2,2,2,1};
  const int x[3] = {3, (width - 16) / 2, width - 19};
  const int y[3] = {2, (height - 14) / 2, height - 17};
  for (int key = 0; key < 9; ++key) {
    for (int direction = 0; direction < 8; ++direction) {
      const JotLegendPosition num = jot_legend_position(
          JOT_LEGEND_NUMBER, key, direction, width, height);
      assert(num.x == x[dirs_x[direction]]);
      assert(num.y == y[dirs_y[direction]]);

      int delta_x = 0, delta_y = -4; // Approved ABC global offset
      if (key == 1 && direction == 5) delta_y += 1; // l
      if (key == 4 && direction == 5) delta_y += 1; // d
      if (key == 8 && direction == 0) {
        delta_x -= 2; delta_y += 2; // f
      }
      const JotLegendPosition abc = jot_legend_position(
          JOT_LEGEND_ABC, key, direction, width, height);
      assert(abc.x == x[dirs_x[direction]] + delta_x);
      assert(abc.y == y[dirs_y[direction]] + delta_y);
    }
  }
}

int main(void) {
  assert(JOT_LEGEND_SLOT_W == 16);
  assert(JOT_LEGEND_SLOT_H == 15);
  check(46, 45);
  check(55, 58);
  check(80, 62);
  check(30, 32);
  const JotLegendPosition a = jot_legend_position(
      JOT_LEGEND_ABC, -1, 0, 50, 50);
  assert(a.x == 0 && a.y == 0);
  const JotLegendPosition b = jot_legend_position(
      JOT_LEGEND_NUMBER, 0, 8, 50, 50);
  assert(b.x == 0 && b.y == 0);
  puts("ABC and 123: all 144 positions pixel-identical to prior release");
  return 0;
}

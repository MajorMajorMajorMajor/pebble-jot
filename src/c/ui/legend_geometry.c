#include "ui/legend_geometry.h"

// ONE source of truth for peripheral label geometry.
// Shared baseline: the fully calibrated ABC positioning.
enum {
  LEGEND_BASE_FONT_OFFSET_Y = 4,
  // Numeric labels have not received the ABC optical calibration yet.
// Change this to 0 to try the same corrected baseline on 123.
  NUMBER_FACE_VERTICAL_ADJUST_Y = 4
};

// Optical corrections live HERE for both faces (key, then direction).
// ABC is hardware-approved: please leave these three offsets unchanged.
static const JotLegendPosition s_abc_optical[9][8] = {
  [1][5] = { 0, 1 }, // l  on N: down 1
  [4][5] = { 0, 1 }, // d  on O: down 1
  [8][0] = {-2, 2 }  // f  on S: left 2, down 2
};

// 123 CALIBRATION — safely editable without changing ABC.
// Keys: 0=7, 1=8, 2=9, 3=4, 4=5, 5=6, 6=1, 7=2, 8=3
// Directions: 0=NW, 1=N, 2=NE, 3=E, 4=SE, 5=S, 6=SW, 7=W
// Example (uncomment to try): [1][1] = {0, 1},  // 8's ^ down 1
// Positive X = right, positive Y = down.
static const JotLegendPosition s_number_optical[9][8] = {{0}};

JotLegendPosition jot_legend_position(
    JotLegendFace face, int key, int direction, int width, int height) {
  if ((face != JOT_LEGEND_ABC && face != JOT_LEGEND_NUMBER) ||
      key < 0 || key >= 9 || direction < 0 || direction >= 8)
    return (JotLegendPosition){0, 0};

  static const uint8_t compass_x[8] = {0,1,2,2,2,1,0,0};
  static const uint8_t compass_y[8] = {0,0,0,1,2,2,2,1};
  const int col[3] = {3, (width - 16) / 2, width - 19};
  const int row[3] = {
    2 - LEGEND_BASE_FONT_OFFSET_Y,
    (height - 14) / 2 - LEGEND_BASE_FONT_OFFSET_Y,
    height - 17 - LEGEND_BASE_FONT_OFFSET_Y
  };
  const JotLegendPosition offset = face == JOT_LEGEND_ABC ?
      s_abc_optical[key][direction] : s_number_optical[key][direction];
  return (JotLegendPosition){
    .x = col[compass_x[direction]] + offset.x,
    .y = row[compass_y[direction]] + offset.y +
         (face == JOT_LEGEND_NUMBER ? NUMBER_FACE_VERTICAL_ADJUST_Y : 0)
  };
}

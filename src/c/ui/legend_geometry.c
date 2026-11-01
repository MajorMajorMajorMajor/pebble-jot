#include "ui/legend_geometry.h"

// ONE source of truth for peripheral label geometry.
// Shared baseline: the fully calibrated ABC positioning.
enum {
  LEGEND_BASE_FONT_OFFSET_Y = 4,
  // Numeric labels have not received the ABC optical calibration yet.
// Change this to 0 to try the same corrected baseline on 123  
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
static const JotLegendPosition s_number_optical[9][8] = {
  // "7" key
  [0][3] = {+2, 0},  // E
   
  // "8" key
  [1][0] = {0, 3},   // NW
  [1][1] = {1, 3},   // N
  [1][2] = {0, 3},   // NE
  [1][4] = {2, 0},   // NE

  // "9" key
  [2][5] = {0, 2},   // S
  

  // "4" key
  [3][0] = {-2, 2},  // NW
  [3][2] = {+1, 1},  // NE
  [3][7] = {-2, 2},  // W
  [3][6] = {-2, 2},  // SW
  

  // "6" key
  [5][0] = {-2, 2},  // NW
  [5][2] = {+2, 2},  // NE
  [5][3] = {+2, 2},  // E
  [5][4] = {+2, 2},  // SE
  [5][6] = {+0, 2},  // SW

  // "1" key
  [6][0] = {0, 5},   // NW
  [6][3] = {1, 2},    // E

  // "2" key
  [7][0] = {-1, 3}, // NW
  [7][2] = {+2, 3}, // NE

  // "3" key
  [8][3] = {+2, -1}, // E

};

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
    .y = row[compass_y[direction]] + offset.y
  };
}

// A common optical correction for the *central numeric glyphs* (7-9,
// 4-6, 1-3 and wide 0). Pebble's font text frame isn't its visible ink
// box; all ten numerals looked 1px too high on Emery. Keep this
// face-wide correction separate from per-digit corrections and avoid
// changing UIFont.height_px, which is used in other layouts.
// TODO(#52): source optical alignment from explicit font metrics instead.
enum { NUMBER_CENTER_OFFSET_Y = 1 };

// Big centre glyph offsets. Calibrate individual digits here after wrist
// testing; entries default to zero (apart from the global numeric shift).
// Key indices are the same as for the peripheral compass above.
// ABC's existing approved centre positions are unchanged.
static const JotLegendPosition s_abc_center_optical[9] = {
  [0] = {0, 0}
};

// 123 centre glyphs: indices 0..8 => 7 8 9 / 4 5 6 / 1 2 3,
// index 9 => wide zero. Offset {x, y}: +x right; +y down.
// Example (not enabled): [1] = {-1, 2}, moves '8' left 1, down 2.
static const JotLegendPosition s_number_center_optical[10] = {
  [0] = {0, 0}
};

JotLabelFrame jot_key_text_frame(JotTextPlacement placement,
    JotLegendFace face, int key, JotLabelMetrics m) {
  // Pure arithmetic exactly reproduces the previous Pebble GRect frames.
  // Per-glyph centre offsets apply ONLY to that key and keyboard face.
  JotLegendPosition offset = {0, 0};
  if (placement == JOT_TEXT_CENTER || placement == JOT_TEXT_ZERO) {
    if (face == JOT_LEGEND_ABC && key >= 0 && key < 9)
      offset = s_abc_center_optical[key];
    else if (face == JOT_LEGEND_NUMBER && key >= 0 && key < 10) {
      offset = s_number_center_optical[key];
      offset.y += NUMBER_CENTER_OFFSET_Y;
    }
  }

  switch (placement) {
    case JOT_TEXT_ZERO:
      // The wide 0 was previously painted in a separate special case.
      return (JotLabelFrame){offset.x, m.bounds_h / 2 - 22 + offset.y,
                             m.bounds_w, 36};
    case JOT_TEXT_CENTER:
      return (JotLabelFrame){
          m.box_x + m.box_w / 2 - m.text_w / 2 + offset.x,
          m.box_y + m.box_h / 2 - m.font_h / 2 - 2 + offset.y,
          m.text_w, m.font_h};
    case JOT_TEXT_RIGHT:
      return (JotLabelFrame){
          m.box_x + m.box_w - m.text_w - m.padding,
          m.box_y + m.box_h / 2 - m.font_h / 2,
          m.text_w, m.font_h};
    case JOT_TEXT_BOTTOM:
      // Preserve the original 0.75 multiplier and cast-to-int truncation.
      return (JotLabelFrame){
          m.box_x + m.box_w / 2 - m.text_w / 2,
          (int)(m.box_y + m.box_h - m.font_h * 0.75 - m.padding),
          m.text_w, m.font_h};
  }
  return (JotLabelFrame){0, 0, 0, 0};
}

#pragma once
#include <stdint.h>

// Shared 16x15 compass slots for both keyboard faces.
typedef enum { JOT_LEGEND_ABC, JOT_LEGEND_NUMBER } JotLegendFace;
enum { JOT_LEGEND_SLOT_W = 16, JOT_LEGEND_SLOT_H = 15 };
typedef struct { int16_t x, y; } JotLegendPosition;

// key 0..8: A N I H O R T E S / 7 8 9 4 5 6 1 2 3.
// direction 0..7: NW N NE E SE S SW W.
// Tune ALL base and individual positions in legend_geometry.c.
JotLegendPosition jot_legend_position(
    JotLegendFace face, int key, int direction, int width, int height);

// All text-label frames (ABC, 123, the wide zero, and legacy edge slots)
// use the same pure-C layout calculation. Pebble measures text and paints
// the frame; this module owns every coordinate and optical override.
typedef enum {
  JOT_TEXT_CENTER,
  JOT_TEXT_RIGHT,
  JOT_TEXT_BOTTOM,
  JOT_TEXT_ZERO
} JotTextPlacement;

typedef struct {
  int box_x, box_y, box_w, box_h;
  int text_w, font_h, padding;
  int bounds_w, bounds_h;
} JotLabelMetrics;

typedef struct { int x, y, w, h; } JotLabelFrame;

// Key indices: ABC 0..8 (A N I / H O R / T E S),
// 123 0..8 (7 8 9 / 4 5 6 / 1 2 3), 9=wide zero.
// Out-of-range keys (e.g. ABC toggle) keep unadjusted base geometry.
JotLabelFrame jot_key_text_frame(JotTextPlacement placement,
    JotLegendFace face, int key, JotLabelMetrics metrics);

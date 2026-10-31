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

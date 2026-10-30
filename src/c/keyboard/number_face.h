#pragma once
#include <stdbool.h>

// Same 14 touch cells/19 gesture indices as ABC. Printable bindings
// follow docs/gesture-map.csv; unsupported actions remain reserved.
const char *jot_number_face_binding(int touch_index, int gesture_index);

// NW, N, NE, E, SE, S, SW, W: only visible punctuation is labelled.
// Working cursor/Return/Backspace/Space shortcuts are documented, not drawn.
const char *jot_number_face_legend(int touch_index, int direction);
// Bottom-row numeric cells: 0 uses two columns (index 9), and
// the separate one-column Space uses index 14.
int jot_number_face_bottom_touch(int column);

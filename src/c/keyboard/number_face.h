#pragma once
#include <stdbool.h>

// Same 14 touch cells/19 gesture indices as ABC. Printable bindings
// follow docs/gesture-map.csv; unsupported actions remain reserved.
const char *jot_number_face_binding(int touch_index, int gesture_index);

// Straight swipes in NW, N, NE, E, SE, S, SW, W order. Legends only
// advertise printable symbols; action glyphs are native 1px drawings.
const char *jot_number_face_legend(int touch_index, int direction);
typedef enum {
  JOT_NUM_PICTO_NONE = 0,
  JOT_NUM_PICTO_UP, JOT_NUM_PICTO_DOWN,
  JOT_NUM_PICTO_RETURN, JOT_NUM_PICTO_BACKSPACE,
  JOT_NUM_PICTO_TAB, JOT_NUM_PICTO_SPACE
} JotNumberPictogram;
JotNumberPictogram jot_number_face_pictogram(int touch_index, int direction);

// The bottom-right quarter of the screen is a real mini Space tap
// target, inside the shared wide ZERO key. No geometry/gesture changes.
bool jot_number_face_small_space_tap(int touch_index, int gesture_index,
                                     int start_x, int screen_width);

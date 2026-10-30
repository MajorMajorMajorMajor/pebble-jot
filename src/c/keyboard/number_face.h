#pragma once
// Numeric face: the existing 14 touch indexes and 19 recognizer gestures,
// with every printable output drawn from docs/gesture-map.csv.
const char *jot_number_face_binding(int touch_index, int gesture_index);
// Small NW/N/NE/E/SE/S/SW/W labels for number keys 0..8.
const char *jot_number_face_legend(int touch_index, int direction);

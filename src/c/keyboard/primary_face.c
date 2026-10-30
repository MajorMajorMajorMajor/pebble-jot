#include "keyboard/primary_face.h"

// Compact 9x8 lookup table. Canonical gesture directions: NW/N/NE/E/SE/S/SW/W.
// Compose on A NW is deliberately not implemented or labeled.
static const char s_letters[9][8] = {
  { 0,   0,   0,   0, 'v',  0,   0,   0 }, // a
  { 0,   0,   0,   0,   0, 'l',  0,   0 }, // n
  { 0,   0,   0,   0,   0,   0, 'x',  0 }, // i
  { 0,   0,   0, 'k',   0,   0,   0,  0 }, // h
  {'q', 'u', 'p', 'b', 'j', 'd', 'g', 'c'},    // o
  { 0,   0,   0,   0,   0,   0,   0, 'm'}, // r
  { 0,   0, 'y',   0,   0,   0,   0,  0 }, // t
  { 0, 'w',   0, 'z', ':', '.', ',',  0 }, // e
  {'f',   0,   0,   0,   0,   0,   0,  0 }  // s
};

char jot_primary_face_legend(int key, int direction) {
  if (key < 0 || key >= 9 || direction < 0 || direction >= 8)
    return 0;
  return s_letters[key][direction];
}

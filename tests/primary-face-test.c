#include <assert.h>
#include <stdio.h>
#include "keyboard/primary_face.h"

static void check_legends(void) {
  static const char expected[9][8] = {
    { 0,   0,   0,   0, 'v',  0,   0,   0 },
    { 0,   0,   0,   0,   0, 'l',  0,   0 },
    { 0,   0,   0,   0,   0,   0, 'x',  0 },
    { 0,   0,   0, 'k',   0,   0,   0,  0 },
    {'q', 'u', 'p', 'b', 'j', 'd', 'g', 'c'},
    { 0,   0,   0,   0,   0,   0,   0, 'm'},
    { 0,   0, 'y',   0,   0,   0,   0,  0 },
    { 0, 'w',   0, 'z', ':', '.', ',',  0 },
    {'f',   0,   0,   0,   0,   0,   0,  0 }
  };
  int visible = 0;
  for (int key = 0; key < 9; ++key) {
    for (int dir = 0; dir < 8; ++dir) {
      const char glyph = jot_primary_face_legend(key, dir);
      assert(glyph == expected[key][dir]);
      if (glyph) {
        ++visible;
        assert((glyph >= 'a' && glyph <= 'z') ||
               glyph == ',' || glyph == '.' || glyph == ':');
      }
    }
  }
  assert(visible == 20);
  // Never draw the unimplemented Compose at A NW or a control gesture.
  assert(jot_primary_face_legend(0, 0) == 0);
  for (int key = -2; key < 12; ++key) {
    assert(jot_primary_face_legend(key, -1) == 0);
    assert(jot_primary_face_legend(key, 8) == 0);
  }
  for (int dir = 0; dir < 8; ++dir) {
    assert(jot_primary_face_legend(-1, dir) == 0);
    assert(jot_primary_face_legend(9, dir) == 0);
  }
}

static void check_optical_offsets(void) {
  for (char glyph = 'a'; glyph <= 'z'; ++glyph) {
    const JotPrimaryLegendOffset offset =
        jot_primary_face_optical_offset(glyph);
    assert(offset.x == (glyph == 'f' ? -2 : 0));
    assert(offset.y == (glyph == 'f' ? 2 :
                        (glyph == 'l' || glyph == 'd') ? 1 : 0));
  }
  for (const char *punctuation = ",.:"; *punctuation; ++punctuation) {
    const JotPrimaryLegendOffset offset =
        jot_primary_face_optical_offset(*punctuation);
    assert(offset.x == 0 && offset.y == 0);
  }
}

int main(void) {
  check_legends();
  check_optical_offsets();
  puts("All 20 alphabet primary legends and compass directions verified");
  return 0;
}

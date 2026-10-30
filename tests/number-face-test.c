// Exhaustive mapping regression against docs/gesture-map.csv.
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "keyboard/number_face.h"
static const char *const expected[9][19] = {
  { "7", "", "", "-", "", "", "$", "", "", "", "", "÷", "", "", "¥", "", "", "¹", "¹" },
  { "8", "^", "´", "!", "\\", "", "/", "+", "`", "ˇ", "’", "¡", "—", "", "–", "×", "‘", "²", "²" },
  { "9", "", "\n", "", "€", "=", "", "?", "", "", "\n", "", "£", "±", "", "¿", "", "", "" },
  { "4", "", "%", "", "_", "", "[", "(", "{", "", "‰", "", "¬", "", "]", ")", "}", "¼", "¼" },
  { "5", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "a", "a" },
  { "6", "", "}", ")", "]", "", "@", "", "|", "", "{", "(", "[", "", "ª", "", "¦", "ⁿ", "ⁿ" },
  { "1", "¨", "", "*", "", "", "\b", "<", "~", "˝", "", "†", "", "", "\b", "‹", "˜", "∫", "∫" },
  { "2", "", "'", "", ":", ".", ",", "", "\"", "", "”", "", "„", "…", "‚", "", "“", "Π", "Π" },
  { "3", "&", "°", ">", " ", "", ";", "#", "", "§", "0", "3", " ", "", ";", "£", "", "Σ", "Σ" },
};
int main(void) {
  for (int k=0;k<9;k++)
    for (int g=0;g<19;g++) {
      const char *got=jot_number_face_binding(k,g);
      assert(got && strcmp(got,expected[k][g])==0);
    }
  assert(strcmp(jot_number_face_binding(9,0),"0")==0);
  assert(strcmp(jot_number_face_binding(9,17),"0")==0);
  for (int g=0;g<19;g++) {
    const char *s=jot_number_face_binding(9,g);
    assert(s);
    if (g==4||g==5||g==6||g==12||g==13||g==14||g==18)
      assert(strcmp(s," ")==0);
  }
  assert(jot_number_face_binding(9,-1)==NULL);
  assert(jot_number_face_binding(9,19)==NULL);
  assert(jot_number_face_binding(-1,0)==NULL);
  assert(jot_number_face_binding(10,0)==NULL);
  assert(jot_number_face_binding(0,19)==NULL);
  // Never display a promised action until it actually works.
  assert(jot_number_face_legend(0,0)==NULL); // 7 NW Compose
  assert(jot_number_face_pictogram(0,0)==JOT_NUM_PICTO_NONE);
  assert(jot_number_face_legend(2,1)==NULL);
  assert(jot_number_face_pictogram(2,1)==JOT_NUM_PICTO_UP);
  assert(jot_number_face_pictogram(2,2)==JOT_NUM_PICTO_RETURN);
  assert(jot_number_face_pictogram(6,4)==JOT_NUM_PICTO_NONE);
  assert(jot_number_face_pictogram(6,6)==JOT_NUM_PICTO_BACKSPACE);
  assert(jot_number_face_pictogram(8,4)==JOT_NUM_PICTO_SPACE);
  assert(jot_number_face_pictogram(8,5)==JOT_NUM_PICTO_DOWN);
  assert(jot_number_face_pictogram(7,1)==JOT_NUM_PICTO_NONE);
  assert(jot_number_face_legend(-1,0)==NULL);
  // No unsupported Unicode boxes or invisible controls in the tiny
  // font-based legends. Real UTF-8 gesture bindings remain unchanged.
  for (int key=0;key<9;key++)
    for (int d=0;d<8;d++) {
      const char *label=jot_number_face_legend(key,d);
      if (label) {
        assert(label[0]>=' ' && label[0]<='~');
        assert(label[1]=='\0');
      }
    }
  // 0 genuinely spans two cells and Space has its own cell.
  assert(jot_number_face_bottom_touch(0)==-1);
  assert(jot_number_face_bottom_touch(1)==9);
  assert(jot_number_face_bottom_touch(2)==9);
  assert(jot_number_face_bottom_touch(3)==14);
  assert(jot_number_face_bottom_touch(4)==-1);
  // Unsupported Tab is not advertised or inserted.
  assert(!*jot_number_face_binding(6,4));
  assert(!*jot_number_face_binding(6,12));
  assert(jot_number_face_pictogram(6,4)==JOT_NUM_PICTO_NONE);
  puts("Canonical numeric bindings and eight-direction legends passed");
  return 0;
}

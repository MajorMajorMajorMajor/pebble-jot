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
  { "1", "¨", "", "*", "\t", "", "\b", "<", "~", "˝", "", "†", "\t", "", "\b", "‹", "˜", "∫", "∫" },
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
  assert(jot_number_face_pictogram(6,4)==JOT_NUM_PICTO_TAB);
  assert(jot_number_face_pictogram(6,6)==JOT_NUM_PICTO_BACKSPACE);
  assert(jot_number_face_pictogram(8,4)==JOT_NUM_PICTO_SPACE);
  assert(jot_number_face_pictogram(8,5)==JOT_NUM_PICTO_DOWN);
  assert(jot_number_face_pictogram(7,1)==JOT_NUM_PICTO_NONE);
  assert(jot_number_face_legend(-1,0)==NULL);
  // Big zero on the left two-thirds, tappable Space on the right third.
  assert(!jot_number_face_small_space_tap(9,0,149,200));
  assert(jot_number_face_small_space_tap(9,0,150,200));
  assert(jot_number_face_small_space_tap(9,0,199,200));
  assert(!jot_number_face_small_space_tap(9,0,200,200));
  assert(!jot_number_face_small_space_tap(9,5,199,200));
  assert(!jot_number_face_small_space_tap(8,0,190,200));
  assert(!jot_number_face_small_space_tap(9,0,-1,200));
  assert(!jot_number_face_small_space_tap(9,0,150,0));
  puts("Canonical numeric bindings and eight-direction legends passed");
  return 0;
}

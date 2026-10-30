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
  assert(jot_number_face_legend(0,0));
  assert(strcmp(jot_number_face_legend(2,1),"↑")==0);
  assert(strcmp(jot_number_face_legend(8,5),"↓")==0);
  assert(jot_number_face_legend(-1,0)==NULL);
  puts("Canonical numeric bindings and eight-direction legends passed");
  return 0;
}

// Numeric face mappings are separately testable without the Pebble SDK.
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "keyboard/number_face.h"

int main(void) {
  static const char *const digits[] = {
    "7","8","9","4","5","6","1","2","3"
  };
  static const char *const punct[] = {
    "(",")","/","-",".",":",",","?","!"
  };
  for (int key=0; key<9; ++key) {
    assert(strcmp(jot_number_face_binding(key,0),digits[key])==0);
    assert(strcmp(jot_number_face_binding(key,5),punct[key])==0);
    for (int g=1; g<19; ++g) {
      if(g==5) continue;
      assert(strcmp(jot_number_face_binding(key,g),"")==0);
    }
  }
  assert(strcmp(jot_number_face_binding(9,0),"0")==0);
  assert(strcmp(jot_number_face_binding(9,5)," ")==0);
  assert(strcmp(jot_number_face_binding(9,18)," ")==0);
  assert(strcmp(jot_number_face_binding(9,3),"")==0);
  assert(strcmp(jot_number_face_binding(9,7),"")==0);
  for(int key=10;key<14;++key)
    assert(jot_number_face_binding(key,0)==NULL);
  assert(jot_number_face_binding(-1,0)==NULL);
  assert(jot_number_face_binding(14,0)==NULL);
  puts("123 face: nine digits, punctuation and wide-zero controls OK");
  return 0;
}

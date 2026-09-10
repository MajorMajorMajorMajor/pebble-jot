#include "keys.h"

static Key keys[] = {  
  [KEY_ENGLISH_O] = {
    .label_center = "O"
  },
  [KEY_ENGLISH_N] = {
    .label_center = "N"
  }  
};

Key* get_keys(){
  return keys;
}
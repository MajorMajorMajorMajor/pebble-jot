#include "keys.h"

static const Key s_keys[KEY_COUNT] = {  
  [KEY_ENGLISH_O] = {
    .label_center = "O"
  },
  [KEY_ENGLISH_N] = {
    .label_center = "N"
  }  
};

const Key* get_keys(KeyId key_id){  
  return &s_keys[key_id];
}
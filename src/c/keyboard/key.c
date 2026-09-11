#include "keyboard/key.h"
#include "keyboard/key_label.h"

static const Key s_keys[KEY_COUNT] = {  
  [KEY_ENGLISH_O] = {
    .labels = {
      [KEY_LABEL_CENTER]= {}
  },
  [KEY_ENGLISH_N] = {
    .label_center = "N"
  }  
};

const Key *get_key(KeyId key_id) {
  return &s_keys[key_id];
}
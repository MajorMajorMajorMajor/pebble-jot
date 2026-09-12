#include "keyboard/key.h"
#include "keyboard/key_label.h"

static const Key s_keys[KEY_COUNT] = {  
  [KEY_ENGLISH_O] = {
    .labels = {
      // KeyLabelType type defaults to 0 = KEY_LABEL_TEXT
      [KEY_LABEL_CENTER] = { .text = "o" },
      [KEY_LABEL_RIGHT]  = { .text = "b" }
    }
  },
  [KEY_ENGLISH_N] = {
    .labels = {
      [KEY_LABEL_CENTER] = { .text = "n" }      
    }
  }  
};

const Key *get_key(KeyId key_id) {
  return &s_keys[key_id];
}
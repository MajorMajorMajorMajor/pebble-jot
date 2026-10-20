#include "keyboard/key.h"
#include "keyboard/key_label.h"

static const Key s_keys[KEY_COUNT] = {  
  [KEY_ENGLISH_O] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "o" },
      { .slot = KEY_LABEL_RIGHT,  .type = KEY_LABEL_TYPE_TEXT, .text = "b" },
    }
  },
  [KEY_ENGLISH_N] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "n" },
    }
  }  
};

const Key *get_key(KeyId key_id) {
  return &s_keys[key_id];
}
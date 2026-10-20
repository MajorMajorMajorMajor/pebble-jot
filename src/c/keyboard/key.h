// vim: set sw=2 ts=2 et:
#pragma once

#include "keyboard/key_label.h"

typedef struct {
  const KeyLabel labels[KEY_LABEL_COUNT];
} Key;

typedef enum {
  KEY_ENGLISH_A,
  KEY_ENGLISH_N,
  KEY_ENGLISH_I,
  KEY_ENGLISH_H,
  KEY_ENGLISH_O,
  KEY_ENGLISH_R,
  KEY_ENGLISH_T,
  KEY_ENGLISH_E,
  KEY_ENGLISH_S,

  KEY_ENGLISH_SPACEBAR,
  KEY_ENGLISH_123,
  KEY_ENGLISH_MENU,
  KEY_ENGLISH_BACKSPACE,
  KEY_ENGLISH_RETURN,
  
  KEY_COUNT
} KeyId;

const Key *get_key(KeyId key_id);
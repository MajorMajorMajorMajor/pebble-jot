// vim: set sw=2 ts=2 et:
#pragma once

#include "keyboard/key_label.h"

typedef struct {
  const KeyLabel labels[KEY_LABEL_COUNT];
} Key;

typedef enum {
  KEY_ENGLISH_O,
  KEY_ENGLISH_N,
  KEY_COUNT
} KeyId;

const Key *get_key(KeyId key_id);
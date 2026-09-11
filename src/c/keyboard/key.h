// vim: set sw=2 ts=2 et:
#pragma once

typedef struct {
  const char* label_center;
} Key;

typedef enum {
  KEY_ENGLISH_O,
  KEY_ENGLISH_N,
  KEY_COUNT
} KeyId;

const Key *get_key(KeyId key_id);
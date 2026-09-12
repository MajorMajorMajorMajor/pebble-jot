#pragma once

#include <pebble.h>

typedef enum {  
  KEY_LABEL_CENTER,
  KEY_LABEL_RIGHT,
  KEY_LABEL_COUNT
} KeyLabelSlot;

typedef enum {
  KEY_LABEL_TEXT, // 0 = default
  KEY_LABEL_IMAGE
} KeyLabelType;

typedef struct {  
  KeyLabelType type;
  union {
    char *text;
    GBitmap* image;
  };  
} KeyLabel;

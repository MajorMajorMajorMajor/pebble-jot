#pragma once

#include <pebble.h>

enum params{ NUMBER_OF_KEYS = 16 };

// types
typedef struct Key {  
  char *center_label;
} Key;

// functions
void init_keys();
Key* get_keys();
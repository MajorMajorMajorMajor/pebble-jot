#include <pebble.h>
#include "keyboard.h"

static Key s_keys[NUMBER_OF_KEYS];

void init_keys() {
  s_keys[0] = (Key) {  
    .center_label = "a"
  };
}

Key* get_keys() {
    return s_keys;
}
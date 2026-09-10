#pragma once

#include <pebble.h>
#include "ui.h"

// types
typedef struct Key {
  Layer *layer;
  char *center_label;
} Key;

// functions
void init_keys();
void key_ui_init(Layer *parent_layer, UIDimensions ui);
void key_ui_deinit();

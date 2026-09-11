#pragma once

#include <pebble.h>
#include "ui.h"

typedef struct {
  Layer * text_layer;
} KeyButton;

void key_ui_init(Layer *parent_layer, UIDimensions ui);
void key_ui_deinit();

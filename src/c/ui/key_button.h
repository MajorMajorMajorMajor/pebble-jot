#pragma once

#include <pebble.h>

#include "keyboard/key.h"

// defined in key_button.c
typedef struct KeyButton KeyButton;

KeyButton *key_button_create(Layer *parent_layer, const Key *key);
void key_button_destroy(KeyButton *key_button);
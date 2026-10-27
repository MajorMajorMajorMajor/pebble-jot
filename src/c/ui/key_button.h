#pragma once

#include <pebble.h>

#include "ui/style.h"
#include "keyboard/key.h"

// defined in key_button.c
typedef struct KeyButtonLayer KeyButtonLayer;

KeyButtonLayer *key_button_create(const Key *key, GRect frame, UIStyle *style);
void key_button_destroy(KeyButtonLayer *button);

Layer *key_button_get_layer(KeyButtonLayer *button);

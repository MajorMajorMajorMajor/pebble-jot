#pragma once

#include <pebble.h>

#include "keyboard/key.h"

// defined in key_button.c
typedef struct KeyButton KeyButton;

KeyButton *key_button_create(const Key *key, GRect frame);
void key_button_destroy(KeyButton *key_button);
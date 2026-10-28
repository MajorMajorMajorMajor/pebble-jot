#pragma once

#include <pebble.h>

typedef struct Keyboard Keyboard;

Keyboard *keyboard_create(void);
void keyboard_destroy(Keyboard *keyboard);

Window *keyboard_get_window(Keyboard *keyboard);

GRect keyboard_get_content_frame(Keyboard *keyboard);

void keyboard_set_content_layer(Keyboard *keyboard, Layer* content_layer);

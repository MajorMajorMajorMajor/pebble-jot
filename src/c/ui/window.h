#pragma once

#include <pebble.h>

#include "ui/style.h"
#include "keyboard/state.h"

typedef struct KeyboardWindow KeyboardWindow; // defined in window.c

KeyboardWindow *keyboard_window_create(
  KeyboardState *state, 
  const UIStyle *style
);

void keyboard_window_destroy(KeyboardWindow *self);
Window *keyboard_window_get_window(KeyboardWindow *self);

void keyboard_window_set_content_layer(KeyboardWindow *self, Layer *content_layer);

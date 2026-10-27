#pragma once

#include <pebble.h>

#include "ui/style.h"
#include "keyboard/state.h"

typedef struct KeyboardWindow KeyboardWindow; // defined in window.c

KeyboardWindow *keyboard_window_create(
  KeyboardState *keyboard_state, 
  const UIStyle *style
);

void keyboard_window_destroy(KeyboardWindow *window);

Window *keyboard_window_get_window(void);

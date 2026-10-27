#pragma once

#include <pebble.h>
#include "keyboard/layout.h"

extern const LayoutId KB_WINDOW_LAYOUT;

typedef struct KeyboardWindow KeyboardWindow; // defined in window.c

void kb_window_init(void);
void kb_window_deinit(void);

Window *kb_window_get_window(void);

#pragma once

#include "layout.h"

typedef struct Keyboard Keyboard;

Keyboard *keyboard_create(void);
void keyboard_destroy(Keyboard *keyboard);

LayoutId keyboard_get_active_layout_id(const Keyboard* keyboard);
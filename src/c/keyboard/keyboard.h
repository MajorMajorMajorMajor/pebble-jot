#pragma once

#include "layout.h"

void keyboard_init(void);
void keyboard_deinit(void);

LayoutId keyboard_get_active_layout_id(void);
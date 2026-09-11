#pragma once

#include "layout.h"

void keyboard_init();
void keyboard_deinit();

const Layout *keyboard_get_active_layout();
#pragma once

#include <pebble.h>

#include "ui/style.h"
#include "keyboard/layout.h"

typedef struct KeyGrid KeyGrid; // defined in key_grid.c

KeyGrid *key_grid_create(const Layout *layout, GRect frame, const UIStyle *style);
void key_grid_destroy(KeyGrid *grid);

Layer *key_grid_get_layer(KeyGrid *grid);
// touch index 0..13 from the existing recognizer, or -1 to clear.
void key_grid_highlight_touch_key(KeyGrid *grid, int touch_index);
void key_grid_release_touch_key(KeyGrid *grid);

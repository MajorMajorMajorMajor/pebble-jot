#pragma once

#include <pebble.h>

#include "ui/style.h"
#include "keyboard/layout.h"
#include "keyboard/shift.h"

typedef struct KeyGrid KeyGrid; // defined in key_grid.c

KeyGrid *key_grid_create(const Layout *layout, GRect frame,
                         const UIStyle *style, bool right_hand);
// Reframe the existing keys in place, without allocating another grid.
void key_grid_set_right_hand(KeyGrid *grid, bool right_hand);
void key_grid_set_shift_mode(KeyGrid *grid, JotShiftMode mode);
void key_grid_destroy(KeyGrid *grid);

Layer *key_grid_get_layer(KeyGrid *grid);
// touch index 0..13 from the existing recognizer, or -1 to clear.
void key_grid_highlight_touch_key(KeyGrid *grid, int touch_index);
void key_grid_release_touch_key(KeyGrid *grid);

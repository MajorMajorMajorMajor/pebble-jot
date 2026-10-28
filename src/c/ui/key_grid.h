#pragma once

#include <pebble.h>

#include "ui/style.h"
#include "keyboard/layout.h"

typedef struct KeyGrid KeyGrid; // defined in key_grid.c

KeyGrid *key_grid_create(const Layout *layout, GRect frame, const UIStyle *style);
void key_grid_destroy(KeyGrid *grid);

Layer *key_grid_get_layer(KeyGrid *grid);

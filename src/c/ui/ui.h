#pragma once

#include <pebble.h>

#include "keyboard/layout.h"

typedef struct UIDimensions{
  GSize textbox_size;
  GSize key_grid_size;
} UIDimensions;


UIDimensions layout_compute_ui_dimensions(GSize size);
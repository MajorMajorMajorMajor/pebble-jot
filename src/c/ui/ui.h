#pragma once

#include <pebble.h>

typedef struct UIDimensions{
  GSize textbox_size;
  GSize cell_size;  
} UIDimensions;


UIDimensions layout_compute_ui_dimensions(GSize size);
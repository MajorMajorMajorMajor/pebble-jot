#pragma once

#include <pebble.h>


typedef struct UIStyle {
  struct {
    int x;
  } bar;

} UIStyle;



typedef struct UIDimensions{
  GSize textbox_size;
  GSize key_grid_size;
} UIDimensions;


UIDimensions layout_compute_ui_dimensions(GSize size);
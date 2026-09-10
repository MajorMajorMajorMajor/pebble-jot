#include <pebble.h>
#include "ui.h"

// static UIDimensions s_ui_dimensions;

UIDimensions layout_compute_ui_dimensions(GSize size) {
  int textbox_height = 20;

  int cell_width = size.w / 4;
  int cell_height = (size.h - textbox_height) / 4;

  GSize textbox_size = {
    .w = size.w, 
    .h = textbox_height 
  };

  GSize cell_size = {
    .w = cell_width,
    .h = cell_height
  };

  return (UIDimensions) {
    .textbox_size = textbox_size,
    .cell_size = cell_size
  };
}


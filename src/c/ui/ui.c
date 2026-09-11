#include <pebble.h>
#include "keyboard/layout.h"

#include "ui.h"



// static UIDimensions s_ui_dimensions;

UIDimensions layout_compute_ui_dimensions(GSize size) {
  // Size the textbox
  int textbox_width = size.w;  
  int textbox_height = 40;   

  GSize textbox_size = {
    .w = textbox_width, 
    .h = textbox_height 
  };


  // Remaining vertical space taken up by key grid
  int key_grid_width = size.w;
  int key_grid_height = size.h - textbox_height;
  GSize key_grid_size = {
    .w = key_grid_width,
    .h = key_grid_height
  };

  return (UIDimensions) {
    .textbox_size = textbox_size,
    .key_grid_size = key_grid_size
  };
}


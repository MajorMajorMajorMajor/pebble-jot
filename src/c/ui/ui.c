#include <pebble.h>

#include "ui/window.h"
#include "ui.h"


void ui_init(void) {
  // Initialize keyboard window
  kb_window_init();

  Window *window = kb_window_get_window();
  // Push the keyboard window  
  const bool animated = true;

  window_stack_push(window, animated);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
}


void ui_deinit(void) {
  kb_window_deinit();
}

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

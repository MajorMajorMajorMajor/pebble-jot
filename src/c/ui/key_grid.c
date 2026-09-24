#include <pebble.h>

#include "keyboard/layout.h"

#include "ui/key_button.h"

#include "ui/key_grid.h"

struct KeyGrid {  
  Layer *layer;  

  size_t button_count; // one button per key
  KeyButton **buttons; 
};

// create a key grid
KeyGrid *key_grid_create(const Layout *layout, GRect frame) {
  Layer *layer;
  size_t button_count;
  KeyButton **buttons;  

  int cell_width, cell_height; // geometry in pixels

  // Layer  
  layer = layer_create(frame);

  // divide the frame size evenly into grid cells
  cell_width  = (float) frame.size.w / layout->cols;
  cell_height = (float) frame.size.h / layout->rows;


  // Create buttons
  button_count = layout->key_count;
  buttons = calloc(button_count, sizeof *buttons);

  size_t i;
  for (i = 0; i < button_count; i++) {
    const LayoutKeyPosition *pos = &layout->keys[i];

    int x = cell_width  * pos->col;
    int y = cell_height * pos->row;
    int w = cell_width  * pos->col_span;
    int h = cell_height * pos->row_span;

    GRect button_frame = GRect(x, y, w, h);
    const Key* key = get_key(pos->key_id);

    buttons[i] = key_button_create(key, button_frame);   
    
    // add the buttons to the base layer
    layer_add_child(layer, key_button_get_layer(buttons[i]));
  }

  // Construct new object
  KeyGrid *grid = malloc(sizeof *grid);
  *grid = (KeyGrid){
    .layer = layer, 
    .button_count = button_count, 
    .buttons = buttons
  };  
  
  return grid;
}

void key_grid_destroy(KeyGrid *grid) {
  KeyGrid *this = grid;

  // destroy the buttons
  size_t i;
  for (i = 0; i < this->button_count; i++) {
    key_button_destroy(this->buttons[i]);
  }

  // destroy the base layer
  layer_destroy(this->layer);
}

// shows the grid for `layout_id` and hides the rest
static void prv_show(LayoutId layout_id) {
  KeyGrid *grid;

  for (LayoutId i = 0; i < LAYOUT_COUNT; i++) {
    grid = &s_key_grids[i];

    layer_set_hidden(grid->layer, layout_id != i); // hide non-matching layouts
  }
}
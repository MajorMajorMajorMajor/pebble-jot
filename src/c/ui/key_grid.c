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
  
  // Layer  
  layer = layer_create(frame);

  // Create buttons
  button_count = layout->key_count;
  buttons = calloc(button_count, sizeof *buttons);

  size_t i;
  for (i = 0; i < button_count; i++) {
    const LayoutKeyPosition *pos = &layout->keys[i];

    // divide the frame size evenly into grid cells
    int x1 = frame.size.w * pos->col / layout->cols;
    int x2 = frame.size.w * (pos->col + pos->col_span) / layout->cols;

    int y1 = frame.size.h * pos->row / layout->rows;
    int y2 = frame.size.h * (pos->row + pos->row_span) / layout->rows;

    GRect button_frame = GRect(x1, y1, x2 - x1, y2 - y1);
    const Key* key = get_key(pos->key_id);

    buttons[i] = key_button_create(key, button_frame);
   
    // if a button failed to create, roll back the creation process
    if (!buttons[i]) {
      APP_LOG(APP_LOG_LEVEL_ERROR,  "Failed to create key button %u (key_id=%u)", i, (unsigned)pos->key_id);      
      for (size_t j = 0; j < i; j++) {
        key_button_destroy(buttons[j]);
      }

      free(buttons);
      layer_destroy(layer);
      return NULL;
    }
    
    // add the button to the base layer
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
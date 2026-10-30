#include <pebble.h>

#include "keyboard/layout.h"

#include "ui/key_button.h"

#include "ui/key_grid.h"

// Warm paper shows through the inset key borders and rounded corners.
// These are drawing details only: KeyButton frames remain the same.
static void prv_draw_notebook_grid(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorFromRGB(249, 242, 219));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, GColorFromRGB(230, 220, 196));
  graphics_context_set_stroke_width(ctx, 1);
  for (int y = 18; y < bounds.size.h; y += 24) {
    graphics_draw_line(ctx, GPoint(0, y),
                       GPoint(bounds.size.w - 1, y));
  }
}

struct KeyGrid {  
  Layer *layer;  

  size_t button_count; // one button per key
  KeyButtonLayer **buttons; 
};

// create a key grid
KeyGrid *key_grid_create(const Layout *layout, GRect frame, const UIStyle *style) {
  Layer *layer;
  size_t button_count;
  KeyButtonLayer **buttons;  
  
  // Layer  
  layer = layer_create(frame);
  if(!layer) {
    return NULL;
  }
  layer_set_update_proc(layer, prv_draw_notebook_grid);

  // Create buttons
  button_count = layout->key_count;
  buttons = calloc(button_count, sizeof *buttons);
  if (!buttons) {
    layer_destroy(layer);
    return NULL;
  }

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

    buttons[i] = key_button_create(key, button_frame, style);
   
    // if a button failed to create, roll back the creation process
    if (!buttons[i]) {
      APP_LOG(APP_LOG_LEVEL_ERROR,  "Failed to create key button %u (key_id=%u)", i, (unsigned)pos->key_id);      
      for (size_t j = 0; j < i; j++) {
        if(buttons[j]) {
          key_button_destroy(buttons[j]);
          buttons[j] = NULL;
        }
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
  if(!grid){
    for (i = 0; i < button_count; i++) {
      key_button_destroy(buttons[i]);
    }
    
    free(buttons);
    layer_destroy(layer);
    return NULL;
  }
  *grid = (KeyGrid){
    .layer = layer, 
    .button_count = button_count, 
    .buttons = buttons
  };  
  
  return grid;
}

void key_grid_destroy(KeyGrid *grid) {  
  KeyGrid *this = grid;
  if (!this)
    return;

  // destroy the buttons
  size_t i;
  for (i = 0; i < this->button_count; i++) {

    if (this->buttons[i]) {
      key_button_destroy(this->buttons[i]);
      this->buttons[i] = NULL;
    }    
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Destroying layer at %p", this->layer);
  // destroy the base layer
  if (this->layer) {
      layer_destroy(this->layer);
      this->layer = NULL;
  }

  free(this->buttons);
  free(this);
}

Layer *key_grid_get_layer(KeyGrid *grid) {
  return grid->layer;
}

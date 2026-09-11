#include <pebble.h>
#include "keyboard/keyboard.h"
#include "keyboard/layout.h"

#include "ui/ui.h"
#include "ui/key_button.h"

// bind together the physical layer object with the layout it's supposed to represent
typedef struct {
  Layer *layer;
  const Layout *layout;
} KeyGrid;

// one grid per layout
static KeyGrid s_key_grids[LAYOUT_COUNT];

// create a key grid
static void prv_create(Layer *parent_layer, LayoutId layout_id) {    
  // Layer
  GRect frame = layer_get_bounds(parent_layer);
  Layer *grid_layer = layer_create(frame);

  // Layout
  const Layout *layout = get_layout(layout_id);

  // Instantiate grid
  s_key_grids[layout_id] = (KeyGrid){.layer = grid_layer, .layout=layout};

  // Create buttons

  // Add layers to the hierarchy
  layer_add_child(parent_layer, grid_layer);
}

static void prv_destroy(LayoutId layout_id) {      
  // destroy layer
  Layer *layer = s_key_grids[layout_id].layer;
  layer_destroy(layer);
}

// shows the grid for `layout_id` and hides the rest
static void prv_show(LayoutId layout_id) {  
  KeyGrid *grid;

  for (LayoutId i = 0; i < LAYOUT_COUNT; i++) {
    grid = &s_key_grids[i];

    layer_set_hidden(grid->layer, layout_id != i); // hide non-matching layouts
  }
}

void key_grid_ui_init(Layer *parent_layer, UIDimensions ui) {    
  // create grids for all layouts
  for (LayoutId i = 0; i < LAYOUT_COUNT; i++) {
    prv_create(parent_layer, i);
  }

  // show the initial layout
  LayoutId initial_layout_id = keyboard_get_active_layout_id();
  prv_show(initial_layout_id);
}

void key_grid_ui_deinit() {      
  for (LayoutId i = 0; i < LAYOUT_COUNT; i++) {
    prv_destroy(i);
  }
}
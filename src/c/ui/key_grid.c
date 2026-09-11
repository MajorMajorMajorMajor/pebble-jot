#include <pebble.h>
#include "keyboard/keyboard.h"
#include "keyboard/layout.h"

#include "ui/ui.h"

// bind together the physical layer object with the layout it's supposed to represent
typedef struct KeyGrid {
  Layer layer;
  Layout layout;
}

// one grid per layout
static Layer *s_grid_layers[LAYOUT_COUNT];

static Layer *prv_make_grid_layer(Layout L, UIDimensions ui){
  
}


void key_grid_ui_init(Layer *parent_layer, UIDimensions ui) {    
  const Layout *initial_layout = keyboard_get_active_layout();

  grid_layer = prv_make_grid_layer(initial_layout, ui);

  initial_layout->keys;
}
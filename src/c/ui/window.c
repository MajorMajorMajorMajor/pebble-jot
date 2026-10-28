#include <pebble.h>

#include "common.h"

#include "keyboard/layout.h"
#include "keyboard/state.h"

#include "ui/style.h"
#include "ui/key_grid.h"
#include "ui/click.h"

#include "ui/window.h"

// later, when we have other windows (KeyboardWindow, SettingsWindow, etc.),
// we can factor out the common code into UIWindow and include a UIWindow here 
// instead of a Window.
struct KeyboardWindow {
  // abstract data
  KeyboardState *state;  
  const UIStyle *style;

  // concrete UI elements
  Window *window;
  KeyGrid *grid;
  Layer *content_layer;
};

// Future logic for showing different key grids
// shows the grid for `layout_id` and hides the rest
// static void prv_show(LayoutId layout_id) {
//   KeyGrid *grid;

//   for (LayoutId i = 0; i < LAYOUT_COUNT; i++) {
//     grid = &s_key_grids[i];

//     layer_set_hidden(grid->layer, layout_id != i); // hide non-matching layouts
//   }
// }

// forward declarations of private methods
static void prv_detach_layer(KeyboardWindow *self, Layer* layer);

UIDimensions compute_ui_dimensions(const KeyboardWindow *self) {
  Layer *root_layer = window_get_root_layer(self->window);  

  const GRect bounds = layer_get_bounds(root_layer);

  const unsigned int content_height = self->style->content_height_px;
  
  // content frame uses the whole width and goes at the top of the frame
  GRect content_frame = bounds;
  content_frame.size.h = content_height;

  // key button grid uses the whole width and goes below the content
  GRect grid_frame = bounds;
  grid_frame.origin.y = content_height;
  grid_frame.size.h -= content_height;  // subtract the content height

  return (struct UIDimensions) {
    .content_frame = content_frame,
    .grid_frame = grid_frame
  };
}


static void prv_window_load(Window *window) {
  KeyboardWindow *self = window_get_user_data(window);  
  PEBBLE_ASSERT(self);

  KeyboardState *keyboard_state = self->state;
  
  // Build components
  Layer *root_layer = window_get_root_layer(window);
    
  // Establish dimensions
  UIDimensions dimensions = compute_ui_dimensions(self);

  // ## 1. Grid Layer  

  // TODO: move this logic into calculate_grid_frame()
  const Layout *layout = get_layout(keyboard_state->active_layout_id);  

  // TODO: Refactor key_grid to accept style
  self->grid = key_grid_create(layout, dimensions.grid_frame);
  PEBBLE_ASSERT(self->grid);
  
  Layer *grid_layer = key_grid_get_layer(self->grid);
  
  // add the grid to the root layer  
  layer_add_child(root_layer, grid_layer);
  

  // ## 2. Content Layer
  keyboard_window_set_content_layer(self, self->content_layer);  
}

// Pebble OS invokes this when the window is destroyed, or (in the future) 
// to free resources bound to the window when it is not on screen. 
static void prv_window_unload(Window *window) {
  KeyboardWindow *self = window_get_user_data(window);  
  PEBBLE_ASSERT(self);

  if (self->grid) {
    key_grid_destroy(self->grid); 
    self->grid = NULL;
  }

  // Don't unload the content layer, it's owned by the client app;
  // Just detach it if it's still on our window
  if(self->content_layer) {
    prv_detach_layer(self, self->content_layer);
  }
}

KeyboardWindow *keyboard_window_create(KeyboardState *state, const UIStyle *style) {
  // Allocate space for struct on the heap
  KeyboardWindow *self = malloc(sizeof *self);
  if (!self) {
    return NULL;
  }

  // Create the Pebble window
  Window *window = window_create();
  if (!window) {
    free(self);
    return NULL;
  }
  
  // Populate data object
  *self = (KeyboardWindow) {
    .window = window,
    .state = state,
    .style = style,

    // populated on load
    .grid = NULL,

    // populated by consumer
    .content_layer = NULL,
  };

  // Store reference to the data object inside the window so .load and .unload handlers can access it
  window_set_user_data(window, self);

  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  
  return self;
}

// Invoked by controlling UI code to indicate that this window needs to be destroyed
void keyboard_window_destroy(KeyboardWindow *self) {    
  if (!self) {
    return;
  }

  // destroy the window
  window_destroy(self->window);
  self->window = NULL;

  // free thyself
  free(self);
}

Window *keyboard_window_get_window(KeyboardWindow *self) {
  return self->window;
}

static void prv_detach_layer(KeyboardWindow *self, Layer* layer) {
  if(!self || !layer) {
    return;
  }
   
  // if the layer is currently attached to our window, detach it
  if(self->window && layer_get_window(layer) == self->window) {    
    layer_remove_from_parent(self->content_layer);
  }
}

void keyboard_window_set_content_layer(KeyboardWindow *self, Layer *content_layer) {
  // first, let's check if the content layer is being unset
  if (content_layer == NULL && self->content_layer) {
    prv_detach_layer(self, self->content_layer); // detach previous content layer
  }
  
  // update local reference
  self->content_layer = content_layer;

  // place the layer if it exists
  if (!content_layer) {
    return;
  }  
  
  // resize the layer to the content dimensions
  UIDimensions dimensions = compute_ui_dimensions(self);
  layer_set_frame(content_layer, dimensions.content_frame);  

  // if it's already attached to our window, we're done
  if(layer_get_window(content_layer) == self->window) {
    return;
  }

  // otherwise, add it the content layer (behind the grid)
  Layer *grid_layer = key_grid_get_layer(self->grid);
  layer_insert_below_sibling(content_layer, grid_layer);  
}

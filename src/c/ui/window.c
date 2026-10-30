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
static KeyboardWindow *s_loaded_keyboard;

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
static void prv_detach_content_layer(KeyboardWindow *self);
static void prv_attach_content_layer(KeyboardWindow *self);

UIDimensions compute_ui_dimensions(const KeyboardWindow *self) {
  Layer *root_layer = window_get_root_layer(self->window);  

  const GRect bounds = layer_get_bounds(root_layer);

  const unsigned int content_height = self->style->content_height_px;
  
  // content frame uses the whole width and goes at the top of the frame
  GRect content_frame = bounds;
  content_frame.size.h = content_height;

  // key button grid uses the whole width and goes below the content
  GRect grid_frame = bounds;
  grid_frame.origin.y += content_height; // align the top of the grid to the bottom of the content
  grid_frame.size.h -= content_height;   // use the remaining space for the grid

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

  // create grid layer
  const Layout *layout = get_layout(keyboard_state->active_layout_id);    
  self->grid = key_grid_create(layout, dimensions.grid_frame, self->style);
  PEBBLE_ASSERT(self->grid);
      
  // add grid layer to the window
  Layer *grid_layer = key_grid_get_layer(self->grid);
  layer_add_child(root_layer, grid_layer);
  s_loaded_keyboard = self;

  // add content layer to the window
  prv_attach_content_layer(self);
}

// Pebble OS invokes this when the window is destroyed, or (in the future) 
// to free resources bound to the window when it is not on screen. 
static void prv_window_unload(Window *window) {
  KeyboardWindow *self = window_get_user_data(window);
  if (s_loaded_keyboard == self) s_loaded_keyboard = NULL;  
  PEBBLE_ASSERT(self);

  // Don't unload the content layer, it's owned by the client app;
  // Just detach it if it's still on our window
  if(self->content_layer) {
    prv_detach_content_layer(self);
  }

  // Destroy the things we created ourselves on .load
  if (self->grid) {
    key_grid_destroy(self->grid); 
    self->grid = NULL;
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

static void prv_attach_content_layer(KeyboardWindow *self) {  
  // Layer can only be attached if if we got far enough into the .load to place the grid
  if (!self->content_layer || !self->grid) { 
    return;
  }

  // resize the layer to the content dimensions
  UIDimensions dimensions = compute_ui_dimensions(self);
  layer_set_frame(self->content_layer, dimensions.content_frame);  
  
  // if it's already attached to our window, this will remove it and reattach it
  // in the correct z-order
  Layer *grid_layer = key_grid_get_layer(self->grid);
  if (grid_layer) {
    layer_insert_below_sibling(self->content_layer, grid_layer);  
  }
}

static void prv_detach_content_layer(KeyboardWindow *self) {
  if(!self || !self->content_layer) {
    return;
  }
   
  // if the layer is currently attached to our window, detach it
  if(self->window && layer_get_window(self->content_layer) == self->window) {    
    layer_remove_from_parent(self->content_layer);
  }
}

// Resizes the supplied layer with layer_set_frame to fit into the content area of the keyboard window.
// Before destroying the layer, be sure to detach it by setting the keyboard content layer to NULL.
void keyboard_window_set_content_layer(KeyboardWindow *self, Layer *content_layer) {
  // first, let's check if the content layer is being unset
  if (content_layer != self->content_layer) {    
    prv_detach_content_layer(self);
    self->content_layer = content_layer;
  }
    
  prv_attach_content_layer(self);
}

void keyboard_window_highlight_touch_key(int touch_index) {
  if (s_loaded_keyboard && s_loaded_keyboard->grid)
    key_grid_highlight_touch_key(s_loaded_keyboard->grid, touch_index);
}

void keyboard_window_release_touch_key(void) {
  if (s_loaded_keyboard && s_loaded_keyboard->grid)
    key_grid_release_touch_key(s_loaded_keyboard->grid);
}

bool keyboard_window_number_face_active(void) {
  return s_loaded_keyboard && s_loaded_keyboard->state &&
         s_loaded_keyboard->state->active_layout_id == LAYOUT_ENGLISH_123;
}

bool keyboard_window_toggle_number_face(void) {
  KeyboardWindow *self = s_loaded_keyboard;
  if (!self || !self->grid || !self->state) return false;
  const LayoutId next_id = self->state->active_layout_id ==
                           LAYOUT_ENGLISH_ABC ? LAYOUT_ENGLISH_123 :
                                                 LAYOUT_ENGLISH_ABC;
  const UIDimensions frames = compute_ui_dimensions(self);
  // Allocate before removing the current face. Failed allocations leave
  // existing keys fully usable, with no half-switched state.
  KeyGrid *next = key_grid_create(get_layout(next_id),
                                 frames.grid_frame, self->style);
  if (!next) return false;
  KeyGrid *old = self->grid;
  layer_add_child(window_get_root_layer(self->window),
                  key_grid_get_layer(next));
  self->grid = next;
  self->state->active_layout_id = next_id;
  prv_attach_content_layer(self);
  key_grid_destroy(old); // Also cancels any outstanding key pulse.
  return true;
}

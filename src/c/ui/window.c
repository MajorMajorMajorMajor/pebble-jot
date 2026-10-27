#include <pebble.h>

#include "common.h"

#include "keyboard/layout.h"
#include "keyboard/state.h"

#include "ui/style.h"
#include "ui/key_grid.h"
#include "ui/alert.h"
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


static void prv_window_load(Window *window) {
  KeyboardWindow *self = window_get_user_data(window);  
  PEBBLE_ASSERT(self);

  KeyboardState *keyboard_state = self->state;
  
  // Build components
  Layer *root_layer = window_get_root_layer(window);
  
  // Establish dimensions
  const GRect bounds = layer_get_bounds(root_layer);
  const UIDimensions ui = layout_compute_ui_dimensions(bounds.size);


  APP_LOG(APP_LOG_LEVEL_DEBUG, "Window: origin=(%d, %d), size=(%d, %d)", 
    bounds.origin.x, bounds.origin.y, bounds.size.w, bounds.size.h);

    
  // ## 1. Grid Layer  

  // TODO: move this logic into calculate_grid_frame()
  const Layout *layout = get_layout(keyboard_state->active_layout_id);
  const GRect grid_frame = GRect(
    0, // align to left edge
    ui.textbox_size.h, 
    ui.key_grid_size.w, 
    ui.key_grid_size.h
  );  
  self->grid = key_grid_create(layout, grid_frame);
  PEBBLE_ASSERT(self->grid);
  
  Layer *grid_layer = key_grid_get_layer(self->grid);
  
  // add the grid to the root layer  
  layer_add_child(root_layer, grid_layer);
  

  // ## 2. Content Layer

  // Use the supplied content layer, else fall back to a debug view
  if (!self->content_layer) {
    TextLayer *alert_layer = alert_ui_init(ui);
    PEBBLE_ASSERT(alert_layer);

    self->content_layer = text_layer_get_layer(alert_layer);
  }

  // set the frame size
  // TODO: move this logic into calculate_content_frame()  
  GRect content_frame = {0};
  content_frame.size = ui.textbox_size;      
  layer_set_frame(self->content_layer, content_frame);  

  // add it the content layer (behind the grid)
  layer_insert_below_sibling(self->content_layer, grid_layer);
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

  // Don't unload the content layer, it's owned by the client app
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

  if (self->content_layer) {        
    // oops! Consumers should set content layer to NULL first!!
    alert_ui_deinit();    
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

void keyboard_window_set_content_layer(
    KeyboardWindow *self, Layer *content_layer) {

  return;
}

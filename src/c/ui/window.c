#include <pebble.h>

#include "common.h"
#include "keyboard/layout.h"

// #include "ui/ui.h"
#include "ui/key_grid.h"
#include "ui/alert.h"
#include "ui/click.h"

#include "ui/window.h"

const LayoutId KB_WINDOW_LAYOUT = LAYOUT_ENGLISH_ABC;

// later, when we have other windows (KeyboardWindow, SettingsWindow, etc.),
// we can factor out the common code into UIWindow and include a UIWindow here 
// instead of a Window.
struct KeyboardWindow {
  Window *window;

  KeyGrid *grid;
};

typedef struct KeyboardWindow KeyboardWindow; // defined in window.c

static KeyboardWindow s_window;

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
  Layer *window_layer = window_get_root_layer(window);
  
  // Establish dimensions
  const GRect bounds = layer_get_bounds(window_layer);
  const UIDimensions ui = layout_compute_ui_dimensions(bounds.size);


  APP_LOG(APP_LOG_LEVEL_DEBUG, "Window: origin=(%d, %d), size=(%d, %d)", bounds.origin.x, bounds.origin.y, bounds.size.w, bounds.size.h);

  // Make grid
  const GRect grid_bounds = GRect(
    0, // align to left edge
    ui.textbox_size.h, 
    ui.key_grid_size.w, 
    ui.key_grid_size.h
  );
  const Layout *layout = get_layout(KB_WINDOW_LAYOUT);
  s_window.grid = key_grid_create(layout, grid_bounds);
  PEBBLE_ASSERT(s_window.grid);
  layer_add_child(window_layer, key_grid_get_layer(s_window.grid));
  
  // Make alert area  
  TextLayer* alert_layer = alert_ui_init(ui);
  PEBBLE_ASSERT(alert_layer);  
  layer_add_child(window_layer, text_layer_get_layer(alert_layer));
}

// Pebble OS invokes this when the window is destroyed, or (in the future) 
// to free resources bound to the window when it is not on screen. 
static void prv_window_unload(Window *window) {  
  if (s_window.grid) {
    key_grid_destroy(s_window.grid); 
    s_window.grid = NULL;
  }
  alert_ui_deinit();
}

// Invoked by controlling UI code
void kb_window_init(void) {
  PEBBLE_ASSERT(!s_window.window); // don't initialize on top of an existing window 

  // Create the window
  Window *window = window_create();  
  PEBBLE_ASSERT(window);

  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  
  // Populate static structure
  s_window = (KeyboardWindow) {
    .window = window,
    .grid = NULL
  };

  
}

// Invoked by controlling UI code to indicate that this window needs to be destroyed
void kb_window_deinit(void) {
  window_destroy(s_window.window);
  s_window.window = NULL; // 
}

Window *kb_window_get_window(void) {
  return s_window.window;
}

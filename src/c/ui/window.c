#include <pebble.h>

#include "ui.h"
#include "key_button.h"
#include "alert.h"
#include "click.h"
#include "window.h"


static Window *s_window;


static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  UIDimensions ui = layout_compute_ui_dimensions(bounds.size);
  

  alert_ui_init(window_layer, ui);
  
  key_ui_init(window_layer, ui);  
}


static void prv_window_unload(Window *window) {
  alert_ui_deinit();
  key_ui_deinit();
}

void window_init(void) {
  s_window = window_create();
  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;

  window_stack_push(s_window, animated);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);
}

void window_deinit(void) {
  window_destroy(s_window);
}

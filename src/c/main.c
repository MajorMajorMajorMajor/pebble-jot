// vim: set sw=2 ts=2 et:
#include <pebble.h>

#include "common.h"
#include "keyboard/keyboard.h"

#include "ui/alert.h"

// TODO: move this into keyboard window
#include "ui/touch.h"


int main(void) {
  // initialize systems
  Keyboard *keyboard = keyboard_create();
  PEBBLE_ASSERT(keyboard);
  
  Window *keyboard_window = keyboard_get_window(keyboard);

  // set up content layer
  GRect frame = keyboard_get_content_frame(keyboard);
  TextLayer *alert_layer = alert_ui_init(frame);

  keyboard_set_content_layer(keyboard, (Layer *)alert_layer);

  // Push the keyboard window  
  const bool animated = true;
  window_stack_push(keyboard_window, animated);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", keyboard_window);

  touch_init();  

  // trigger main loop
  app_event_loop();

  // deinitialize systems
  touch_deinit();
  keyboard_destroy(keyboard);  
}

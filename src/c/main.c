// vim: set sw=2 ts=2 et:
#include <pebble.h>
#include "keyboard/keyboard.h"
#include "ui/window.h"
#include "ui/touch.h"


int main(void) {
  // initialize systems
  keyboard_init();
  window_init();  
  touch_init();  

  // trigger main loop
  app_event_loop();

  // deinitialize systems
  touch_deinit();
  window_deinit();
  keyboard_deinit();
}

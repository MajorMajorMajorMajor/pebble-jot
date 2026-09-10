// vim: set sw=2 ts=2 et:
#include <pebble.h>
#include "ui/window.h"
#include "ui/touch.h"


int main(void) {
  // initialize systems
  window_init();  
  touch_init();
  // init_keys();

  // trigger main loop
  app_event_loop();

  // deinitialize systems
  touch_deinit();
  window_deinit();
}

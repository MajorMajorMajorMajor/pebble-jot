// vim: set sw=2 ts=2 et:
#include <pebble.h>
#include "window.h"
#include "touch.h"

int main(void) {
  // initialize systems
  window_init();  
  touch_init();

  // trigger main loop
  app_event_loop();

  // deinitialize systems
  touch_deinit();
  window_deinit();
}

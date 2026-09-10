// vim: set sw=2 ts=2 et:
#include <pebble.h>
#include "window.h"

int main(void) {
  window_init();  

  app_event_loop();

  window_deinit();
}

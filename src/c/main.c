// vim: set sw=2 ts=2 et:
#include <pebble.h>

// keyboard logic
#include "keyboard/keyboard.h"

// ui controls
#include "ui/ui.h"
#include "ui/touch.h"


int main(void) {
  // initialize systems
  keyboard_init();
  ui_init();  
  touch_init();  

  // trigger main loop
  app_event_loop();

  // deinitialize systems

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Begin deinitialization");
  touch_deinit();
  ui_deinit();
  keyboard_deinit();
}

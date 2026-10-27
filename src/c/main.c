// vim: set sw=2 ts=2 et:
#include <pebble.h>

// keyboard logic
#include "common.h"
#include "keyboard/keyboard.h"

// ui controls
#include "ui/ui.h"
#include "ui/touch.h"


int main(void) {
  // initialize systems
  Keyboard *keyboard = keyboard_create();
  PEBBLE_ASSERT(keyboard);
  
  // keyboard_init();
  ui_init();  
  touch_init();  

  // trigger main loop
  app_event_loop();

  // deinitialize systems
  touch_deinit();
  ui_deinit();
  keyboard_destroy(keyboard);  
}

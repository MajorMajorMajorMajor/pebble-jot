#include <pebble.h>

#include "ui/style.h"

#include "keyboard/layout.h"
#include "keyboard/keyboard.h"

struct Keyboard {
  LayoutId active_layout_id;
  UIStyle style;
};

Keyboard *keyboard_create(void) {
  // Create keyboard object
  Keyboard *keyboard = malloc(sizeof *keyboard);
  if(!keyboard) {
    return NULL;
  }  

  // Set initial layout
  keyboard->active_layout_id = LAYOUT_ENGLISH_ABC;  

  // Style
  keyboard->style = get_ui_style();

  return keyboard;
}

void keyboard_destroy(Keyboard *keyboard){
  free(keyboard);
}

LayoutId keyboard_get_active_layout_id(const Keyboard* keyboard) {
  return keyboard->active_layout_id;
}

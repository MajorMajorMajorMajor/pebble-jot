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
  Keyboard *new_keyboard = malloc(sizeof *new_keyboard);
  if(!new_keyboard) {
    return NULL;
  }  

  // Set initial layout
  new_keyboard->active_layout_id = LAYOUT_ENGLISH_ABC;  

  // Style
  new_keyboard->style = get_ui_style();

  return new_keyboard;
}

void keyboard_destroy(Keyboard *keyboard){
  free(keyboard);
}

LayoutId keyboard_get_active_layout_id(const Keyboard* keyboard) {
  return keyboard->active_layout_id;
}

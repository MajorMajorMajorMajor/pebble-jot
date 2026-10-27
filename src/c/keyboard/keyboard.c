#include <pebble.h>

#include "keyboard/layout.h"
#include "keyboard/keyboard.h"

struct Keyboard {
  LayoutId active_layout_id;
};

Keyboard *keyboard_create(void) {
  Keyboard *new_keyboard = malloc(sizeof *new_keyboard);
  if(!new_keyboard) {
    return NULL;
  }
  new_keyboard->active_layout_id = LAYOUT_ENGLISH_ABC;

  return new_keyboard;
}

void keyboard_destroy(Keyboard *keyboard){
  free(keyboard);
}

LayoutId keyboard_get_active_layout_id(const Keyboard* keyboard) {
  return keyboard->active_layout_id;
}
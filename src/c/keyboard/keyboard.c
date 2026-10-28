#include <pebble.h>

#include "common.h"

#include "ui/style.h"
#include "ui/window.h"

#include "keyboard/layout.h"
#include "keyboard/state.h"

#include "keyboard/keyboard.h"


struct Keyboard {
  KeyboardState state;  
  UIStyle style;
  KeyboardWindow *window;
};

Keyboard *keyboard_create(void) {
  // Create keyboard object
  Keyboard *keyboard = malloc(sizeof *keyboard);
  if(!keyboard) {
    return NULL;
  }  

  // Set initial layout
  keyboard->state = (KeyboardState){0};
  keyboard->state.active_layout_id = LAYOUT_ENGLISH_ABC;  

  // Style
  keyboard->style = get_ui_style();  

  // Window
  keyboard->window = keyboard_window_create(
    &keyboard->state,
    &keyboard->style
  );
  if (!keyboard->window) {
    keyboard_destroy(keyboard);
    return NULL;
  }

  return keyboard;
}

void keyboard_destroy(Keyboard *keyboard) {
  if (!keyboard) {
    return;
  }

  if (keyboard->window) {
    keyboard_window_destroy(keyboard->window);
    keyboard->window = NULL;
  }

  free(keyboard);
}


Window *keyboard_get_window(Keyboard *keyboard) {
  if(!keyboard) {
    return NULL;
  }

  return keyboard_window_get_window(keyboard->window);
}

GRect keyboard_get_content_frame(Keyboard *keyboard) {  
  return compute_ui_dimensions(keyboard->window).content_frame;
}

void keyboard_set_content_layer(Keyboard *keyboard, Layer* content_layer) {
  PEBBLE_ASSERT(keyboard);

  keyboard_window_set_content_layer(keyboard->window, content_layer);
}

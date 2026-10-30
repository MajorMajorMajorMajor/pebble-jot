#pragma once

#include <pebble.h>

#include "ui/style.h"
#include "keyboard/state.h"

typedef struct KeyboardWindow KeyboardWindow; // defined in window.c

typedef struct UIDimensions {
  GRect content_frame;
  GRect grid_frame;
} UIDimensions;

KeyboardWindow *keyboard_window_create(KeyboardState *state, const UIStyle *style);
void keyboard_window_destroy(KeyboardWindow *self);

Window *keyboard_window_get_window(KeyboardWindow *self);

void keyboard_window_set_content_layer(KeyboardWindow *self, Layer *content_layer);

UIDimensions compute_ui_dimensions(const KeyboardWindow *self);
// Visual feedback uses only the currently loaded keyboard, if any.
void keyboard_window_highlight_touch_key(int touch_index);

void keyboard_window_release_touch_key(void);

// Switch between equally-sized 4x4 faces; does not recreate the editor.
// Returns false if the alternate grid could not be allocated.
bool keyboard_window_toggle_number_face(void);
bool keyboard_window_number_face_active(void);

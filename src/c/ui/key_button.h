#pragma once

#include <pebble.h>

#include "ui/style.h"
#include "keyboard/key.h"
#include "keyboard/shift.h"

// defined in key_button.c
typedef struct KeyButtonLayer KeyButtonLayer;

KeyButtonLayer *key_button_create(const Key *key, GRect frame, const UIStyle *style);
// Visual-only pressed state; never changes touch hitboxes or gesture mapping.
void key_button_set_feedback_level(KeyButtonLayer *button, uint8_t level);
// Repaint the existing base/center layers; never allocate a per-legend layer.
void key_button_set_shift_mode(KeyButtonLayer *button, JotShiftMode mode);
void key_button_destroy(KeyButtonLayer *button);

Layer *key_button_get_layer(KeyButtonLayer *button);

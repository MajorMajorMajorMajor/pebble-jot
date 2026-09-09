// vim: set sw=2 ts=2 et:
#pragma once

#include <pebble.h>

enum {
    ALERT_MAX_LENGTH = 200
};
void alert_set_text(const char *new_text);

TextLayer* alert_ui_init(Layer *parent_layer, GRect frame);
void alert_ui_deinit();
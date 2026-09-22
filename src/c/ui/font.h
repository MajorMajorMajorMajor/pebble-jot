#pragma once

#include <pebble.h>

typedef struct UIFont {
  const char *font_key;
  GFont gfont; // Achtung! GFont is a pointer! (typedef FontInfo* GFont;)
  int height_px;
} UIFont;

const UIFont *get_ui_font(const char *font_key);
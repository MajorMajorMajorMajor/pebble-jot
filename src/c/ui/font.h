#include <pebble.h>

typedef struct UIFont {
  char *font_key;
  GFont font;
  int height_px;
} UIFont;
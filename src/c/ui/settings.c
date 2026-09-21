#include <pebble.h>

#include "settings.h"

static UISettings s_ui_settings = {
  .label_center_font_key = FONT_KEY_GOTHIC_24,
  .label_peripheral_font_key = FONT_KEY_GOTHIC_18_BOLD
};

const UISettings *get_ui_settings(void) {
  return &s_ui_settings;
}

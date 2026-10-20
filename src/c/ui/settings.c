#include <pebble.h>

#include "settings.h"

static UISettings s_ui_settings = {
  .style = {
    .label_center_font_key = FONT_KEY_GOTHIC_28_BOLD,
    .label_peripheral_font_key = FONT_KEY_GOTHIC_24,
  }
};

const UISettings *get_ui_settings(void) {
  return &s_ui_settings;
}

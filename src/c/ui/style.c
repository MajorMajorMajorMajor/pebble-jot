#include <pebble.h>

#include "ui/settings.h"
#include "ui/font.h"

#include "ui/style.h"

static UIStyle s_ui_style;

void ui_style_init(void)
{
  const UISettings *settings = get_ui_settings();
  const UIStyleSettings style = settings->style;

  s_ui_style = (UIStyle) {
    .label_center_font = get_ui_font(style.label_center_font_key),
    .label_peripheral_font = get_ui_font(style.label_peripheral_font_key)
  };
}

const UIStyle *get_ui_style(void) {
  return &s_ui_style;
}
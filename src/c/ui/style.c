#include <pebble.h>

#include "common.h"
#include "ui/settings.h"
#include "ui/font.h"

#include "ui/style.h"

static UIStyle s_ui_style;

void ui_style_init(void) {
  const UISettings *settings = get_ui_settings();
  const UIStyleSettings style = settings->style;

  s_ui_style = (UIStyle) {
    .label_center_font = get_ui_font(style.label_center_font_key),
    .label_peripheral_font = get_ui_font(style.label_peripheral_font_key),

    .key_corner_radius = 3,
    
    .key_outline_color = GColorDarkGray,
    .key_background_color = GColorShockingPink,
    
    .label_center_color = GColorBlack,
    .label_peripheral_color = GColorDarkGray,

    .label_padding_px = 1,
  };

  // Make sure the fonts were found and actually loaded
  PEBBLE_ASSERT(s_ui_style.label_center_font);
  PEBBLE_ASSERT(s_ui_style.label_center_font->gfont);
  
  PEBBLE_ASSERT(s_ui_style.label_peripheral_font);
  PEBBLE_ASSERT(s_ui_style.label_peripheral_font->gfont);
}

void ui_style_deinit(void) {
  s_ui_style = (UIStyle){0};
}

const UIStyle *get_ui_style(void) {
  return &s_ui_style;
}

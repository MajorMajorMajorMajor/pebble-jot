#include <pebble.h>

#include "common.h"
#include "ui/settings.h"
#include "ui/font.h"

#include "ui/style.h"

UIStyle get_ui_style(void) {
  const UISettings *settings = get_ui_settings();  

  UIStyle style = (UIStyle) {
    .label_center_font = get_ui_font(settings->style.label_center_font_key),
    .label_peripheral_font = get_ui_font(settings->style.label_peripheral_font_key),

    .key_corner_radius = 3,
    
    .key_outline_color = GColorDarkGray,
    .key_background_color = GColorShockingPink,
    
    .label_center_color = GColorBlack,
    .label_peripheral_color = GColorDarkGray,

    .label_padding_px = 1,
  };

  // Make sure the fonts were found and actually loaded
  PEBBLE_ASSERT(style.label_center_font);
  PEBBLE_ASSERT(style.label_center_font->gfont);
  
  PEBBLE_ASSERT(style.label_peripheral_font);
  PEBBLE_ASSERT(style.label_peripheral_font->gfont);

  return style;
}

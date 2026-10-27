#pragma once

#include "ui/font.h"

typedef struct UIStyle {
  const UIFont *label_center_font;
  const UIFont *label_peripheral_font;
  
  uint16_t key_corner_radius;

  GColor key_background_color;
  GColor key_outline_color;
  
  GColor label_center_color;
  GColor label_peripheral_color;

  int label_padding_px;
  // ... more style settings
} UIStyle;

UIStyle get_ui_style(void);

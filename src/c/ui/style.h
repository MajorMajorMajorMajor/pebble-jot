#pragma once

#include "ui/font.h"

typedef struct UIStyle {
  const UIFont *label_center_font;
  const UIFont *label_peripheral_font;
  
  // ... more style settings
} UIStyle;

void ui_style_init(void);

const UIStyle *get_ui_style(void);
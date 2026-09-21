#pragma once

typedef struct {
  // style settings
  struct {
    const char *label_center_font_key;
    const char *label_peripheral_font_key;
  } style;
} UISettings;

const UISettings *get_ui_settings(void);


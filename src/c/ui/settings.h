#pragma once

typedef struct {
  // style settings
  const struct UIStyleSettings{
    const char *label_center_font_key;
    const char *label_peripheral_font_key;
  } style;
} UISettings;

typedef struct UIStyleSettings UIStyleSettings;

const UISettings *get_ui_settings(void);


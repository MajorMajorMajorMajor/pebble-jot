#pragma once

typedef struct UIStyleSettings {
    const char *label_center_font_key;
    const char *label_peripheral_font_key;
} UIStyleSettings;

typedef struct {
  // style settings 
  UIStyleSettings style;
  
} UISettings;



const UISettings *get_ui_settings(void);


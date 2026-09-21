#include "ui/style.h"
#include "ui/font.h"

typedef struct {
  const char *label_center_font_key;
  const char *label_peripheral_font_key;
} UISettings;

const UISettings *get_ui_settings(void);

static UISettings s_ui_settings = {
  .label_center_font_key = FONT_KEY_GOTHIC_24,
  .label_peripheral_font_key = FONT_KEY_GOTHIC_18_BOLD
};

const UISettings *get_ui_settings(void) {
  return &s_ui_settings;
}


static UIStyle s_ui_style;


void ui_style_init(void)
{
  const UISettings *settings = get_ui_settings();

  s_ui_style = (UIStyle) {
    .label_center_font = get_ui_font(settings->label_center_font_key),
    .label_peripheral_font = get_ui_font(settings->label_peripheral_font_key)
  };
}

const UIStyle *get_ui_style(void) {
  return &s_ui_style;
}
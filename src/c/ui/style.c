#include "ui/style.h"
#include "ui/font.h"

static UIStyle s_ui_style;

UIStyle *get_style() {
  return &s_ui_style;
}

void ui_style_init():
{
  s_ui_style = (UIStyle) {
    .label_center_font = get_ui_font()
  }
};

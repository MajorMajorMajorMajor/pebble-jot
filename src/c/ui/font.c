#include <pebble.h>

#include "common.h"

#include "font.h"


// Initialize height_px, leaving font zero-initialized until populated by  ui_font_init()
static UIFont s_fonts[] = {
  {.font_key=FONT_KEY_GOTHIC_18_BOLD, .height_px=0},
  {.font_key=FONT_KEY_GOTHIC_24,      .height_px=0}  
};

const UIFont *get_ui_font(const char *font_key) {    
  size_t i;

  for (i = 0; i < ARRAY_SIZE(s_fonts); i++) {
    UIFont *ui_font = &s_fonts[i];

    if (strcmp(ui_font->font_key, font_key) == 0) {
      // initialize font face the first time it's needed
      if (ui_font->font == NULL) {
        ui_font->font = fonts_get_system_font(font_key);
      }      
      return ui_font;
    }
  }
    
  return NULL;
}
#include <pebble.h>

// Do not include <string.h>, relying on on string functions
// provided by <pebble.h> instead.
// 
// https://developer.rebble.io/docs/c/Standard_C/
// #include <string.h> 

#include "common.h"

#include "font.h"


// Initialize height_px, leaving gfont zero-initialized until populated by get_ui_font()
static UIFont s_fonts[] = {
  {.font_key=FONT_KEY_GOTHIC_18,            .height_px=24},
  {.font_key=FONT_KEY_GOTHIC_18_BOLD,       .height_px=24},
  {.font_key=FONT_KEY_ROBOTO_CONDENSED_21,  .height_px=28},
  {.font_key=FONT_KEY_GOTHIC_24,            .height_px=32},
  {.font_key=FONT_KEY_GOTHIC_24_BOLD,       .height_px=32},
  {.font_key=FONT_KEY_GOTHIC_28,            .height_px=38},
  {.font_key=FONT_KEY_GOTHIC_28_BOLD,       .height_px=38},
  {.font_key=FONT_KEY_BITHAM_30_BLACK,      .height_px=40},
  {.font_key=FONT_KEY_BITHAM_42_LIGHT,      .height_px=56},
  {.font_key=FONT_KEY_BITHAM_42_BOLD,       .height_px=56},
};

const UIFont *get_ui_font(const char *font_key) {    
  size_t i;

  for (i = 0; i < ARRAY_SIZE(s_fonts); i++) {
    UIFont *ui_font = &s_fonts[i];

    if (strcmp(ui_font->font_key, font_key) == 0) {
      // initialize font face the first time it's needed
      if (ui_font->gfont == NULL) {
        ui_font->gfont = fonts_get_system_font(ui_font->font_key);
      }      
      return ui_font;
    }
  }
    
  return NULL;
}

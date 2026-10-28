// vim: set sw=2 ts=2 et:
#include <pebble.h>
#include "alert.h"

// text
static TextLayer *s_textbox;
static char s_text[ALERT_MAX_LENGTH];

void alert_set_text(const char *new_text) {  
  snprintf(s_text, sizeof s_text, "%s", new_text);
  layer_mark_dirty((Layer*)s_textbox);
}

TextLayer* alert_ui_init(GRect frame){
    // Textbox    
    s_textbox = text_layer_create(frame); 
    if(!s_textbox) {
      return NULL;
    }

    text_layer_set_text(s_textbox, s_text);
    text_layer_set_text_alignment(s_textbox, GTextAlignmentLeft);
    text_layer_set_background_color(s_textbox, GColorYellow);
    
    alert_set_text("Input text");    

    return s_textbox;
}

void alert_ui_deinit(void){
  if(s_textbox) {
    text_layer_destroy(s_textbox);
    s_textbox = NULL;
  }
}

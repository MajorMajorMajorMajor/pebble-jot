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

TextLayer* alert_ui_init(
  Layer *parent_layer, 
  GRect frame // location and size
){
    s_textbox = text_layer_create(frame); 
    text_layer_set_text(s_textbox, s_text);
    text_layer_set_text_alignment(s_textbox, GTextAlignmentLeft);
    
    alert_set_text("Input text");

    layer_add_child(parent_layer, text_layer_get_layer(s_textbox));

    return s_textbox;
}

void alert_ui_deinit(){
  text_layer_destroy(s_textbox);
}
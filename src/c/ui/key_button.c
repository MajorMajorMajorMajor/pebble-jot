#include <pebble.h>

#include "common.h"

#include "keyboard/key.h"
#include "keyboard/key_label.h"

#include "ui/font.h"
#include "ui/style.h"
#include "ui/key_button.h"

struct KeyButton {
  const Key *key;
  Layer *base_layer;
  Layer *label_layers[KEY_LABEL_COUNT];
};


static void prv_draw_key_label(Layer *layer, GContext* ctx) {
  const UIStyle *style = get_ui_style();

  const KeyLabel *key_label = *(const KeyLabel **)layer_get_data(layer);

  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Drawing key label %p (%d: %s)", key_label, key_label->type, key_label->text);
  // paranoia; not expecting null pointers here, because layers shouldn't be generated for null labels
  if (key_label == NULL) {    
    return;
  }
  

  // figure out where the label should go
  GRect bounds = layer_get_bounds(layer);
  GRect box = grect_crop(bounds, 1);

  // draw the label
  switch (key_label->type) {
    case KEY_LABEL_TYPE_NONE: {      
      // No label, no draw
      break;
    }
    case KEY_LABEL_TYPE_IMAGE: {
      // not implemented yet
      break;
    }
    case KEY_LABEL_TYPE_TEXT: {       
      // font geometry and color
      
      GFont  text_font = NULL;
      GRect  text_frame;
      GColor text_color;
      GTextOverflowMode text_overflow_mode = GTextOverflowModeWordWrap;
      GTextAlignment text_alignment = GTextAlignmentCenter;
      char*  text;
      
      text = key_label->text;

      switch (key_label->slot) {
        case KEY_LABEL_CENTER: {  
          const UIFont *ui_font;
          ui_font = style->label_center_font;      
          text_font = ui_font->gfont;
          text_color = style->label_center_color;
                  
          GSize text_size = graphics_text_layout_get_content_size(
            text, ui_font->gfont, box, text_overflow_mode, text_alignment);

          // center the label horizontally and vertically
          int w = text_size.w;
          int h = ui_font->height_px;

          int x = (box.origin.x) + (box.size.w / 2) - (w / 2);
          int y = (box.origin.y) + (box.size.h / 2) - (h / 2);                    
          
          text_frame = GRect(x, y, w, h);
          break;
        }
        case KEY_LABEL_RIGHT: {          
          const UIFont *ui_font;
          ui_font = style->label_peripheral_font;
          text_font = ui_font->gfont;
          text_color = style->label_peripheral_color;
                  
          GSize text_size = graphics_text_layout_get_content_size(
            text, ui_font->gfont, box, text_overflow_mode, text_alignment);

          // center the label vertically and position it on the right side
          int text_w = text_size.w;
          int text_h = ui_font->height_px;

          int text_x = (box.origin.x) + (box.size.w) - (text_w) - (style->label_padding_px);
          int text_y = (box.origin.y) + (box.size.h / 2) - (text_h / 2);                    
          
          text_frame = GRect(text_x, text_y, text_w, text_h);
          break;          
        }
        case KEY_LABEL_COUNT: {
          PEBBLE_ASSERT(0);
          break;        
        }            
      }
      graphics_context_set_text_color(ctx, text_color);        
      graphics_draw_text(
        ctx,   
        key_label->text, // char *text; durable storage
        
        text_font, // GFont font
        text_frame,  // GRect box
        text_overflow_mode, // GTextOverflowMode overflow_mode
        text_alignment, // GTextAlignment alignment
        NULL // GTextAttributes text_attributes; optional
      );      
    }
  }
}


static void prv_label_create(KeyButton *this, const KeyLabel *key_label){    
  if (key_label->type == KEY_LABEL_TYPE_NONE) {
    return;
  }

  // create a new label layer, as big as the button's base layer
  GRect bounds = layer_get_bounds(this->base_layer);

  Layer *new_layer = layer_create_with_data(bounds, sizeof(KeyLabel *));  
  PEBBLE_ASSERT(new_layer);

  // put a pointer to the key label into the data region
  *(const KeyLabel **)layer_get_data(new_layer) = key_label;


  // set the drawing function for it
  layer_set_update_proc(new_layer, prv_draw_key_label);  

  // add a reference to it to the key button object
  KeyLabelSlot label_slot = key_label->slot;
  this->label_layers[label_slot] = new_layer;

  // add it to the UI hierarchy
  layer_add_child(this->base_layer, new_layer);
}


// Maybe we don't need our own destroy method if it turns out that destroying 
// the base layer would automatically destroy child layers?
//
// @TODO: Verify this?
static void prv_label_destroy(KeyButton *this, const KeyLabel *key_label) {
  if (key_label->type == KEY_LABEL_TYPE_NONE) {
    // no label, nothing to destroy
    return;
  }
  KeyLabelSlot label_slot = key_label->slot;
  
  Layer* layer = this->label_layers[label_slot];  
  if (layer) {
    layer_destroy(layer);
    this->label_layers[label_slot] = NULL;
  }
}

void prv_draw_key_background (Layer *layer, GContext *ctx){
  const UIStyle *style = get_ui_style();
  graphics_context_set_fill_color(ctx, style->key_background_color);

  GRect bounds = layer_get_bounds(layer);
  uint16_t corner_radius = style->key_corner_radius;
  GCornerMask corner_mask = GCornersAll;  
  graphics_fill_rect(ctx, bounds, corner_radius, corner_mask);
  graphics_draw_round_rect(ctx, bounds, style->key_corner_radius);
}

KeyButton *key_button_create(const Key *key, GRect frame) {
    int i;

    Layer *base_layer = layer_create(frame);
    if (!base_layer) {
      return NULL;
    }

    layer_set_update_proc(base_layer, prv_draw_key_background);
    
    KeyButton *new_key_button = malloc(sizeof *new_key_button);

    if(!new_key_button) {
      layer_destroy(base_layer);
      return NULL;
    }

    *new_key_button = (KeyButton){
        .key = key,
        .base_layer = base_layer,
        .label_layers = {0}
    };
    
    // create labels and populate .label_layers
    for (i = 0; i < KEY_LABEL_COUNT; i++) {
        const KeyLabel *key_label = &key->labels[i];
        prv_label_create(new_key_button, key_label);
    }

    return new_key_button;
}

void key_button_destroy(KeyButton *button){  
  KeyButton *this = button;
  
  const Key *key = this->key;
  
  // destroy label layers
  int i;
  for (i = 0; i < KEY_LABEL_COUNT; i++) {    
      const KeyLabel *key_label = &key->labels[i];
      prv_label_destroy(this, key_label);
  }    

  // destroy the base layer
  if(this->base_layer) {
    layer_destroy(this->base_layer);
    this->base_layer = NULL;
  }

  // free
  free(this);
}

Layer *key_button_get_layer(KeyButton *button) {
  return button->base_layer;
}

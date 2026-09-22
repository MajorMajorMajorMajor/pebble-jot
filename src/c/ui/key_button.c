// #include <assert.h>

#include <pebble.h>

#include "keyboard/key.h"
#include "keyboard/key_label.h"

#include "ui/font.h"
#include "ui/style.h"
#include "ui/key_button.h"

static TextLayer *key_button_layers;

struct KeyButton {
  const Key *key;
  Layer *base_layer;
  Layer *label_layers[KEY_LABEL_COUNT];
};


static void prv_draw_key_label(struct Layer *layer, GContext* ctx) {
  const UIStyle *style = get_ui_style();

  const KeyLabel *key_label = layer_get_data(layer);
  // paranoia; not expecting null pointers here, because layers shouldn't be generated for null labels
  if (key_label == NULL) {
    return;
  }

  // figure out where the label should go
  GRect bounds = layer_get_bounds(layer);
  GRect box = grect_crop(bounds, 1);

  // draw the label
  switch (key_label->type) {
    case KEY_LABEL_TYPE_IMAGE: {
      // not implemented yet
      break;
    }
    case KEY_LABEL_TYPE_TEXT: { 
      // font geometry and color
      
      GFont  text_font;
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
          // assert(0);
          break;        
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
}


static void prv_label_create(KeyButton *this, const KeyLabel *key_label){    
  // create a new label layer, as big as the button's base layer
  GRect bounds = layer_get_bounds(this->base_layer);
  Layer *new_layer = layer_create(bounds);

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
static void prv_label_destroy(KeyButton *this, const KeyLabel *key_label){
  KeyLabelSlot label_slot = key_label->slot;
  
  Layer* layer = this->label_layers[label_slot];
  layer_destroy(layer);
}


KeyButton *key_button_create(Layer *parent_layer, const Key *key){
    Layer *base_layer = layer_create(layer_get_bounds(parent_layer));    

    int i;

    KeyButton *new_key_button = malloc(sizeof *new_key_button);
    *new_key_button = (KeyButton){
        .key = key,
        .base_layer = base_layer,
        // .label_layers: zero initialized        
    };
    
    for (i = 0; i < KEY_LABEL_COUNT; i++) {
        const KeyLabel *key_label = &key->labels[i];
        prv_label_create(new_key_button, key_label);
    }    

    return new_key_button;
}

void key_button_destroy(KeyButton *this){
  int i;
  
  const Key *key = this->key;
  
  // destroy label layers
  for (i = 0; i < KEY_LABEL_COUNT; i++) {
      const KeyLabel *key_label = &key->labels[i];
      prv_label_destroy(this, key_label);
  }    

  // destroy the base layer
  layer_destroy(this->base_layer);

  // free
  free(this);
}

void key_ui_init(Layer *parent_layer, UIDimensions ui) {
    // get the active layout
    const Layout *layout = keyboard_get_active_layout();

    // draw it
    for (unsigned int i = 0; i < layout->key_count; i++) {
        const Key *key = get_key(layout->keys[i].key_id);

        int row = i / 4;
        int col = i % 4;

        GPoint origin = {
        .x = col * ui.cell_size.w,
        .y = ui.textbox_size.h + (row * ui.cell_size.h)
        };
        Layer *key_layer = layer_create_with_data((GRect){origin, ui.cell_size}, sizeof(Key));
        s_keys[i].layer = key_layer;
        s_keys[i].center_label = key->label_center;  // @TODO: Define actual labels for all the keys

        Key *layer_data = layer_get_data(key_layer);
        *layer_data = s_keys[i]; // write a copy of the key data to the layer

        layer_set_update_proc(key_layer, prv_update_key_layer);

        // text_layer_set_text(key_layer, keylabel_0);
        // text_layer_set_background_color(key_layer, GColorVividCerulean);
        // text_layer_set_text_alignment(key_layer, GTextAlignmentCenter);

        layer_add_child(parent_layer, key_layer);
        // draw the center label text
        char *text = key->center_label;
        // const GFont font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK); // @ TODO: Make the font and colors configurable
        const GFont font = fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM); // @ TODO: Make the font and colors configurable

        GTextOverflowMode overflow_mode = GTextOverflowModeWordWrap;
        GTextAlignment alignment = GTextAlignmentCenter;


        GSize text_size = graphics_text_layout_get_content_size(text, font, box, overflow_mode, alignment);
        int inset_top = (bounds.size.h - text_size.h)/3;
        GRect box_vcenter = grect_inset(bounds, GEdgeInsets(inset_top));

        graphics_context_set_text_color(ctx, GColorBlack);
        graphics_draw_text(ctx, text, font, box_vcenter, overflow_mode, alignment, NULL);

    }
}

void key_ui_deinit() {
    for (int i = 0; i < NUMBER_OF_KEYS; i++) {
        layer_destroy(s_keys[i].layer);
    }
}

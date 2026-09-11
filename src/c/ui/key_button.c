#include <pebble.h>

#include "keyboard/key_label.h"
#include "keyboard/keyboard.h"
#include "keyboard/layout.h"

#include "ui/ui.h"
#include "ui/key_button.h"

static TextLayer *key_button_layers;

struct KeyButton {
  const Key *key;
  Layer *base_layer;
  Layer *label_center;  
};


typedef enum {
  LABEL_TYPE_TEXT,
  LABEL_TYPE_IMAGE
} LabelType;




static void prv_label_create(KeyButton *this, KeyLabelSlot pos, char *label_text){
  const Layer *base_layer = this->base_layer;
  GRect frame = layer_get_bounds(base_layer);

  Layer *label_layer = layer_create(frame);



}

KeyButton *key_button_create(Layer *parent_layer, const Key *key){    
    Layer *base_layer = layer_create(layer_get_bounds(parent_layer));
  
    KeyButton *key_button = malloc(sizeof *key_button);
    *key_button = (KeyButton){
        .base_layer = base_layer,
        .key = key
    };

    prv_label_create(base_layer, )

    return key_button;
}

void key_button_destroy(KeyButton *key_button){
    layer_destroy(key_button->base_layer);
    free(key_button);
}


static void prv_update_key_layer(struct Layer *layer, GContext* ctx){
  Key *key = layer_get_data(layer);
  
  // make a 1-pixel margin
  GRect bounds = layer_get_bounds(layer);
  GRect box = grect_crop(bounds, 1);

  // draw a rounded rectangle for the button
  graphics_context_set_fill_color(ctx, GColorRichBrilliantLavender);
  graphics_fill_rect(ctx, box, 4, GCornersAll);

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
  

  return;
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

    }
}

void key_ui_deinit() {
    for (int i = 0; i < NUMBER_OF_KEYS; i++) {    
        layer_destroy(s_keys[i].layer);
    }  
}

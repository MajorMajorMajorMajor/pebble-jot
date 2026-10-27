#include <pebble.h>

#include "common.h"

#include "keyboard/key.h"
#include "keyboard/key_label.h"

#include "ui/font.h"
#include "ui/style.h"
#include "ui/key_button.h"

struct KeyLabelLayer {
  const KeyLabel *key_label;
  const UIStyle *style;

  Layer *layer;
};

typedef struct KeyLabelLayer KeyLabelLayer;

struct KeyButtonLayer {
  const Key *key;
  const UIStyle *style;

  Layer *base_layer;
  KeyLabelLayer *label_layers[KEY_LABEL_COUNT];
};

// public in key_button.h
// typedef struct KeyButtonLayer KeyButtonLayer;


static void prv_draw_key_label(Layer *layer, GContext* ctx) {
  const KeyLabelLayer *label = *(const KeyLabelLayer **)layer_get_data(layer);

  // paranoia; not expecting null pointers here, because layers shouldn't be generated for null labels
  if (!label || !label->key_label || label->key_label->type == KEY_LABEL_TYPE_NONE) {
    return;
  }

  const KeyLabel* key_label = label->key_label;
  const UIStyle* style = label->style;

  
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Drawing key label %p (%d: %s)", key_label, key_label->type, key_label->text);

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


      const char *text = key_label->text;

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
        case KEY_LABEL_BOTTOM: {
          const UIFont *ui_font;
          ui_font = style->label_peripheral_font;
          text_font = ui_font->gfont;
          text_color = style->label_peripheral_color;

          GSize text_size = graphics_text_layout_get_content_size(
            text, ui_font->gfont, box, text_overflow_mode, text_alignment);

          // center the label horizontally and position it on the bottom side
          int w = text_size.w;
          int h = ui_font->height_px;

          int x = (box.origin.x) + (box.size.w / 2) - (w / 2);
          int y = (box.origin.y) + (box.size.h) - (h * 0.75) - (style->label_padding_px);

          text_frame = GRect(x, y, w, h);
          break;
        }
        default: {
          PEBBLE_ASSERT(0);
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


static KeyLabelLayer *prv_label_create(const KeyLabel *key_label, GRect frame, const UIStyle *style){
  // No label, no layer
  if (key_label->type == KEY_LABEL_TYPE_NONE) {
    return NULL;
  }

  // Allocate a new label layer with metadata
  KeyLabelLayer *this = malloc(sizeof *this);
  if(!this) {
    return NULL;
  }

  *this = (KeyLabelLayer){
    .key_label = key_label,
    .style = style,

    // populated below
    .layer = NULL,
  };

  // Create the underlying Layer object
  this->layer = layer_create_with_data(frame, sizeof(KeyLabelLayer*));
  if(!this->layer) {
    free(this);
    return NULL;
  }

  // populate layer data and set the drawing function
  *(const KeyLabelLayer **)layer_get_data(this->layer) = this;
  layer_set_update_proc(this->layer, prv_draw_key_label);

  return this;
}


// Maybe we don't need our own destroy method if it turns out that destroying
// the base layer would automatically destroy child layers?
//
// @TODO: Verify this?
static void prv_label_destroy(KeyLabelLayer *this) {
  if(this->layer) {
    layer_destroy(this->layer);
  }

  free(this);
}

static void prv_draw_key_background (Layer *layer, GContext *ctx){
  const KeyButtonLayer *this = *(KeyButtonLayer **)layer_get_data(layer);

  graphics_context_set_fill_color(ctx, this->style->key_background_color);
  graphics_context_set_stroke_color(ctx, this->style->key_outline_color);

  GRect bounds = layer_get_bounds(layer);
  uint16_t corner_radius = this->style->key_corner_radius;
  GCornerMask corner_mask = GCornersAll;
  graphics_fill_rect(ctx, bounds, corner_radius, corner_mask);
  graphics_draw_round_rect(ctx, bounds, this->style->key_corner_radius);
}

KeyButtonLayer *key_button_create(const Key *key, GRect frame, UIStyle *style) {
    PEBBLE_ASSERT(key);
    PEBBLE_ASSERT(style);

    int i;

    // Allocate this object
    KeyButtonLayer *this = malloc(sizeof *this);
    if(!this) {
      return NULL;
    }

    *this = (KeyButtonLayer){
        .key = key,
        .style = style,

        // dynamic objects initialized below
        .base_layer = NULL,
        .label_layers = {0}
    };


    // Create base layer
    this->base_layer = layer_create_with_data(frame, sizeof(KeyButtonLayer*));
    if (!this->base_layer) {
      free(this);
      return NULL;
    }

    // Configure base layer and associate with the button
    layer_set_update_proc(this->base_layer, prv_draw_key_background);
    *(KeyButtonLayer**)layer_get_data(this->base_layer) = this;


    // create labels and populate .label_layers
    for (i = 0; i < KEY_LABEL_COUNT; i++) {
        // Abstract label definition
        const KeyLabel *key_label = &key->labels[i];

        if (key_label->type == KEY_LABEL_TYPE_NONE) {
          continue;
        }

        // create a new label layer, as big as the button's base layer
        GRect label_frame = layer_get_bounds(this->base_layer);
        KeyLabelLayer *new_label = prv_label_create(key_label, label_frame, style);
        PEBBLE_ASSERT(new_label);

        // add it to the UI hierarchy
        layer_add_child(this->base_layer, new_label->layer);

        // add to new button
        this->label_layers[key_label->slot] = new_label;
    }

    return this;
}

void key_button_destroy(KeyButtonLayer *button){
  KeyButtonLayer *this = button;

  // destroy label layers
  int i;
  for (i = 0; i < KEY_LABEL_COUNT; i++) {
    if(this->label_layers[i]) {
      prv_label_destroy(this->label_layers[i]);
      this->label_layers[i] = NULL;
    }
  }

  // destroy the base layer
  if(this->base_layer) {
    layer_destroy(this->base_layer);
    this->base_layer = NULL;
  }

  // free
  free(this);
}

Layer *key_button_get_layer(KeyButtonLayer *button) {
  return button->base_layer;
}

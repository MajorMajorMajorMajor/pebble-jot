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
  const Key *key;

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


// Visual-only glyphs: keep the original key labels and gesture mapping.
// The left column is ink-dark; the wide Space key carries the gold accent.
static void prv_draw_control_icon(GContext *ctx, const Key *key, GRect bounds) {
  const int cx = bounds.size.w / 2;
  const int cy = bounds.size.h / 2 - 1;
  const GColor cream = GColorFromRGB(255, 250, 239);
  const GColor charcoal = GColorFromRGB(41, 40, 43);
  const bool is_space = key == get_key(KEY_ENGLISH_SPACEBAR);
  graphics_context_set_stroke_color(ctx, is_space ? charcoal : cream);
  graphics_context_set_stroke_width(ctx, 3);

  if (key == get_key(KEY_ENGLISH_MENU)) {
    // Save: an 18px floppy-disk silhouette, legible in the 50px key.
    graphics_draw_round_rect(ctx, GRect(cx - 9, cy - 9, 18, 18), 2);
    graphics_draw_line(ctx, GPoint(cx - 5, cy - 8),
                       GPoint(cx + 4, cy - 8));
    graphics_draw_rect(ctx, GRect(cx - 5, cy + 1, 10, 6));
    graphics_draw_line(ctx, GPoint(cx + 3, cy - 8),
                       GPoint(cx + 3, cy - 4));
  } else if (key == get_key(KEY_ENGLISH_BACKSPACE)) {
    // Backspace: a strong left arrow, not a tiny textual "<==".
    graphics_draw_line(ctx, GPoint(cx - 10, cy), GPoint(cx + 9, cy));
    graphics_draw_line(ctx, GPoint(cx - 10, cy), GPoint(cx - 2, cy - 8));
    graphics_draw_line(ctx, GPoint(cx - 10, cy), GPoint(cx - 2, cy + 8));
  } else if (key == get_key(KEY_ENGLISH_RETURN)) {
    // Return: bent arrow, distinct from backspace.
    graphics_draw_line(ctx, GPoint(cx + 8, cy - 9),
                       GPoint(cx + 8, cy + 5));
    graphics_draw_line(ctx, GPoint(cx + 8, cy + 5),
                       GPoint(cx - 9, cy + 5));
    graphics_draw_line(ctx, GPoint(cx - 9, cy + 5),
                       GPoint(cx - 2, cy - 2));
    graphics_draw_line(ctx, GPoint(cx - 9, cy + 5),
                       GPoint(cx - 2, cy + 12));
  } else if (is_space) {
    // Space: the simplest possible symbol, deliberately not another word.
    graphics_draw_line(ctx, GPoint(cx - 16, cy + 2),
                       GPoint(cx + 16, cy + 2));
  }
}

static void prv_draw_key_label(Layer *layer, GContext* ctx) {
  const KeyLabelLayer *label = layer_get_data(layer);

  // paranoia; not expecting null pointers here, because layers shouldn't be generated for null labels
  if (!label || !label->key_label || label->key_label->type == KEY_LABEL_TYPE_NONE) {
    return;
  }

  const KeyLabel* key_label = label->key_label;
  const UIStyle* style = label->style;
  const bool special = label->key == get_key(KEY_ENGLISH_MENU) ||
                       label->key == get_key(KEY_ENGLISH_123) ||
                       label->key == get_key(KEY_ENGLISH_BACKSPACE) ||
                       label->key == get_key(KEY_ENGLISH_RETURN);

  
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Drawing key label %p (%d: %s)", key_label, key_label->type, key_label->text);

  // figure out where the label should go
  GRect bounds = layer_get_bounds(layer);
  GRect box = grect_crop(bounds, 1);
  if (key_label->slot == KEY_LABEL_CENTER &&
      (label->key == get_key(KEY_ENGLISH_MENU) ||
       label->key == get_key(KEY_ENGLISH_BACKSPACE) ||
       label->key == get_key(KEY_ENGLISH_RETURN) ||
       label->key == get_key(KEY_ENGLISH_SPACEBAR))) {
    prv_draw_control_icon(ctx, label->key, bounds);
    return;
  }

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
          // Four-letter SAVE needs the compact bold face within the
          // existing 50px menu cell; all other key sizes stay unchanged.
          ui_font = label->key == get_key(KEY_ENGLISH_MENU) ?
                    style->label_peripheral_font :
                    style->label_center_font;
          text_font = ui_font->gfont;
          text_color = special ? GColorWhite : style->label_center_color;

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
          text_color = special ? GColorWhite : style->label_peripheral_color;

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
          text_color = special ? GColorWhite : style->label_peripheral_color;

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


static KeyLabelLayer *prv_label_create(const KeyLabel *key_label, const Key *key,
                                       GRect frame, const UIStyle *style){
  // No label, no layer
  if (key_label->type == KEY_LABEL_TYPE_NONE) {
    return NULL;
  }
  

  // Create the underlying Layer object and allocate storage using Pebble's layer API
  Layer* layer = layer_create_with_data(frame, sizeof(KeyLabelLayer));  // (void *) pointer to KeyLabelLayer
  if(!layer) {    
    return NULL;
  }  

  // Get a pointer to the storage allocated for us by the Layer
  KeyLabelLayer* label = layer_get_data(layer);  
  *label = (KeyLabelLayer){
    .key_label = key_label,
    .style = style,
    .key = key,
    .layer = layer,
  };

  // populate layer data and set the drawing function  
  layer_set_update_proc(layer, prv_draw_key_label);

  return label;
}


static void prv_label_destroy(KeyLabelLayer *label) {
  if (!label) {
    return;
  }

  if(label->layer) {
    layer_destroy(label->layer);    
  }  

  // nothing to free! allocation was done by pebble's Layer
}

static void prv_draw_key_background (Layer *layer, GContext *ctx){
  const KeyButtonLayer *button = layer_get_data(layer);
  const UIStyle *style = button->style;

  const bool special = button->key == get_key(KEY_ENGLISH_MENU) ||
                       button->key == get_key(KEY_ENGLISH_123) ||
                       button->key == get_key(KEY_ENGLISH_BACKSPACE) ||
                       button->key == get_key(KEY_ENGLISH_RETURN);
  const bool space = button->key == get_key(KEY_ENGLISH_SPACEBAR);
  const GRect bounds = layer_get_bounds(layer);
  const int w = bounds.size.w;
  const int h = bounds.size.h;
  const GColor charcoal = GColorFromRGB(41, 40, 43);

  // The face and 3px down-shadow sit *inside* the unchanged cell geometry.
  // Touch classification still uses the full 4x4 rectangles, with no gaps.
  const GRect shadow = GRect(2, 4, w - 4, h - 6);
  const GRect face = GRect(2, 1, w - 4, h - 6);
  graphics_context_set_fill_color(ctx, charcoal);
  graphics_fill_rect(ctx, shadow, style->key_corner_radius, GCornersAll);
  graphics_context_set_fill_color(ctx,
      special ? charcoal :
      space ? GColorFromRGB(255, 209, 88) : style->key_background_color);
  graphics_fill_rect(ctx, face, style->key_corner_radius, GCornersAll);
  graphics_context_set_stroke_color(ctx, style->key_outline_color);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_round_rect(ctx, face, style->key_corner_radius);
}

KeyButtonLayer *key_button_create(const Key *key, GRect frame, const UIStyle *style) {
    PEBBLE_ASSERT(key);
    PEBBLE_ASSERT(style);

    int i;

    // Create a layer and allocate memory
    Layer *base_layer = layer_create_with_data(frame, sizeof(KeyButtonLayer));
    if (!base_layer) {      
      return NULL;
    }
    
    // Write the button struct into layer storage
    KeyButtonLayer *button = layer_get_data(base_layer);
    *button = (KeyButtonLayer){
        .key = key,
        .style = style,
        
        .base_layer = base_layer, // link back to the base layer (that contains the memory allocation for this entire object)

        // dynamic objects initialized below
        .label_layers = {0}
    };

    // Set drawing function for the button itself
    layer_set_update_proc(button->base_layer, prv_draw_key_background);

    // create labels and populate .label_layers
    for (i = 0; i < KEY_LABEL_COUNT; i++) {
        // Abstract label definition
        const KeyLabel *key_label = &key->labels[i];

        if (key_label->type == KEY_LABEL_TYPE_NONE) {
          continue;
        }

        // create a new label layer, as big as the button's base layer
        GRect label_frame = layer_get_bounds(button->base_layer);
        KeyLabelLayer *new_label = prv_label_create(key_label, button->key, label_frame, button->style);
        if (!new_label){
          // safe to call destroy on partially initialized button, because the destructor
          // skips null pointers and we initialized .label_layers to {0}
          key_button_destroy(button); 
          return NULL;
        }        

        // add it to the UI hierarchy
        layer_add_child(button->base_layer, new_label->layer);

        // add to new button
        button->label_layers[i] = new_label;
    }

    return button;
}

void key_button_destroy(KeyButtonLayer *button){  
  if (!button) {
    return;
  }

  // destroy label layers
  int i;
  for (i = 0; i < KEY_LABEL_COUNT; i++) {
    if(button->label_layers[i]) {
      prv_label_destroy(button->label_layers[i]);
      button->label_layers[i] = NULL;
    }
  }

  // destroy the base layer
  if(button->base_layer) {
    layer_destroy(button->base_layer);    
  }

  // no need to free the button pointer, since memory was managed by the base layer
  // destroyed by layer_destroy() above
}

Layer *key_button_get_layer(KeyButtonLayer *button) {
  return button->base_layer;
}

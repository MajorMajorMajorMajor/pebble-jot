#include <pebble.h>

#include "common.h"

#include "keyboard/key.h"
#include "keyboard/key_label.h"
#include "keyboard/number_face.h"
#include <stddef.h>

#include "ui/font.h"
#include "ui/style.h"
#include "ui/key_button.h"
#include "ui/key_feedback_curve.h"

typedef struct KeyButtonLayer KeyButtonLayer;

struct KeyLabelLayer {
  const KeyLabel *key_label;
  const UIStyle *style;
  const Key *key;
  const KeyButtonLayer *owner;

  Layer *layer;
};

typedef struct KeyLabelLayer KeyLabelLayer;

struct KeyButtonLayer {
  const Key *key;
  const UIStyle *style;

  Layer *base_layer;
  uint8_t feedback_level; // 0=rest, 255=full press.
  KeyLabelLayer *label_layers[KEY_LABEL_COUNT];
};

// public in key_button.h
// typedef struct KeyButtonLayer KeyButtonLayer;


// Visual-only glyphs: keep the original key labels and gesture mapping.
// The left column is ink-dark; the wide Space key carries the gold accent.
static void prv_draw_control_icon(GContext *ctx, const Key *key,
                                  GRect bounds, uint8_t level) {
  const int cx = bounds.size.w / 2;
  const int cy = bounds.size.h / 2 - 1;
  const GColor cream = GColorFromRGB(255, 250, 239);
  const GColor charcoal = GColorFromRGB(41, 40, 43);
  const bool is_space = key == get_key(KEY_ENGLISH_SPACEBAR) ||
                        key == get_key(KEY_NUMBER_SPACEBAR);
  // Hold glyph contrast fixed; toggling cream/white at an intensity
  // threshold looked like a separate flash over the fill transition.
  (void)level;
  graphics_context_set_stroke_color(ctx,
      key == get_key(KEY_NUMBER_SPACEBAR) ? GColorWhite :
      is_space ? charcoal : cream);
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
                       label->key == get_key(KEY_NUMBER_ABC) ||
                       label->key == get_key(KEY_ENGLISH_BACKSPACE) ||
                       label->key == get_key(KEY_ENGLISH_RETURN);

  
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Drawing key label %p (%d: %s)", key_label, key_label->type, key_label->text);

  // figure out where the label should go
  GRect bounds = layer_get_bounds(layer);
  GRect box = grect_crop(bounds, 1);
  if (key_label->slot == KEY_LABEL_CENTER &&
      label->key == get_key(KEY_NUMBER_ZERO)) {
    // The whole 2-column key is ZERO; no nested virtual Space target.
    graphics_context_set_text_color(ctx, GColorFromRGB(0, 40, 60));
    graphics_draw_text(ctx, "0",
        fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
        GRect(0, bounds.size.h / 2 - 22, bounds.size.w, 36),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    return;
  }
  if (key_label->slot == KEY_LABEL_CENTER &&
      (label->key == get_key(KEY_ENGLISH_MENU) ||
       label->key == get_key(KEY_ENGLISH_BACKSPACE) ||
       label->key == get_key(KEY_ENGLISH_RETURN) ||
       label->key == get_key(KEY_ENGLISH_SPACEBAR) ||
       label->key == get_key(KEY_NUMBER_SPACEBAR))) {
    prv_draw_control_icon(ctx, label->key, bounds,
                          label->owner->feedback_level);
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
          // The save key now draws an icon; ordinary central glyphs
          // keep the large familiar face and their original touch cell.
          ui_font = label->key == get_key(KEY_ENGLISH_MENU) ||
                    label->key == get_key(KEY_NUMBER_ABC) ?
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
          int y = (box.origin.y) + (box.size.h / 2) - (h / 2) - 2;

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
      graphics_draw_text(ctx, key_label->text, text_font, text_frame,
                         text_overflow_mode, text_alignment, NULL);
      // Labels never move or change weight during an A-style fill pulse.
    }
  }
}


static KeyLabelLayer *prv_label_create(const KeyLabel *key_label, const Key *key,
                                       GRect frame, const UIStyle *style,
                                       const KeyButtonLayer *owner){
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
    .owner = owner,
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

// One-pixel inset contour follows the rounded face just inside its ink
// outline. Segments share endpoints so the top/left highlight is unbroken
// around the upper corners, including the top-right shoulder. The lower
// bevel is reserved for light keys and follows both bottom corners.
static void prv_draw_key_bevel(GContext *ctx, GRect face, bool special,
                               bool space) {
  const int x0 = face.origin.x + 1;
  const int y0 = face.origin.y + 1;
  const int x1 = face.origin.x + face.size.w - 2;
  const int y1 = face.origin.y + face.size.h - 2;
  const GColor highlight = special ? GColorFromRGB(94, 96, 99) :
                            space ? GColorFromRGB(255, 255, 170) :
                                    GColorWhite;

  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(ctx, highlight);

  // Left edge -> rounded upper-left -> full top -> rounded upper-right.
  graphics_draw_line(ctx, GPoint(x0, y1 - 6), GPoint(x0, y0 + 5));
  graphics_draw_line(ctx, GPoint(x0, y0 + 5), GPoint(x0 + 1, y0 + 3));
  graphics_draw_line(ctx, GPoint(x0 + 1, y0 + 3), GPoint(x0 + 2, y0 + 1));
  graphics_draw_line(ctx, GPoint(x0 + 2, y0 + 1), GPoint(x0 + 4, y0));
  graphics_draw_line(ctx, GPoint(x0 + 4, y0), GPoint(x1 - 4, y0));
  graphics_draw_line(ctx, GPoint(x1 - 4, y0), GPoint(x1 - 2, y0 + 1));
  graphics_draw_line(ctx, GPoint(x1 - 2, y0 + 1), GPoint(x1 - 1, y0 + 3));
  graphics_draw_line(ctx, GPoint(x1 - 1, y0 + 3), GPoint(x1, y0 + 5));

  if (!special) {
    // Subtle lower/right-side ink bevel, also exactly one pixel thick.
    graphics_context_set_stroke_color(ctx,
        space ? GColorFromRGB(194, 127, 0) :
                GColorFromRGB(177, 147, 96));
    graphics_draw_line(ctx, GPoint(x0, y1 - 5), GPoint(x0 + 1, y1 - 3));
    graphics_draw_line(ctx, GPoint(x0 + 1, y1 - 3),
                       GPoint(x0 + 2, y1 - 1));
    graphics_draw_line(ctx, GPoint(x0 + 2, y1 - 1), GPoint(x0 + 4, y1));
    graphics_draw_line(ctx, GPoint(x0 + 4, y1), GPoint(x1 - 4, y1));
    graphics_draw_line(ctx, GPoint(x1 - 4, y1), GPoint(x1 - 2, y1 - 1));
    graphics_draw_line(ctx, GPoint(x1 - 2, y1 - 1),
                       GPoint(x1 - 1, y1 - 3));
    graphics_draw_line(ctx, GPoint(x1 - 1, y1 - 3), GPoint(x1, y1 - 5));
  }
}


// Native 1px pictograms, not Unicode arrows: the small system font
// lacks several arrow/utility glyphs on Emery. Coordinates are local
// to a 16x15 compass label slot.
static void prv_draw_number_pictogram(GContext *ctx,
                                     JotNumberPictogram icon,
                                     int x, int y) {
  const int cx = x + 8, cy = y + 7;
  graphics_context_set_stroke_width(ctx, 1);
  switch (icon) {
    case JOT_NUM_PICTO_UP:
    case JOT_NUM_PICTO_DOWN: {
      const int sign = icon == JOT_NUM_PICTO_DOWN ? 1 : -1;
      graphics_draw_line(ctx, GPoint(cx, cy - 4), GPoint(cx, cy + 4));
      graphics_draw_line(ctx, GPoint(cx - 3, cy + sign),
                              GPoint(cx, cy + sign * 4));
      graphics_draw_line(ctx, GPoint(cx + 3, cy + sign),
                              GPoint(cx, cy + sign * 4));
      break;
    }
    case JOT_NUM_PICTO_RETURN:
      graphics_draw_line(ctx, GPoint(cx + 5, cy - 4),
                              GPoint(cx + 5, cy + 2));
      graphics_draw_line(ctx, GPoint(cx + 5, cy + 2),
                              GPoint(cx - 5, cy + 2));
      graphics_draw_line(ctx, GPoint(cx - 5, cy + 2),
                              GPoint(cx - 2, cy - 1));
      graphics_draw_line(ctx, GPoint(cx - 5, cy + 2),
                              GPoint(cx - 2, cy + 5));
      break;
    case JOT_NUM_PICTO_BACKSPACE:
      graphics_draw_line(ctx, GPoint(cx + 5, cy), GPoint(cx - 4, cy));
      graphics_draw_line(ctx, GPoint(cx - 4, cy),
                              GPoint(cx - 1, cy - 3));
      graphics_draw_line(ctx, GPoint(cx - 4, cy),
                              GPoint(cx - 1, cy + 3));
      break;
    case JOT_NUM_PICTO_SPACE:
      graphics_draw_line(ctx, GPoint(cx - 5, cy + 2),
                              GPoint(cx + 5, cy + 2));
      break;
    default: break;
  }
}

// Place all eight number-face legends directly on the painted key face:
// no eight extra layers per key, preserving the watch RAM budget.
static void prv_draw_number_legends(GContext *ctx, const Key *key,
                                    GRect bounds) {
  const ptrdiff_t index = key - get_key(KEY_NUMBER_7);
  if (index < 0 || index >= 9) return;
  static const int8_t xs[8] = {0,1,2,2,2,1,0,0};
  static const int8_t ys[8] = {0,0,0,1,2,2,2,1};
  const int w = bounds.size.w, h = bounds.size.h;
  const int col[3] = {3, (w - 16) / 2, w - 19};
  const int row[3] = {2, (h - 14) / 2, h - 17};
  graphics_context_set_text_color(ctx, GColorFromRGB(0, 85, 85));
  const GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  for (int d = 0; d < 8; ++d) {
    const int x = col[xs[d]], y = row[ys[d]];
    const JotNumberPictogram pictogram =
        jot_number_face_pictogram((int)index, d);
    if (pictogram != JOT_NUM_PICTO_NONE) {
      prv_draw_number_pictogram(ctx, pictogram, x, y);
      continue;
    }
    const char *symbol = jot_number_face_legend((int)index, d);
    if (!symbol) continue;
    graphics_draw_text(ctx, symbol, font,
        GRect(x, y, 16, 15),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

// 64-colour Pebble palette: each displayed RGB channel has only four
// levels (0/85/170/255). No computed RGB interpolation, so each step
// is a genuinely different on-device colour. All letter keys use the
// owner's preferred A (fill-only) treatment.
static GColor prv_feedback_fill(bool special, bool space,
                                uint8_t intensity, const UIStyle *style) {
  const uint8_t step = key_feedback_palette_step(intensity);
  if (!step) return special ? GColorFromRGB(41, 40, 43) :
                      space ? GColorFromRGB(255, 196, 0) :
                              style->key_background_color;
  if (special) {
    // The 64-colour Pebble screen has only two usable brighter greys.
    // A sepia/ochre gamut provides THREE distinct restrained steps
    // while keeping contrast against the unchanged cream glyphs.
    // Deep olive -> russet -> warm clay, never the pale grey flash.
    return step == 1 ? GColorFromRGB(85, 85, 0) :
           step == 2 ? GColorFromRGB(170, 85, 0) :
                       GColorFromRGB(170, 85, 85);
  }
  if (space) {
    // Rich gold -> amber -> copper: each stage gets DARKER, never
    // darker/bright/darker (which caused the Spacebar to flash).
    // The original gold and its charcoal dash stay unchanged at rest.
    return step == 1 ? GColorFromRGB(255, 170, 0) :
           step == 2 ? GColorFromRGB(255, 85, 0) :
                       GColorFromRGB(170, 85, 0);
  }
  // Warm cream -> pale peach -> honey -> tan (A's original endpoint).
  return step == 1 ? GColorFromRGB(255, 170, 170) :
         step == 2 ? GColorFromRGB(255, 170, 85) :
                     GColorFromRGB(170, 170, 85);
}

static void prv_draw_key_background (Layer *layer, GContext *ctx){
  const KeyButtonLayer *button = layer_get_data(layer);
  const UIStyle *style = button->style;

  const bool special = button->key == get_key(KEY_ENGLISH_MENU) ||
                       button->key == get_key(KEY_ENGLISH_123) ||
                       button->key == get_key(KEY_NUMBER_ABC) ||
                       button->key == get_key(KEY_ENGLISH_BACKSPACE) ||
                       button->key == get_key(KEY_ENGLISH_RETURN);
  const bool space = button->key == get_key(KEY_ENGLISH_SPACEBAR) ||
                     button->key == get_key(KEY_NUMBER_ZERO) ||
                     button->key == get_key(KEY_NUMBER_SPACEBAR);
  const ptrdiff_t numeric_index = button->key - get_key(KEY_NUMBER_7);
  const bool numeric_digit = numeric_index >= 0 && numeric_index < 9;
  const bool numeric_zero = button->key == get_key(KEY_NUMBER_ZERO);
  const bool numeric_space = button->key == get_key(KEY_NUMBER_SPACEBAR);
  const GRect bounds = layer_get_bounds(layer);
  const int w = bounds.size.w;
  const int h = bounds.size.h;
  const GColor charcoal = GColorFromRGB(41, 40, 43);
  const uint8_t level = button->feedback_level;

  // One-pixel ink outline and flush 1px inner bevel follow the rounded
  // face. All painting stays within the same unchanged 4x4 touch cell.
  const GRect face = GRect(2, 1, w - 4, h - 3);
  // The unchanged face and 1px outline frame the stepped A-style fill.
  GColor fill = prv_feedback_fill(special, space, level, style);
  if (numeric_digit || numeric_zero || numeric_space) {
    // Separate sea-glass face; ABC palette/press curves are untouched.
    // Use actual 64-colour Pebble steps, never interpolated near-greys.
    const uint8_t step = key_feedback_palette_step(level);
    if (numeric_digit)
      fill = !step ? GColorFromRGB(170, 255, 255) :
             step == 1 ? GColorFromRGB(170, 255, 170) :
             step == 2 ? GColorFromRGB(85, 255, 170) :
                         GColorFromRGB(0, 170, 170);
    else if (numeric_zero)
      fill = !step ? GColorFromRGB(0, 170, 170) :
             step == 1 ? GColorFromRGB(0, 255, 170) :
             step == 2 ? GColorFromRGB(85, 255, 255) :
                         GColorFromRGB(170, 255, 255);
    else
      fill = !step ? GColorFromRGB(0, 85, 170) :
             step == 1 ? GColorFromRGB(0, 170, 170) :
             step == 2 ? GColorFromRGB(0, 255, 170) :
                         GColorFromRGB(85, 255, 255);
  }
  graphics_context_set_fill_color(ctx, fill);
  graphics_fill_rect(ctx, face, style->key_corner_radius, GCornersAll);
  // The original ink outline is ALWAYS the same, regardless of the pulse.
  graphics_context_set_stroke_color(ctx, style->key_outline_color);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_round_rect(ctx, face, style->key_corner_radius);
  prv_draw_key_bevel(ctx, face, special, space);
  prv_draw_number_legends(ctx, button->key, bounds);
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
        KeyLabelLayer *new_label = prv_label_create(key_label, button->key,
            label_frame, button->style, button);
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

void key_button_set_feedback_level(KeyButtonLayer *button, uint8_t level) {
  if (!button || button->feedback_level == level) return;
  button->feedback_level = level;
  // Only repaint the key face; the labels and border never pulse.
  if (button->base_layer) layer_mark_dirty(button->base_layer);
  // Control icons may also need repainting to preserve their contrast.
  if (button->key == get_key(KEY_ENGLISH_MENU) ||
      button->key == get_key(KEY_ENGLISH_BACKSPACE) ||
      button->key == get_key(KEY_ENGLISH_RETURN))
    for (int i = 0; i < KEY_LABEL_COUNT; ++i)
      if (button->label_layers[i])
        layer_mark_dirty(button->label_layers[i]->layer);
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

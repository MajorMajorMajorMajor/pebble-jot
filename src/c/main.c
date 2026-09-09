// vim: set sw=2 ts=2 et:
#include <pebble.h>
#include "alert.h"
#include "layout.h"


static Window *s_window;

typedef struct Key {
  Layer *layer;
  char *center_label;
} Key;

enum params{ NUMBER_OF_KEYS = 16 };
static Key s_keys[NUMBER_OF_KEYS];

void prv_init_keys() {
  s_keys[0] = (Key) {  
    .center_label = "a"
  };
}


static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  alert_set_text("Select");
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  alert_set_text("Up");
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  alert_set_text("Down");
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
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

static void prv_touch_handler(const TouchEvent *event, void *context) {
  switch(event->type) {
    case TouchEvent_Touchdown: { 
      char text[64];
      snprintf(text, sizeof text, "Touchdown detected at (%d, %d)", event->x, event->y);
      alert_set_text(text);
      break;
    }      
    case TouchEvent_Liftoff: {
      alert_set_text("Lift-off");
      break;
    }

    case TouchEvent_PositionUpdate: {
      alert_set_text("New position");
      break;
    }

  }  
}

// initialize touch
static void prv_touch_init() {
  // check if touch is enabled
  if (!touch_service_is_enabled()) {
    alert_set_text("Please enable touch in settings in order to use the touchscreen keyboard.");
    return;
  }

  touch_service_subscribe(prv_touch_handler, NULL);
}

static void prv_touch_deinit() {
    touch_service_unsubscribe();
}


static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  UIDimensions ui = layout_compute_ui_dimensions(bounds.size);
  
  // Textbox  
  GRect alert_frame = {
    .origin = {0, 0},
    .size = ui.textbox_size
  };

  alert_ui_init(window_layer, alert_frame);
  alert_set_text("Input text");  

  for (int i = 0; i < NUMBER_OF_KEYS; i++) {    
    int row = i / 4;
    int col = i % 4;

    GPoint origin = {
      .x = col * ui.cell_size.w, 
      .y = ui.textbox_size.h + (row * ui.cell_size.h)
    };
    Layer *key_layer = layer_create_with_data((GRect){origin, ui.cell_size}, sizeof(Key));
    s_keys[i].layer = key_layer;
    s_keys[i].center_label = s_keys[0].center_label;  // @TODO: Define actual labels for all the keys

    Key *layer_data = layer_get_data(key_layer);
    *layer_data = s_keys[i]; // write a copy of the key data to the layer

    layer_set_update_proc(key_layer, prv_update_key_layer);

    // text_layer_set_text(key_layer, keylabel_0);
    // text_layer_set_background_color(key_layer, GColorVividCerulean);
    // text_layer_set_text_alignment(key_layer, GTextAlignmentCenter);

    layer_add_child(window_layer, key_layer);

  }

  // initialize touch
  prv_touch_init();
}


static void prv_window_unload(Window *window) {
  alert_ui_deinit();
  
  for (int i = 0; i < NUMBER_OF_KEYS; i++) {    
    layer_destroy(s_keys[i].layer);
  }  

  prv_touch_deinit();

}

static void prv_init(void) {
  // initialize static data
  prv_init_keys();

  s_window = window_create();
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  prv_deinit();
}

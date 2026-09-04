#include <pebble.h>


static Window *s_window;
static TextLayer *s_textbox;

typedef struct {
  GSize textbox_size;
  GSize key_size;  
} UIDimensions;

static char *keylabel_0 = "A";

UIDimensions prv_compute_ui_dimensions(GSize size) {
  int textbox_height = 20;

  int key_width = size.w / 4;
  int key_height = (size.h - textbox_height) / 4;

  GSize textbox_size = {
    .w = size.w, 
    .h = textbox_height 
  };

  GSize key_size = {
    .w = key_width,
    .h = key_height
  };

  return (UIDimensions) {
    .textbox_size = textbox_size,
    .key_size = key_size
  };
}

enum { NUMBER_OF_KEYS = 16 };
static TextLayer *s_keys[NUMBER_OF_KEYS];

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_textbox, "Select");
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_textbox, "Up");
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_textbox, "Down");
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  UIDimensions ui = prv_compute_ui_dimensions(bounds.size);

  // Textbox
  s_textbox = text_layer_create((GRect){{0, 0}, ui.textbox_size});
  text_layer_set_text(s_textbox, "Input text");
  text_layer_set_text_alignment(s_textbox, GTextAlignmentLeft);
  
  layer_add_child(window_layer, text_layer_get_layer(s_textbox));

  // Key 1
  int i = 0;
  {
    GPoint origin = {
      .x = i * ui.key_size.w, 
      .y = ui.textbox_size.h + (i * ui.key_size.h)
    };
    TextLayer *key = text_layer_create((GRect){origin, ui.key_size});
    s_keys[i] = key;


    text_layer_set_text(key, keylabel_0);
    text_layer_set_background_color(key, GColorVividCerulean);
    text_layer_set_text_alignment(key, GTextAlignmentCenter);

    layer_add_child(window_layer, text_layer_get_layer(key));

  }
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_textbox);
  
  for (int i = 0; i < NUMBER_OF_KEYS; i++) {    
    text_layer_destroy(s_keys[i]);
  }  
}

static void prv_init(void) {
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

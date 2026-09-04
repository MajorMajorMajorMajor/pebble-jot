#include <pebble.h>

static Window *s_window;
static TextLayer *s_textbox;

typedef struct {
  GSize textbox_size;
  GSize cell_size;  
} UIDimensions;

typedef struct {
  Layer *layer;
  char *center_label;
} Key;

enum { NUMBER_OF_KEYS = 16 };
static Key s_keys[NUMBER_OF_KEYS];

void prv_init_keys() {
  s_keys[0] = (Key) {  
    .center_label = "a"
  };
}

UIDimensions prv_compute_ui_dimensions(GSize size) {
  int textbox_height = 20;

  int cell_width = size.w / 4;
  int cell_height = (size.h - textbox_height) / 4;

  GSize textbox_size = {
    .w = size.w, 
    .h = textbox_height 
  };

  GSize cell_size = {
    .w = cell_width,
    .h = cell_height
  };

  return (UIDimensions) {
    .textbox_size = textbox_size,
    .cell_size = cell_size
  };
}

static UIDimensions s_ui_dimensions;

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

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  UIDimensions ui = prv_compute_ui_dimensions(bounds.size);
  s_ui_dimensions = ui;

  // Textbox
  s_textbox = text_layer_create((GRect){{0, 0}, ui.textbox_size});
  text_layer_set_text(s_textbox, "Input text");
  text_layer_set_text_alignment(s_textbox, GTextAlignmentLeft);
  
  layer_add_child(window_layer, text_layer_get_layer(s_textbox));

  for (int i = 0; i < NUMBER_OF_KEYS; i++) {
    Key key = s_keys[i];

    int row = i / 4;
    int col = i % 4;

    GPoint origin = {
      .x = col * ui.cell_size.w, 
      .y = ui.textbox_size.h + (row * ui.cell_size.h)
    };
    Layer *key_layer = layer_create_with_data((GRect){origin, ui.cell_size}, sizeof(Key));
    key.layer = key_layer;
    key.center_label = s_keys[0].center_label;  // @TODO: Define actual labels for all the keys

    Key *layer_data = layer_get_data(key_layer);
    *layer_data = key; // write a copy of the key data to the layer

    layer_set_update_proc(key_layer, prv_update_key_layer);

    // text_layer_set_text(key_layer, keylabel_0);
    // text_layer_set_background_color(key_layer, GColorVividCerulean);
    // text_layer_set_text_alignment(key_layer, GTextAlignmentCenter);

    layer_add_child(window_layer, key_layer);

  }
}


static void prv_window_unload(Window *window) {
  text_layer_destroy(s_textbox);
  
  for (int i = 0; i < NUMBER_OF_KEYS; i++) {    
    layer_destroy(s_keys[i].layer);
  }  
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

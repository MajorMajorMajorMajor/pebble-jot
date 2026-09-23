#include <pebble.h>
#include "keyboard/keyboard.h"
#include "keyboard/layout.h"

#include "ui/ui.h"
#include "ui/key_button.h"

// Create a layer to hold buttons for each key in the layout
typedef struct {  
  Layer *layer;  
  
  unsigned int key_count;
  
  // future name: UIKeyButton?
  KeyButton *ui_key_buttons[];  // array size: key_layout->key_count
} KeyGrid;

// one grid per layout
enum { KEY_GRID_COUNT = LAYOUT_COUNT };
static KeyGrid s_key_grids[KEY_GRID_COUNT];

// create a key grid
KeyGrid *key_grid_create(const Layout *layout, GRect frame) {
  // Layer  
  Layer *layer = layer_create(frame);

  // divide the frame size evenly into grid cells
  int cell_width  = (float) frame.size.w / layout->cols;
  int cell_height = (float) frame.size.h / layout->rows;

  KeyButton *ui_key_buttons[layout->key_count] = malloc(sizeof ui_key_buttons);

  // Create buttons
  size_t i;
  for (i = 0; i < layout->key_count; i++) {
    const LayoutKeyPosition *pos = &layout->keys[i];

    int x = cell_width  * pos->col;
    int y = cell_height * pos->row;
    int w = cell_width  * pos->col_span;
    int h = cell_height * pos->row_span;

    GRect button_frame = GRect(x, y, w, h);

    const Key *key = get_key(pos->key_id);
    KeyButton kb = key_button_create(key, button_frame);

    // s_key_grids
  };  
}

static void prv_destroy(LayoutId layout_id) {
  // destroy layer
  Layer *layer = s_key_grids[layout_id].layer;
  layer_destroy(layer);
}

// shows the grid for `layout_id` and hides the rest
static void prv_show(LayoutId layout_id) {
  KeyGrid *grid;

  for (LayoutId i = 0; i < LAYOUT_COUNT; i++) {
    grid = &s_key_grids[i];

    layer_set_hidden(grid->layer, layout_id != i); // hide non-matching layouts
  }
}

void key_grid_ui_init(Layer *parent_layer, UIDimensions ui) {
  // create grids for all layouts
  for (LayoutId i = 0; i < LAYOUT_COUNT; i++) {
    prv_create(parent_layer, i);
  }

  // show the initial layout
  LayoutId initial_layout_id = keyboard_get_active_layout_id();
  prv_show(initial_layout_id);
}

void key_grid_ui_deinit() {
  for (LayoutId i = 0; i < LAYOUT_COUNT; i++) {
    prv_destroy(i);
  }
}
#include <pebble.h>
#include "click.h"
#include "touch.h"

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  touch_recorder_select();
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  touch_recorder_next_type();
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  touch_recorder_next_direction();
}

void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler); 
}

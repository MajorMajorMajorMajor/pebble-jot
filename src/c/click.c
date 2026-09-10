#include <pebble.h>
#include "alert.h"

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  alert_set_text("Select");
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  alert_set_text("Up");
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  alert_set_text("Down");
}

void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler); 
}
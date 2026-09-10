#include <pebble.h>
#include "alert.h"
#include "touch.h"

static void prv_touch_handler(const TouchEvent *event, void *context) {
  switch(event->type) {
    case TouchEvent_Touchdown: { 
      char text[64] = {0};
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
void touch_init() {
  // check if touch is enabled
  if (!touch_service_is_enabled()) {
    alert_set_text("Please enable touch in settings in order to use the touchscreen keyboard.");
    return;
  }

  touch_service_subscribe(prv_touch_handler, NULL);
}

void touch_deinit() {
    touch_service_unsubscribe();
}

#include <pebble.h>
#include "alert.h"
#include "touch.h"

static void prv_alert_with_coordinates(char *template, int x, int y) {
	char text[64] = {0};
	snprintf(text, sizeof text, template, x, y);
	alert_set_text(text);
}


static void prv_touch_handler(const TouchEvent *event, void *context) {
  switch(event->type) {
    case TouchEvent_Touchdown: { 
      prv_alert_with_coordinates("Touchdown detected at (%d, %d)", event->x, event->y);
      break;
    }      
    case TouchEvent_Liftoff: {
      prv_alert_with_coordinates("Lift-off at (%d, %d)", event->x, event->y);
      break;
    }

    case TouchEvent_PositionUpdate: {
      prv_alert_with_coordinates("New position (%d, %d)", event->x, event->y);
      break;
    }

  }  
}

// initialize touch
void touch_init(void) {
  // check if touch is enabled
  if (!touch_service_is_enabled()) {
    alert_set_text("Please enable touch in settings in order to use the touchscreen keyboard.");
    return;
  }

  touch_service_subscribe(prv_touch_handler, NULL);
}

void touch_deinit(void) {
    touch_service_unsubscribe();
}

#include <pebble.h>

#include "ui/alert.h"
#include "ui/touch.h"

// Temporary on-device trace recorder. Nothing is logged while a trial is running:
// emitting APP_LOG messages inside the callback would disturb touch timing.
enum { TRACE_MAX_EVENTS = 256, FEEDBACK_DELAY_MS = 100 };

typedef struct {
  uint32_t elapsed_ms;
  int16_t x;
  int16_t y;
  TouchEventType type;
  bool non_navigational;
} TraceEvent;

// Incremental statistics for the finger currently touching the screen.
// These are only used for provisional feedback, not for the recorded data.
typedef struct {
  bool active;
  uint32_t start_ms;
  int16_t start_x;
  int16_t start_y;
  int16_t min_x;
  int16_t max_x;
  int16_t min_y;
  int16_t max_y;
  unsigned int move_count;
} StrokeStats;

static const char *const s_keys[] = {
  "A", "N", "I", "H", "O", "R", "T", "E", "S",
  "SPACE", "BACKSPACE", "RETURN", "123", "KB"
};

static const char *const s_gestures[] = {
  "TAP", "UP", "UP_RIGHT", "RIGHT", "DOWN_RIGHT",
  "DOWN", "DOWN_LEFT", "LEFT", "UP_LEFT"
};

static TraceEvent s_events[TRACE_MAX_EVENTS];
static size_t s_event_count;
static uint32_t s_overflow_count;
static uint32_t s_trial_id;
static uint32_t s_trial_start_ms;
static size_t s_key_index;
static size_t s_gesture_index;
static bool s_touch_enabled;
static bool s_recording;
static StrokeStats s_stroke;
static AppTimer *s_feedback_timer;
static char s_feedback_text[80];

static uint32_t prv_now_ms(void) {
  time_t seconds;
  const uint16_t milliseconds = time_ms(&seconds, NULL);
  // Unsigned subtraction handles the wraparound of this 32-bit counter for
  // trials shorter than about 49 days.
  return (uint32_t)seconds * 1000u + milliseconds;
}

static const char *prv_event_name(TouchEventType type) {
  switch (type) {
    case TouchEvent_Touchdown:       return "DOWN";
    case TouchEvent_PositionUpdate: return "MOVE";
    case TouchEvent_Liftoff:        return "UP";
  }
  return "UNKNOWN";
}

static void prv_show_prompt(void) {
  char prompt[80];
  snprintf(prompt, sizeof prompt, "%s %s\nUp:key Down:dir Select:rec",
           s_keys[s_key_index], s_gestures[s_gesture_index]);
  alert_set_text(prompt);
}

static void prv_cancel_feedback(void) {
  if (s_feedback_timer) {
    app_timer_cancel(s_feedback_timer);
    s_feedback_timer = NULL;
  }
}

static void prv_show_feedback(void *context) {
  s_feedback_timer = NULL;
  // A new touchdown may have started while the timer was pending. Never
  // trigger UI work in the middle of a finger's movement.
  if (s_recording && !s_stroke.active) {
    alert_set_text(s_feedback_text);
  }
}

static void prv_include_position(int16_t x, int16_t y) {
  if (x < s_stroke.min_x) s_stroke.min_x = x;
  if (x > s_stroke.max_x) s_stroke.max_x = x;
  if (y < s_stroke.min_y) s_stroke.min_y = y;
  if (y > s_stroke.max_y) s_stroke.max_y = y;
}

static const char *prv_direction(int dx, int dy) {
  const int ax = dx < 0 ? -dx : dx;
  const int ay = dy < 0 ? -dy : dy;

  if (ax == 0 && ay == 0) return "STILL";
  // Call a direction diagonal if both axes contribute substantially.
  if (ax && ay && (ax < ay ? ax * 2 >= ay : ay * 2 >= ax)) {
    if (dy < 0) return dx < 0 ? "UP_LEFT" : "UP_RIGHT";
    return dx < 0 ? "DOWN_LEFT" : "DOWN_RIGHT";
  }
  if (ax >= ay) return dx < 0 ? "LEFT" : "RIGHT";
  return dy < 0 ? "UP" : "DOWN";
}

static void prv_update_stroke(const TouchEvent *event, uint32_t now_ms) {
  switch (event->type) {
    case TouchEvent_Touchdown:
      prv_cancel_feedback();
      s_stroke = (StrokeStats) {
        .active = true,
        .start_ms = now_ms,
        .start_x = event->x,
        .start_y = event->y,
        .min_x = event->x,
        .max_x = event->x,
        .min_y = event->y,
        .max_y = event->y,
      };
      break;

    case TouchEvent_PositionUpdate:
      if (s_stroke.active) {
        prv_include_position(event->x, event->y);
        ++s_stroke.move_count;
      }
      break;

    case TouchEvent_Liftoff: {
      if (!s_stroke.active) return;
      prv_include_position(event->x, event->y);

      // Signed maximum excursion from touchdown, not merely the final delta.
      const int right = s_stroke.max_x - s_stroke.start_x;
      const int left = s_stroke.start_x - s_stroke.min_x;
      const int down = s_stroke.max_y - s_stroke.start_y;
      const int up = s_stroke.start_y - s_stroke.min_y;
      const int dx = right >= left ? right : -left;
      const int dy = down >= up ? down : -up;

      snprintf(s_feedback_text, sizeof s_feedback_text,
               "~%s %lums\nX%c%d Y%c%d (%u upd)",
               prv_direction(dx, dy),
               (unsigned long)(now_ms - s_stroke.start_ms),
               dx < 0 ? '-' : '+', dx < 0 ? -dx : dx,
               dy < 0 ? '-' : '+', dy < 0 ? -dy : dy,
               s_stroke.move_count);

      s_stroke.active = false;
      // Schedule painting after the callback returns. If the next gesture
      // begins first, its touchdown cancels this pending redraw.
      s_feedback_timer = app_timer_register(FEEDBACK_DELAY_MS,
                                             prv_show_feedback, NULL);
      break;
    }
  }
}

static void prv_touch_handler(const TouchEvent *event, void *context) {
  if (!s_recording || !event) {
    return;
  }

  const uint32_t now_ms = prv_now_ms();

  if (s_event_count == TRACE_MAX_EVENTS) {
    ++s_overflow_count;
  } else {
    s_events[s_event_count++] = (TraceEvent) {
      .elapsed_ms = now_ms - s_trial_start_ms,
      .x = event->x,
      .y = event->y,
      .type = event->type,
      .non_navigational = event->non_navigational,
    };
  }

  prv_update_stroke(event, now_ms);
}

static void prv_finish_trial(void) {
  // Stop receiving data before dumping. The logs are emitted only while idle.
  s_recording = false;
  prv_cancel_feedback();
  s_stroke.active = false;

  APP_LOG(APP_LOG_LEVEL_INFO, "TRACE_BEGIN,%lu,%s_%s",
          (unsigned long)s_trial_id,
          s_keys[s_key_index], s_gestures[s_gesture_index]);

  for (size_t i = 0; i < s_event_count; ++i) {
    const TraceEvent *event = &s_events[i];
    APP_LOG(APP_LOG_LEVEL_INFO, "TRACE,%lu,%u,%lu,%s,%d,%d,%u",
            (unsigned long)s_trial_id, (unsigned)i,
            (unsigned long)event->elapsed_ms,
            prv_event_name(event->type),
            event->x, event->y,
            (unsigned)event->non_navigational);
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "TRACE_END,%lu,%u,%lu",
          (unsigned long)s_trial_id,
          (unsigned)s_event_count, (unsigned long)s_overflow_count);

  char message[80];
  snprintf(message, sizeof message, "%s %s: %u events\nSelect:repeat",
           s_keys[s_key_index], s_gestures[s_gesture_index],
           (unsigned)s_event_count);
  alert_set_text(message);
}

void touch_recorder_select(void) {
  if (!s_touch_enabled) {
    return;
  }

  if (s_recording) {
    prv_finish_trial();
    return;
  }

  ++s_trial_id;
  s_event_count = 0;
  s_overflow_count = 0;
  prv_cancel_feedback();
  s_stroke.active = false;
  s_trial_start_ms = prv_now_ms();
  s_recording = true;

  char message[80];
  snprintf(message, sizeof message, "REC %s %s\nSelect:stop",
           s_keys[s_key_index], s_gestures[s_gesture_index]);
  alert_set_text(message);
}

void touch_recorder_next_key(void) {
  if (s_recording || !s_touch_enabled) {
    return;
  }
  s_key_index = (s_key_index + 1) % (sizeof s_keys / sizeof s_keys[0]);
  prv_show_prompt();
}

void touch_recorder_next_gesture(void) {
  if (s_recording || !s_touch_enabled) {
    return;
  }
  s_gesture_index = (s_gesture_index + 1) %
                    (sizeof s_gestures / sizeof s_gestures[0]);
  prv_show_prompt();
}

void touch_init(void) {
  s_touch_enabled = touch_service_is_enabled();
  if (!s_touch_enabled) {
    alert_set_text("Enable touchscreen in Settings to record traces.");
    return;
  }

  touch_service_subscribe(prv_touch_handler, NULL);
  prv_show_prompt();
}

void touch_deinit(void) {
  prv_cancel_feedback();
  if (s_touch_enabled) {
    touch_service_unsubscribe();
    s_touch_enabled = false;
  }
  // An unfinished trial is intentionally not emitted as a completed sample.
  s_recording = false;
  s_stroke.active = false;
}

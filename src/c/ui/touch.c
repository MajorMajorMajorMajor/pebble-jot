#include <pebble.h>

#include "ui/alert.h"
#include "ui/touch.h"

// Temporary on-device trace recorder. Nothing is logged while a trial is running:
// emitting APP_LOG messages inside the callback would disturb touch timing.
enum { TRACE_MAX_EVENTS = 1024, TRACE_MAX_STROKES = 128, FEEDBACK_DELAY_MS = 100 };

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
  int16_t last_x, last_y;
  uint32_t path_length;
  int16_t prev_dx, prev_dy;
  int32_t signed_turn;
  uint16_t turn_steps;
} StrokeStats;

typedef struct {
  uint32_t duration_ms;
  int16_t right, left, down, up;
  uint32_t path_length;
  unsigned int moves;
  const char *direction;
  int16_t end_dx, end_dy;
  int32_t signed_turn;
  uint16_t turn_steps;
  int8_t key_index;
} StrokeSummary;

static const char *const s_keys[] = {
  "A", "N", "I", "H", "O", "R", "T", "E", "S",
  "SPACE", "BACKSPACE", "RETURN", "123", "KB"
};

static const char *const s_gestures[] = {
  "TAP", "UP", "UP_RIGHT", "RIGHT", "DOWN_RIGHT",
  "DOWN", "DOWN_LEFT", "LEFT", "UP_LEFT",
  "RETURN_UP", "RETURN_UP_RIGHT", "RETURN_RIGHT", "RETURN_DOWN_RIGHT",
  "RETURN_DOWN", "RETURN_DOWN_LEFT", "RETURN_LEFT", "RETURN_UP_LEFT",
  "CW", "CCW"
};

typedef enum { MODE_MENU, MODE_TRACE, MODE_TYPING } InputMode;
static InputMode s_mode = MODE_MENU;
static int s_menu_selection;
static char s_typed[160];
static size_t s_typed_len;
static uint32_t s_typing_id;
static int s_touch_key = -1;
static const char *s_last_gesture;
static const char *s_last_binding;
static const char *const s_bindings[14][19] = {
  { "a", "", "", "-", "v", "", "$", "", "", "", "", "÷", "V", "", "¥", "", "", "", "A" }, // a
  { "n", "^", "´", "!", "\\", "l", "/", "+", "`", "ˇ", "’", "¡", "—", "L", "–", "×", "‘", "", "N" }, // n
  { "i", "", "\n", "", "€", "=", "x", "?", "", "", "\n", "", "£", "±", "X", "¿", "", "", "I" }, // i
  { "h", "", "%", "k", "_", "", "", "(", "{", "", "‰", "K", "¬", "", "]", ")", "}", "", "H" }, // h
  { "o", "u", "p", "b", "j", "d", "g", "c", "q", "U", "P", "B", "J", "D", "G", "C", "Q", "", "O" }, // o
  { "r", "", "}", ")", "]", "", "@", "m", "|", "", "{", "(", "", "", "ª", "M", "¶", "", "R" }, // r
  { "t", "¨", "y", "*", "", "", "\b", "<", "~", "˝", "Y", "†", "", "", "\b", "‹", "˜", "", "T" }, // t
  { "e", "w", "'", "z", ":", ".", ",", "", "\"", "W", "”", "Z", "„", "…", "‚", "", "“", "", "E" }, // e
  { "s", "&", "°", ">", " ", "", ";", "#", "f", "§", "º", "›", " ", "", ";", "£", "F", "", "S" }, // s
  { " ", "", "", "", " ", " ", " ", "", "", "", "", "", " ", " ", " ", "", "", "", " " }, // spacebar
  { "\b", "\b", "", "", "", "", "\b", "\b", "\b", "", "", "", "", "", "", "", "", "", "" }, // backspace
  { "\n", "\n", "", "\n", "\n", "\n", "\n", "\n", "", "", "", "", "", "", "", "", "", "", "" }, // enter
  { "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "" }, // 123
  { "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "" }, // hand
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
static StrokeSummary s_summaries[TRACE_MAX_STROKES];
static size_t s_summary_count;
static uint32_t s_summary_overflow;
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
  if (ax && ay && (ax < ay ? ax * 4 >= ay : ay * 4 >= ax)) {
    if (dy < 0) return dx < 0 ? "UP_LEFT" : "UP_RIGHT";
    return dx < 0 ? "DOWN_LEFT" : "DOWN_RIGHT";
  }
  if (ax >= ay) return dx < 0 ? "LEFT" : "RIGHT";
  return dy < 0 ? "UP" : "DOWN";
}

static void prv_track_position(int16_t x, int16_t y) {
  int dx = x - s_stroke.last_x;
  int dy = y - s_stroke.last_y;
  s_stroke.path_length += (uint32_t)(dx < 0 ? -dx : dx)
                        + (uint32_t)(dy < 0 ? -dy : dy);
  if (dx || dy) {
    if (s_stroke.prev_dx || s_stroke.prev_dy) {
      int32_t cross = (int32_t)s_stroke.prev_dx * dy
                    - (int32_t)s_stroke.prev_dy * dx;
      if (cross > 0) { ++s_stroke.signed_turn; ++s_stroke.turn_steps; }
      else if (cross < 0) { --s_stroke.signed_turn; ++s_stroke.turn_steps; }
    }
    s_stroke.prev_dx = dx;
    s_stroke.prev_dy = dy;
  }
  s_stroke.last_x = x;
  s_stroke.last_y = y;
  prv_include_position(x, y);
}

static int prv_key_at(int x, int y) {
  GRect bounds = layer_get_bounds(window_get_root_layer(window_stack_get_top_window()));
  const int top = 40; // Matches UIStyle.content_height_px.
  if (x < 0 || x >= bounds.size.w || y < top || y >= bounds.size.h) return -1;
  const int col = x * 4 / bounds.size.w;
  const int row = (y - top) * 4 / (bounds.size.h - top);
  static const int8_t keys[4][4] = {
    {13, 0, 1, 2}, {12, 3, 4, 5}, {10, 6, 7, 8}, {11, 9, 9, 9}
  };
  return keys[row][col];
}

static void prv_show_typed(void) {
  char view[200];
  const char *tail = s_typed_len > 35 ? s_typed + s_typed_len - 35 : s_typed;
  snprintf(view, sizeof view, "TYPE %lu\n%s", (unsigned long)s_typing_id, tail);
  alert_set_text(view);
}

static void prv_show_menu(void) {
  alert_set_text(s_menu_selection ? "Mode: Trace\nUp/Down:choose Select:go" :
                                    "Mode: Typing\nUp/Down:choose Select:go");
}

static void prv_update_stroke(const TouchEvent *event, uint32_t now_ms) {
  switch (event->type) {
    case TouchEvent_Touchdown:
      prv_cancel_feedback();
      s_touch_key = prv_key_at(event->x, event->y);
      s_stroke = (StrokeStats) {
        .active = true,
        .start_ms = now_ms,
        .start_x = event->x,
        .start_y = event->y,
        .min_x = event->x,
        .max_x = event->x,
        .min_y = event->y,
        .max_y = event->y,
        .last_x = event->x,
        .last_y = event->y,
      };
      break;

    case TouchEvent_PositionUpdate:
      if (s_stroke.active) {
        prv_track_position(event->x, event->y);
        ++s_stroke.move_count;
      }
      break;

    case TouchEvent_Liftoff: {
      if (!s_stroke.active) return;
      prv_track_position(event->x, event->y);

      // Signed maximum excursion from touchdown, not merely the final delta.
      const int right = s_stroke.max_x - s_stroke.start_x;
      const int left = s_stroke.start_x - s_stroke.min_x;
      const int down = s_stroke.max_y - s_stroke.start_y;
      const int up = s_stroke.start_y - s_stroke.min_y;
      const int dx = right >= left ? right : -left;
      const int dy = down >= up ? down : -up;

      const int end_dx = event->x - s_stroke.start_x;
      const int end_dy = event->y - s_stroke.start_y;
      const int return_distance = (end_dx < 0 ? -end_dx : end_dx)
                                + (end_dy < 0 ? -end_dy : end_dy);
      const int excursion = (right > left ? right : left)
                          + (down > up ? down : up);
      const char *direction = prv_direction(dx, dy);
      // Provisional shape heuristics; keep raw features for calibration.
      // Screen Y increases downward: positive turning is clockwise.
      if (excursion >= 16 && return_distance <= 8 &&
          s_stroke.path_length >= (uint32_t)(excursion * 2)) {
        if (s_stroke.turn_steps >= 5 &&
            s_stroke.signed_turn >= 4) direction = "CW";
        else if (s_stroke.turn_steps >= 5 &&
                 s_stroke.signed_turn <= -4) direction = "CCW";
        else {
          static const char *const returns[8] = {
            "RETURN_UP", "RETURN_UP_RIGHT", "RETURN_RIGHT", "RETURN_DOWN_RIGHT",
            "RETURN_DOWN", "RETURN_DOWN_LEFT", "RETURN_LEFT", "RETURN_UP_LEFT"
          };
          const char *outward = prv_direction(dx, dy);
          static const char *const dirs[8] = {
            "UP", "UP_RIGHT", "RIGHT", "DOWN_RIGHT", "DOWN", "DOWN_LEFT", "LEFT", "UP_LEFT"
          };
          for (unsigned i = 0; i < 8; ++i) {
            if (strcmp(outward, dirs[i]) == 0) { direction = returns[i]; break; }
          }
        }
      }
      if (s_summary_count < TRACE_MAX_STROKES) {
        s_summaries[s_summary_count++] = (StrokeSummary) {
          .duration_ms = now_ms - s_stroke.start_ms,
          .right = right, .left = left, .down = down, .up = up,
          .path_length = s_stroke.path_length,
          .moves = s_stroke.move_count, .direction = direction,
          .end_dx = end_dx, .end_dy = end_dy,
          .signed_turn = s_stroke.signed_turn,
          .turn_steps = s_stroke.turn_steps,
          .key_index = s_touch_key
        };
      } else {
        ++s_summary_overflow;
      }

      snprintf(s_feedback_text, sizeof s_feedback_text,
               "~%s %lums\nX%c%d Y%c%d (%u upd)",
               direction,
               (unsigned long)(now_ms - s_stroke.start_ms),
               dx < 0 ? '-' : '+', dx < 0 ? -dx : dx,
               dy < 0 ? '-' : '+', dy < 0 ? -dy : dy,
               s_stroke.move_count);

      s_stroke.active = false;
      if (s_mode == MODE_TYPING) {
        int gesture_index = -1;
        for (unsigned i = 0; i < sizeof s_gestures / sizeof s_gestures[0]; ++i) {
          if (strcmp(direction, s_gestures[i]) == 0) { gesture_index = (int)i; break; }
        }
        if (strcmp(direction, "STILL") == 0) gesture_index = 0;
        const char *binding = (s_touch_key >= 0 && gesture_index >= 0) ?
                              s_bindings[s_touch_key][gesture_index] : "";
        APP_LOG(APP_LOG_LEVEL_INFO, "TYPE,%lu,%s,%s,%s,%lu,%u,%d,%d,%d,%d,%lu,%ld",
                (unsigned long)++s_typing_id,
                s_touch_key >= 0 ? s_keys[s_touch_key] : "NONE", direction,
                *binding ? binding : "UNBOUND",
                (unsigned long)(now_ms - s_stroke.start_ms), s_stroke.move_count,
                right, left, down, up, (unsigned long)s_stroke.path_length,
                (long)s_stroke.signed_turn);
        if (*binding) {
          if (binding[0] == '\b' && s_typed_len) {
            // UTF-8 safe backspace.
            do { --s_typed_len; } while (s_typed_len &&
                ((unsigned char)s_typed[s_typed_len] & 0xc0) == 0x80);
            s_typed[s_typed_len] = 0;
          } else if (binding[0] != '\b') {
            size_t n = strlen(binding);
            if (s_typed_len + n < sizeof s_typed) {
              memcpy(s_typed + s_typed_len, binding, n + 1);
              s_typed_len += n;
            }
          }
        }
        prv_show_typed();
        break;
      }
      // Schedule painting after the callback returns. If the next gesture
      // begins first, its touchdown cancels this pending redraw.
      s_feedback_timer = app_timer_register(FEEDBACK_DELAY_MS,
                                             prv_show_feedback, NULL);
      break;
    }
  }
}

static void prv_touch_handler(const TouchEvent *event, void *context) {
  if ((s_mode == MODE_TRACE && !s_recording) || s_mode == MODE_MENU || !event) {
    return;
  }

  const uint32_t now_ms = prv_now_ms();

  if (s_mode == MODE_TYPING) {
    prv_update_stroke(event, now_ms);
    return;
  }

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

  for (size_t i = 0; i < s_summary_count; ++i) {
    const StrokeSummary *st = &s_summaries[i];
    APP_LOG(APP_LOG_LEVEL_INFO,
            "STROKE,%lu,%u,%lu,%u,%d,%d,%d,%d,%lu,%s,%d,%d,%ld,%u",
            (unsigned long)s_trial_id, (unsigned)i,
            (unsigned long)st->duration_ms, st->moves,
            st->right, st->left, st->down, st->up,
            (unsigned long)st->path_length, st->direction,
            st->end_dx, st->end_dy, (long)st->signed_turn,
            (unsigned)st->turn_steps);
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "STROKE_END,%lu,%u,%lu",
          (unsigned long)s_trial_id, (unsigned)s_summary_count,
          (unsigned long)s_summary_overflow);

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
  if (s_mode == MODE_MENU) {
    s_mode = s_menu_selection ? MODE_TRACE : MODE_TYPING;
    if (s_mode == MODE_TYPING) { s_typed_len = 0; s_typed[0] = 0; s_typing_id = 0; prv_show_typed(); }
    else prv_show_prompt();
    return;
  }
  if (s_mode == MODE_TYPING) { s_mode = MODE_MENU; prv_show_menu(); return; }
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
  s_summary_count = 0;
  s_summary_overflow = 0;
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
  if (s_mode == MODE_MENU) { s_menu_selection = 1 - s_menu_selection; prv_show_menu(); return; }
  if (s_mode == MODE_TYPING) return;
  if (s_recording || !s_touch_enabled) {
    return;
  }
  s_key_index = (s_key_index + 1) % (sizeof s_keys / sizeof s_keys[0]);
  prv_show_prompt();
}

void touch_recorder_next_gesture(void) {
  if (s_mode == MODE_MENU) { s_menu_selection = 1 - s_menu_selection; prv_show_menu(); return; }
  if (s_mode == MODE_TYPING) return;
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
  prv_show_menu();
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

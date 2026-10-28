#include <pebble.h>

#include "ui/alert.h"
#include "ui/touch.h"

// Temporary on-device trace recorder. Nothing is logged while a trial is running:
// emitting APP_LOG messages inside the callback would disturb touch timing.
enum {
  TRACE_MAX_EVENTS = 1024, TRACE_MAX_STROKES = 128,
  FEEDBACK_DELAY_MS = 100, TRACE_PACK_EVENTS = 7,
  TRACE_LOG_LINES_PER_TICK = 4, TRACE_LOG_QUEUE_LIMIT = 3
};

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
  // Peak radial displacement and the origin-anchored signed polygon area.
  // Both accumulate with constant memory, including after trace-buffer overflow.
  int16_t peak_dx, peak_dy;
  uint32_t peak_sq;
  int32_t signed_area2;
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
  int16_t peak_dx, peak_dy;
  int32_t signed_area2;
  int32_t reversal_dot;
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
// Trace mode selectors are independent of the detected starting key.
// 0: Tap/Hold (2), 1: Swipe (8), 2: Return (8), 3: Circle (2).
static uint8_t s_trace_family;
static uint8_t s_trace_choice;
static int s_trial_key = -1;
static bool s_trial_key_set;
static const uint8_t s_trace_choices[] = { 2, 8, 8, 2 };
static const char *const s_trace_labels[4][8] = {
  { "TAP", "HOLD" },
  { "UP", "UP_RIGHT", "RIGHT", "DOWN_RIGHT", "DOWN", "DOWN_LEFT", "LEFT", "UP_LEFT" },
  { "RETURN_UP", "RETURN_UP_RIGHT", "RETURN_RIGHT", "RETURN_DOWN_RIGHT",
    "RETURN_DOWN", "RETURN_DOWN_LEFT", "RETURN_LEFT", "RETURN_UP_LEFT" },
  { "CW", "CCW" }
};

static const char *prv_trace_label(void) {
  return s_trace_labels[s_trace_family][s_trace_choice];
}

// Immutable snapshot queued on stop. A new trial can use s_events immediately,
// while timer callbacks export the previous one without blocking button clicks.
typedef struct TraceDump {
  struct TraceDump *next;
  uint32_t id;
  int key;
  const char *label;
  size_t event_count, stroke_count;
  uint32_t event_overflow, stroke_overflow;
  size_t event_pos, stroke_pos;
  uint8_t phase;
  TraceEvent *events;
  StrokeSummary *strokes;
} TraceDump;
static TraceDump *s_dump_head, *s_dump_tail;
static uint8_t s_dump_count;
static AppTimer *s_dump_timer;

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
  snprintf(prompt, sizeof prompt, "Trace %s\nUp:type Down:dir Sel:rec",
           prv_trace_label());
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

static const char *prv_direction(int dx, int dy, int key_index) {
  const int ax = dx < 0 ? -dx : dx;
  const int ay = dy < 0 ? -dy : dy;

  if (ax == 0 && ay == 0) return "STILL";
  // At the physical edge, the outward component is compressed by the screen.
  // Keep central-key diagonals strict to avoid classifying drift as a diagonal.
  int diagonal_ratio = 4;
  if ((dx > 0 && (key_index == 2 || key_index == 5 || key_index == 8)) ||
      (dx < 0 && (key_index == 10 || key_index == 11 ||
                  key_index == 12 || key_index == 13))) {
    diagonal_ratio = 8;
  }
  if (ax && ay && (ax < ay ? ax * diagonal_ratio >= ay :
                             ay * diagonal_ratio >= ax)) {
    if (dy < 0) return dx < 0 ? "UP_LEFT" : "UP_RIGHT";
    return dx < 0 ? "DOWN_LEFT" : "DOWN_RIGHT";
  }
  if (ax >= ay) return dx < 0 ? "LEFT" : "RIGHT";
  return dy < 0 ? "UP" : "DOWN";
}

static void prv_track_position(int16_t x, int16_t y) {
  int dx = x - s_stroke.last_x;
  int dy = y - s_stroke.last_y;
  const int old_x = s_stroke.last_x - s_stroke.start_x;
  const int old_y = s_stroke.last_y - s_stroke.start_y;
  const int rel_x = x - s_stroke.start_x;
  const int rel_y = y - s_stroke.start_y;
  const uint32_t radial_sq = (uint32_t)(rel_x * rel_x + rel_y * rel_y);
  if (radial_sq > s_stroke.peak_sq) {
    s_stroke.peak_sq = radial_sq;
    s_stroke.peak_dx = rel_x;
    s_stroke.peak_dy = rel_y;
  }
  // Twice the signed area swept around touchdown (shoelace formula).
  // On screen coordinates, positive means clockwise.
  s_stroke.signed_area2 += (int32_t)old_x * rel_y - (int32_t)old_y * rel_x;
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
      // Assign the trial key from its first touchdown, not the button selector.
      if (s_mode == MODE_TRACE && !s_trial_key_set) {
        s_trial_key = s_touch_key;
        s_trial_key_set = true;
      }
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

      // Extrema remain useful diagnostics, but the outward direction comes
      // from the farthest point, not a combination of unrelated axis extrema.
      const int right = s_stroke.max_x - s_stroke.start_x;
      const int left = s_stroke.start_x - s_stroke.min_x;
      const int down = s_stroke.max_y - s_stroke.start_y;
      const int up = s_stroke.start_y - s_stroke.min_y;
      const int dx = s_stroke.peak_dx;
      const int dy = s_stroke.peak_dy;
      const int end_dx = event->x - s_stroke.start_x;
      const int end_dy = event->y - s_stroke.start_y;
      const int peak_major = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ?
                             (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy);
      const int32_t reversal_dot = (int32_t)s_stroke.peak_sq -
                                   (int32_t)end_dx * dx - (int32_t)end_dy * dy;
      const int width = s_stroke.max_x - s_stroke.min_x;
      const int height = s_stroke.max_y - s_stroke.min_y;
      const int min_span = width < height ? width : height;
      const int max_span = width > height ? width : height;
      const int32_t area_abs = s_stroke.signed_area2 < 0 ?
                               -s_stroke.signed_area2 : s_stroke.signed_area2;
      const char *direction = prv_direction(dx, dy, s_touch_key);

      // Circle first: reject a skinny drag or wiggle using both area coverage
      // and aspect ratio. No square roots, floating point, or stored samples.
      if (s_stroke.peak_sq >= 400 && s_stroke.path_length >= 60 &&
          min_span >= 12 && min_span * 4 >= max_span &&
          area_abs * 4 >= 3 * width * height) {
        direction = s_stroke.signed_area2 > 0 ? "CW" : "CCW";
      } else if (s_stroke.peak_sq >= 100 &&
                 reversal_dot >= 4 * peak_major &&
                 reversal_dot * 8 >= (int32_t)s_stroke.peak_sq) {
        // Finger need not finish close to touchdown. Only the displacement
        // *back along the outward axis* matters; preserve all 8 directions.
        static const char *const returns[8] = {
          "RETURN_UP", "RETURN_UP_RIGHT", "RETURN_RIGHT", "RETURN_DOWN_RIGHT",
          "RETURN_DOWN", "RETURN_DOWN_LEFT", "RETURN_LEFT", "RETURN_UP_LEFT"
        };
        static const char *const dirs[8] = {
          "UP", "UP_RIGHT", "RIGHT", "DOWN_RIGHT", "DOWN", "DOWN_LEFT", "LEFT", "UP_LEFT"
        };
        for (unsigned i = 0; i < 8; ++i) {
          if (strcmp(direction, dirs[i]) == 0) { direction = returns[i]; break; }
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
          .peak_dx = dx, .peak_dy = dy,
          .signed_area2 = s_stroke.signed_area2,
          .reversal_dot = reversal_dot,
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
        APP_LOG(APP_LOG_LEVEL_INFO, "TYPE,%lu,%s,%s,%s,%lu,%u,%d,%d,%d,%d,%lu,%ld,%d,%d,%ld,%ld",
                (unsigned long)++s_typing_id,
                s_touch_key >= 0 ? s_keys[s_touch_key] : "NONE", direction,
                *binding ? binding : "UNBOUND",
                (unsigned long)(now_ms - s_stroke.start_ms), s_stroke.move_count,
                right, left, down, up, (unsigned long)s_stroke.path_length,
                (long)s_stroke.signed_turn, dx, dy,
                (long)s_stroke.signed_area2, (long)reversal_dot);
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

// Packet format: TRACE_PACK,trial,first_seq,base_ms,hex
// Each 10-digit hex sample is (dt:16, x:8, y:8, flags:8), dt relative
// to the packet's base_ms. flags: 0=DOWN,1=MOVE,2=UP, plus bit 2 for
// non_navigational. A decoder accompanies this patch.
static char prv_hex_digit(unsigned n) {
  return "0123456789ABCDEF"[n & 15];
}

static void prv_hex_byte(char *out, uint8_t v) {
  out[0] = prv_hex_digit(v >> 4);
  out[1] = prv_hex_digit(v);
}

static void prv_export_tick(void *context);

static void prv_schedule_export(void) {
  if (!s_dump_timer && s_dump_head && !s_recording) {
    s_dump_timer = app_timer_register(10, prv_export_tick, NULL);
  }
}

static void prv_free_dump(TraceDump *dump) {
  free(dump->events);
  free(dump->strokes);
  free(dump);
}

// Copy out just the actual samples; no 1024-event duplicate is kept permanently.
static bool prv_queue_trial(void) {
  TraceDump *dump = calloc(1, sizeof *dump);
  if (!dump) return false;
  if (s_event_count) {
    dump->events = malloc(s_event_count * sizeof *dump->events);
    if (!dump->events) { prv_free_dump(dump); return false; }
    memcpy(dump->events, s_events, s_event_count * sizeof *dump->events);
  }
  if (s_summary_count) {
    dump->strokes = malloc(s_summary_count * sizeof *dump->strokes);
    if (!dump->strokes) { prv_free_dump(dump); return false; }
    memcpy(dump->strokes, s_summaries,
           s_summary_count * sizeof *dump->strokes);
  }
  dump->id = s_trial_id;
  dump->key = s_trial_key;
  dump->label = prv_trace_label();
  dump->event_count = s_event_count;
  dump->stroke_count = s_summary_count;
  dump->event_overflow = s_overflow_count;
  dump->stroke_overflow = s_summary_overflow;
  if (s_dump_tail) s_dump_tail->next = dump;
  else s_dump_head = dump;
  s_dump_tail = dump;
  ++s_dump_count;
  return true;
}

static void prv_export_tick(void *context) {
  s_dump_timer = NULL;
  if (s_recording || !s_dump_head) return;
  unsigned emitted = 0;
  while (s_dump_head && emitted < TRACE_LOG_LINES_PER_TICK) {
    TraceDump *d = s_dump_head;
    if (d->phase == 0) {
      APP_LOG(APP_LOG_LEVEL_INFO, "TRACE_BEGIN,%lu,%s_%s",
              (unsigned long)d->id,
              d->key >= 0 ? s_keys[d->key] : "NONE", d->label);
      d->phase = 1;
    } else if (d->phase == 1) {
      if (d->event_pos == d->event_count) { d->phase = 2; continue; }
      const size_t first = d->event_pos;
      const TraceEvent *base_event = &d->events[first];
      const uint32_t base_ms = base_event->elapsed_ms;
      char packed[TRACE_PACK_EVENTS * 10 + 1];
      size_t used = 0;
      while (d->event_pos < d->event_count && used < TRACE_PACK_EVENTS) {
        const TraceEvent *e = &d->events[d->event_pos];
        const uint32_t dt = e->elapsed_ms - base_ms;
        if (dt > 65535) break;
        // Touchscreen coordinates normally fit within an unsigned byte;
        // preserve unusual/out-of-bounds values using a legacy record.
        if (e->x < 0 || e->x > 255 || e->y < 0 || e->y > 255) break;
        char *out = packed + used * 10;
        prv_hex_byte(out,     (uint8_t)(dt >> 8));
        prv_hex_byte(out + 2, (uint8_t)dt);
        prv_hex_byte(out + 4, (uint8_t)e->x);
        prv_hex_byte(out + 6, (uint8_t)e->y);
        uint8_t flags = (e->type == TouchEvent_Touchdown ? 0 :
                         e->type == TouchEvent_PositionUpdate ? 1 : 2);
        if (e->non_navigational) flags |= 4;
        prv_hex_byte(out + 8, flags);
        ++used;
        ++d->event_pos;
      }
      if (used) {
        packed[used * 10] = '\0';
        APP_LOG(APP_LOG_LEVEL_INFO, "TRACE_PACK,%lu,%u,%lu,%s",
                (unsigned long)d->id, (unsigned)first,
                (unsigned long)base_ms, packed);
      } else {
        const TraceEvent *e = &d->events[d->event_pos++];
        APP_LOG(APP_LOG_LEVEL_INFO, "TRACE,%lu,%u,%lu,%s,%d,%d,%u",
                (unsigned long)d->id, (unsigned)first,
                (unsigned long)e->elapsed_ms, prv_event_name(e->type),
                e->x, e->y, (unsigned)e->non_navigational);
      }
    } else if (d->phase == 2) {
      if (d->stroke_pos == d->stroke_count) { d->phase = 3; continue; }
      const StrokeSummary *st = &d->strokes[d->stroke_pos];
      APP_LOG(APP_LOG_LEVEL_INFO,
              "STROKE,%lu,%u,%lu,%u,%d,%d,%d,%d,%lu,%s,%d,%d,%ld,%u,%d,%d,%ld,%ld,%s",
              (unsigned long)d->id, (unsigned)d->stroke_pos,
              (unsigned long)st->duration_ms, st->moves,
              st->right, st->left, st->down, st->up,
              (unsigned long)st->path_length, st->direction,
              st->end_dx, st->end_dy, (long)st->signed_turn,
              (unsigned)st->turn_steps, st->peak_dx, st->peak_dy,
              (long)st->signed_area2, (long)st->reversal_dot,
              st->key_index >= 0 ? s_keys[st->key_index] : "NONE");
      ++d->stroke_pos;
    } else if (d->phase == 3) {
      APP_LOG(APP_LOG_LEVEL_INFO, "STROKE_END,%lu,%u,%lu",
              (unsigned long)d->id, (unsigned)d->stroke_count,
              (unsigned long)d->stroke_overflow);
      d->phase = 4;
    } else {
      APP_LOG(APP_LOG_LEVEL_INFO, "TRACE_END,%lu,%u,%lu",
              (unsigned long)d->id, (unsigned)d->event_count,
              (unsigned long)d->event_overflow);
      s_dump_head = d->next;
      if (!s_dump_head) s_dump_tail = NULL;
      --s_dump_count;
      prv_free_dump(d);
    }
    ++emitted;
  }
  prv_schedule_export();
}

static void prv_finish_trial(void) {
  s_recording = false;
  prv_cancel_feedback();
  s_stroke.active = false;

  const bool queued = prv_queue_trial();
  char message[80];
  if (queued) {
    snprintf(message, sizeof message, "%s %s: %u events\nSaved; Select:again",
             s_trial_key >= 0 ? s_keys[s_trial_key] : "NONE",
             prv_trace_label(), (unsigned)s_event_count);
    prv_schedule_export();
  } else {
    // Never pretend data was exported if the copy could not be allocated.
    APP_LOG(APP_LOG_LEVEL_ERROR, "TRACE_QUEUE_FAILED,%lu,%u",
            (unsigned long)s_trial_id, (unsigned)s_event_count);
    snprintf(message, sizeof message, "Export memory full\nTrial NOT saved");
  }
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

  if (s_dump_count >= TRACE_LOG_QUEUE_LIMIT) {
    alert_set_text("Export queue full\nSelect again after export");
    prv_schedule_export();
    return;
  }
  ++s_trial_id;
  s_trial_key = -1;
  s_trial_key_set = false;
  s_event_count = 0;
  s_overflow_count = 0;
  s_summary_count = 0;
  s_summary_overflow = 0;
  prv_cancel_feedback();
  s_stroke.active = false;
  s_trial_start_ms = prv_now_ms();
  s_recording = true;

  char message[80];
  snprintf(message, sizeof message, "REC %s\nTouch key; Select:stop",
           prv_trace_label());
  alert_set_text(message);
}

void touch_recorder_next_type(void) {
  if (s_mode == MODE_MENU) { s_menu_selection = 1 - s_menu_selection; prv_show_menu(); return; }
  if (s_mode == MODE_TYPING) return;
  if (s_recording || !s_touch_enabled) return;
  s_trace_family = (s_trace_family + 1) % 4;
  s_trace_choice = 0;
  prv_show_prompt();
}

void touch_recorder_next_direction(void) {
  if (s_mode == MODE_MENU) { s_menu_selection = 1 - s_menu_selection; prv_show_menu(); return; }
  if (s_mode == MODE_TYPING) return;
  if (s_recording || !s_touch_enabled) return;
  s_trace_choice = (s_trace_choice + 1) % s_trace_choices[s_trace_family];
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
  if (s_dump_timer) { app_timer_cancel(s_dump_timer); s_dump_timer = NULL; }
  // Flush on orderly shutdown rather than silently losing finished trials.
  // This may take time when exiting, but never blocks the normal stop button.
  while (s_dump_head) {
    prv_export_tick(NULL);
    if (s_dump_timer) { app_timer_cancel(s_dump_timer); s_dump_timer = NULL; }
  }
}

#include <pebble.h>

#include "ui/alert.h"
#include "ui/touch.h"
#include "jot/jot.h"
#include "jot/store.h"
#include "jot/reader.h"
#include "jot/reader_touch.h"

// Temporary on-device trace recorder. Nothing is logged while a trial is running:
// emitting APP_LOG messages inside the callback would disturb touch timing.
enum {
  TRACE_MAX_EVENTS = 1024, TRACE_MAX_STROKES = 128,
  FEEDBACK_DELAY_MS = 100, TRACE_PACK_EVENTS = 6,
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

typedef enum { MODE_MENU, MODE_TRACE, MODE_TYPING, MODE_JOT } InputMode;
static InputMode s_mode = MODE_JOT;
static int s_menu_selection;
static char s_typed[160];
static size_t s_typed_len;
static uint32_t s_typing_id;
static int s_touch_key = -1;

// A touch originating in the 40 px editor text strip is reserved for
// peek/read mode, never for keyboard gestures.
static bool s_header_touch_active;
static int16_t s_header_touch_x, s_header_touch_y;
static uint32_t s_header_touch_ms;
// List touch is deliberately limited to stationary taps: a scrolling
// gesture must not accidentally activate the new-note pill.
static bool s_list_touch_active;
static bool s_list_touch_moved;
static bool s_list_touch_dragging;
static bool s_list_touch_can_drag;
static int16_t s_list_touch_x, s_list_touch_y, s_list_touch_last_y;
static uint32_t s_list_touch_ms;
// Track the reader's Back tap independently of list and keyboard gestures.
// Movement cancellation is sticky, so returning to the start still cannot
// accidentally activate Back on release.
static bool s_reader_touch_active;
static bool s_reader_touch_moved;
static int16_t s_reader_touch_x, s_reader_touch_y;
static uint32_t s_reader_touch_ms;
// src/c/ui/touch.c describes the currently implemented bindings only.
// docs/gesture-map.md and docs/gesture-map.csv preserve the complete original
// English-LH keyboard design, including reserved-but-not-yet-implemented
// gestures. Do not treat blank entries here as available shortcuts.
static const char *const s_bindings[14][19] = {
  { "a", "", "", "-", "v", "", "$", "", "", "", "", "÷", "V", "", "¥", "", "", "7", "A" }, // a
  { "n", "^", "´", "!", "\\", "l", "/", "+", "`", "ˇ", "’", "¡", "—", "L", "–", "×", "‘", "8", "N" }, // n
  { "i", "", "\n", "", "€", "=", "x", "?", "", "", "\n", "", "£", "±", "X", "¿", "", "9", "I" }, // i
  { "h", "", "%", "k", "_", "", "", "(", "{", "", "‰", "K", "¬", "", "]", ")", "}", "4", "H" }, // h
  { "o", "u", "p", "b", "j", "d", "g", "c", "q", "U", "P", "B", "J", "D", "G", "C", "Q", "5", "O" }, // o
  { "r", "", "}", ")", "]", "", "@", "m", "|", "", "{", "(", "", "", "ª", "M", "¶", "6", "R" }, // r
  { "t", "¨", "y", "*", "", "", "\b", "<", "~", "˝", "Y", "†", "", "", "\b", "‹", "˜", "1", "T" }, // t
  { "e", "w", "'", "z", ":", ".", ",", "", "\"", "W", "”", "Z", "„", "…", "‚", "", "“", "2", "E" }, // e
  { "s", "&", "°", ">", " ", "", ";", "#", "f", "§", "º", "›", " ", "", ";", "£", "F", "3", "S" }, // s
  { " ", "", "", "", " ", " ", " ", "", "", "", "", "", " ", " ", " ", "", "", "", "0" }, // spacebar
  { "\b", "\b", "", "", "", "", "\b", "\b", "\b", "", "", "", "", "", "", "", "", "", "" }, // backspace
  { "\n", "\n", "", "\n", "\n", "\n", "\n", "\n", "", "", "", "", "", "", "", "", "", "", "" }, // enter
  { "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "" }, // 123
  { "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "" }, // hand
};
// Existing, reserved cursor gestures from docs/gesture-map.csv.
// These are not newly claimed shortcuts: the original workbook assigns
// Space E/W, I N/Return N, and S S/Return S to these commands.
static bool prv_jot_cursor_command(int key, int gesture) {
  JotCursorMove move;
  if (key == 9 && gesture == 3) move = JOT_CURSOR_RIGHT;         // Space E
  else if (key == 9 && gesture == 7) move = JOT_CURSOR_LEFT;     // Space W
  else if (key == 2 && gesture == 1) move = JOT_CURSOR_UP;       // I N
  else if (key == 8 && gesture == 5) move = JOT_CURSOR_DOWN;     // S S
  else if (key == 2 && gesture == 9) move = JOT_CURSOR_HOME;     // I Return N
  else if (key == 8 && gesture == 13) move = JOT_CURSOR_END;     // S Return S
  else return false;

  // Movement never changes note contents or marks a clean note dirty.
  jot_store_move_cursor(move);
  jot_refresh_editor();
  return true;
}

// In the canonical English-LH map, Backspace NE/E/SE/S deletes the next
// character. Keep the diagnostic typing mode's legacy bindings intact.
static bool prv_jot_forward_delete_command(int key, int gesture) {
  if (key != 10 || (gesture != 2 && gesture != 3 &&
                    gesture != 4 && gesture != 5)) return false;
  jot_delete_forward();
  return true;
}

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
// An export is complete only after TRACE_END has been emitted for its trial.
// This is a watch-side indicator, not an acknowledgement from the phone.
static uint32_t s_last_exported_trial_id;
static uint32_t s_failed_export_trial_id;

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
  if (s_dump_count) {
    snprintf(prompt, sizeof prompt, "Trace %s\nExporting %u trial(s)...",
             prv_trace_label(), (unsigned)s_dump_count);
  } else if (s_failed_export_trial_id) {
    snprintf(prompt, sizeof prompt, "Trace %s\nEXPORT FAILED #%lu",
             prv_trace_label(), (unsigned long)s_failed_export_trial_id);
  } else if (s_last_exported_trial_id) {
    snprintf(prompt, sizeof prompt, "Trace %s\nReady (trial #%lu)",
             prv_trace_label(), (unsigned long)s_last_exported_trial_id);
  } else {
    snprintf(prompt, sizeof prompt, "Trace %s\nUp:type Down:dir Sel:rec",
             prv_trace_label());
  }
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
  if (dx > 0 && (key_index == 2 || key_index == 5 || key_index == 8)) {
    // I/R/S start at the right edge: outward x motion is often compressed.
    diagonal_ratio = 10;
  } else if (dx < 0 && (key_index == 10 || key_index == 11 ||
                       key_index == 12 || key_index == 13)) {
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
  char prompt[80];
  const char *mode = s_menu_selection ? "Trace" : "Typing";
  if (s_dump_count) {
    snprintf(prompt, sizeof prompt, "Mode: %s\nExporting %u trial(s)...",
             mode, (unsigned)s_dump_count);
  } else if (s_failed_export_trial_id) {
    snprintf(prompt, sizeof prompt, "Mode: %s\nEXPORT FAILED #%lu",
             mode, (unsigned long)s_failed_export_trial_id);
  } else if (s_last_exported_trial_id) {
    snprintf(prompt, sizeof prompt, "Mode: %s\nReady (trial #%lu)",
             mode, (unsigned long)s_last_exported_trial_id);
  } else {
    snprintf(prompt, sizeof prompt, "Mode: %s\nUp/Down:choose Select:go",
             mode);
  }
  alert_set_text(prompt);
}

// Refresh only at state changes, not on every log line: repaints during
// export would compete with the logger and defeat the responsiveness fix.
static void prv_show_export_status(void) {
  if (s_recording) return;
  if (s_mode == MODE_TRACE) prv_show_prompt();
  else if (s_mode == MODE_MENU) prv_show_menu();
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

      // Circle first. Two confidence bands distinguish a real loop from
      // straight/return strokes. Larger loops may be relatively narrow;
      // smaller loops need strong enclosed-area evidence. All integer math.
      const bool normal_loop =
          s_stroke.peak_sq >= 400 && s_stroke.path_length >= 60 &&
          min_span >= 12 && min_span * 5 >= max_span &&
          area_abs >= (11 * width * height + 19) / 20;
      const bool compact_loop =
          s_stroke.peak_sq >= 225 && s_stroke.path_length >= 45 &&
          min_span >= 9 && min_span * 6 >= max_span &&
          area_abs >= width * height;
      if (normal_loop || compact_loop) {
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
      if (s_mode == MODE_TRACE && s_summary_count < TRACE_MAX_STROKES) {
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
      } else if (s_mode == MODE_TRACE) {
        ++s_summary_overflow;
      }

      if (s_mode == MODE_TRACE) snprintf(s_feedback_text, sizeof s_feedback_text,
               "~%s %lums\nX%c%d Y%c%d (%u upd)",
               direction,
               (unsigned long)(now_ms - s_stroke.start_ms),
               dx < 0 ? '-' : '+', dx < 0 ? -dx : dx,
               dy < 0 ? '-' : '+', dy < 0 ? -dy : dy,
               s_stroke.move_count);

      s_stroke.active = false;
      if (s_mode == MODE_TYPING || s_mode == MODE_JOT) {
        int gesture_index = -1;
        for (unsigned i = 0; i < sizeof s_gestures / sizeof s_gestures[0]; ++i) {
          if (strcmp(direction, s_gestures[i]) == 0) { gesture_index = (int)i; break; }
        }
        if (strcmp(direction, "STILL") == 0) gesture_index = 0;
        const char *binding = (s_touch_key >= 0 && gesture_index >= 0) ?
                              s_bindings[s_touch_key][gesture_index] : "";
        if (s_mode == MODE_JOT) {
          // The KB corner is also a convenient touch shortcut to note history.
          if (s_touch_key == 13 && gesture_index == 0) jot_show_list();
          else if (!prv_jot_cursor_command(s_touch_key, gesture_index) &&
                   !prv_jot_forward_delete_command(s_touch_key, gesture_index))
            jot_input(binding);
          break;
        }
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
  if (!event) return;

  if (s_mode == MODE_JOT && jot_reader_is_open()) {
    const uint32_t now_ms = prv_now_ms();
    if (event->type == TouchEvent_Touchdown) {
      s_reader_touch_active = true;
      s_reader_touch_moved = false;
      s_reader_touch_x = event->x;
      s_reader_touch_y = event->y;
      s_reader_touch_ms = now_ms;
    } else if ((event->type == TouchEvent_PositionUpdate ||
                event->type == TouchEvent_Liftoff) && s_reader_touch_active) {
      const int dx = event->x - s_reader_touch_x;
      const int dy = event->y - s_reader_touch_y;
      // An excursion beyond tap slop is permanent for this stroke.
      if (dx > JOT_READER_TAP_SLOP || dx < -JOT_READER_TAP_SLOP ||
          dy > JOT_READER_TAP_SLOP || dy < -JOT_READER_TAP_SLOP)
        s_reader_touch_moved = true;

      if (event->type == TouchEvent_Liftoff) {
        const JotReaderTouchLayout layout = jot_reader_touch_layout();
        const JotReaderTouchAction action = jot_reader_touch_action(
            jot_reader_is_peek(), s_reader_touch_x, s_reader_touch_y,
            event->x, event->y, s_reader_touch_moved,
            now_ms - s_reader_touch_ms, &layout);
        s_reader_touch_active = false;
        if (action == JOT_READER_TOUCH_BACK ||
            action == JOT_READER_TOUCH_DISMISS_PEEK) {
          jot_reader_back();
        } else if (action == JOT_READER_TOUCH_EDIT) {
          // The same safe window-stack transition as physical Select.
          jot_reader_edit_current();
        }
      }
    }
    return;
  }
  s_reader_touch_active = false;

  if (s_mode == MODE_JOT && jot_list_is_open()) {
    const uint32_t now_ms = prv_now_ms();
    if (event->type == TouchEvent_Touchdown) {
      s_list_touch_active = true;
      s_list_touch_moved = false;
      s_list_touch_dragging = false;
      s_list_touch_can_drag = jot_list_can_drag(event->y);
      s_list_touch_x = event->x;
      s_list_touch_y = event->y;
      s_list_touch_last_y = event->y;
      s_list_touch_ms = now_ms;
    } else if ((event->type == TouchEvent_PositionUpdate ||
                event->type == TouchEvent_Liftoff) && s_list_touch_active) {
      const int dx = event->x - s_list_touch_x;
      const int dy = event->y - s_list_touch_y;
      const int abs_dx = dx < 0 ? -dx : dx;
      const int abs_dy = dy < 0 ? -dy : dy;
      // Once a swipe exceeds the tap slop, it stays a swipe even if the
      // finger comes back to its starting position before release.
      if (abs_dx > 10 || abs_dy > 10) s_list_touch_moved = true;
      if (!s_list_touch_dragging && s_list_touch_can_drag &&
          abs_dy > 10 && abs_dy >= abs_dx) {
        s_list_touch_dragging = true;
        jot_list_drag((int16_t)dy);
      } else if (s_list_touch_dragging) {
        jot_list_drag((int16_t)(event->y - s_list_touch_last_y));
      }
      s_list_touch_last_y = event->y;

      if (event->type == TouchEvent_Liftoff) {
        const bool tap = !s_list_touch_moved &&
                         now_ms - s_list_touch_ms < 700;
        s_list_touch_active = false;
        s_list_touch_dragging = false;
        if (tap) jot_list_tap(event->x, event->y);
      }
    }
    return;
  }
  s_list_touch_active = false;
  s_list_touch_dragging = false;

  if ((s_mode == MODE_TRACE && !s_recording) || s_mode == MODE_MENU) return;

  const uint32_t now_ms = prv_now_ms();

  if (s_mode == MODE_JOT) {
    if (event->type == TouchEvent_Touchdown) {
      s_header_touch_active = event->y >= 0 && event->y < 40;
      if (s_header_touch_active) {
        s_header_touch_x = event->x;
        s_header_touch_y = event->y;
        s_header_touch_ms = now_ms;
        s_stroke.active = false;
        return;
      }
    }
    if (s_header_touch_active) {
      if (event->type == TouchEvent_Liftoff) {
        const int dx = event->x - s_header_touch_x;
        const int dy = event->y - s_header_touch_y;
        const bool tap = event->y >= 0 && event->y < 40 &&
                         dx >= -12 && dx <= 12 &&
                         dy >= -12 && dy <= 12 &&
                         now_ms - s_header_touch_ms < 700;
        s_header_touch_active = false;
        if (tap) jot_reader_peek();
      }
      return;
    }
  }

  if (s_mode == MODE_TYPING || s_mode == MODE_JOT) {
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
      s_last_exported_trial_id = d->id;
      s_dump_head = d->next;
      if (!s_dump_head) s_dump_tail = NULL;
      --s_dump_count;
      prv_free_dump(d);
      // "Ready" appears only after the queue has drained through TRACE_END.
      prv_show_export_status();
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
  if (queued) {
    // The trial is only queued here; it has not yet reached TRACE_END.
    prv_show_export_status();
    prv_schedule_export();
  } else {
    // Never display Ready after an allocation failure: the sample was lost.
    s_failed_export_trial_id = s_trial_id;
    APP_LOG(APP_LOG_LEVEL_ERROR, "TRACE_QUEUE_FAILED,%lu,%u",
            (unsigned long)s_trial_id, (unsigned)s_event_count);
    prv_show_export_status();
  }
}

void touch_recorder_select(void) {
  if (s_mode == MODE_MENU) {
    s_mode = s_menu_selection ? MODE_TRACE : MODE_TYPING;
    if (s_mode == MODE_TYPING) { s_typed_len = 0; s_typed[0] = 0; s_typing_id = 0; prv_show_typed(); }
    else prv_show_prompt();
    return;
  }
  if (s_mode == MODE_TYPING) { s_mode = MODE_MENU; prv_show_menu(); return; }
  if (s_mode == MODE_JOT) { jot_editor_select(); return; }
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
  // Starting a new trial acknowledges any earlier export allocation error.
  // The old trial cannot be recovered, but future successes can show Ready.
  s_failed_export_trial_id = 0;
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

void touch_recorder_back(void) {
  if (s_mode == MODE_JOT) {
    jot_editor_back();
  } else {
    // Diagnostic/trace modes retain their normal OS Back behavior.
    window_stack_pop(true);
  }
}

void touch_recorder_next_type(void) {
  if (s_mode == MODE_MENU) { s_menu_selection = 1 - s_menu_selection; prv_show_menu(); return; }
  if (s_mode == MODE_TYPING) return;
  if (s_mode == MODE_JOT) { jot_show_list(); return; }
  if (s_recording || !s_touch_enabled) return;
  s_trace_family = (s_trace_family + 1) % 4;
  s_trace_choice = 0;
  prv_show_prompt();
}

void touch_recorder_next_direction(void) {
  if (s_mode == MODE_MENU) { s_menu_selection = 1 - s_menu_selection; prv_show_menu(); return; }
  if (s_mode == MODE_TYPING) return;
  if (s_mode == MODE_JOT) { jot_show_list(); return; }
  if (s_recording || !s_touch_enabled) return;
  s_trace_choice = (s_trace_choice + 1) % s_trace_choices[s_trace_family];
  prv_show_prompt();
}

void touch_enter_harness(void) {
  if (s_mode != MODE_JOT || jot_list_is_open()) return;
  jot_save_now();
  s_mode = MODE_MENU;
  prv_show_menu();
}

void touch_init(void) {
  jot_init();
  s_touch_enabled = touch_service_is_enabled();
  if (!s_touch_enabled) {
    alert_set_text("Enable touchscreen in Settings to record traces.");
    return;
  }

  touch_service_subscribe(prv_touch_handler, NULL);
  jot_refresh_editor();
}

void touch_deinit(void) {
  jot_deinit();
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

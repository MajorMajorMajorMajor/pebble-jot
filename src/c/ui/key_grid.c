#include <pebble.h>

#include "keyboard/layout.h"

#include "ui/key_button.h"

#include "ui/key_grid.h"
#include "ui/key_feedback_curve.h"

// Warm paper shows through the inset key borders and rounded corners.
// These are drawing details only: KeyButton frames remain the same.
static void prv_draw_notebook_grid(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorFromRGB(249, 242, 219));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, GColorFromRGB(230, 220, 196));
  graphics_context_set_stroke_width(ctx, 1);
  for (int y = 18; y < bounds.size.h; y += 24) {
    graphics_draw_line(ctx, GPoint(0, y),
                       GPoint(bounds.size.w - 1, y));
  }
}

struct KeyGrid {  
  Layer *layer;  

  size_t button_count; // one button per key
  KeyButtonLayer **buttons;
  const Layout *layout;
  int pressed_button; // -1 = no touch in progress.
  int touch_index;
  uint8_t level, release_level;
  uint32_t phase_start_ms;
  uint32_t touch_start_ms;
  uint16_t release_hold_ms;
  bool releasing;
  AppTimer *pulse_timer; // One timer total; no per-key animation allocs.
};

static uint32_t prv_clock_ms(void) {
  time_t seconds;
  const uint16_t milliseconds = time_ms(&seconds, NULL);
  return (uint32_t)seconds * 1000u + milliseconds;
}

static void prv_pulse_tick(void *context);

static void prv_cancel_pulse(KeyGrid *grid) {
  if (!grid->pulse_timer) return;
  app_timer_cancel(grid->pulse_timer);
  grid->pulse_timer = NULL;
}

static void prv_schedule_pulse(KeyGrid *grid) {
  grid->pulse_timer = app_timer_register(
      KEY_FEEDBACK_FRAME_MS, prv_pulse_tick, grid);
  if (!grid->pulse_timer && grid->pressed_button >= 0) {
    // Animation failure never leaves a key stuck highlighted.
    grid->level = grid->releasing ? 0 : 255;
    key_button_set_feedback_level(
        grid->buttons[grid->pressed_button], grid->level);
    if (grid->releasing) grid->pressed_button = -1;
  }
}

static void prv_pulse_tick(void *context) {
  KeyGrid *grid = context;
  grid->pulse_timer = NULL;
  if (grid->pressed_button < 0) return;
  const uint32_t elapsed = prv_clock_ms() - grid->phase_start_ms;
  if (grid->releasing) {
    const uint16_t duration = key_feedback_release_ms(grid->touch_index);
    // Short taps get a minimum visible dwell rather than blinking off.
    const uint32_t fade_ms = elapsed < grid->release_hold_ms ?
                             0 : elapsed - grid->release_hold_ms;
    const uint8_t progress = key_feedback_ease(fade_ms, duration);
    grid->level = (uint16_t)grid->release_level * (255 - progress) / 255;
  } else {
    // Ease IN while held. Every stage is a deliberately different palette
    // colour; no intermediate computed RGB value can collapse to rest.
    const uint8_t progress = key_feedback_ease(elapsed,
                                               KEY_FEEDBACK_PRESS_MS);
    const uint8_t start = key_feedback_initial_level(grid->touch_index);
    grid->level = start +
       (uint16_t)(255 - start) * progress / 255;
  }
  key_button_set_feedback_level(
      grid->buttons[grid->pressed_button], grid->level);
  if (grid->releasing &&
      elapsed >= grid->release_hold_ms +
                 key_feedback_release_ms(grid->touch_index)) {
    grid->pressed_button = -1;
  } else if (!grid->releasing && elapsed >= KEY_FEEDBACK_PRESS_MS) {
    // Remain lit until liftoff without running an idle timer.
  } else {
    prv_schedule_pulse(grid);
  }
}

// create a key grid
KeyGrid *key_grid_create(const Layout *layout, GRect frame, const UIStyle *style) {
  Layer *layer;
  size_t button_count;
  KeyButtonLayer **buttons;  
  
  // Layer  
  layer = layer_create(frame);
  if(!layer) {
    return NULL;
  }
  layer_set_update_proc(layer, prv_draw_notebook_grid);

  // Create buttons
  button_count = layout->key_count;
  buttons = calloc(button_count, sizeof *buttons);
  if (!buttons) {
    layer_destroy(layer);
    return NULL;
  }

  size_t i;
  for (i = 0; i < button_count; i++) {
    const LayoutKeyPosition *pos = &layout->keys[i];

    // divide the frame size evenly into grid cells
    int x1 = frame.size.w * pos->col / layout->cols;
    int x2 = frame.size.w * (pos->col + pos->col_span) / layout->cols;

    int y1 = frame.size.h * pos->row / layout->rows;
    int y2 = frame.size.h * (pos->row + pos->row_span) / layout->rows;

    GRect button_frame = GRect(x1, y1, x2 - x1, y2 - y1);
    const Key* key = get_key(pos->key_id);

    buttons[i] = key_button_create(key, button_frame, style);
   
    // if a button failed to create, roll back the creation process
    if (!buttons[i]) {
      APP_LOG(APP_LOG_LEVEL_ERROR,  "Failed to create key button %u (key_id=%u)", i, (unsigned)pos->key_id);      
      for (size_t j = 0; j < i; j++) {
        if(buttons[j]) {
          key_button_destroy(buttons[j]);
          buttons[j] = NULL;
        }
      }

      free(buttons);
      layer_destroy(layer);
      return NULL;
    }
    
    // add the button to the base layer
    layer_add_child(layer, key_button_get_layer(buttons[i]));
  }

  // Construct new object
  KeyGrid *grid = malloc(sizeof *grid);
  if(!grid){
    for (i = 0; i < button_count; i++) {
      key_button_destroy(buttons[i]);
    }
    
    free(buttons);
    layer_destroy(layer);
    return NULL;
  }
  *grid = (KeyGrid){
    .layer = layer, 
    .button_count = button_count,
    .buttons = buttons,
    .layout = layout,
    .pressed_button = -1,
    .touch_index = -1
  };  
  
  return grid;
}

void key_grid_destroy(KeyGrid *grid) {
  KeyGrid *this = grid;
  if (!this) return;
  prv_cancel_pulse(this);

  // destroy the buttons
  size_t i;
  for (i = 0; i < this->button_count; i++) {

    if (this->buttons[i]) {
      key_button_destroy(this->buttons[i]);
      this->buttons[i] = NULL;
    }    
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Destroying layer at %p", this->layer);
  // destroy the base layer
  if (this->layer) {
      layer_destroy(this->layer);
      this->layer = NULL;
  }

  free(this->buttons);
  free(this);
}

Layer *key_grid_get_layer(KeyGrid *grid) {
  return grid->layer;
}

void key_grid_highlight_touch_key(KeyGrid *grid, int touch_index) {
  if (!grid || !grid->layout) return;
  // Recognizer order differs from the visual layout's last five keys.
  static const KeyId abc_keys[] = {
    KEY_ENGLISH_A, KEY_ENGLISH_N, KEY_ENGLISH_I,
    KEY_ENGLISH_H, KEY_ENGLISH_O, KEY_ENGLISH_R,
    KEY_ENGLISH_T, KEY_ENGLISH_E, KEY_ENGLISH_S,
    KEY_ENGLISH_SPACEBAR, KEY_ENGLISH_BACKSPACE, KEY_ENGLISH_RETURN,
    KEY_ENGLISH_123, KEY_ENGLISH_MENU
  };
  static const KeyId number_keys[] = {
    KEY_NUMBER_7, KEY_NUMBER_8, KEY_NUMBER_9,
    KEY_NUMBER_4, KEY_NUMBER_5, KEY_NUMBER_6,
    KEY_NUMBER_1, KEY_NUMBER_2, KEY_NUMBER_3,
    KEY_NUMBER_ZERO, KEY_ENGLISH_BACKSPACE, KEY_ENGLISH_RETURN,
    KEY_NUMBER_ABC, KEY_ENGLISH_MENU, KEY_NUMBER_SPACEBAR
  };
  const KeyId *touch_keys = grid->layout == get_layout(LAYOUT_ENGLISH_123) ?
                            number_keys : abc_keys;
  int target = -1;
  if (touch_index >= 0 &&
      touch_index < (grid->layout == get_layout(LAYOUT_ENGLISH_123) ? 15 : 14)) {
    const KeyId id = touch_keys[touch_index];
    for (size_t i = 0; i < grid->button_count; ++i) {
      if (grid->layout->keys[i].key_id == id) {
        target = (int)i;
        break;
      }
    }
  }
  prv_cancel_pulse(grid);
  if (grid->pressed_button >= 0) {
    key_button_set_feedback_level(grid->buttons[grid->pressed_button], 0);
  }
  grid->pressed_button = target;
  grid->touch_index = touch_index;
  grid->releasing = false;
  grid->phase_start_ms = prv_clock_ms();
  grid->touch_start_ms = grid->phase_start_ms;
  grid->level = target < 0 ? 0 : key_feedback_initial_level(touch_index);
  // Controls begin with the quieter sepia step; the approved letter and
  // Space feedback still starts at its original intensity.
  if (target >= 0) {
    key_button_set_feedback_level(grid->buttons[target], grid->level);
    prv_schedule_pulse(grid);
  }
}

void key_grid_release_touch_key(KeyGrid *grid) {
  if (!grid || grid->pressed_button < 0 || grid->releasing) return;
  prv_cancel_pulse(grid);
  const uint32_t now_ms = prv_clock_ms();
  const uint32_t held_ms = now_ms - grid->touch_start_ms;
  // Compute the press phase at liftoff, even if the 25ms timer has not
  // fired yet. Never rewind the highlight when beginning the fade.
  const uint8_t progress = key_feedback_ease(
      held_ms, KEY_FEEDBACK_PRESS_MS);
  const uint8_t start = key_feedback_initial_level(grid->touch_index);
  const uint8_t at_release = start +
      (uint16_t)(255 - start) * progress / 255;
  if (at_release > grid->level) grid->level = at_release;
  grid->release_level = grid->level;
  grid->release_hold_ms = held_ms < KEY_FEEDBACK_MIN_VISIBLE_MS ?
      KEY_FEEDBACK_MIN_VISIBLE_MS - held_ms : 0;
  grid->phase_start_ms = now_ms;
  grid->releasing = true;
  prv_schedule_pulse(grid);
}

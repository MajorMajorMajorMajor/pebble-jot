// Source-level checks for Pebble-only painting and event dispatch.
'use strict';
const fs = require('node:fs');
const assert = require('node:assert/strict');
const read = path => fs.readFileSync(path,'utf8');
const touch = read('src/c/ui/touch.c');
const window = read('src/c/ui/window.c');
const button = read('src/c/ui/key_button.c');
const grid = read('src/c/ui/key_grid.c');
const jot = read('src/c/jot/jot.c');
const primary = read('src/c/keyboard/primary_face.c');

// Only the ABC face R key owns the up/down mode controls.
assert.match(touch,/!keyboard_window_number_face_active\(\) &&\s*s_touch_key == 5 && gesture_index == 1/);
assert.match(touch,/!keyboard_window_number_face_active\(\) &&\s*s_touch_key == 5 && gesture_index == 5/);
assert.match(touch,/keyboard_window_cycle_shift\(\)/);
assert.match(touch,/keyboard_window_reset_shift\(\)/);
assert.match(touch,/jot_shift_prepare\(&mode, binding, uppercase\)/);
assert.match(touch,/else prv_jot_input_abc\(binding\)/);
assert.match(touch,/if \(numeric\) jot_input\(numeric\)/);
assert.match(touch,/s_touch_key == 13 && gesture_index == 0/);
assert.match(touch,/keyboard_window_cycle_flip\(gesture_index == 17 \? 1 : -1\)/);
assert.match(touch,/keyboard_window_toggle_number_face\(\)/);
assert.match(touch,/if \(repeated\) break/);

// No layer allocations on Shift. Both the central letter and every blue
// peripheral letter are painted in uppercase in either Shift state.
assert.match(button,/jot_shift_upper\(key_label->text\[0\]\)/);
assert.match(button,/graphics_draw_text\(ctx, text, text_font/);
assert.match(button,/jot_shift_active\(shift_mode\) \? jot_shift_upper\(symbol\) : symbol/);
assert.match(button,/void key_button_set_shift_mode/);
assert.match(button,/layer_mark_dirty\(button->base_layer\)/);
assert.match(button,/layer_mark_dirty\(button->label_layers\[i\]->layer\)/);
assert.match(button,/graphics_fill_triangle/);
assert.match(button,/mode == JOT_SHIFT_LOCKED/);
assert.match(button,/jot_shift_active\(mode\)/);
assert.match(grid,/key_button_set_shift_mode\(grid->buttons\[i\], mode\)/);
assert.match(window,/key_grid_set_shift_mode\(next, self->shift_mode\)/);
assert.match(window,/key_grid_set_shift_mode\(self->grid, mode\)/);
assert.match(jot,/keyboard_window_reset_shift\(\)/);
assert.match(primary,/'q', 'u', 'p', 'b', 'j', 'd', 'g', 'c'/);
console.log('R-key Shift dispatch, uppercase legends and lifecycle routing passed');

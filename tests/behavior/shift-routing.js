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
// R's up indicator has *three* distinct visible states without needing
// extra room: thin teal outline, double blue outline, filled blue.
// Their outer triangle footprint and R-down cancel glyph are identical.
assert.match(button,/const GColor teal = GColorFromRGB\(0, 170, 170\)/);
assert.match(button,/const GColor blue = GColorFromRGB\(0, 85, 170\)/);
assert.match(button,/mode == JOT_SHIFT_OFF \? teal : blue/);
assert.match(button,/if \(mode == JOT_SHIFT_LOCKED\) \{[\s\S]*?for \(int row = 0; row <= bottom - top; \+\+row\)/);
// Once is now a raster outline rather than two aliasing sloped strokes.
// Mirror the two 3px side runs exactly; keep the original 2px base
// and the original 13x8 silhouette. Off and lock are unchanged.
assert.match(button,/else if \(mode == JOT_SHIFT_ONCE\) \{/);
assert.match(button,/row <= 2 \|\| row >= bottom - top - 1/);
assert.match(button,/GPoint\(cx - half \+ 2, y\)/);
assert.match(button,/GPoint\(cx \+ half - 2, y\)/);
assert.match(button,/GPoint\(cx - half, bottom\)|GPoint\(cx - half_width, bottom\)/);
assert.match(button,/GPoint\(cx \+ half_width, bottom\)/);
// Rasterized left/right thickness and bounds are symmetrical at each row.
for(let row=0;row<=7;row++){
  const half=Math.floor(row*6/7);
  const left=[-half, row<=2||row>=6?half:-half+2];
  const right=[row<=2||row>=6?-half:half-2, half];
  assert.equal(left[0],-right[1]);
  assert.equal(left[1],-right[0]);
  assert.ok(left[0]>=-6 && right[1]<=6);
  assert.ok(left[1]>=left[0] && right[1]>=right[0]);
}
assert.doesNotMatch(button,/graphics_context_set_stroke_width\(ctx, 2\)/);
assert.doesNotMatch(button,/graphics_fill_triangle\(ctx/);
assert.doesNotMatch(button,/graphics_fill_rect\(ctx, GRect\(cx - 6, 13, 12, 3\)/);
assert.match(button,/if \(jot_shift_active\(mode\)\) \{[\s\S]*?graphics_context_set_stroke_color\(ctx, teal\)/);
assert.match(button,/for \(int row = 0; row < 8; \+\+row\)/);
assert.match(grid,/key_button_set_shift_mode\(grid->buttons\[i\], mode\)/);
assert.match(window,/key_grid_set_shift_mode\(next, self->shift_mode\)/);
assert.match(window,/key_grid_set_shift_mode\(self->grid, mode\)/);
assert.match(jot,/keyboard_window_reset_shift\(\)/);
assert.match(primary,/'q', 'u', 'p', 'b', 'j', 'd', 'g', 'c'/);
console.log('R-key Shift dispatch, uppercase legends and lifecycle routing passed');

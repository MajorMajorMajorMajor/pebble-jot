'use strict';
const fs = require('node:fs');
const assert = require('node:assert/strict');
const touch = fs.readFileSync('src/c/ui/touch.c','utf8');
const paint = fs.readFileSync('src/c/ui/key_button.c','utf8');
const jot = fs.readFileSync('src/c/jot/jot.c','utf8');
const store = fs.readFileSync('src/c/jot/store.c','utf8');
// A/NW exists only on the ABC face and takes priority over regular input.
assert.match(touch,/!keyboard_window_number_face_active\(\) &&\s*s_touch_key == 0 && gesture_index == 8/);
assert.match(touch,/jot_compose\(\);/);
assert.match(touch,/s_touch_key == 5 && gesture_index == 1/);
assert.match(touch,/s_touch_key == 13 && gesture_index == 0/);
assert.match(touch,/jot_number_face_binding\(s_touch_key, gesture_index\)/);
assert.match(jot,/jot_compose_at\(jot_store_text\(\), jot_store_cursor\(\), &edit\)/);
assert.match(jot,/jot_store_replace_before_cursor\(edit.codepoints_before_cursor,/);
assert.match(jot,/prv_schedule_autosave\(\)/);
assert.match(store,/s_length - removed \+ added >= sizeof s_text/);
assert.match(store,/memmove\(s_text \+ start \+ added, s_text \+ s_cursor,/);
assert.match(paint,/if \(index == 0\) \{/);
assert.match(paint,/JOT_LEGEND_ABC, 0, 0, bounds.size.w, bounds.size.h/);
const cap = paint.slice(paint.indexOf('if (index == 0) {'),
                        paint.indexOf('// The R key carries'));
assert.match(cap,/graphics_context_set_text_color\(ctx, GColorFromRGB\(255, 250, 239\)\)/);
assert.match(cap,/graphics_draw_text\(ctx, "C", font,/);
assert.match(cap,/JOT_LEGEND_ABC, 0, 0, bounds.size.w, bounds.size.h/);
// The "Before" design is a true corner inlay, flush to the A face with
// matching outer radius and an inner lower-right curve, not a square badge.
const bg = paint.slice(paint.indexOf('static void prv_draw_key_background'),
                       paint.indexOf('KeyButtonLayer *key_button_create'));
assert.match(bg,/if \(button->key == get_key\(KEY_ENGLISH_A\)\)/);
assert.match(bg,/GRect\(face.origin.x \+ 1, face.origin.y \+ 1,/);
assert.match(bg,/JOT_LEGEND_SLOT_W, JOT_LEGEND_SLOT_H/);
assert.match(bg,/style->key_corner_radius - 1/);
assert.match(bg,/GCornerTopLeft \| GCornerBottomRight/);
assert.ok(bg.indexOf('graphics_fill_rect(ctx, inlay_frame') <
          bg.indexOf('graphics_draw_round_rect(ctx, face'),
          'inlay must be painted under the unchanged outline and bevel');
assert.doesNotMatch(cap,/graphics_fill_rect\(ctx/);
assert.doesNotMatch(bg,/layer_create|malloc|realloc/);
console.log('A-NW Compose routing, atomic storage and compass label passed');

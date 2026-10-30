// Jot 1.8.0 Pocket Notebook theme: never trade input reliability for paint.
// Run: node tests/notebook-style.test.js
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');

const style = fs.readFileSync('src/c/ui/style.c', 'utf8');
const styleHeader = fs.readFileSync('src/c/ui/style.h', 'utf8');
const grid = fs.readFileSync('src/c/ui/key_grid.c', 'utf8');
const key = fs.readFileSync('src/c/ui/key_button.c', 'utf8');
const alert = fs.readFileSync('src/c/ui/alert.c', 'utf8');
const touch = fs.readFileSync('src/c/ui/touch.c', 'utf8');
const layout = fs.readFileSync('src/c/keyboard/layout.c', 'utf8');
const keyLabels = fs.readFileSync('src/c/keyboard/key.c', 'utf8');

assert.match(styleHeader, /JOT_EDITOR_CONTENT_HEIGHT\s*=\s*44/);
assert.match(style, /\.content_height_px\s*=\s*JOT_EDITOR_CONTENT_HEIGHT/);
assert.match(style, /\.key_corner_radius\s*=\s*6/);
assert.match(style, /\.key_background_color\s*=\s*GColorFromRGB\(255,\s*250,\s*239\)/);
assert.match(grid, /layer_set_update_proc\(layer,\s*prv_draw_notebook_grid\)/);
assert.match(grid, /frame\.size\.w\s*\*\s*pos->col\s*\/\s*layout->cols/);
assert.match(grid, /frame\.size\.h\s*\*\s*pos->row\s*\/\s*layout->rows/);
assert.match(layout, /\.rows\s*=\s*4/);
assert.match(layout, /\.cols\s*=\s*4/);
assert.match(layout, /\{KEY_ENGLISH_SPACEBAR,\s*3,\s*1,\s*1,\s*3\}/);
assert.match(touch, /const int top = JOT_EDITOR_CONTENT_HEIGHT/);
assert.match(touch, /s_header_touch_active\s*=\s*event->y\s*>=\s*0\s*&&\s*event->y\s*<\s*JOT_EDITOR_CONTENT_HEIGHT/);
assert.match(touch, /const bool tap\s*=\s*event->y\s*>=\s*0\s*&&\s*event->y\s*<\s*JOT_EDITOR_CONTENT_HEIGHT/);
assert.match(touch, /s_touch_key == 13 && gesture_index == 0/);

assert.match(key, /const GRect shadow = GRect\(2,\s*4,\s*w - 4,\s*h - 6\)/);
assert.match(key, /const GRect face = GRect\(2,\s*1,\s*w - 4,\s*h - 6\)/);
assert.match(key, /graphics_draw_round_rect\(ctx,\s*face,\s*style->key_corner_radius\)/);
assert.match(key, /prv_draw_control_icon\(ctx,\s*label->key,\s*bounds\)/);
for (const symbol of ['MENU', 'BACKSPACE', 'RETURN', 'SPACEBAR']) {
  assert.match(key, new RegExp('key == get_key\\(KEY_ENGLISH_' + symbol + '\\)'));
}
assert.match(key, /GColorFromRGB\(255,\s*209,\s*88\)/);
assert.match(keyLabels, /\[KEY_ENGLISH_MENU\][\s\S]*?\.text = "SAVE"/);
assert.match(keyLabels, /\[KEY_ENGLISH_123\][\s\S]*?\.text = "123"/);

assert.match(alert, /graphics_draw_round_rect\(ctx,/);
assert.match(alert, /GColorFromRGB\(185,\s*72,\s*56\)/);
assert.match(alert, /jot_caret_layout\(/);
assert.match(alert, /layer_set_clips\(s_root,\s*true\)/);
assert.match(alert, /s_caret_index = cursor/);
assert.doesNotMatch(alert, /EDITOR_PADDING_X\s*=\s*(?!3\b)\d+/);

console.log('Pocket Notebook visual styling and unchanged gesture geometry verified');

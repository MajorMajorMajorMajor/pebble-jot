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
assert.match(style, /\.key_background_color\s*=\s*GColorFromRGB\(255,\s*238,\s*190\)/);
assert.match(grid, /layer_set_update_proc\(layer,\s*prv_draw_notebook_grid\)/);
assert.match(grid, /frame\.size\.w\s*\*\s*physical_col\s*\/\s*layout->cols/);
assert.match(grid, /frame\.size\.h\s*\*\s*pos->row\s*\/\s*layout->rows/);
assert.match(grid, /jot_flip_visual_column\(right_hand, \(int\)pos->col\)/);
assert.match(layout, /\.rows\s*=\s*4/);
assert.match(layout, /\.cols\s*=\s*4/);
assert.match(layout, /\{KEY_ENGLISH_SPACEBAR,\s*3,\s*1,\s*1,\s*3\}/);
assert.match(touch, /const int top = jot_flip_bottom\(mode\) \? 0/);
assert.match(touch, /s_header_touch_active = event->y >= editor_top/);
assert.match(touch, /const bool tap = event->y >= editor_top/);
assert.match(touch, /s_touch_key == 13 && gesture_index == 0/);

assert.doesNotMatch(key, /const GRect shadow\s*=/);
assert.match(key, /const GRect face = GRect\(2,\s*1,\s*w - 4,\s*h - 3\)/);
assert.match(key, /static void prv_draw_key_bevel\(/);
assert.match(key, /prv_draw_key_bevel\(ctx, face, special, space\)/);
assert.match(key, /graphics_context_set_stroke_color\(ctx, highlight\)/);
assert.match(key, /graphics_context_set_stroke_color\(ctx, style->key_outline_color\)/);
assert.match(key, /graphics_context_set_stroke_width\(ctx, 1\)/);
assert.doesNotMatch(key, /graphics_context_set_stroke_width\(ctx, 2\)/);
assert.match(key, /GPoint\(x0, y1 - 6\), GPoint\(x0, y0 \+ 5\)/);
assert.match(key, /GPoint\(x0 \+ 4, y0\), GPoint\(x1 - 4, y0\)/);
assert.match(key, /GPoint\(x1 - 1, y0 \+ 3\), GPoint\(x1, y0 \+ 5\)/);
assert.match(key, /if \(!special\)/);
assert.match(key, /GPoint\(x0 \+ 4, y1\), GPoint\(x1 - 4, y1\)/);
assert.match(key, /GColorFromRGB\(255, 196, 0\)/);
assert.match(key, /\(h \/ 2\) - 2/);
assert.match(key, /graphics_draw_round_rect\(ctx,\s*face,\s*style->key_corner_radius\)/);
assert.match(key, /prv_draw_control_icon\(ctx,\s*label->key,\s*bounds,\s*label->owner->feedback_level\)/);
for (const symbol of ['MENU', 'BACKSPACE', 'RETURN', 'SPACEBAR']) {
  assert.match(key, new RegExp('key == get_key\\(KEY_ENGLISH_' + symbol + '\\)'));
}
assert.doesNotMatch(key, /GColorFromRGB\(255,\s*209,\s*88\)/);
assert.match(keyLabels, /\[KEY_ENGLISH_MENU\][\s\S]*?\.text = "SAVE"/);
assert.match(keyLabels, /\[KEY_ENGLISH_123\][\s\S]*?\.text = "123"/);

// Strong 2px filled ink rim, not the previous single stroked rectangle.
assert.match(alert, /graphics_fill_rect\(ctx, bounds, 5, GCornersAll\)/);
assert.match(alert, /graphics_fill_rect\(ctx, GRect\(2, 1, bounds\.size\.w - 4,/);
assert.match(alert, /GColorFromRGB\(231,\s*72,\s*58\)/);
assert.match(alert, /GColorFromRGB\(150,\s*172,\s*181\)/);
assert.match(alert, /EDITOR_MARGIN_RULE_X\s*=\s*10/);
assert.match(alert, /EDITOR_PADDING_LEFT\s*=\s*18/);
assert.match(alert, /EDITOR_PADDING_RIGHT\s*=\s*5/);
assert.match(alert, /snprintf\(s_text, sizeof s_text, "Write something\.\.\."\)/);
assert.match(alert, /GRect\(EDITOR_PADDING_LEFT, -scroll_y, width, height\)/);
assert.match(alert, /jot_caret_layout\(/);
assert.match(alert, /static GFont prv_font\(void\)\s*\{\s*return fonts_get_system_font\(FONT_KEY_GOTHIC_18_BOLD\);\s*\}/);
assert.match(alert, /text_layer_set_font\(s_textbox, font\)/);
assert.match(alert, /const GFont font = prv_font\(\)/);
assert.match(alert, /const GSize size = prv_measure\(s_display_text, font, width\)/);
assert.match(alert, /layer_set_clips\(s_root,\s*true\)/);
assert.match(alert, /s_caret_index = cursor/);
assert.doesNotMatch(alert, /EDITOR_PADDING_X\s*=/);

console.log('Pocket Notebook visual styling and unchanged gesture geometry verified');

// #29: palette-aware touch-in and touch-out, A fill-only styling.
const keyGrid = fs.readFileSync('src/c/ui/key_grid.c', 'utf8');
const keyButton = fs.readFileSync('src/c/ui/key_button.c', 'utf8');
const keyboardWindow = fs.readFileSync('src/c/ui/window.c', 'utf8');
const feedbackTouch = fs.readFileSync('src/c/ui/touch.c', 'utf8');
const feedbackCurve = fs.readFileSync('src/c/ui/key_feedback_curve.c', 'utf8');
assert.match(keyGrid, /void key_grid_highlight_touch_key\(/);
assert.match(keyGrid, /KEY_ENGLISH_SPACEBAR, KEY_ENGLISH_BACKSPACE, KEY_ENGLISH_RETURN/);
assert.match(keyGrid, /void key_grid_release_touch_key\(/);
assert.match(keyGrid, /app_timer_register\(\s*KEY_FEEDBACK_FRAME_MS, prv_pulse_tick, grid\)/);
assert.match(keyGrid, /key_feedback_ease\(elapsed,\s*KEY_FEEDBACK_PRESS_MS\)/);
assert.match(keyGrid, /KEY_FEEDBACK_MIN_VISIBLE_MS/);
assert.match(keyGrid, /key_feedback_initial_level\(touch_index\)/);
assert.match(keyGrid, /key_feedback_initial_level\(grid->touch_index\)/);
assert.match(keyGrid, /grid->release_hold_ms/);
assert.match(keyGrid, /prv_cancel_pulse\(this\)/);
assert.match(keyButton, /void key_button_set_feedback_level\(/);
assert.match(keyButton, /prv_feedback_fill\(special, space, level, style\)/);
assert.match(keyButton, /key_feedback_palette_step\(intensity\)/);
assert.match(keyButton, /GColorFromRGB\(255, 170, 170\)/);
assert.match(keyButton, /GColorFromRGB\(255, 170, 85\)/);
assert.match(keyButton, /GColorFromRGB\(170, 170, 85\)/);
assert.match(keyButton, /step == 1 \? GColorFromRGB\(85, 85, 0\)/);
assert.match(keyButton, /step == 2 \? GColorFromRGB\(170, 85, 0\)/);
assert.match(keyButton, /GColorFromRGB\(170, 85, 85\)/);
assert.doesNotMatch(keyButton, /GColorFromRGB\(170, 170, 170\)/);
assert.match(keyButton, /step == 2 \? GColorFromRGB\(255, 85, 0\)/);
assert.match(keyButton, /GColorFromRGB\(170, 85, 0\)/);
assert.doesNotMatch(keyButton, /level >= 128 \? GColorWhite : cream/);
assert.match(keyButton, /is_space \? charcoal : cream/);
assert.match(keyButton, /graphics_context_set_stroke_color\(ctx, style->key_outline_color\)/);
assert.match(keyButton, /prv_draw_key_bevel\(ctx, face, special, space\)/);
assert.doesNotMatch(keyButton, /prv_blend\(/);
assert.doesNotMatch(keyButton, /text_frame.origin.x \+= 1/);
assert.match(feedbackCurve, /return KEY_FEEDBACK_RELEASE_MS/);
assert.match(keyboardWindow, /void keyboard_window_highlight_touch_key\(/);
assert.match(keyboardWindow, /void keyboard_window_release_touch_key\(/);
assert.match(feedbackTouch, /keyboard_window_release_touch_key\(\)/);

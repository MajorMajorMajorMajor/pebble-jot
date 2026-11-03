'use strict';
// Visual source contract: no Pebble SDK needed to guard editor-only polish.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const src = fs.readFileSync('src/c/ui/alert.c','utf8');
const between = (begin, end) => {
  const a=src.indexOf(begin),b=src.indexOf(end,a+begin.length);
  assert(a>=0 && b>a);
  return src.slice(a,b);
};
const background = between('static void prv_background(', 'static void prv_draw_caret(');
const reflow = between('static void prv_reflow(', 'static void prv_status(');
const status = between('static void prv_status(', 'void alert_set_text(');
const editor = between('void alert_set_editor(', 'Layer *alert_ui_init(');

assert.match(background, /GPoint\(EDITOR_MARGIN_RULE_X, 1\)/);
assert.match(background, /GPoint\(EDITOR_MARGIN_RULE_X, bounds\.size\.h - 2\)/);
assert.doesNotMatch(background, /GColorFromRGB\(188, 161, 124\)/);
assert.doesNotMatch(background, /GPoint\(4, bounds\.size\.h - 4\)/);
assert.match(background, /if \(s_editor_mode && s_dirty_mark\)/);
assert.match(background, /const int x = 6, y = 8/);
assert.match(background, /GPoint\(x - 2, y - 2\)/);
assert.match(background, /GPoint\(x \+ 2, y \+ 2\)/);
assert.match(status, /s_editor_mode && dirty && !save_error && !full/);
assert.match(status, /layer_mark_dirty\(s_root\)/);
assert.match(status, /const char \*label = save_error \? "SAVE ERROR" :\s*full \? "FULL" : ""/);
assert.doesNotMatch(status, /dirty \? "\*" : ""/);
assert.match(reflow, /s_editor_mode && s_placeholder \? GColorFromRGB\(164, 151, 130\)/);
assert.match(reflow, /GColorFromRGB\(41, 40, 43\)/);
assert.match(reflow, /EDITOR_PADDING_LEFT/);
assert.match(editor, /s_placeholder = !\*note/);
assert.match(editor, /prv_status\(dirty, save_error, full\)/);
assert.match(src, /EDITOR_PADDING_LEFT = 18/);
assert.match(src, /EDITOR_MARGIN_RULE_X = 10/);
assert.match(src, /layer_set_clips\(s_root, true\)/);
console.log('Editor polish: margin rule, asterisk priority, placeholder and caret geometry passed');

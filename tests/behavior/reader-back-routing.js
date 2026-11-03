'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const source = p => fs.readFileSync(p, 'utf8');
const reader = source('src/c/jot/reader.c');
const touch = source('src/c/ui/touch.c');
const review = source('src/c/jot/reader_review.c');
const model = source('src/c/jot/reader_touch.c');
const section = (start, end) => {
  const a = reader.indexOf(start);
  const b = reader.indexOf(end, a + start.length);
  assert.ok(a !== -1 && b > a, 'Missing Reader section: ' + start);
  return reader.slice(a, b);
};
const back = section('void jot_reader_back(void)', 'void jot_reader_dismiss_peek(void)');
assert.match(back, /prv_cancel_button_repeat\(\)/);
assert.match(back, /touch_cancel_kinetic\(\)/);
assert.match(back, /if \(!jot_reader_is_open\(\)\) return/);
const modal = back.indexOf('jot_reader_review_back(&s_review)');
const home = back.indexOf('jot_reader_back_should_scroll(s_scroll)');
const exit = back.indexOf('window_stack_pop(true)');
assert.ok(modal >= 0 && modal < home && home < exit,
          'Delete, scroll-home, exit precedence must remain intact');
assert.match(back, /if \(jot_reader_back_should_scroll\(s_scroll\)\) \{\s*s_scroll = 0;\s*prv_update_scroll\(\);\s*return;/);
assert.match(review, /bool jot_reader_back_should_scroll\(int scroll_y\) \{\s*return scroll_y > 0;/);

const peek = section('void jot_reader_dismiss_peek(void)', 'static void prv_draw_back(');
assert.match(peek, /if \(!jot_reader_is_peek\(\)\) return/);
assert.match(peek, /prv_cancel_button_repeat\(\)/);
assert.match(peek, /touch_cancel_kinetic\(\)/);
assert.match(peek, /window_stack_pop\(true\)/);
assert.doesNotMatch(peek, /jot_reader_back\(/);

const select = section('static void prv_reader_select(', 'static void prv_reader_select_hold(');
assert.match(select, /if \(s_is_peek\) \{\s*jot_reader_dismiss_peek\(\);\s*return;/);
assert.match(touch, /if \(action == JOT_READER_TOUCH_BACK\) \{\s*jot_reader_back\(\);/);
assert.match(touch, /action == JOT_READER_TOUCH_DISMISS_PEEK\) \{\s*jot_reader_dismiss_peek\(\);/);
assert.match(model, /if \(prv_back_hit\(start_x, start_y, layout\) &&\s*prv_back_hit\(end_x, end_y, layout\)\)/);

const chrome = section('static void prv_draw_chrome(', '// Solid closed-state nub');
const paper = section('static void prv_draw_paper(', 'static void prv_update_scroll(');
assert.match(chrome, /GRect\(0, 0, width, READER_HEADER_H - 3\)/);
assert.match(paper, /GRect\(12, 0, width - 24, 28\)/);
assert.match(reader, /GRect\(0, READER_HEADER_H, width, paper_h\)/);
assert.match(reader, /GRect\(0, READER_META_H, width, view_h\)/);
assert.doesNotMatch(reader, /jot_reader_page_text_top|jot_reader_page_first_rule/,
                    'Continuous paper experiment must stay off main');
console.log('Reader Back scroll-home: modal priority, stable layout, quick Peek and input routing passed');

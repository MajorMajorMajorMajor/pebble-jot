'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const read = path => fs.readFileSync(path, 'utf8');
const reader = read('src/c/jot/reader.c');
const touch = read('src/c/jot/reader_touch.c');
const dispatch = read('src/c/ui/touch.c');
const page = read('src/c/jot/reader_page.c');

function inside(begin, end) {
  const a = reader.indexOf(begin), b = reader.indexOf(end, a + begin.length);
  assert(a >= 0 && b > a, 'Missing region: '+begin);
  return reader.slice(a, b);
}
const chrome = inside('static void prv_draw_chrome(', '// Solid closed-state nub');
assert.match(chrome, /const int header_y = -s_scroll/);
assert.match(chrome, /if \(jot_reader_page_visible_header\(s_scroll\) > 0\)/);
assert.match(chrome, /GRect\(0, header_y, width, READER_HEADER_H - 3\)/);
assert.match(chrome, /const int footer_y = bounds.size.h - READER_FOOTER_H/);
assert.doesNotMatch(chrome, /graphics_fill_rect\(ctx, bounds,/); // No opaque overlay on note.
const paper = inside('static void prv_draw_paper(', 'static void prv_update_scroll(');
assert.match(paper, /jot_reader_page_first_rule\(s_scroll\)/);
assert.match(paper, /const int meta_y = READER_HEADER_H - s_scroll/);
assert.match(paper, /GRect\(12, meta_y, width - 24, 28\)/);
assert.match(paper, /separator_y = meta_y \+ 29/);
const scrolled = inside('static void prv_update_scroll(', 'static void prv_measure_text(');
assert.match(scrolled, /jot_reader_page_text_top\(s_scroll\)/);
assert.match(scrolled, /layer_mark_dirty\(s_paper\)/);
assert.match(scrolled, /layer_mark_dirty\(s_chrome\)/);
const layout = inside('static void prv_reflow_page(', 'static void prv_update_rank(');
assert.match(layout, /layer_set_frame\(s_paper, GRect\(0, 0, width, page_height\)\)/);
assert.match(layout, /layer_set_frame\(s_viewport, GRect\(0, 0, width, page_height\)\)/);
const loaded = inside('static void prv_reader_load(', 'static void prv_reader_unload(');
assert.ok(loaded.indexOf('s_paper = layer_create(') <
          loaded.indexOf('s_chrome = layer_create('),
          'Opaque header must be drawn AFTER page and text');
assert.match(loaded, /const int page_h = jot_reader_page_view_height\(bounds.size.h\)/);
const back = inside('void jot_reader_back(void)', 'void jot_reader_dismiss_peek(void)');
assert.ok(back.indexOf('jot_reader_review_back(&s_review)') <
          back.indexOf('jot_reader_page_back_to_top(s_scroll)'),
          'Delete/ActionBar Back must take priority over page scroll');
assert.match(back,/s_scroll = 0;\s*prv_update_scroll\(\);\s*return;/);
assert.match(back,/window_stack_pop\(true\)/);
const peek = inside('void jot_reader_dismiss_peek(void)', 'static void prv_draw_back(');
assert.match(peek,/prv_cancel_button_repeat\(\)/);
assert.match(peek,/window_stack_pop\(true\)/);
assert.doesNotMatch(peek,/jot_reader_back\(/);
assert.match(dispatch,/action == JOT_READER_TOUCH_DISMISS_PEEK\) \{\s*jot_reader_dismiss_peek\(\)/);
assert.match(reader,/if \(s_is_peek\) \{\s*jot_reader_dismiss_peek\(\);\s*return;/);
assert.match(touch,/jot_reader_page_visible_header\(layout \? layout->scroll_y : 0\)/);
assert.match(touch,/jot_reader_page_visible_header\(layout->scroll_y\)/);
assert.match(touch,/y >= 0 && y < viewport_bottom/);
assert.match(page,/JOT_READER_HEADER_H \+ JOT_READER_META_H/);
console.log('Continuous paper: scrolled header, timestamp, ruled lines, clipping, Back/Peek and touch routes passed');

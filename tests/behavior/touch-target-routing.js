'use strict';
// Pebble-specific routing contracts complement the pure C hitbox tests.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const read = p => fs.readFileSync(p, 'utf8');
const jot = read('src/c/jot/jot.c');
const jotHeader = read('src/c/jot/jot.h');
const touch = read('src/c/ui/touch.c');
const reader = read('src/c/jot/reader.c');
const hit = read('src/c/jot/reader_touch.c');
const list = read('src/c/jot/list_touch.c');

assert.match(list, /x >= screen_width - JOT_LIST_TOUCH_NEW_RIGHT_REACH/);
assert.match(list, /y >= 0 && y < JOT_LIST_TOUCH_HEADER_H/);
assert.match(jot, /jot_list_touch_new_hit\(start_x, start_y, screen.size.w\) &&\s*jot_list_touch_new_hit\(end_x, end_y, screen.size.w\)/);
assert.match(jot, /start_rank >= 0 && start_rank == end_rank/);
assert.match(jotHeader, /void jot_list_tap\(int16_t start_x, int16_t start_y,\s*int16_t end_x, int16_t end_y\)/);
assert.match(touch, /jot_list_tap\(s_list_touch_x, s_list_touch_y,\s*event->x, event->y\)/);
assert.match(touch, /if \(tap && !s_list_touch_suppress_tap\)/);
assert.match(touch, /if \(abs_dx > 10 \|\| abs_dy > 10\) s_list_touch_moved = true/);

assert.match(hit, /layout->screen_width - JOT_READER_HEADER_BADGE_SPACE/);
assert.match(hit, /prv_back_hit\(start_x, start_y, layout\) &&\s*prv_back_hit\(end_x, end_y, layout\)/);
assert.match(hit, /prv_paper_hit\(start_x, start_y, layout\) &&\s*prv_paper_hit\(end_x, end_y, layout\)/);
assert.match(reader, /s_review.delete_mode \|\| s_review.armed\) return false/);
assert.match(touch, /s_reader_touch_blocked = jot_reader_action_overlay_hit/);
assert.match(touch, /const bool blocked = s_reader_touch_blocked \|\|/);
assert.match(touch, /if \(blocked \|\| s_reader_touch_suppress_tap\) return/);
assert.match(touch, /s_reader_touch_moved = true/);
assert.match(reader, /"TAP page  SEL menu"/);
console.log('Expanded touch targets preserve start/end safety, action bar and delete guards');

'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const s = fs.readFileSync('src/c/jot/reader.c','utf8');
const model = fs.readFileSync('src/c/jot/reader_repeat.c','utf8');
// Immediate raw-button press and release: no extra waiting on short clicks.
assert.match(s,/window_raw_click_subscribe\(BUTTON_ID_UP,\s*prv_reader_scroll_press,\s*prv_reader_scroll_release, NULL\)/);
assert.match(s,/window_raw_click_subscribe\(BUTTON_ID_DOWN,\s*prv_reader_scroll_press,\s*prv_reader_scroll_release, NULL\)/);
assert.match(s,/click_recognizer_get_button_id\(ref\)/);
assert.match(s,/jot_reader_repeat_advance\(&s_button_repeat, &s_scroll,\s*s_scroll_max, JOT_READER_REPEAT_CLICK_PX\)/);
assert.match(s,/app_timer_register\(\s*JOT_READER_REPEAT_DELAY_MS,\s*prv_reader_repeat_tick, NULL\)/);
assert.match(s,/app_timer_register\(\s*JOT_READER_REPEAT_INTERVAL_MS,\s*prv_reader_repeat_tick, NULL\)/);
assert.match(s,/jot_reader_repeat_advance\(&s_button_repeat, &s_scroll,\s*s_scroll_max, JOT_READER_REPEAT_STEP_PX\)/);
assert.match(s,/static void prv_reader_scroll_release\(/);
assert.match(s,/prv_cancel_button_repeat\(\)/);
// Native action bar must retain single clicks for note browsing, not repeat deletion/browse.
assert.match(s,/if \(s_review.open && !s_is_peek\) \{[\s\S]*?window_single_click_subscribe\(BUTTON_ID_UP, prv_reader_up\)/);
assert.match(s,/window_single_click_subscribe\(BUTTON_ID_SELECT, prv_reader_select\)/);
assert.match(s,/window_long_click_subscribe\(BUTTON_ID_SELECT, 800,\s*prv_reader_select_hold, NULL\)/);
// Leaving the reader, entering an overlay, or touching to drag cancels timers.
for (const fn of ['void jot_reader_back(void)',
                 'static void prv_reader_unload(Window *window)',
                 'void jot_reader_drag(int16_t finger_delta_y)',
                 'static void prv_reader_select(ClickRecognizerRef ref, void *context)',
                 'static void prv_reader_select_hold(ClickRecognizerRef ref, void *context)']) {
  const i = s.indexOf(fn);
  assert.ok(i>=0, 'missing '+fn);
  assert.ok(s.slice(i,i+260).includes('prv_cancel_button_repeat()'),fn);
}
assert.match(s,/if \(!jot_reader_is_open\(\) \|\| s_review.open \|\|\s*!s_button_repeat.direction\)/);
assert.match(model,/jot_reader_repeat_cancel\(state\)/);
console.log('Reader raw-button, deferred timer, lifecycle and action-bar safeguards passed');

// Structural checks for Jot's editor/list/reader navigation wiring.
// Pebble window transitions still need physical Emery verification.
'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');

const jot = fs.readFileSync('src/c/jot/jot.c', 'utf8');
const reader = fs.readFileSync('src/c/jot/reader.c', 'utf8');
const touch = fs.readFileSync('src/c/ui/touch.c', 'utf8');
const click = fs.readFileSync('src/c/ui/click.c', 'utf8');
const store = fs.readFileSync('src/c/jot/store.c', 'utf8');

function functionBody(src, marker) {
  const start = src.indexOf(marker);
  assert.ok(start !== -1, 'missing function: ' + marker);
  const firstBrace = src.indexOf('{', start);
  assert.ok(firstBrace !== -1);
  let depth = 0;
  for (let i = firstBrace; i < src.length; ++i) {
    if (src[i] === '{') ++depth;
    else if (src[i] === '}') {
      --depth;
      if (depth === 0) return src.slice(firstBrace + 1, i);
    }
  }
  throw new Error('Unbalanced source function ' + marker);
}

const fromReader = functionBody(jot, 'bool jot_reader_edit(int slot)');
assert.match(fromReader, /jot_store_open\(slot\)/);
assert.match(fromReader, /s_editing_slot\s*=\s*slot/);
assert.match(fromReader, /window_stack_remove\(s_list_window,\s*false\)/);

const finish = functionBody(jot, 'static void prv_finish_saved_edit(void)');
const flushAt = finish.indexOf('if (!prv_flush())');
const newAt = finish.indexOf('jot_store_new()');
const listAt = finish.indexOf('prv_push_list_at(rank)');
const readerAt = finish.indexOf('jot_reader_show(slot, rank, jot_store_count())');
assert.ok(flushAt >= 0 && newAt > flushAt && listAt > newAt &&
          readerAt > listAt, 'must commit before rebuilding reader');
assert.match(finish, /if \(!prv_flush\(\)\)\s*\{\s*jot_refresh_editor\(\);\s*return;/);
assert.match(finish, /if \(!jot_store_new\(\)\)/);
assert.match(finish, /s_editing_slot\s*=\s*-1/);
assert.match(finish, /s_storage_error\s*=\s*true;\s*prv_redraw_list\(\)/);
// The replacement reader is populated from storage, not the stale snapshot.
assert.match(functionBody(reader, 'bool jot_reader_show(int slot, int rank, int total)'),
             /jot_store_export\(slot,\s*s_note,/);

const editorSelect = functionBody(jot, 'void jot_editor_select(void)');
assert.match(editorSelect, /s_editing_slot\s*>=\s*0/);
assert.match(editorSelect, /prv_finish_saved_edit\(\)/);
assert.match(editorSelect, /if \(!prv_flush\(\)\)\s*\{\s*jot_refresh_editor\(\);\s*return;/);
assert.match(editorSelect, /jot_store_active_slot\(\)/);
assert.match(editorSelect, /prv_rank_of_slot\(active\)/);
assert.match(editorSelect, /if \(active >= 0 && !jot_store_new\(\)\)/);
assert.match(editorSelect, /prv_push_list_at\(rank\)/);
assert.doesNotMatch(editorSelect, /window_stack_pop\(/);

const editorBack = functionBody(jot, 'void jot_editor_back(void)');
assert.match(editorBack, /prv_finish_saved_edit\(\)/);
assert.match(editorBack, /jot_show_list\(\)/);

assert.match(functionBody(touch, 'void touch_recorder_select(void)'),
             /if \(s_mode == MODE_JOT\) \{\s*jot_editor_select\(\);/);
assert.match(functionBody(touch, 'void touch_recorder_back(void)'),
             /jot_editor_back\(\)/);
assert.match(click, /window_single_click_subscribe\(BUTTON_ID_BACK,\s*prv_back_click_handler\)/);
assert.match(functionBody(reader, 'static void prv_reader_select('),
             /jot_reader_edit\(s_slot\)/);
assert.match(functionBody(reader, 'static void prv_reader_back('),
             /window_stack_pop\(true\)/);


// A list-only Back must have a way out: at a nonzero rank go to the top;
// at the top, safely finish the app (rather than bouncing to the editor).
const listBack = functionBody(jot, 'static void prv_list_back(');
assert.match(listBack, /if \(s_confirm_delete\)/);
assert.match(listBack, /s_selected_rank\s*>\s*0/);
assert.match(listBack, /s_selected_rank\s*=\s*0/);
assert.match(listBack, /if \(prv_flush\(\)\) window_stack_pop_all\(true\)/);

// The list '+' returns to a fresh editor only after saving an existing
// draft, and ignores touch gestures outside the pill.
const listTap = functionBody(jot, 'void jot_list_tap(int16_t x, int16_t y)');
assert.match(listTap, /jot_list_is_open\(\)/);
assert.match(listTap, /s_confirm_delete\s*\|\|\s*jot_backup_active\(\)/);
assert.match(listTap, /x\s*<\s*width\s*-\s*101/);
assert.match(listTap, /if \(!prv_flush\(\)\)/);
assert.match(listTap, /if \(!jot_store_new\(\)\)/);
assert.match(listTap, /s_editing_slot\s*=\s*-1/);
assert.match(listTap, /window_stack_pop\(true\)/);

const touchHandler = functionBody(touch, 'static void prv_touch_handler(');
assert.match(touchHandler, /jot_list_is_open\(\)/);
assert.match(touchHandler, /s_list_touch_active/);
assert.match(touchHandler, /now_ms\s*-\s*s_list_touch_ms\s*<\s*700/);
assert.match(touchHandler, /if \(tap\) jot_list_tap\(event->x, event->y\)/);

assert.match(store, /int jot_store_active_slot\(void\)\s*\{\s*return s_active_slot;/);
assert.match(functionBody(jot, 'void jot_show_list(void)'),
             /if \(!prv_flush\(\)\)\s*\{\s*jot_refresh_editor\(\);\s*return;/);

console.log('Jot screen navigation wiring checks passed');

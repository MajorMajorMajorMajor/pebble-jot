// Guard the touch SAVE → committed reader workflow (#24).
// These checks verify ordering and routing in Pebble-dependent C source.
// The watch SDK build and actual window behavior still need Emery testing.
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');

const jot = fs.readFileSync('src/c/jot/jot.c', 'utf8');
const touch = fs.readFileSync('src/c/ui/touch.c', 'utf8');
const key = fs.readFileSync('src/c/keyboard/key.c', 'utf8');
const paint = fs.readFileSync('src/c/ui/key_button.c', 'utf8');
const reader = fs.readFileSync('src/c/jot/reader.c', 'utf8');
const store = fs.readFileSync('src/c/jot/store.c', 'utf8');
const header = fs.readFileSync('src/c/jot/jot.h', 'utf8');

function body(src, marker) {
  const at = src.indexOf(marker);
  assert.ok(at !== -1, 'Missing '+marker);
  const first = src.indexOf('{', at);
  assert.ok(first >= 0);
  let nesting = 0;
  for (let i = first; i < src.length; i++) {
    if (src[i] === '{') nesting++;
    else if (src[i] === '}' && --nesting === 0) return src.slice(first + 1, i);
  }
  throw Error('Unbalanced C function: ' + marker);
}

assert.match(header, /void jot_editor_touch_save\(void\)/);
const save = body(jot, 'void jot_editor_touch_save(void)');

// Existing-note editing uses the already tested reader-return behavior.
assert.match(save, /if \(s_editing_slot >= 0\)\s*\{\s*prv_finish_saved_edit\(\);\s*return;/);

// A new capture may not navigate if list/reader is already open or the
// list window could not be created.
assert.match(save, /!s_list_window\s*\|\|\s*jot_reader_is_open\(\)\s*\|\|\s*jot_list_is_open\(\)/);

// Empty editor must not open an unrelated saved note or allocate a new one.
assert.match(save, /if \(!jot_store_length\(\)\)\s*\{[^}]*return;/);

// Preserve text on a failed persistence write: no list or reader until
// commit. Use persistent slot and its current rank after flushing.
const commitAt = save.indexOf('if (!prv_flush())');
const slotAt = save.indexOf('jot_store_active_slot()');
const rankAt = save.indexOf('prv_rank_of_slot(slot)');
const listAt = save.indexOf('prv_push_list_at(rank)');
const readerAt = save.indexOf('jot_reader_show(slot, rank, jot_store_count())');
const clearAt = save.indexOf('if (!jot_store_new())');
assert.ok([commitAt,slotAt,rankAt,listAt,readerAt,clearAt].every(x => x !== -1));
assert.ok(commitAt < slotAt && slotAt < rankAt &&
          rankAt < listAt && listAt < readerAt && readerAt < clearAt,
          'Required order: flush → active slot → ranked list → reader → clear');
assert.match(save, /if \(!prv_flush\(\)\)\s*\{\s*jot_refresh_editor\(\);\s*return;/);
assert.match(save, /slot < 0 \|\| rank < 0/);

// Reader/list allocation failures must unwind the stack and show the
// existing text in the editor, without resetting its buffer.
assert.match(save, /!jot_list_is_open\(\)\s*\|\|\s*!s_list_layer\s*\|\|\s*!s_cards_layer/);
assert.match(save, /if \(jot_list_is_open\(\)\) window_stack_pop\(false\)/);
assert.match(save, /s_storage_error = true;\s*jot_refresh_editor\(\);\s*return;/);
assert.match(save, /s_editing_slot = -1/);
assert.match(save, /s_input_full = false/);
assert.match(save, /s_storage_error = false/);
assert.match(body(reader, 'bool jot_reader_show('), /jot_store_export\(slot,\s*s_note,/);

// The user-tested physical Select and Back still take their own routes.
assert.match(body(jot, 'void jot_editor_select(void)'), /prv_push_list_at\(rank\)/);
assert.doesNotMatch(body(jot, 'void jot_editor_select(void)'), /jot_editor_touch_save\(/);
assert.match(body(jot, 'void jot_editor_back(void)'), /jot_show_list\(\)/);
assert.match(body(touch, 'void touch_recorder_select(void)'), /jot_editor_select\(\)/);

// SAVE repurposes only the stationary menu-corner tap in Jot mode;
// original non-tap/typing mode mapping stays intact.
const stroke = body(touch, 'static void prv_update_stroke(');
assert.match(stroke, /if \(s_mode == MODE_JOT\)/);
assert.match(stroke, /s_touch_key == 13 && gesture_index == 0/);
assert.match(stroke, /jot_editor_touch_save\(\)/);
assert.doesNotMatch(stroke, /s_touch_key == 13 && gesture_index == 0\) jot_show_list\(\)/);
assert.match(stroke, /APP_LOG\(APP_LOG_LEVEL_INFO, "TYPE,/);
assert.match(key, /\[KEY_ENGLISH_MENU\][\s\S]*?"SAVE"/);
assert.doesNotMatch(key, /\[KEY_ENGLISH_MENU\][\s\S]*?"KB"/);
assert.match(paint, /label->key == get_key\(KEY_ENGLISH_MENU\)\s*\?\s*style->label_peripheral_font/);

// The store preserves an existing auto-saved slot (no duplicate record)
// and a failed persist write never marks the note clean.
assert.match(body(store, 'int jot_store_active_slot(void)'), /return s_active_slot/);
assert.match(body(store, 'bool jot_store_flush(void)'), /if \(!s_dirty\) return true/);
assert.match(body(store, 'bool jot_store_flush(void)'), /if \(written != \(int\)sizeof record\) return false/);
assert.match(body(store, 'bool jot_store_new(void)'), /if \(!jot_store_flush\(\)\) return false/);

console.log('Touch SAVE: commit ordering, rollback, key routing and reader stack verified');

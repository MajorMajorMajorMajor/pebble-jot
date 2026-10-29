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


// The NEW row is focusable directly above the first saved note.
const listUp = functionBody(jot, 'static void prv_list_up(');
const listDown = functionBody(jot, 'static void prv_list_down(');
const listSelect = functionBody(jot, 'static void prv_list_select(');
const listHold = functionBody(jot, 'static void prv_list_hold(');
assert.match(listUp, /if \(prv_restore_list_focus\(\)\) return;/);
assert.match(listUp, /jot_list_focus_up\(s_selected_rank\)/);
assert.match(listDown, /if \(prv_restore_list_focus\(\)\) return;/);
assert.match(listDown, /jot_list_focus_down\(s_selected_rank,\s*jot_store_count\(\)\)/);

const resume = functionBody(jot, 'static bool prv_restore_list_focus(void)');
assert.match(resume, /if \(s_list_focus_valid\) return false/);
assert.match(resume, /jot_list_touch_first_full_card\(/);
assert.match(resume, /s_selected_rank\s*=\s*jot_list_focus_initial\(first,\s*count\)/);
assert.match(resume, /s_list_focus_valid\s*=\s*true/);
assert.doesNotMatch(resume, /prv_follow_list_focus\(\)/); // First press preserves exact drag offset
assert.match(jot, /const bool selected\s*=\s*s_list_focus_valid\s*&&\s*rank\s*==\s*s_selected_rank/);

assert.match(listSelect, /if \(prv_restore_list_focus\(\)\) return;/);
assert.match(listSelect, /s_selected_rank\s*==\s*JOT_LIST_FOCUS_NEW/);
assert.match(listSelect, /prv_list_new_note\(\)/);
assert.match(listHold, /if \(prv_restore_list_focus\(\)\) return;/);
assert.match(listHold, /s_selected_rank\s*>=\s*0/);
assert.match(jot, /graphics_draw_text\(ctx,\s*"\+ NEW"/);
assert.match(jot, /new_selected\s*=\s*s_list_focus_valid\s*&&\s*s_selected_rank\s*==\s*JOT_LIST_FOCUS_NEW/);
assert.doesNotMatch(jot, /char usage\[16\]/); // No longer shows x/16.

const listBack = functionBody(jot, 'static void prv_list_back(');
assert.match(listBack, /if \(s_confirm_delete\)/);
assert.match(listBack, /if \(!s_list_focus_valid\)/);
assert.match(listBack, /prv_follow_list_focus\(\)/);
assert.match(listBack, /jot_list_focus_back\(s_selected_rank,\s*jot_store_count\(\)\)/);
assert.match(listBack, /if \(focus != JOT_LIST_FOCUS_EXIT\)/);
assert.match(listBack, /if \(prv_flush\(\)\) window_stack_pop_all\(true\)/);

// One safe note creation path serves both Select on NEW and a stationary tap.
// It preserves unsaved text on any failed save.
const listNew = functionBody(jot, 'static void prv_list_new_note(void)');
assert.match(listNew, /if \(!prv_flush\(\)\)/);
assert.match(listNew, /if \(!jot_store_new\(\)\)/);
assert.match(listNew, /s_editing_slot\s*=\s*-1/);
assert.match(listNew, /window_stack_pop\(true\)/);

const listTap = functionBody(jot, 'void jot_list_tap(int16_t x, int16_t y)');
assert.match(listTap, /jot_list_is_open\(\)/);
assert.match(listTap, /s_confirm_delete\s*\|\|\s*jot_backup_active\(\)/);
assert.match(listTap, /x\s*<\s*screen\.size\.w\s*-\s*LIST_NEW_PILL_X_RIGHT/);
assert.match(listTap, /jot_list_touch_hit_card\(/);
assert.match(listTap, /prv_open_list_note\(rank\)/);
assert.match(jot, /layer_set_clips\(s_cards_layer,\s*true\)/);
assert.match(jot, /layer_set_update_proc\(s_cards_layer,\s*prv_cards_draw\)/);
assert.match(jot, /rank \* row_h - s_list_scroll_px \+ JOT_LIST_CARD_Y_PAD/);
const listDrag = functionBody(jot, 'void jot_list_drag(int16_t delta_y)');
assert.match(listDrag, /jot_list_touch_clamp_scroll\(/);
assert.match(listDrag, /s_list_focus_valid\s*=\s*false/);
assert.match(listDrag, /const bool had_focus = s_list_focus_valid/);
assert.doesNotMatch(listDrag, /s_selected_rank\s*=/);
assert.match(listDrag, /if \(had_focus\) prv_redraw_list\(\)/);
assert.match(listDrag, /jot_backup_active\(\)/);
const openNote = functionBody(jot, 'static void prv_open_list_note(int rank)');
assert.match(openNote, /s_list_focus_valid\s*=\s*true/);
assert.match(openNote, /jot_reader_show\(slot,\s*rank,\s*count\)/);
assert.match(functionBody(jot, 'static void prv_list_select('),
             /prv_open_list_note\(s_selected_rank\)/);
assert.match(listTap, /prv_list_new_note\(\)/);

const touchHandler = functionBody(touch, 'static void prv_touch_handler(');
assert.match(touchHandler, /jot_list_is_open\(\)/);
assert.match(touchHandler, /s_list_touch_active/);
assert.match(touchHandler, /now_ms\s*-\s*s_list_touch_ms\s*<\s*700/);
assert.match(touchHandler, /if \(tap\) jot_list_tap\(event->x, event->y\)/);
assert.match(touchHandler, /s_list_touch_moved = true/);
assert.match(touchHandler, /s_list_touch_can_drag = jot_list_can_drag\(event->y\)/);
assert.match(touchHandler, /jot_list_drag\(\(int16_t\)dy\)/);
assert.match(touchHandler, /jot_list_drag\(\(int16_t\)\(event->y - s_list_touch_last_y\)\)/);
assert.match(touchHandler, /const bool tap = !s_list_touch_moved/);

assert.match(store, /int jot_store_active_slot\(void\)\s*\{\s*return s_active_slot;/);
assert.match(functionBody(jot, 'void jot_show_list(void)'),
             /if \(!prv_flush\(\)\)\s*\{\s*jot_refresh_editor\(\);\s*return;/);

console.log('Jot screen navigation wiring checks passed');

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
// The saved reader's physical Select and content tap share exactly one
// existing-note edit transition, with a preserved reader on failure.
assert.match(functionBody(reader, 'static void prv_reader_select('),
             /jot_reader_review_select\(&s_review\)/);
assert.match(functionBody(reader, 'static void prv_reader_select('),
             /JOT_REVIEW_EDIT\) jot_reader_edit_current\(\)/);
assert.match(functionBody(reader, 'static void prv_reader_select_hold('),
             /jot_reader_review_hold\(&s_review\)/);
assert.match(reader, /window_long_click_subscribe\(BUTTON_ID_SELECT,\s*800,/);
const sharedEdit = functionBody(reader, 'bool jot_reader_edit_current(void)');
assert.match(sharedEdit, /!jot_reader_is_open\(\)\s*\|\|\s*s_is_peek\s*\|\|\s*s_slot\s*<\s*0/);
assert.match(sharedEdit, /if \(!jot_reader_edit\(s_slot\)\)/);
assert.match(sharedEdit, /s_open_error\s*=\s*true/);
assert.match(sharedEdit, /window_stack_pop\(true\)/);
assert.ok(sharedEdit.indexOf('if (!jot_reader_edit(s_slot))') <
          sharedEdit.indexOf('window_stack_pop(true)'),
          'only successful saved-note opens may leave the reader');
const touchLayout = functionBody(reader, 'JotReaderTouchLayout jot_reader_touch_layout(void)');
assert.match(touchLayout, /layout\.screen_width\s*=\s*layer_get_bounds\(s_viewport\)\.size\.w/);
assert.match(touchLayout, /layout\.screen_height\s*=\s*bounds\.size\.h/);
assert.match(touchLayout, /layout\.text_height\s*=\s*s_text_height/);
assert.match(touchLayout, /layout\.scroll_y\s*=\s*s_scroll/);
assert.match(touchLayout, /layout\.scroll_max\s*=\s*s_scroll_max/);
const readerDrag = functionBody(reader, 'void jot_reader_drag(int16_t finger_delta_y)');
assert.match(readerDrag, /jot_reader_is_open\(\)/);
assert.match(readerDrag, /jot_reader_touch_scroll_after_drag\(/);
assert.match(readerDrag, /prv_update_scroll\(\)/);
const reflow = functionBody(reader, 'static void prv_reflow_page(void) {');
assert.match(reflow, /jot_reader_review_paper_width\(/);
assert.match(reflow, /layer_set_frame\(s_paper,/);
assert.match(reflow, /layer_set_frame\(s_viewport,/);
assert.match(reflow, /if \(layer_get_bounds\(s_viewport\)\.size\.w == width\) return/);
assert.match(reflow, /prv_measure_text\(\)/);
assert.match(functionBody(reader, 'static void prv_mark_review(void)'), /prv_reflow_page\(\)/);
assert.match(functionBody(reader, 'static void prv_reader_back('),
             /jot_reader_back\(\)/);
const reviewMove = functionBody(reader, 'static void prv_update_rank(int rank)');
assert.match(reviewMove, /malloc\(JOT_NOTE_TEXT_BYTES\)/);
assert.match(reviewMove, /jot_store_export\(/);
assert.match(reviewMove, /free\(s_note\)/);
assert.match(reviewMove, /s_scroll\s*=\s*0/);
assert.match(reviewMove, /jot_reader_focus_rank\(rank\)/);
const reviewDelete = functionBody(reader, 'static void prv_confirm_delete(void)');
assert.match(reviewDelete, /jot_store_delete\(s_slot\)/);
assert.match(reviewDelete, /jot_reader_focus_rank\(next_rank\)/);
assert.match(reviewDelete, /s_scroll\s*=\s*0/);
assert.match(reviewDelete, /window_stack_pop\(true\)/);
assert.match(functionBody(jot, 'void jot_reader_focus_rank(int rank)'),
             /s_selected_rank\s*=\s*jot_list_focus_initial\(rank, jot_store_count\(\)\)/);
assert.match(functionBody(reader, 'void jot_reader_back(void)'),
             /jot_reader_review_back\(&s_review\)/);
assert.match(functionBody(reader, 'void jot_reader_back(void)'),
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

// Both ordinary reader and draft peek display a contextual Back target,
// but the label reflects the actual destination. The reader's physical
// and touch routes use the same pop helper and never save note bytes.
const readerChrome = functionBody(reader, 'static void prv_draw_chrome(');
assert.match(readerChrome, /s_is_peek\s*\?\s*"EDIT"\s*:\s*"NOTES"/);
assert.doesNotMatch(readerChrome, /"CLOSE"/);
assert.match(readerChrome, /prv_draw_back\(ctx\)/);
assert.doesNotMatch(readerChrome, /prv_draw_pencil\(ctx\)/);
assert.match(functionBody(reader, 'void jot_reader_dismiss_peek(void)'),
             /if \(jot_reader_is_peek\(\)\) jot_reader_back\(\)/);
const readerTouch = fs.readFileSync('src/c/jot/reader_touch.c', 'utf8');
assert.match(readerTouch, /JOT_READER_TOUCH_BACK/);
assert.match(readerTouch, /JOT_READER_TOUCH_DISMISS_PEEK/);
assert.match(readerTouch, /JOT_READER_TOUCH_EDIT/);
assert.match(readerTouch, /prv_visible_text_hit\(start_x,\s*start_y,\s*layout\)/);
assert.match(readerTouch, /prv_visible_text_hit\(end_x,\s*end_y,\s*layout\)/);
assert.match(readerTouch, /duration_ms\s*>=\s*JOT_READER_TAP_MAX_MS/);

const touchHandler = functionBody(touch, 'static void prv_touch_handler(');
assert.match(touchHandler, /jot_list_is_open\(\)/);
assert.match(touchHandler, /jot_reader_touch_action\(/);
assert.match(touchHandler, /s_reader_touch_moved\s*=\s*true/);
assert.match(touchHandler, /jot_reader_touch_can_drag\(/);
assert.match(touchHandler, /jot_reader_action_overlay_hit\(/);
assert.match(reader, /action_bar_layer_create\(\)/);
assert.match(reader, /action_bar_layer_add_to_window\(s_action_bar,\s*s_window\)/);
assert.match(reader, /action_bar_layer_remove_from_window\(s_action_bar\)/);
const readerLoad = functionBody(reader, 'static void prv_reader_load(Window *window)');
const readerUnload = functionBody(reader, 'static void prv_reader_unload(Window *window)');
assert.match(readerLoad, /window_set_click_config_provider\(window, prv_reader_clicks\)/);
assert.match(readerUnload, /action_bar_layer_remove_from_window\(s_action_bar\)/);
assert.match(readerUnload, /window_set_click_config_provider\(window, prv_reader_clicks\)/);
const showReader = functionBody(reader, 'bool jot_reader_show(int slot, int rank, int total)');
assert.match(showReader, /window_set_click_config_provider\(s_window, prv_reader_clicks\);\s*window_stack_push\(s_window, true\)/);
const peekReader = functionBody(reader, 'bool jot_reader_peek(void)');
assert.match(peekReader, /window_set_click_config_provider\(s_window, prv_reader_clicks\);\s*window_stack_push\(s_window, true\)/);
assert.match(functionBody(reader, 'void jot_reader_back(void)'), /jot_reader_review_back\(&s_review\)/);
assert.match(reader, /action_bar_layer_set_click_config_provider\(s_action_bar,/);
assert.match(reader, /action_bar_layer_clear_icon\(s_action_bar,\s*BUTTON_ID_UP\)/);
assert.match(reader, /s_review\.armed\s*\?\s*s_icons\[ICON_CHECK\]/);
assert.match(reader, /if \(s_review\.armed\)\s*action_bar_layer_set_icon\(s_action_bar, BUTTON_ID_DOWN, s_icons\[ICON_X\]\)/);
const updateIcons = functionBody(reader, 'static void prv_mark_review(void)');
assert.match(updateIcons, /s_rank\s*<=\s*0/);
assert.match(updateIcons, /s_rank\s*>=\s*jot_store_count\(\)\s*-\s*1/);
assert.match(updateIcons, /action_bar_layer_clear_icon\(s_action_bar, BUTTON_ID_UP\)/);
assert.match(updateIcons, /action_bar_layer_clear_icon\(s_action_bar, BUTTON_ID_DOWN\)/);
assert.match(updateIcons, /action_bar_layer_clear_icon\(s_action_bar, BUTTON_ID_SELECT\)/);
assert.match(updateIcons, /layer_mark_dirty\(s_danger_badge\)/);
const badge = functionBody(reader, 'static void prv_draw_danger_badge(');
assert.match(badge, /!s_review\.open\s*\|\|\s*!s_review\.delete_mode/);
assert.match(badge, /GColorFromRGB\(204,\s*42,\s*46\)/);
assert.match(badge, /const int top = bounds\.size\.h \/ 3/);
assert.match(badge, /const int bottom = \(bounds\.size\.h \* 2\) \/ 3/);
assert.match(badge, /GRect\(0, top, bounds\.size\.w, bottom - top\)/);
assert.match(badge, /graphics_fill_rect\(ctx, focus, 0, GCornerNone\)/);
assert.match(badge, /graphics_draw_bitmap_in_rect\(ctx,/);
assert.match(functionBody(reader, 'static void prv_browse('), /s_review_message\s*=\s*NULL;/);
assert.doesNotMatch(reader, /"FIRST NOTE"|"LAST NOTE"/);
assert.match(reader, /GPoint\(bounds\.size\.w \+ 14, bounds\.size\.h \/ 2\), 20/);
assert.match(reader, /s_review\.armed\s*\?\s*"Confirm delete\?"\s*:/);
assert.match(reader, /action_bar_layer_set_background_color\(\s*s_action_bar, prv_action_bar_background\(\)\)/);
assert.match(reader, /static GColor prv_action_bar_background\(void\) \{ return prv_ink\(\); \}/);
assert.doesNotMatch(reader, /GColorFromRGBA\(/); // Firmware didn't blend alpha.
assert.doesNotMatch(reader, /action_bar_layer_set_background_color\(s_action_bar,\s*s_review\.delete_mode/);
assert.match(reader, /graphics_fill_circle\(ctx,\s*GPoint\(bounds\.size\.w \+ 14, bounds\.size\.h \/ 2\), 20\)/);
assert.doesNotMatch(reader, /graphics_fill_oval\(/); // Private PebbleOS API, not in public SDK.
assert.doesNotMatch(reader, /prv_draw_actions|prv_action_panel/);
assert.match(touchHandler, /if \(blocked \|\| s_reader_touch_suppress_tap\) return/);
assert.match(touchHandler, /s_reader_touch_can_drag\s*&&/);
assert.match(touchHandler, /abs_dy\s*>\s*JOT_READER_TAP_SLOP\s*&&\s*abs_dy\s*>=\s*abs_dx/);
assert.match(touchHandler, /jot_reader_drag\(\(int16_t\)dy\)/);
assert.match(touchHandler, /jot_reader_drag\(\(int16_t\)movement\)/);
assert.match(touchHandler, /JOT_READER_TOUCH_BACK/);
assert.match(touchHandler, /JOT_READER_TOUCH_DISMISS_PEEK/);
assert.match(touchHandler, /JOT_READER_TOUCH_EDIT/);
assert.match(touchHandler, /jot_reader_touch_layout\(\)/);
assert.match(touchHandler, /jot_reader_back\(\)/);
assert.match(touchHandler, /jot_reader_edit_current\(\)/);
assert.doesNotMatch(touchHandler, /jot_reader_edit\(s_slot\)/); // One shared route.
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

// Verify the editor's caret cannot silently become part of the note text.
// This checks source-level architectural invariants; actual pixel placement
// still needs the Pebble Time 2 hardware test described in issue #16.
'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');

const alert = fs.readFileSync('src/c/ui/alert.c', 'utf8');
const jot = fs.readFileSync('src/c/jot/jot.c', 'utf8');
const store = fs.readFileSync('src/c/jot/store.c', 'utf8');
const header = fs.readFileSync('src/c/ui/alert.h', 'utf8');

assert.match(alert, /static Layer \*s_caret_layer;/);
assert.match(alert, /s_caret_layer\s*=\s*layer_create\(/);
assert.match(alert, /layer_set_update_proc\(s_caret_layer,\s*prv_draw_caret\)/);
assert.match(alert, /graphics_context_set_stroke_width\(ctx,\s*1\)/);
assert.match(alert, /graphics_draw_line\(ctx,/);
assert.match(alert, /layer_add_child\(s_root,\s*s_caret_layer\)/);
assert.match(alert, /layer_set_hidden\(s_caret_layer,\s*!s_editor_mode\)/);
const layout = fs.readFileSync('src/c/ui/caret_layout.c', 'utf8');
assert.match(alert, /jot_caret_layout\(/);
assert.match(alert, /prv_caret_measure_width/);
assert.match(alert, /caret_top\s*\+=\s*caret\.row\s*\*\s*step/);
assert.match(alert, /s_caret_x\s*=\s*EDITOR_PADDING_LEFT\s*\+\s*caret\.x/);
assert.match(alert, /const int width\s*=\s*bounds\.size\.w\s*-\s*EDITOR_PADDING_LEFT\s*-\s*EDITOR_PADDING_RIGHT/);

// Positioning must follow the real start of a displayed row, not the
// longest suffix that happens to fit in an empty row.
assert.doesNotMatch(alert, /prv_visual_line_start/);
assert.match(layout, /const size_t word_start\s*=\s*at/);
assert.match(layout, /const size_t word_end\s*=\s*at/);
assert.match(layout, /prv_break_at\(&result, cursor, word_start\)/);
assert.match(layout, /prv_break_at\(&result, cursor, pos\)/);
assert.match(layout, /prv_break_at\(&result, cursor, line_start\)/);
assert.match(layout, /if \(text\[cursor\] == '\\n'\)/);

// Caret must reach below the baseline and leave space under the last line.
assert.match(alert, /s_caret_height\s*=\s*step\s*\+\s*2/);
assert.match(alert, /EDITOR_DESCENDER_GUARD\s*=\s*5/);
assert.match(alert, /caret_top\s*\+\s*s_caret_height\s*\+\s*EDITOR_DESCENDER_GUARD/);

// The TextLayer reads the full note directly from the existing store buffer.
// No second large static copy, premature truncation or inserted caret glyph.
assert.match(alert, /s_display_text\s*=\s*note/);
assert.match(alert, /text_layer_set_text\(s_textbox,\s*s_display_text\)/);
assert.doesNotMatch(alert, /snprintf\(s_text,\s*sizeof s_text,\s*"%s",\s*note\)/);
assert.doesNotMatch(alert, /s_text\s*\[\s*cursor\s*\]\s*=/);
assert.doesNotMatch(alert, /s_text\s*\[\s*s_caret_index\s*\]\s*=\s*'\|'/);
assert.doesNotMatch(alert, /memmove\(s_text\s*\+/);

// The insertion caret remains a view concern. Cursor index is from the
// note store; note storage and the complete keyboard layout aren't touched.
assert.match(jot, /alert_set_editor\(jot_store_text\(\),\s*jot_store_cursor\(\)/);
assert.match(header, /void alert_set_editor\(const char \*note, size_t cursor,/);
assert.match(store, /s_text\s*\+\s*offset,\s*chunk/); // Byte-perfect chunk persistence.

console.log('Editor caret overlay architecture checks passed');

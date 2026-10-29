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
assert.match(alert, /memcpy\(through_caret,\s*s_text,\s*s_caret_index\)/);

// Text displayed by a TextLayer must be copied unchanged from the note.
// No inserted pipe or other glyph may shift characters at the cursor.
assert.match(alert, /snprintf\(s_text,\s*sizeof s_text,\s*"%s",\s*note\)/);
assert.doesNotMatch(alert, /s_text\s*\[\s*cursor\s*\]\s*=/);
assert.doesNotMatch(alert, /s_text\s*\[\s*s_caret_index\s*\]\s*=\s*'\|'/);
assert.doesNotMatch(alert, /memmove\(s_text\s*\+/);

// The insertion caret remains a view concern. Cursor index is from the
// note store; note storage and the complete keyboard layout aren't touched.
assert.match(jot, /alert_set_editor\(jot_store_text\(\),\s*jot_store_cursor\(\)/);
assert.match(header, /void alert_set_editor\(const char \*note, size_t cursor,/);
assert.match(store, /memcpy\(record\.text,\s*s_text,\s*s_length\s*\+\s*1\)/);

console.log('Editor caret overlay architecture checks passed');

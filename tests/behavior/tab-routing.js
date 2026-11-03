'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const read = p => fs.readFileSync(p, 'utf8');
const abc = read('src/c/ui/touch.c');
const nums = read('src/c/keyboard/number_face.c');
const editor = read('src/c/ui/alert.c');
const reader = read('src/c/jot/reader.c');
const store = read('src/c/jot/store.c');

function row(source, comment) {
  const line = source.split('\n').find(s=>s.endsWith('// ' + comment));
  assert.ok(line, 'Missing canonical keyboard row ' + comment);
  const match = line.match(/^\s*\{(.*)\},\s*\/\//);
  assert.ok(match, 'Unrecognized canonical bindings');
  return JSON.parse('[' + match[1] + ']');
}
assert.equal(row(abc, 't')[4], '\t');   // Letter T, down-right.
assert.equal(row(nums, '1')[4], '\t');  // Numeric 1, down-right.
assert.equal(row(abc, 't')[0], 't');
assert.equal(row(nums, '1')[0], '1');
assert.ok(abc.includes('else prv_jot_input_abc(binding)'));
assert.ok(abc.includes('jot_number_face_binding(s_touch_key, gesture_index)'));
assert.ok(editor.includes('jot_tab_display_make(note, cursor, &next)'));
assert.ok(editor.includes('s_display_text = next.text'));
assert.ok(editor.includes('s_caret_index = next.cursor'));
assert.ok(editor.includes('jot_caret_layout('));
assert.ok(reader.includes('jot_tab_display_make(text, 0, &next)'));
assert.ok(reader.includes('text_layer_set_text(s_text_layer, next.text)'));
assert.ok(store.includes('const size_t added = strlen(binding)'));
console.log('Tab gesture routing and shared display/caret contracts passed');

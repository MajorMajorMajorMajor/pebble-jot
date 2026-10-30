// Prevent the Emery virtual-size regression in Jot 1.9.0.
// The SDK metadata limits .text + .data + .bss to 65535 bytes.
// Full note buffers live in the store or on a short-lived heap allocation.
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');

const read = name => fs.readFileSync(name, 'utf8');
const store = read('src/c/jot/store.c');
const storeHeader = read('src/c/jot/store.h');
const jot = read('src/c/jot/jot.c');
const alert = read('src/c/ui/alert.c');
const reader = read('src/c/jot/reader.c');
const backup = read('src/c/jot/backup.c');

assert.match(storeHeader, /JOT_NOTE_TEXT_BYTES\s*=\s*4096/);
assert.match(store, /static char s_text\[JOT_NOTE_TEXT_BYTES\]/);
assert.doesNotMatch(store, /static char s_read_text\[JOT_NOTE_TEXT_BYTES\]/);
assert.match(store, /static char \*s_read_text/);
assert.match(store, /s_read_text\s*=\s*malloc\(JOT_NOTE_TEXT_BYTES\)/);
assert.match(store, /void jot_store_deinit\(void\)/);
assert.match(jot, /jot_store_deinit\(\)/);

assert.doesNotMatch(alert, /static char s_text\[JOT_NOTE_TEXT_BYTES\]/);
assert.match(alert, /static const char \*s_display_text/);
assert.match(alert, /s_display_text\s*=\s*note/);
assert.match(alert, /static char s_measure_small\[128\]/);
assert.doesNotMatch(alert, /char piece\[JOT_NOTE_TEXT_BYTES\]/);
assert.match(alert, /realloc\(s_measure_heap, count \+ 1\)/);
assert.match(alert, /free\(s_measure_heap\)/);

for (const source of [reader, backup]) {
  assert.doesNotMatch(source, /static char s_note\[JOT_NOTE_TEXT_BYTES\]/);
  assert.match(source, /static char \*s_note/);
  assert.match(source, /s_note\s*=\s*malloc\(JOT_NOTE_TEXT_BYTES\)/);
  assert.match(source, /free\(s_note\)/);
}
assert.match(store, /uint32_t serial;\s*uint32_t timestamp;\s*uint32_t revision;\s*uint16_t length;\s*int8_t bank;/);
console.log('Emery RAM budget architecture guards passed');

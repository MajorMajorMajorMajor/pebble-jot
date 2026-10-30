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
const touch = read('src/c/ui/touch.c');

assert.match(storeHeader, /JOT_NOTE_TEXT_BYTES\s*=\s*4096/);
assert.match(store, /static char s_text\[JOT_NOTE_TEXT_BYTES\]/);
assert.match(store, /static NoteDirectory s_directory/);
assert.match(store, /static bool s_directory_valid/);
assert.match(store, /if \(s_directory_valid && !prv_directory_has\(i\)\) continue/);
assert.match(store, /if \(!prv_directory_set\(s_active_slot\)\) return false/);
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
// The diagnostic trace harness is rarely used, but its historical fixed
// arrays consumed > 15 KiB of every notebook build (.bss), even when idle.
// A 65,535-byte virtual image cannot afford always-on diagnostic buffers.
assert.doesNotMatch(touch, /static TraceEvent s_events\[TRACE_MAX_EVENTS\]/);
assert.doesNotMatch(touch, /static StrokeSummary s_summaries\[TRACE_MAX_STROKES\]/);
assert.match(touch, /static TraceEvent \*s_events/);
assert.match(touch, /static StrokeSummary \*s_summaries/);
assert.match(touch, /s_events\s*=\s*malloc\(TRACE_MAX_EVENTS\s*\*\s*sizeof \*s_events\)/);
assert.match(touch, /s_summaries\s*=\s*malloc\(TRACE_MAX_STROKES\s*\*\s*sizeof \*s_summaries\)/);
assert.match(touch, /if \(!s_events \|\| !s_summaries\)/);
assert.match(touch, /static void prv_release_capture_buffers\(void\)/);
assert.match(touch, /const bool queued = prv_queue_trial\(\);[\s\S]*?prv_release_capture_buffers\(\)/);
assert.match(touch, /void touch_deinit\(void\)[\s\S]*?prv_release_capture_buffers\(\)/);
console.log('Emery RAM budget architecture guards passed');

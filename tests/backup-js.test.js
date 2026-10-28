// Run with: node tests/backup-js.test.js
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');

const listeners = {};
const storage = {};
const acks = [];
let pageUrl = null;
const phone = {
  addEventListener(name, callback) { listeners[name] = callback; },
  sendAppMessage(data, success) { acks.push(data); if (success) success(); },
  openURL(url) { pageUrl = url; }
};
const localStorage = {
  getItem(key) { return storage[key] ?? null; },
  setItem(key, value) { storage[key] = value; }
};
const program = fs.readFileSync(
  path.join(__dirname, '../src/js/pebble-js-app.js'), 'utf8');
vm.runInNewContext(program, {
  Pebble: phone, localStorage, console,
  encodeURIComponent, decodeURIComponent
}, {filename: 'src/js/pebble-js-app.js'});

function message(kind, session, index, extras = {}) {
  listeners.appmessage({payload: {
    BackupKind: kind, BackupSession: session,
    BackupIndex: index, ...extras
  }});
}
function snapshot() {
  return JSON.parse(storage['jot.backup.v1']);
}

message(1, 42, -1, {BackupTotal: 2});
message(2, 42, 0, {BackupTimestamp: 1730000000, BackupText: 'Hi 📝 café'});
message(2, 42, 1, {BackupTimestamp: 1730000001, BackupText: 'line 1\nline 2 𝄞'});
assert.equal(storage['jot.backup.v1'], undefined, 'no early commit');
message(3, 42, 2);
assert.equal(snapshot().notes.length, 2);
assert.equal(snapshot().notes[0].text, 'Hi 📝 café');
assert.equal(snapshot().notes[1].text, 'line 1\nline 2 𝄞');
message(3, 42, 2);
assert.equal(acks.at(-1).BackupStatus, 1, 'repeated END must be acknowledged');

message(1, 43, -1, {BackupTotal: 2});
message(2, 43, 0, {BackupTimestamp: 1, BackupText: 'incomplete'});
assert.equal(snapshot().session, 42, 'partial snapshot must not replace old backup');

listeners.showConfiguration();
assert.ok(pageUrl.startsWith('https://majormajormajormajor.github.io/pebble-jot/export.html#'));
const restored = JSON.parse(Buffer.from(pageUrl.split('#')[1], 'base64').toString('utf8'));
assert.equal(restored.notes[1].text, 'line 1\nline 2 𝄞');

console.log('Jot backup JS tests passed');

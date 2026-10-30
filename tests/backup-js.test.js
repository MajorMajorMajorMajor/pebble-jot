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
const project = JSON.parse(fs.readFileSync(path.join(__dirname, '../package.json'), 'utf8'));
const buildScript = fs.readFileSync(path.join(__dirname, '../wscript'), 'utf8');
assert.equal(project.pebble.enableMultiJS, false,
  'Single-file companion must not require webpack');
assert.ok(buildScript.includes("src/js/**/*.js"),
  'The build must bundle the src/js companion');
assert.ok(fs.existsSync(path.join(__dirname, '../src/js/pebble-js-app.js')),
  'Legacy companion entry point is required');

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

// Multi-packet Unicode note, with a retransmitted middle packet.
message(1, 44, -1, {BackupTotal: 1});
message(2, 44, 0, {BackupPart: 0, BackupFinal: 0,
  BackupTimestamp: 200, BackupText: 'First 📝 '});
message(2, 44, 0, {BackupPart: 1, BackupFinal: 0,
  BackupTimestamp: 200, BackupText: 'second café '});
message(2, 44, 0, {BackupPart: 1, BackupFinal: 0,
  BackupTimestamp: 200, BackupText: 'second café '});
message(2, 44, 0, {BackupPart: 2, BackupFinal: 1,
  BackupTimestamp: 200, BackupText: 'third'});
message(3, 44, 1);
assert.equal(snapshot().notes[0].text, 'First 📝 second café third');
assert.equal(snapshot().notes[0].created, 200);
assert.equal(acks.at(-1).BackupPart, 0);

// Reject premature END, an unexpected packet part and a 257-note total.
message(1, 45, -1, {BackupTotal: 1});
message(2, 45, 0, {BackupPart: 1, BackupFinal: 1,
  BackupTimestamp: 201, BackupText: 'bad'});
assert.equal(acks.at(-1).BackupStatus, 0);
message(2, 45, 0, {BackupPart: 0, BackupFinal: 0,
  BackupTimestamp: 201, BackupText: 'unfinished'});
message(3, 45, 1);
assert.equal(acks.at(-1).BackupStatus, 0);
assert.equal(snapshot().session, 44);
message(1, 46, -1, {BackupTotal: 257});
assert.equal(acks.at(-1).BackupStatus, 0);
message(1, 47, -1, {BackupTotal: 256});
assert.equal(acks.at(-1).BackupStatus, 1);

message(1, 43, -1, {BackupTotal: 2});
message(2, 43, 0, {BackupTimestamp: 1, BackupText: 'incomplete'});
assert.equal(snapshot().session, 44, 'partial snapshot must not replace old backup');

listeners.showConfiguration();
assert.ok(pageUrl.startsWith('https://majormajormajormajor.github.io/pebble-jot/export.html#'));
const restored = JSON.parse(Buffer.from(pageUrl.split('#')[1], 'base64').toString('utf8'));
assert.equal(restored.notes[0].text, 'First 📝 second café third');

console.log('Jot backup JS tests passed');

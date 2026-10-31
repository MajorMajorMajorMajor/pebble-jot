// Saving notes off-watch: whole snapshots, multipart retransmissions,
// and rejection of incomplete or invalid transfers. No packaging checks.
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');

const listeners = {};
const saved = {};
const acknowledgements = [];
let openedUrl = null;
const phone = {
  addEventListener(name, listener) { listeners[name] = listener; },
  sendAppMessage(data, success) {
    acknowledgements.push(data);
    if (success) success();
  },
  openURL(url) { openedUrl = url; }
};
const storage = {
  getItem(key) { return saved[key] ?? null; },
  setItem(key, value) { saved[key] = value; }
};
const script = fs.readFileSync(
  path.join(__dirname, '../../src/js/pebble-js-app.js'), 'utf8');
vm.runInNewContext(script, {
  Pebble: phone, localStorage: storage, console,
  encodeURIComponent, decodeURIComponent
});

function packet(kind, session, index, extra = {}) {
  listeners.appmessage({payload: {
    BackupKind: kind, BackupSession: session,
    BackupIndex: index, ...extra
  }});
}
function snapshot() {
  return JSON.parse(saved['jot.backup.v1']);
}
function lastAck() {
  return acknowledgements.at(-1);
}

function complete_snapshot_is_atomic() {
  packet(1, 42, -1, {BackupTotal: 2});
  packet(2, 42, 0, {BackupTimestamp: 1730000000, BackupText: 'Hi 📝 café'});
  packet(2, 42, 1, {BackupTimestamp: 1730000001, BackupText: 'line 1\nline 2 𝄞'});
  assert.equal(saved['jot.backup.v1'], undefined); // No partial snapshot.
  packet(3, 42, 2);
  assert.equal(snapshot().notes.length, 2);
  assert.equal(snapshot().notes[0].text, 'Hi 📝 café');
  assert.equal(snapshot().notes[1].text, 'line 1\nline 2 𝄞');
  packet(3, 42, 2);
  assert.equal(lastAck().BackupStatus, 1); // Repeated END is idempotent.
}

function multipart_retransmission_is_idempotent() {
  packet(1, 44, -1, {BackupTotal: 1});
  packet(2, 44, 0, {BackupPart: 0, BackupFinal: 0,
    BackupTimestamp: 200, BackupText: 'First 📝 '});
  packet(2, 44, 0, {BackupPart: 1, BackupFinal: 0,
    BackupTimestamp: 200, BackupText: 'second café '});
  packet(2, 44, 0, {BackupPart: 1, BackupFinal: 0,
    BackupTimestamp: 200, BackupText: 'second café '});
  packet(2, 44, 0, {BackupPart: 2, BackupFinal: 1,
    BackupTimestamp: 200, BackupText: 'third'});
  packet(3, 44, 1);
  assert.equal(snapshot().notes[0].text, 'First 📝 second café third');
  assert.equal(snapshot().notes[0].created, 200);
}

function invalid_transfer_preserves_previous_backup() {
  packet(1, 45, -1, {BackupTotal: 1});
  packet(2, 45, 0, {BackupPart: 1, BackupFinal: 1,
    BackupTimestamp: 201, BackupText: 'out of order'});
  assert.equal(lastAck().BackupStatus, 0);
  packet(2, 45, 0, {BackupPart: 0, BackupFinal: 0,
    BackupTimestamp: 201, BackupText: 'unfinished'});
  packet(3, 45, 1);
  assert.equal(lastAck().BackupStatus, 0); // Missing final packet.
  assert.equal(snapshot().session, 44);

  packet(1, 46, -1, {BackupTotal: 257});
  assert.equal(lastAck().BackupStatus, 0); // Watch can store max 256.
  packet(1, 47, -1, {BackupTotal: 256});
  assert.equal(lastAck().BackupStatus, 1);

  packet(1, 43, -1, {BackupTotal: 2});
  packet(2, 43, 0, {BackupTimestamp: 1, BackupText: 'incomplete'});
  assert.equal(snapshot().session, 44); // Can't replace saved backup.
}

function exported_snapshot_is_readable() {
  listeners.showConfiguration();
  assert.ok(openedUrl.startsWith('https://majormajormajormajor.github.io/pebble-jot/export.html#'));
  const restored = JSON.parse(Buffer.from(openedUrl.split('#')[1], 'base64').toString('utf8'));
  assert.equal(restored.notes[0].text, 'First 📝 second café third');
}

complete_snapshot_is_atomic();
multipart_retransmission_is_idempotent();
invalid_transfer_preserves_previous_backup();
exported_snapshot_is_readable();
console.log('Backup behavior: transactions, retransmissions, and export passed');

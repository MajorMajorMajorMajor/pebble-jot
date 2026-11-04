// Ensure every C AppMessage field is defined by the package metadata.
// CI's clean SDK build can't catch a developer's stale incremental header,
// but it can distinguish a missing source declaration from that cache issue.
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const keys = new Set(JSON.parse(fs.readFileSync('package.json','utf8')).pebble.messageKeys);
const backup = fs.readFileSync('src/c/jot/backup.c','utf8');
const used = Array.from(backup.matchAll(/MESSAGE_KEY_([A-Za-z0-9_]+)/g), m => m[1]);
for (const key of used) {
  assert.ok(keys.has(key), 'Missing declared Pebble message key: ' + key);
}
for (const key of ['BackupSerial','BackupRevision']) {
  assert.ok(keys.has(key), 'Missing sync message key: ' + key);
  assert.ok(used.includes(key), 'Sync message key not used by watch: ' + key);
}
assert.equal(used.length > 0, true);
console.log('Watch AppMessage keys and package metadata agree');

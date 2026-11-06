// #76: verify background state transitions don't leak into the editor UI.
// Behavioral BEGIN/NOTE/END, forking, deletion and failed receipts are
// exercised separately in backup.js with the real PebbleKit-JS handler.
'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs');
const read=p=>fs.readFileSync(p,'utf8');
const backup=read('src/c/jot/backup.c');
const store=read('src/c/jot/store.c');
const pkjs=read('src/js/pebble-js-app.js');
assert.match(backup,/connection_service_subscribe/);
assert.match(backup,/pebble_app_connection_handler = prv_connection_changed/);
assert.match(backup,/connection_service_peek_pebble_app_connection/);
assert.match(backup,/s_auto_full = true; \/\/ Never trust a previous app lifetime/);
assert.match(backup,/prv_auto_schedule\(1200\)/);
assert.match(backup,/jot_backup_note_committed\(int slot\)/);
assert.match(backup,/jot_backup_note_deleted\(uint32_t serial\)/);
assert.match(backup,/s_mode == BACKUP_ONE/);
assert.match(backup,/MESSAGE_KEY_BackupMode/);
assert.match(backup,/s_mode == BACKUP_DELETED/);
assert.match(backup,/s_total = mode == BACKUP_ONE/);
assert.match(backup,/jot_store_dirty\(\)/);
assert.match(backup,/prv_begin\(true, BACKUP_FULL, -1, 0\)/);
assert.match(backup,/bool jot_backup_active\(void\) \{ return s_active && s_user_started; \}/);
assert.match(backup,/if \(s_active && !s_user_started\)/);
assert.match(store,/jot_backup_note_committed\(s_active_slot\)/);
assert.match(store,/jot_backup_note_deleted\(deleted_serial\)/);
assert.ok(store.indexOf('jot_backup_note_committed(s_active_slot)') >
  store.indexOf('index->revision = header.revision'),
  'commit event only after new bank header is durable');
assert.match(pkjs,/mergeWatchNotes\(pending.notes,session,pending.mode,pending.deletedSerial\)/);
assert.match(pkjs,/mode===2/);
assert.match(pkjs,/forkedFrom:id/);
assert.match(pkjs,/writeNotebook\(db\); \/\/ Atomic/);
console.log('Seamless sync: event/connection/transport/UI guards passed');

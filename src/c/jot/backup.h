#pragma once
#include <pebble.h>

// Manual, non-destructive backup to the PebbleKit JS companion on the phone.
// The backup is marked complete only after the phone reports a successful
// localStorage write of all notes.
void jot_backup_init(void);
void jot_backup_deinit(void);
void jot_backup_start(void);
bool jot_backup_active(void);
const char *jot_backup_status(void);
void jot_backup_cancel(void);
// Queue incremental sync after a successful persistent watch save/delete.
// These calls never transmit AppMessage packets inside storage operations.
void jot_backup_note_committed(int slot);
void jot_backup_note_deleted(uint32_t serial);

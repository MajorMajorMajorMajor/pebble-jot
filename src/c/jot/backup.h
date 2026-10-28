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

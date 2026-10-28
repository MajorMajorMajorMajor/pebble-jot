#pragma once
#include <pebble.h>

// Jot owns note persistence and the full-screen note browser; the editor uses
// the existing keyboard window and its content layer.
void jot_init(void);
void jot_deinit(void);
void jot_input(const char *binding);
void jot_new_note(void);
void jot_show_list(void);
bool jot_list_is_open(void);
void jot_refresh_editor(void);
void jot_save_now(void);
// Invalidate the note browser when backup status changes.
void jot_backup_ui_changed(void);

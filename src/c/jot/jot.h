#pragma once
#include <pebble.h>

// Jot owns note persistence and the full-screen note browser; the editor uses
// the existing keyboard window and its content layer.
void jot_init(void);
void jot_deinit(void);
void jot_input(const char *binding);
// Delete forward at the cursor without advancing it.
void jot_delete_forward(void);
void jot_new_note(void);
// Context-aware editor actions: save then show list or originating reader.
// On storage failure, keep editor and its draft visible.
void jot_editor_select(void);
void jot_editor_back(void);
void jot_show_list(void);
bool jot_list_is_open(void);
// Handle a short, stationary tap on the notes-list + pill.
// Tap an actual card to open the reader, or tap + NEW to start a note.
void jot_list_tap(int16_t x, int16_t y);
// Vertical drags are accepted only from within the clipped card viewport.
bool jot_list_can_drag(int16_t y);
void jot_list_drag(int16_t delta_y);
void jot_refresh_editor(void);
void jot_save_now(void);
// Switch from a read-only saved-note page back to the keyboard editor.
// Returns false on storage error; does not leave the reader in that case.
bool jot_reader_edit(int slot);

// Invalidate the note browser when backup status changes.
void jot_backup_ui_changed(void);

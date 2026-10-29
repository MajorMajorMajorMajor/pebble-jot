#pragma once
#include <pebble.h>

// A read-only, full-screen view of a saved note. The reader owns its Window
// and copies the selected note into a bounded buffer; it never writes notes.
bool jot_reader_init(void);
void jot_reader_deinit(void);
bool jot_reader_show(int slot, int rank, int total);
bool jot_reader_is_open(void);
// Same context-sensitive exit for physical Back and header touch Back.
// Saved reader -> notes list; draft peek -> unchanged editor.
void jot_reader_back(void);

// Preview the unsaved current editor buffer without changing storage.
// Back, Select, or a touchscreen tap returns to the same editor.
bool jot_reader_peek(void);
bool jot_reader_is_peek(void);
void jot_reader_dismiss_peek(void);

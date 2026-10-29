#pragma once
#include <pebble.h>

// A read-only, full-screen view of a saved note. The reader owns its Window
// and copies the selected note into a bounded buffer; it never writes notes.
bool jot_reader_init(void);
void jot_reader_deinit(void);
bool jot_reader_show(int slot, int rank, int total);
bool jot_reader_is_open(void);

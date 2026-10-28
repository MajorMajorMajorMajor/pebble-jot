#pragma once
#include <pebble.h>

// Pebble apps have 4 KiB of persistent storage, so keep notes bounded.
enum { JOT_NOTE_CAPACITY = 16, JOT_NOTE_TEXT_BYTES = 160 };

void jot_store_init(void);
const char *jot_store_text(void);
size_t jot_store_length(void);
bool jot_store_dirty(void);
int jot_store_count(void);

// UTF-8 characters (or \b to delete the final codepoint). Refuses to
// create a seventeenth note rather than silently overwriting existing text.
bool jot_store_input(const char *binding);
bool jot_store_flush(void);
bool jot_store_new(void);
bool jot_store_open(int slot);
bool jot_store_delete(int slot);

// Indices are newest-first. Returns -1 if rank is out of range.
int jot_store_slot_at(int rank);
// Reads a note preview and timestamp from persistence or the active buffer.
bool jot_store_preview(int slot, char *out, size_t capacity, time_t *created);
// Reads full note text and its timestamp, including the active buffer.
// Call jot_store_flush() before starting a consistent export.
bool jot_store_export(int slot, char *out, size_t capacity, time_t *created);

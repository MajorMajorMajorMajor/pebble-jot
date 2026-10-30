#pragma once
#include <pebble.h>

// Modern Emery firmware allows up to 1 MiB per app, but each persist value
// remains <=256 bytes. 256 slots share that pool; each note is <=4095 bytes.
enum { JOT_NOTE_CAPACITY = 256, JOT_NOTE_TEXT_BYTES = 4096 };

void jot_store_init(void);
const char *jot_store_text(void);
size_t jot_store_length(void);
bool jot_store_dirty(void);

// The insertion cursor is a UTF-8 byte boundary in the active note.
// It is transient editor state, never written to the on-watch record.
typedef enum {
  JOT_CURSOR_LEFT,
  JOT_CURSOR_RIGHT,
  JOT_CURSOR_UP,
  JOT_CURSOR_DOWN,
  JOT_CURSOR_HOME,
  JOT_CURSOR_END
} JotCursorMove;

size_t jot_store_cursor(void);
bool jot_store_set_cursor(size_t offset);
bool jot_store_move_cursor(JotCursorMove movement);
int jot_store_count(void);
// -1 for a fresh draft; otherwise the active saved record slot.
int jot_store_active_slot(void);

// UTF-8 text is inserted at the cursor; \\b removes the previous codepoint.
// Refuses to create a 257th note rather than silently overwriting text.
bool jot_store_input(const char *binding);
// Delete next UTF-8 codepoint; returns false when already at end.
bool jot_store_delete_forward(void);
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

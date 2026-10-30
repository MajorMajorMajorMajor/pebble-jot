#pragma once
#include <pebble.h>

// The editor must display the *entire* extended note, without truncation.
#include "jot/store.h"
enum { ALERT_MAX_LENGTH = JOT_NOTE_TEXT_BYTES };

// Diagnostic/trace display (retains its independent mode).
void alert_set_text(const char *new_text);

// Multiline note editor: renders actual newlines, wraps long lines and
// follows the insertion point at the end of the note. Never changes notes.
void alert_set_editor(const char *note, size_t cursor, bool dirty,
                      bool save_error, bool full);

// Owns the editor content area; its height is shared by style and touch input.
Layer *alert_ui_init(GRect frame);
void alert_ui_deinit(void);

#pragma once
#include <pebble.h>
#include "jot/reader_touch.h"

// A read-only, full-screen view of a saved note. The reader owns its Window
// and copies the selected note into a bounded buffer; it never writes notes.
bool jot_reader_init(void);
void jot_reader_deinit(void);
bool jot_reader_show(int slot, int rank, int total);
bool jot_reader_is_open(void);
// Physical/header Back returns a scrolled page to the top first, then
// exits on the next Back. The native action bar/delete flow still has
// priority. An ordinary Peek page tap/Select bypasses scroll-home and
// immediately returns to the unchanged draft.
void jot_reader_back(void);

// Current saved reader -> existing note editor. Physical Select and paper
// tap call the same transition; deleting/armed states cannot edit, and
// storage-open failure leaves the reader intact.
bool jot_reader_edit_current(void);
// Actual visible paper dimensions for the reader's touch hit testing.
JotReaderTouchLayout jot_reader_touch_layout(void);
// Direct finger scrolling inside the clipped text viewport. This never
// edits a saved note or dismisses a draft Peek.
void jot_reader_drag(int16_t finger_delta_y);
// Returns false when inertia is clamped or the reader has closed.
bool jot_reader_kinetic_step(int delta_scroll);
// Prevent a tap or drag behind the visible action bar / decorative nub.
bool jot_reader_action_overlay_hit(int16_t x, int16_t y);

// Preview the unsaved current editor buffer without changing storage.
// Back, Select, or a touchscreen tap returns to the same editor.
bool jot_reader_peek(void);
bool jot_reader_is_peek(void);
void jot_reader_dismiss_peek(void);

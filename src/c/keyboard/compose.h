#pragma once
#include <stddef.h>
#include <stdbool.h>

// A pure UTF-8 transform: no note/heap/storage mutation. All positions
// refer to byte boundaries in the existing note, not visual columns.
typedef struct {
  size_t codepoints_before_cursor; // 1 for cycle, 2 for a Compose pair.
  const char *replacement;          // Static UTF-8 catalog, never allocated.
} JotComposeEdit;

// First try the two immediately preceding Unicode codepoints as a pair.
// If no pair matches, cycle the single codepoint before the cursor.
// Does not care when/how those characters were entered, including pasted
// text, existing notes, cursor movement, or numeric gesture input.
bool jot_compose_at(const char *text, size_t cursor, JotComposeEdit *out);

// Canonical MessagEase character-pair table, exposed for regression coverage.
size_t jot_compose_pair_count(void);

#pragma once
#include <stdbool.h>
#include <stddef.h>

// Tabs remain U+0009 in note storage, backup and exports. This view is
// display-only, with 4-column stops reset by each explicit newline.
typedef struct {
  const char *text;
  size_t cursor; // Position in the expanded displayed UTF-8 bytes.
  char *owned;   // NULL on the allocation-free (no-tabs) path.
} JotTabDisplay;

bool jot_tab_display_make(const char *text, size_t cursor, JotTabDisplay *out);
void jot_tab_display_release(JotTabDisplay *view);

#include "ui/tab_display.h"
#include <stdlib.h>
#include <string.h>

enum { TAB_STOP_COLUMNS = 4 };

static bool prv_continuation(unsigned char c) {
  return (c & 0xC0) == 0x80;
}

bool jot_tab_display_make(const char *text, size_t cursor, JotTabDisplay *out) {
  if (!out) return false;
  *out = (JotTabDisplay){0};
  if (!text) return false;
  const size_t length = strlen(text);
  if (cursor > length) cursor = length;
  out->text = text;
  out->cursor = cursor;
  if (!strchr(text, '\t')) return true; // No memory or rendering overhead.

  // Count only the additional visible spaces, not a static 16K buffer.
  // A UTF-8 codepoint uses one tab column regardless of byte length.
  size_t extra = 0, column = 0;
  for (size_t at = 0; at < length; ++at) {
    const unsigned char ch = (unsigned char)text[at];
    if (ch == '\n') column = 0;
    else if (ch == '\t') {
      const size_t spaces = TAB_STOP_COLUMNS - column % TAB_STOP_COLUMNS;
      if (extra > (size_t)-1 - (spaces - 1)) return false;
      extra += spaces - 1;
      column += spaces;
    } else if (!prv_continuation(ch)) ++column;
  }
  if (extra > (size_t)-1 - length - 1) return false;
  char *expanded = malloc(length + extra + 1);
  if (!expanded) return false; // Original draft remains intact.

  column = 0;
  size_t dst = 0;
  for (size_t src = 0; src < length; ++src) {
    if (src == cursor) out->cursor = dst;
    const unsigned char ch = (unsigned char)text[src];
    if (ch == '\t') {
      const size_t spaces = TAB_STOP_COLUMNS - column % TAB_STOP_COLUMNS;
      memset(expanded + dst, ' ', spaces);
      dst += spaces;
      column += spaces;
    } else {
      expanded[dst++] = (char)ch;
      if (ch == '\n') column = 0;
      else if (!prv_continuation(ch)) ++column;
    }
  }
  if (cursor == length) out->cursor = dst;
  expanded[dst] = '\0';
  out->text = expanded;
  out->owned = expanded;
  return true;
}

void jot_tab_display_release(JotTabDisplay *view) {
  if (!view) return;
  free(view->owned);
  *view = (JotTabDisplay){0};
}

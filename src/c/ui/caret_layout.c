#include "ui/caret_layout.h"
#include <stdbool.h>
#include <string.h>

static bool prv_continuation(unsigned char byte) {
  return (byte & 0xC0) == 0x80;
}

static size_t prv_next(const char *text, size_t offset, size_t length) {
  if (offset >= length) return length;
  ++offset;
  while (offset < length && prv_continuation((unsigned char)text[offset]))
    ++offset;
  return offset;
}

static bool prv_space(char c) { return c == ' ' || c == '\t'; }

// Once a break is detected, it affects caret positions AT the beginning
// of that next visual row, too. Track all breaks, including ones discovered
// by looking ahead to the end of the word containing the cursor.
static void prv_break_at(JotCaretLayout *result, size_t cursor,
                         size_t break_offset) {
  if (break_offset <= cursor) {
    ++result->row;
    result->line_start = break_offset;
  }
}

JotCaretLayout jot_caret_layout(const char *text, size_t cursor,
                                int width, JotCaretMeasureWidth measure,
                                void *context) {
  JotCaretLayout result = {0, 0, 0};
  if (!text || !measure || width < 1) return result;
  const size_t length = strlen(text);
  if (cursor > length) cursor = length;

  size_t line_start = 0; // Current visual row used for word fitting.
  for (size_t at = 0; at < length;) {
    if (text[at] == '\n') {
      line_start = at + 1;
      prv_break_at(&result, cursor, line_start);
      at = line_start;
      continue;
    }
    if (prv_space(text[at])) {
      at = prv_next(text, at, length);
      continue;
    }

    // Group complete words before deciding whether they fit. Otherwise the
    // current character might appear to fit on a preceding line, although
    // Pebble wraps the *whole word* ahead of that insertion position.
    const size_t word_start = at;
    while (at < length && text[at] != '\n' && !prv_space(text[at]))
      at = prv_next(text, at, length);
    const size_t word_end = at;

    if (measure(text, line_start, word_end, context) <= width)
      continue;

    if (word_start > line_start) {
      // A complete word that does not fit after previous text begins a
      // new row. Any spaces immediately before it belong to the old row.
      line_start = word_start;
      prv_break_at(&result, cursor, word_start);
    }

    // A word itself may exceed the viewport. Pebble then breaks it across
    // rows; advance in UTF-8 codepoints, never in raw bytes.
    if (measure(text, line_start, word_end, context) > width) {
      for (size_t pos = word_start; pos < word_end;) {
        const size_t next = prv_next(text, pos, length);
        if (pos > line_start &&
            measure(text, line_start, next, context) > width) {
          line_start = pos;
          prv_break_at(&result, cursor, pos);
        }
        pos = next;
      }
    }
  }

  size_t visible_end = cursor;
  // Terminal spaces before a hard line break are invisible when Pebble
  // wraps. Keep the stored cursor offset; only adjust its drawn position.
  if (text[cursor] == '\n') {
    while (visible_end > result.line_start &&
           prv_space(text[visible_end - 1])) --visible_end;
  }
  if (visible_end > result.line_start)
    result.x = measure(text, result.line_start, visible_end, context);
  if (result.x < 0) result.x = 0;
  if (result.x > width) result.x = width;
  return result;
}

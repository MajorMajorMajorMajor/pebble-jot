#include "ui/caret_layout.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// Controlled fake font: all glyphs 6px wide except spaces/tabs at 3px.
// UTF-8 continuation bytes aren't additional glyphs.
static int measure(const char *text, size_t first, size_t last, void *unused) {
  (void)unused;
  int width = 0;
  for (size_t p = first; p < last; ++p) {
    unsigned char c = (unsigned char)text[p];
    if ((c & 0xC0) == 0x80) continue;
    width += (c == ' ' || c == '\t') ? 3 : 6;
  }
  return width;
}

static void expect(const char *text, size_t cursor, int width,
                   int row, int x, size_t line_start) {
  const JotCaretLayout p = jot_caret_layout(text, cursor, width, measure, NULL);
  if (p.row != row || p.x != x || p.line_start != line_start) {
    fprintf(stderr, "Bad caret: text='%s' offset=%zu viewport=%d -> "
            "(%d, %d) start=%zu; expected (%d, %d) start=%zu\n",
            text, cursor, width, p.row, p.x, p.line_start,
            row, x, line_start);
    assert(0);
  }
}

int main(void) {
  expect("", 0, 30, 0, 0, 0);
  expect("abcdef", 2, 60, 0, 12, 0);
  expect("abcdef", 6, 60, 0, 36, 0);
  expect("abc de", 2, 30, 0, 12, 0);
  expect("abc de", 4, 30, 1, 0, 4); // Whole next word wraps ahead of cursor.
  expect("abc de", 6, 30, 1, 12, 4);
  // Pebble's renderer permits a break BEFORE a literal hyphen. If a
  // compound is modelled as one unbreakable word, the text is painted
  // on different rows from the caret (the real "looking-glass" bug).
  const char *compound = "one looking-glass";
  const size_t dash = (size_t)(strchr(compound, '-') - compound);
  expect(compound, dash, 80, 1, 0, dash);
  expect(compound, dash + 1, 80, 1, 6, dash);
  expect(compound, strlen(compound), 80, 1, 36, dash);
  expect("one looking glass", strlen("one looking glass"),
         80, 1, 30, strlen("one looking "));
  // A compound that fits must remain on one row, including its hyphen.
  expect("one looking-glass", strlen("one looking-glass"), 110,
         0, 99, 0);
  // Exact reported sentence, including its quotation marks.
  const char *alice = "\"Alice through the looking-glass\"";
  const size_t alice_dash = (size_t)(strchr(alice, '-') - alice);
  expect(alice, alice_dash, 160, 1, 0, alice_dash);
  expect(alice, strlen(alice), 160, 1, 42, alice_dash);
  // Hyphen-only, repeated hyphens and UTF-8 text must never stall.
  expect("-", 1, 30, 0, 6, 0);
  expect("--", 2, 30, 0, 12, 0);
  const char *cafe = "a caf\xC3\xA9-\xC3\xA9t\xC3\xA9";
  const JotCaretLayout compound_utf8 = jot_caret_layout(
      cafe, strlen(cafe), 40, measure, NULL);
  assert(compound_utf8.row >= 1);
  assert(compound_utf8.x >= 0 && compound_utf8.x <= 40);
  expect("abc de\nf", 6, 30, 1, 12, 4); // BEFORE LF, stay on earlier row.
  expect("abc de\nf", 7, 30, 2, 0, 7);  // AFTER LF, start fresh row.
  expect("abcdef", 5, 30, 1, 0, 5);
  expect("abcdef", 6, 30, 1, 6, 5); // Long word splits at a codepoint.
  expect("abcdefg", 6, 30, 1, 6, 5);
  expect("abcdefg", 7, 30, 1, 12, 5);
  expect("abc   \ndef", 6, 30, 0, 18, 0); // Trailing spaces are hidden.
  expect("abc   \ndef", 7, 30, 1, 0, 7);
  expect("a\n\nb", 2, 30, 1, 0, 2); // Empty explicit line.
  expect("a\n\nb", 3, 30, 2, 0, 3);
  // Two multibyte codepoints wrap as indivisible characters.
  expect("abcd\xC3\xA9\xC3\xA9", 8, 30, 1, 6, 6);

  // Repro: a wrapped sentence's final short word must remain at left,
  // not inherit nearly a full row's width as the old backwards scan did.
  const char *long_line = "This is a long line that will wrap ar";
  const JotCaretLayout caret = jot_caret_layout(
      long_line, strlen(long_line), 170, measure, NULL);
  assert(caret.row > 0);
  assert(caret.x < 80);
  const char *previous_row = "Line 5 with text long enough to wrap around\nLine 6";
  const size_t line6 = (size_t)(strstr(previous_row, "Line 6") - previous_row);
  const JotCaretLayout before = jot_caret_layout(
      previous_row, line6 - 1, 150, measure, NULL);
  const JotCaretLayout after = jot_caret_layout(
      previous_row, line6, 150, measure, NULL);
  assert(after.row == before.row + 1);
  assert(after.x == 0);
  assert(before.x < 150);

  // Every UTF-8 codepoint boundary stays in the text viewport, and rows
  // only move forward across the note. Cursor movement never changes text.
  const char *sample = "one two three \xf0\x9f\x98\x8a\nverylongwordwithoutspaces";
  const char *original = sample;
  int previous = 0;
  const size_t total = strlen(sample);
  for (size_t pos = 0; pos <= total; ++pos) {
    if (pos < total && ((unsigned char)sample[pos] & 0xC0) == 0x80)
      continue;
    const JotCaretLayout point = jot_caret_layout(
        sample, pos, 40, measure, NULL);
    assert(point.row >= previous);
    assert(point.x >= 0 && point.x <= 40);
    assert(point.line_start <= pos);
    previous = point.row;
  }
  assert(sample == original);
  puts("Caret line-wrap positioning tests passed");
  return 0;
}

// Canonical Unicode pair composition, retroactive cursor navigation and
// cycle behavior, independent of Pebble SDK / persistent storage.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "keyboard/compose.h"

static void expect(const char *text, size_t cursor, int count,
                   const char *replacement) {
  JotComposeEdit edit = {99, "stale"};
  const bool found = jot_compose_at(text, cursor, &edit);
  assert(found == (replacement != NULL));
  if (replacement) {
    assert(edit.codepoints_before_cursor == (size_t)count);
    assert(strcmp(edit.replacement, replacement) == 0);
  } else {
    assert(edit.codepoints_before_cursor == 0);
    assert(edit.replacement == NULL);
  }
}

static void pairs_and_navigation(void) {
  assert(jot_compose_pair_count() > 200);
  expect("oe", 2, 2, "œ");
  expect("OE", 2, 2, "Œ");
  expect("ae", 2, 2, "æ");
  expect("a´", strlen("a´"), 2, "á");
  expect("s^", 2, 2, "š");
  expect("TM", 2, 2, "™");
  expect("al", 2, 2, "α");
  expect("pi", 2, 2, "π");
  expect("oo", 2, 2, "∞");
  expect("..", 2, 2, "…");
  expect("!?", 2, 2, "‽");

  // Both earlier chars are considered, even in the middle of a saved note.
  const char *middle = "alpha oe omega";
  expect(middle, 8, 2, "œ");
  const char *utf8 = "😀œ β";
  // "œ" was typed by another method; cycle from its value, not timing.
  expect(utf8, strlen("😀œ"), 1, "¤");

  // Empty or unrecognized prefixes are no-ops; invalid UTF-8 positions
  // and offsets outside the buffer are never used for replacement.
  expect("", 0, 0, NULL);
  expect("z", 0, 0, NULL);
  expect("😀", 1, 0, NULL); // Continuation byte.
  expect("abc", 99, 0, NULL);
  expect(NULL, 0, 0, NULL);
}

static void cycles(void) {
  static const char *const s_cycle[] = {
    "s", "š", "ş", "ŝ", "ß", "∫", "s"
  };
  for (unsigned i = 0; i < 6; ++i)
    expect(s_cycle[i], strlen(s_cycle[i]), 1, s_cycle[i+1]);
  expect("S", 1, 1, "Š");
  expect("Š", strlen("Š"), 1, "Ş");
  expect("Ş", strlen("Ş"), 1, "Ŝ");
  expect("Ŝ", strlen("Ŝ"), 1, "S");
  // The raw math integral entered from numeric 1 must join the same loop.
  expect("∫", strlen("∫"), 1, "s");
  expect("e", 1, 1, "è");
  expect("è", strlen("è"), 1, "é");
  expect("é", strlen("é"), 1, "ê");
  // Punctuation pairs take precedence; the fallback still works on Unicode.
  expect("x", 1, 1, "×");
}

int main(void) {
  pairs_and_navigation();
  cycles();
  puts("Compose: canonical pairs, retroactive cursor and Unicode cycles passed");
  return 0;
}

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ui/tab_display.h"
#include "ui/caret_layout.h"

static void expect(const char *src, size_t cursor, const char *want, size_t at) {
  JotTabDisplay d = {0};
  assert(jot_tab_display_make(src, cursor, &d));
  assert(strcmp(d.text, want) == 0);
  assert(d.cursor == at);
  jot_tab_display_release(&d);
  assert(!d.text && !d.owned);
}
static int measure(const char *s, size_t first, size_t last, void *ctx) {
  (void)ctx;
  int width = 0;
  for (size_t at = first; at < last; ++at)
    if (((unsigned char)s[at] & 0xC0) != 0x80) width += 6;
  return width;
}
int main(void) {
  JotTabDisplay view = {0};
  const char *plain = "café";
  assert(jot_tab_display_make(plain, 99, &view));
  assert(view.text == plain && !view.owned);
  assert(view.cursor == strlen(plain));
  jot_tab_display_release(&view);
  assert(!jot_tab_display_make(NULL, 0, &view));
  assert(!jot_tab_display_make("abc", 1, NULL));
  expect("", 0, "", 0);
  expect("\t", 0, "    ", 0);
  expect("\t", 1, "    ", 4);
  expect("a\tb", 0, "a   b", 0);
  expect("a\tb", 1, "a   b", 1);
  expect("a\tb", 2, "a   b", 4);
  expect("a\tb", 3, "a   b", 5);
  expect("abc\tZ", 4, "abc Z", 4);
  expect("abcd\tZ", 5, "abcd    Z", 8);
  expect("abcde\tX", 6, "abcde   X", 8);
  expect("\t\t", 2, "        ", 8);
  expect("abc\n\tX", 5, "abc\n    X", 8);
  expect("abc\t\ndef\tY", strlen("abc\t\ndef\tY"),
         "abc \ndef Y", strlen("abc \ndef Y"));
  expect("😀\té", strlen("😀\t"), "😀   é", strlen("😀   "));
  assert(jot_tab_display_make("one\tword", 4, &view));
  JotCaretLayout caret = jot_caret_layout(view.text, view.cursor,
                                           90, measure, NULL);
  assert(caret.row == 0 && caret.x == 24);
  jot_tab_display_release(&view);
  assert(jot_tab_display_make("a\tword", 2, &view));
  caret = jot_caret_layout(view.text, view.cursor, 30, measure, NULL);
  assert(caret.row == 1 && caret.x == 0 && caret.line_start == 4);
  jot_tab_display_release(&view);
  puts("Tabs: UTF-8, stops, caret positions and wrapping passed");
  return 0;
}

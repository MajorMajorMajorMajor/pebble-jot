// Run: cc -std=c11 -Wall -Wextra -Werror -I tests/fake -I src/c
//          tests/store-test.c src/c/jot/store.c -o /tmp/jot-store-test
//       /tmp/jot-store-test
#include <assert.h>
#include <stdio.h>
#include <pebble.h>
#include "jot/store.h"

#define STORAGE_KEY_FIRST 5000u
#define SLOT_CAPACITY 256u
#define RECORD_KEYS (2u * (JOT_NOTE_TEXT_BYTES / SLOT_CAPACITY + 1u))
#define LEGACY_KEY_FIRST 4100u

typedef struct {
  bool exists;
  size_t size;
  uint8_t bytes[SLOT_CAPACITY];
} FakeEntry;

static FakeEntry s_entries[JOT_NOTE_CAPACITY * RECORD_KEYS];
static FakeEntry s_legacy[16];
static bool s_fail_next_delete;
static bool s_fail_next_write;

static FakeEntry *prv_entry(uint32_t key) {
  if (key >= LEGACY_KEY_FIRST && key < LEGACY_KEY_FIRST + 16)
    return &s_legacy[key - LEGACY_KEY_FIRST];
  if (key < STORAGE_KEY_FIRST ||
      key >= STORAGE_KEY_FIRST + JOT_NOTE_CAPACITY * RECORD_KEYS) return NULL;
  return &s_entries[key - STORAGE_KEY_FIRST];
}

int persist_read_data(uint32_t key, void *buffer, size_t buffer_size) {
  FakeEntry *entry = prv_entry(key);
  if (!entry || !entry->exists) return E_DOES_NOT_EXIST;
  if (buffer_size < entry->size) return E_ERROR;
  memcpy(buffer, entry->bytes, entry->size);
  return (int)entry->size;
}

int persist_write_data(uint32_t key, const void *data, size_t size) {
  if (s_fail_next_write) {
    s_fail_next_write = false;
    return E_ERROR;
  }
  FakeEntry *entry = prv_entry(key);
  if (!entry || !data || size > SLOT_CAPACITY) return E_ERROR;
  memcpy(entry->bytes, data, size);
  entry->size = size;
  entry->exists = true;
  return (int)size;
}

status_t persist_delete(uint32_t key) {
  FakeEntry *entry = prv_entry(key);
  if (s_fail_next_delete) {
    s_fail_next_delete = false;
    return E_ERROR;
  }
  if (!entry || !entry->exists) return E_DOES_NOT_EXIST;
  entry->exists = false;
  entry->size = 0;
  // Official Pebble SDK contract: success is S_TRUE, not S_SUCCESS.
  return S_TRUE;
}

static bool has_slot_header(int slot) {
  const size_t first = (size_t)slot * RECORD_KEYS;
  return s_entries[first].exists || s_entries[first + RECORD_KEYS / 2].exists;
}

static void expect_note(int slot, const char *expected) {
  char result[JOT_NOTE_TEXT_BYTES];
  assert(jot_store_export(slot, result, sizeof result, NULL));
  assert(strcmp(result, expected) == 0);
}

int main(void) {
  jot_store_init();
  assert(jot_store_count() == 0);
  assert(jot_store_input("first"));
  assert(jot_store_flush());
  const int first = jot_store_slot_at(0);
  assert(first >= 0);

  assert(jot_store_new());
  assert(jot_store_input("second"));
  assert(jot_store_flush());
  const int second = jot_store_slot_at(0);
  assert(second >= 0 && second != first);
  assert(jot_store_count() == 2);

  // Deleting the active, freshly created note must update RAM and flash.
  assert(jot_store_delete(second));
  assert(jot_store_count() == 1);
  assert(jot_store_slot_at(0) == first);
  assert(jot_store_slot_at(1) == -1);
  assert(!has_slot_header(second));

  // The user can immediately enter a new note after that deletion.
  assert(jot_store_input("third"));
  assert(jot_store_flush());
  const int third = jot_store_slot_at(0);
  assert(third == second);  // Freed slot was recycled.
  assert(jot_store_count() == 2);
  expect_note(third, "third");
  expect_note(first, "first");

  // Relaunch should not resurrect ghosts or drop the remaining notes.
  jot_store_init();
  assert(jot_store_count() == 2);
  expect_note(jot_store_slot_at(0), "third");
  expect_note(jot_store_slot_at(1), "first");

  assert(jot_store_open(third));
  assert(jot_store_delete(first)); // Delete an older, non-active note.
  assert(jot_store_count() == 1);
  assert(jot_store_input("!"));
  assert(jot_store_flush());
  expect_note(third, "third!");

  // A genuine storage failure must leave the index unchanged.
  s_fail_next_delete = true;
  assert(!jot_store_delete(third));
  assert(jot_store_count() == 1);
  assert(has_slot_header(third));
  expect_note(third, "third!");

  assert(jot_store_delete(third));
  assert(jot_store_count() == 0);
  assert(jot_store_slot_at(0) == -1);
  assert(jot_store_input("fresh after delete"));
  assert(jot_store_flush());
  assert(jot_store_count() == 1);
  expect_note(jot_store_slot_at(0), "fresh after delete");

  // Multiline notes are represented by literal LF bytes. Backspace must
  // remove an entire UTF-8 codepoint, including a newline, in one gesture.
  assert(jot_store_new());
  assert(jot_store_input("first line\n"));
  assert(strcmp(jot_store_text(), "first line\n") == 0);
  assert(jot_store_input("\b"));
  assert(strcmp(jot_store_text(), "first line") == 0);
  assert(jot_store_input("\n\n"));
  assert(jot_store_input("\xC3\xA9")); // e-acute, two UTF-8 bytes
  assert(strcmp(jot_store_text(), "first line\n\n\xC3\xA9") == 0);
  assert(jot_store_input("\b"));
  assert(strcmp(jot_store_text(), "first line\n\n") == 0);
  assert(jot_store_input("\b"));
  assert(strcmp(jot_store_text(), "first line\n") == 0);
  assert(jot_store_input("\b"));
  assert(strcmp(jot_store_text(), "first line") == 0);
  assert(jot_store_input("\nsecond line"));
  assert(jot_store_flush());
  const int multiline_slot = jot_store_slot_at(0);
  assert(multiline_slot >= 0);
  expect_note(multiline_slot, "first line\nsecond line");
  // The list preview should include the second line of a saved note.
  char list_preview[64];
  time_t preview_created = 0;
  assert(jot_store_preview(multiline_slot, list_preview,
                           sizeof list_preview, &preview_created));
  assert(strcmp(list_preview, "first line\nsecond line") == 0);
  assert(preview_created != 0);
  jot_store_init();
  assert(jot_store_open(multiline_slot));
  assert(strcmp(jot_store_text(), "first line\nsecond line") == 0);
  assert(jot_store_input("\b"));
  assert(strcmp(jot_store_text(), "first line\nsecond lin") == 0);

  // Cursor navigation must not mark an otherwise saved note as dirty.
  assert(jot_store_new());
  assert(jot_store_input("ABCDE"));
  assert(jot_store_flush());
  assert(!jot_store_dirty());
  assert(jot_store_cursor() == 5);
  assert(jot_store_move_cursor(JOT_CURSOR_LEFT));
  assert(jot_store_move_cursor(JOT_CURSOR_LEFT));
  assert(jot_store_cursor() == 3);
  assert(!jot_store_dirty());
  assert(jot_store_input("\xC3\xA9")); // Insert e-acute before D.
  assert(strcmp(jot_store_text(), "ABC\xC3\xA9" "DE") == 0);
  assert(jot_store_cursor() == 5);
  assert(!jot_store_set_cursor(4)); // The second UTF-8 byte is not a boundary.
  assert(jot_store_cursor() == 5);
  assert(jot_store_input("\b"));
  assert(strcmp(jot_store_text(), "ABCDE") == 0);
  assert(jot_store_cursor() == 3);
  assert(jot_store_input("!"));
  assert(strcmp(jot_store_text(), "ABC!DE") == 0);
  assert(jot_store_cursor() == 4);
  assert(jot_store_move_cursor(JOT_CURSOR_HOME));
  assert(jot_store_cursor() == 0);
  assert(!jot_store_move_cursor(JOT_CURSOR_LEFT));
  assert(jot_store_input("Z"));
  assert(strcmp(jot_store_text(), "ZABC!DE") == 0);
  assert(jot_store_move_cursor(JOT_CURSOR_END));
  assert(jot_store_cursor() == jot_store_length());
  assert(!jot_store_move_cursor(JOT_CURSOR_RIGHT));
  assert(jot_store_flush());
  const int cursor_slot = jot_store_slot_at(0);
  jot_store_init();
  assert(jot_store_open(cursor_slot));
  assert(strcmp(jot_store_text(), "ZABC!DE") == 0);
  assert(jot_store_cursor() == jot_store_length());

  // Up/Down traverses explicit newline-delimited lines, retaining a
  // preferred codepoint column when an intermediate line is shorter.
  assert(jot_store_new());
  assert(jot_store_input("ABCDE\nxy\n12345"));
  assert(jot_store_set_cursor(3)); // ABC|DE
  assert(jot_store_move_cursor(JOT_CURSOR_DOWN));
  assert(jot_store_cursor() == 8); // xy| (clamped from column 3)
  assert(jot_store_move_cursor(JOT_CURSOR_DOWN));
  assert(jot_store_cursor() == 12); // 123|45, restored column 3
  assert(jot_store_move_cursor(JOT_CURSOR_UP));
  assert(jot_store_cursor() == 8);
  assert(jot_store_move_cursor(JOT_CURSOR_UP));
  assert(jot_store_cursor() == 3);
  assert(!jot_store_move_cursor(JOT_CURSOR_UP));
  assert(jot_store_move_cursor(JOT_CURSOR_DOWN));
  assert(jot_store_cursor() == 8);
  assert(jot_store_input("Q")); // edit at end of short middle line
  assert(strcmp(jot_store_text(), "ABCDE\nxyQ\n12345") == 0);
  assert(jot_store_input("\b"));
  assert(strcmp(jot_store_text(), "ABCDE\nxy\n12345") == 0);
  assert(jot_store_move_cursor(JOT_CURSOR_HOME));
  assert(jot_store_move_cursor(JOT_CURSOR_END));
  assert(!jot_store_move_cursor(JOT_CURSOR_DOWN));
  assert(jot_store_set_cursor(6)); // before x in second line
  assert(jot_store_input("\b")); // delete preceding LF; merge lines
  assert(strcmp(jot_store_text(), "ABCDExy\n12345") == 0);
  assert(jot_store_cursor() == 5);

  // Full-buffer rejection must preserve the note and insertion point.
  assert(jot_store_new());
  char limit_text[JOT_NOTE_TEXT_BYTES];
  memset(limit_text, 'q', sizeof limit_text - 1);
  limit_text[sizeof limit_text - 1] = '\0';
  assert(jot_store_input(limit_text));
  assert(jot_store_length() == JOT_NOTE_TEXT_BYTES - 1);
  assert(jot_store_set_cursor(80));
  assert(!jot_store_input("x"));
  assert(jot_store_cursor() == 80);
  assert(strcmp(jot_store_text(), limit_text) == 0);
  assert(jot_store_input("\b"));
  assert(jot_store_cursor() == 79);
  assert(jot_store_input("x"));
  assert(jot_store_length() == JOT_NOTE_TEXT_BYTES - 1);
  assert(jot_store_cursor() == 80);
  assert(jot_store_text()[79] == 'x');

  // Forward delete removes the following character without moving the caret.
  assert(jot_store_new());
  assert(!jot_store_delete_forward()); // Empty draft, no hidden note created.
  assert(jot_store_input("ABCDE"));
  assert(jot_store_flush());
  assert(!jot_store_dirty());
  assert(!jot_store_delete_forward()); // At EOF: clean no-op.
  assert(!jot_store_dirty());
  assert(jot_store_set_cursor(2));
  assert(jot_store_delete_forward());  // AB|CDE -> AB|DE
  assert(strcmp(jot_store_text(), "ABDE") == 0);
  assert(jot_store_cursor() == 2);
  assert(jot_store_dirty());
  assert(jot_store_flush());
  assert(jot_store_set_cursor(0));
  assert(jot_store_delete_forward());
  assert(strcmp(jot_store_text(), "BDE") == 0);
  assert(jot_store_cursor() == 0);
  assert(jot_store_input("A"));
  assert(strcmp(jot_store_text(), "ABDE") == 0);
  assert(jot_store_cursor() == 1);

  // UTF-8 characters must not be split: delete accented/emoji characters.
  assert(jot_store_new());
  assert(jot_store_input("a\xC3\xA9" "b"));
  assert(jot_store_set_cursor(1));
  assert(jot_store_delete_forward());
  assert(strcmp(jot_store_text(), "ab") == 0);
  assert(jot_store_cursor() == 1);
  assert(jot_store_input("\xF0\x9F\x98\x8A"));
  assert(jot_store_set_cursor(1));
  assert(jot_store_delete_forward());
  assert(strcmp(jot_store_text(), "ab") == 0);
  assert(jot_store_cursor() == 1);
  assert(jot_store_input("\b")); // Old backward-delete still works.
  assert(strcmp(jot_store_text(), "b") == 0);

  // Deleting a newline forward joins lines and survives save/reopen.
  assert(jot_store_new());
  assert(jot_store_input("first\nsecond"));
  assert(jot_store_set_cursor(5));
  assert(jot_store_delete_forward());
  assert(strcmp(jot_store_text(), "firstsecond") == 0);
  assert(jot_store_cursor() == 5);
  assert(jot_store_flush());
  const int forward_slot = jot_store_slot_at(0);
  jot_store_init();
  assert(jot_store_open(forward_slot));
  assert(strcmp(jot_store_text(), "firstsecond") == 0);
  assert(!jot_store_delete_forward());
  assert(!jot_store_dirty());

  // A full note can delete forward and insert again without a FULL error.
  assert(jot_store_new());
  assert(jot_store_input(limit_text));
  assert(jot_store_set_cursor(80));
  assert(jot_store_delete_forward());
  assert(jot_store_length() == JOT_NOTE_TEXT_BYTES - 2);
  assert(jot_store_cursor() == 80);
  assert(jot_store_input("Z"));
  assert(jot_store_length() == JOT_NOTE_TEXT_BYTES - 1);
  assert(jot_store_text()[80] == 'Z');

  // Navigation depends on the active slot to highlight the right note.
  // This remains stable across flushing and changes on reopening a note.
  assert(jot_store_new());
  assert(jot_store_active_slot() == -1);
  assert(jot_store_input("navigation test"));
  const int navigation_slot = jot_store_active_slot();
  assert(navigation_slot >= 0);
  assert(jot_store_flush());
  assert(jot_store_active_slot() == navigation_slot);

  // A failed write must *never* drop the only unsaved draft. The navigation
  // controller refuses to leave the editor if either flush or new fails.
  assert(jot_store_input(" preserved"));
  assert(jot_store_dirty());
  const size_t navigation_cursor = jot_store_cursor();
  s_fail_next_write = true;
  assert(!jot_store_flush());
  assert(jot_store_dirty());
  assert(jot_store_active_slot() == navigation_slot);
  assert(jot_store_cursor() == navigation_cursor);
  assert(strcmp(jot_store_text(), "navigation test preserved") == 0);
  s_fail_next_write = true;
  assert(!jot_store_new()); // A reset must not discard an unsaved note.
  assert(jot_store_dirty());
  assert(jot_store_active_slot() == navigation_slot);
  assert(strcmp(jot_store_text(), "navigation test preserved") == 0);
  assert(jot_store_flush());
  assert(jot_store_new());
  assert(jot_store_active_slot() == -1);
  assert(jot_store_length() == 0);
  assert(jot_store_open(navigation_slot));
  assert(strcmp(jot_store_text(), "navigation test preserved") == 0);
  assert(jot_store_active_slot() == navigation_slot);
  assert(!jot_store_dirty());

  // Touch SAVE after autosave must reuse the same persistent note.
  // The reader can first obtain the committed content; *then* the
  // underlying keyboard can become a new draft without losing that note.
  const int saved_before = jot_store_count();
  const int saved_slot = jot_store_active_slot();
  const size_t saved_cursor = jot_store_cursor();
  assert(saved_slot >= 0);
  assert(jot_store_flush()); // Already clean; no second record.
  assert(jot_store_count() == saved_before);
  assert(jot_store_active_slot() == saved_slot);
  assert(jot_store_cursor() == saved_cursor);
  char saved_copy[JOT_NOTE_TEXT_BYTES];
  time_t saved_created = 0;
  assert(jot_store_export(saved_slot, saved_copy, sizeof saved_copy,
                          &saved_created));
  assert(strcmp(saved_copy, "navigation test preserved") == 0);
  assert(jot_store_new());  // Happens only after reader opens.
  assert(jot_store_active_slot() == -1);
  assert(jot_store_length() == 0);
  assert(jot_store_count() == saved_before);
  assert(jot_store_export(saved_slot, saved_copy, sizeof saved_copy,
                          &saved_created));
  assert(strcmp(saved_copy, "navigation test preserved") == 0);

  // Both active and saved previews preserve LF and complete UTF-8 glyphs.
  assert(jot_store_new());
  assert(jot_store_input("A\nB\xC3\xA9" "Z"));
  const int unicode_slot = jot_store_active_slot();
  assert(unicode_slot >= 0);
  char short_preview[5];
  assert(jot_store_preview(unicode_slot, short_preview,
                           sizeof short_preview, NULL));
  assert(strcmp(short_preview, "A\nB") == 0); // Not half an accented glyph.
  assert(jot_store_flush());
  assert(jot_store_new());
  char unicode_preview[16];
  assert(jot_store_preview(unicode_slot, unicode_preview,
                           sizeof unicode_preview, NULL));
  assert(strcmp(unicode_preview, "A\nB\xC3\xA9" "Z") == 0);

  // Exercise a full 4095-byte note, its last 256-byte chunk and reboot.
  assert(jot_store_new());
  char long_text[JOT_NOTE_TEXT_BYTES];
  memset(long_text, 'L', sizeof long_text - 1);
  long_text[sizeof long_text - 1] = '\0';
  assert(jot_store_input(long_text));
  assert(jot_store_flush());
  const int long_slot = jot_store_active_slot();
  jot_store_init();
  expect_note(long_slot, long_text);
  assert(jot_store_open(long_slot));
  assert(jot_store_cursor() == JOT_NOTE_TEXT_BYTES - 1);
  assert(!jot_store_input("+"));
  assert(jot_store_input("\\b"));
  assert(jot_store_input("X"));
  assert(jot_store_flush());
  jot_store_init();
  assert(jot_store_open(long_slot));
  assert(jot_store_text()[JOT_NOTE_TEXT_BYTES - 2] == 'X');

  // Up to 256 independent notes, beyond the previous 16-note ceiling.
  while (jot_store_count() < JOT_NOTE_CAPACITY) {
    assert(jot_store_new());
    assert(jot_store_input("QA slot"));
    assert(jot_store_flush());
  }
  assert(jot_store_new());
  assert(!jot_store_input("257th"));
  assert(jot_store_count() == JOT_NOTE_CAPACITY);
  assert(jot_store_delete(long_slot));
  assert(jot_store_input("reclaimed"));
  assert(jot_store_flush());
  assert(jot_store_count() == JOT_NOTE_CAPACITY);

  puts("Jot storage regression tests passed");
  return 0;
}

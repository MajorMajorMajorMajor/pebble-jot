// Run: cc -std=c11 -Wall -Wextra -Werror -I tests/fake -I src/c
//          tests/store-test.c src/c/jot/store.c -o /tmp/jot-store-test
//       /tmp/jot-store-test
#include <assert.h>
#include <stdio.h>
#include <pebble.h>
#include "jot/store.h"

#define STORAGE_KEY_FIRST 4100u
#define SLOT_CAPACITY 256u

typedef struct {
  bool exists;
  size_t size;
  uint8_t bytes[SLOT_CAPACITY];
} FakeEntry;

static FakeEntry s_entries[JOT_NOTE_CAPACITY];
static bool s_fail_next_delete;

static FakeEntry *prv_entry(uint32_t key) {
  if (key < STORAGE_KEY_FIRST ||
      key >= STORAGE_KEY_FIRST + JOT_NOTE_CAPACITY) return NULL;
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
  assert(!s_entries[second].exists);

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
  assert(s_entries[third].exists);
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
  jot_store_init();
  assert(jot_store_open(multiline_slot));
  assert(strcmp(jot_store_text(), "first line\nsecond line") == 0);
  assert(jot_store_input("\b"));
  assert(strcmp(jot_store_text(), "first line\nsecond lin") == 0);

  puts("Jot storage regression tests passed");
  return 0;
}

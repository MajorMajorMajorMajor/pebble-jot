// Keeping notes: seven independent persistence and corruption-recovery behaviors.
// Simulated Pebble flash failures; tests do not require the watch or SDK.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pebble.h>
#include "jot/store.h"

enum {
  STORAGE_KEY_FIRST = 5000,
  SLOT_CAPACITY = 256,
  RECORD_KEYS = 2 * (JOT_NOTE_TEXT_BYTES / SLOT_CAPACITY + 1),
  LEGACY_KEY_FIRST = 4100,
  DIRECTORY_KEY = 4999,
  FAKE_STORAGE_LIMIT = 1024 * 1024
};

typedef struct {
  bool exists;
  size_t size;
  uint8_t bytes[SLOT_CAPACITY];
} FakeEntry;

static FakeEntry s_entries[JOT_NOTE_CAPACITY * RECORD_KEYS];
static FakeEntry s_legacy[16];
static FakeEntry s_directory;
static bool s_fail_next_write;
static bool s_fail_next_delete;
static int s_fail_after_writes = -1;
static size_t s_bytes_used;

static FakeEntry *entry_at(uint32_t key) {
  if (key == DIRECTORY_KEY) return &s_directory;
  if (key >= LEGACY_KEY_FIRST && key < LEGACY_KEY_FIRST + 16)
    return &s_legacy[key - LEGACY_KEY_FIRST];
  if (key < STORAGE_KEY_FIRST ||
      key >= STORAGE_KEY_FIRST + JOT_NOTE_CAPACITY * RECORD_KEYS)
    return NULL;
  return &s_entries[key - STORAGE_KEY_FIRST];
}

int persist_read_data(uint32_t key, void *buffer, size_t capacity) {
  FakeEntry *entry = entry_at(key);
  if (!entry || !entry->exists) return E_DOES_NOT_EXIST;
  if (capacity < entry->size) return E_ERROR;
  memcpy(buffer, entry->bytes, entry->size);
  return (int)entry->size;
}

int persist_write_data(uint32_t key, const void *data, size_t size) {
  if (s_fail_next_write) {
    s_fail_next_write = false;
    return E_ERROR;
  }
  if (s_fail_after_writes >= 0) {
    if (s_fail_after_writes == 0) {
      s_fail_after_writes = -1;
      return E_ERROR;
    }
    --s_fail_after_writes;
  }
  FakeEntry *entry = entry_at(key);
  if (!entry || !data || size > SLOT_CAPACITY ||
      s_bytes_used - entry->size + size > FAKE_STORAGE_LIMIT)
    return E_ERROR;
  s_bytes_used = s_bytes_used - entry->size + size;
  memcpy(entry->bytes, data, size);
  entry->size = size;
  entry->exists = true;
  return (int)size;
}

status_t persist_delete(uint32_t key) {
  if (s_fail_next_delete) {
    s_fail_next_delete = false;
    return E_ERROR;
  }
  FakeEntry *entry = entry_at(key);
  if (!entry || !entry->exists) return E_DOES_NOT_EXIST;
  s_bytes_used -= entry->size;
  entry->size = 0;
  entry->exists = false;
  return S_TRUE;
}

static void fresh_store(void) {
  jot_store_deinit();
  memset(s_entries, 0, sizeof s_entries);
  memset(s_legacy, 0, sizeof s_legacy);
  memset(&s_directory, 0, sizeof s_directory);
  s_bytes_used = 0;
  s_fail_next_write = false;
  s_fail_next_delete = false;
  s_fail_after_writes = -1;
  jot_store_init();
}

static void expect_note(int slot, const char *expected) {
  char text[JOT_NOTE_TEXT_BYTES];
  assert(jot_store_export(slot, text, sizeof text, NULL));
  assert(strcmp(text, expected) == 0);
}

// 1. Normal save, restart and note ordering.
static void test_committed_round_trip(void) {
  fresh_store();
  assert(jot_store_input("first"));
  assert(jot_store_flush());
  const int first = jot_store_active_slot();
  assert(jot_store_new());
  assert(jot_store_input("second"));
  assert(jot_store_flush());
  const int second = jot_store_active_slot();
  jot_store_init();  // Simulate relaunch.
  assert(jot_store_count() == 2);
  assert(jot_store_slot_at(0) == second);
  assert(jot_store_slot_at(1) == first);
  expect_note(first, "first");
  expect_note(second, "second");
}

// 2. Damaged/missing acceleration hints must never hide committed notes.
static void test_directory_recovery(void) {
  fresh_store();
  assert(jot_store_input("survives bad directory"));
  assert(jot_store_flush());
  const int slot = jot_store_active_slot();
  assert(s_directory.exists);
  s_directory.bytes[10] ^= 0x80;  // Corrupt checksum-covered directory.
  jot_store_init();
  assert(jot_store_count() == 1);
  expect_note(slot, "survives bad directory");
  s_bytes_used -= s_directory.size;
  s_directory.exists = false;  // Upgrade from an installation without hints.
  s_directory.size = 0;
  jot_store_init();
  assert(jot_store_count() == 1);
  expect_note(slot, "survives bad directory");
}

// 3. Failed first commit must not produce ghosts or discard an unsaved draft.
static void test_failed_saves(void) {
  fresh_store();
  assert(jot_store_input("never committed"));
  s_fail_next_write = true;  // Directory bit cannot be written.
  assert(!jot_store_flush());
  assert(jot_store_dirty());
  jot_store_init();  // Draft in RAM is gone after power loss.
  assert(jot_store_count() == 0);  // But no phantom committed record exists.

  fresh_store();
  assert(jot_store_input("must stay in editor"));
  s_fail_next_write = true;
  assert(!jot_store_new());  // Navigation must not discard the dirty draft.
  assert(jot_store_dirty());
  assert(strcmp(jot_store_text(), "must stay in editor") == 0);
  assert(jot_store_flush());
  const int slot = jot_store_active_slot();
  jot_store_init();
  expect_note(slot, "must stay in editor");
}

// 4. A multi-chunk interrupted update leaves the LAST complete version intact.
static void test_interrupted_commit(void) {
  fresh_store();
  char full[JOT_NOTE_TEXT_BYTES];
  memset(full, 'L', sizeof full - 1);
  full[sizeof full - 1] = '\0';
  assert(jot_store_input(full));
  assert(jot_store_flush());
  const int slot = jot_store_active_slot();
  assert(!jot_store_input("+"));  // Hard limit: 4095 UTF-8 bytes.
  assert(jot_store_set_cursor(2));
  assert(jot_store_delete_forward());
  assert(jot_store_input("Z"));
  s_fail_after_writes = 2;  // Stop after two inactive-bank chunks.
  assert(!jot_store_flush());
  jot_store_init();
  assert(jot_store_open(slot));
  expect_note(slot, full);  // Previous bank, not a partial new version.
  assert(jot_store_set_cursor(2));
  assert(jot_store_delete_forward());
  assert(jot_store_input("Z"));
  assert(jot_store_flush());
  jot_store_init();
  assert(jot_store_open(slot));
  assert(jot_store_text()[2] == 'Z');
}

// 5. Failed deletion cannot remove a note; successful deletion is isolated.
static void test_delete_isolation(void) {
  fresh_store();
  assert(jot_store_input("keep other notes"));
  assert(jot_store_flush());
  const int first = jot_store_active_slot();
  assert(jot_store_new());
  assert(jot_store_input("newest"));
  assert(jot_store_flush());
  const int second = jot_store_active_slot();
  s_fail_next_delete = true;
  assert(!jot_store_delete(first));
  assert(jot_store_count() == 2);
  assert(jot_store_delete(first));
  jot_store_init();
  assert(jot_store_count() == 1);
  expect_note(second, "newest");
  assert(jot_store_input("recycled"));
  assert(jot_store_active_slot() == first);
  assert(jot_store_flush());
}

// 6. Edits must respect whole UTF-8 codepoints, including line breaks.
static void test_utf8_editing(void) {
  fresh_store();
  assert(jot_store_input("A\xC3\xA9\n\xF0\x9F\x98\x8A" "B"));
  assert(!jot_store_set_cursor(2));  // Middle of é.
  assert(!jot_store_set_cursor(5));  // Middle of emoji.
  assert(jot_store_set_cursor(1));
  assert(jot_store_delete_forward());  // Remove é, not one of its bytes.
  assert(strcmp(jot_store_text(), "A\n\xF0\x9F\x98\x8A" "B") == 0);
  assert(jot_store_input("\b"));  // Remove A.
  assert(jot_store_delete_forward());  // Remove LF.
  assert(jot_store_delete_forward());  // Remove entire emoji.
  assert(strcmp(jot_store_text(), "B") == 0);
  assert(jot_store_flush());
  const int slot = jot_store_active_slot();
  jot_store_init();
  expect_note(slot, "B");
}

// 7. The 256-slot and 1 MiB app budget are strict, recoverable limits.
static void test_capacity_and_quota(void) {
  fresh_store();
  while (jot_store_count() < JOT_NOTE_CAPACITY) {
    assert(jot_store_new());
    assert(jot_store_input("slot"));
    assert(jot_store_flush());
  }
  assert(jot_store_new());
  assert(!jot_store_input("257th"));
  assert(jot_store_count() == JOT_NOTE_CAPACITY);

  int failed_slot = -1;
  int committed_full_notes = 0;
  char last_committed[JOT_NOTE_TEXT_BYTES];
  for (int rank = 0; rank < JOT_NOTE_CAPACITY; ++rank) {
    const int slot = jot_store_slot_at(rank);
    assert(jot_store_open(slot));
    const size_t length = jot_store_length();
    char tail[JOT_NOTE_TEXT_BYTES];
    memset(tail, 'q', JOT_NOTE_TEXT_BYTES - 1 - length);
    tail[JOT_NOTE_TEXT_BYTES - 1 - length] = '\0';
    strcpy(last_committed, jot_store_text());
    assert(jot_store_input(tail));
    if (!jot_store_flush()) {
      failed_slot = slot;
      assert(jot_store_dirty());
      break;
    }
    ++committed_full_notes;
  }
  assert(committed_full_notes > 150);
  assert(failed_slot >= 0);
  assert(s_bytes_used <= FAKE_STORAGE_LIMIT);
  jot_store_init();  // Power loss following the rejected quota write.
  assert(jot_store_count() == JOT_NOTE_CAPACITY);
  expect_note(failed_slot, last_committed);
}

int main(void) {
  test_committed_round_trip();
  test_directory_recovery();
  test_failed_saves();
  test_interrupted_commit();
  test_delete_isolation();
  test_utf8_editing();
  test_capacity_and_quota();
  jot_store_deinit();
  puts("Seven independent storage safety scenarios passed");
  return 0;
}

// Keeping notes: ten independent persistence and corruption-recovery behaviors.
// Simulated Pebble flash failures; tests do not require the watch or SDK.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pebble.h>
#include "jot/store.h"
#include "keyboard/compose.h"

enum {
  STORAGE_KEY_FIRST = 5000,
  SLOT_CAPACITY = 256,
  RECORD_KEYS = 2 * (JOT_NOTE_TEXT_BYTES / SLOT_CAPACITY + 1),
  LEGACY_KEY_FIRST = 4100,
  DIRECTORY_KEY = 4999,
  SERIAL_HIGH_WATER_KEY = 4998,
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
static FakeEntry s_serial_high_water;
static bool s_fail_next_write;
static bool s_fail_next_delete;
static int s_fail_after_writes = -1;
static int s_fail_after_deletes = -1;
static size_t s_bytes_used;

static FakeEntry *entry_at(uint32_t key) {
  if (key == DIRECTORY_KEY) return &s_directory;
  if (key == SERIAL_HIGH_WATER_KEY) return &s_serial_high_water;
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
  if (s_fail_after_deletes >= 0) {
    if (s_fail_after_deletes == 0) {
      s_fail_after_deletes = -1;
      return E_ERROR;
    }
    --s_fail_after_deletes;
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
  memset(&s_serial_high_water, 0, sizeof s_serial_high_water);
  s_bytes_used = 0;
  s_fail_next_write = false;
  s_fail_next_delete = false;
  s_fail_after_writes = -1;
  s_fail_after_deletes = -1;
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

// 2. Ranked lookup stays stable across drafts, edits, deletion, reuse and
// restart. A lower-numbered reused slot must sort by serial, not slot ID.
static void test_ranked_slot_lifecycle(void) {
  fresh_store();
  assert(jot_store_count() == 0);
  assert(jot_store_slot_at(-1) == -1);
  assert(jot_store_slot_at(0) == -1);

  assert(jot_store_input("first"));
  const int first = jot_store_active_slot();
  assert(jot_store_count() == 1);  // Draft already appears in the list.
  assert(jot_store_slot_at(0) == first);
  assert(jot_store_flush());
  assert(jot_store_new());
  assert(jot_store_input("second"));
  const int second = jot_store_active_slot();
  assert(jot_store_flush());
  assert(jot_store_new());
  assert(jot_store_input("third"));
  const int third = jot_store_active_slot();
  assert(jot_store_flush());

  assert(jot_store_count() == 3);
  assert(jot_store_slot_at(0) == third);
  assert(jot_store_slot_at(1) == second);
  assert(jot_store_slot_at(2) == first);
  assert(jot_store_slot_at(3) == -1);

  // Editing an old note must preserve creation order.
  assert(jot_store_open(first));
  assert(jot_store_input(" edit"));
  assert(jot_store_flush());
  assert(jot_store_slot_at(2) == first);

  // Failure to delete must not change ranks.
  s_fail_next_delete = true;
  assert(!jot_store_delete(second));
  assert(jot_store_count() == 3);
  assert(jot_store_slot_at(1) == second);
  assert(jot_store_delete(second));
  assert(jot_store_count() == 2);
  assert(jot_store_slot_at(0) == third);
  assert(jot_store_slot_at(1) == first);
  assert(jot_store_slot_at(2) == -1);

  assert(jot_store_new());
  assert(jot_store_input("reused"));
  assert(jot_store_active_slot() == second);
  assert(jot_store_flush());
  assert(jot_store_slot_at(0) == second);
  assert(jot_store_slot_at(1) == third);
  assert(jot_store_slot_at(2) == first);

  jot_store_init();
  assert(jot_store_count() == 3);
  assert(jot_store_slot_at(0) == second);
  assert(jot_store_slot_at(1) == third);
  assert(jot_store_slot_at(2) == first);
  expect_note(second, "reused");
  expect_note(first, "first edit");
  expect_note(third, "third");
}

// 3. Damaged/missing acceleration hints must never hide committed notes.
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

// 4. Failed first commit must not produce ghosts or discard an unsaved draft.
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

// 5. A multi-chunk interrupted update leaves the LAST complete version intact.
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

// 6. Failed deletion cannot remove a note; successful deletion is isolated.
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

// 7. A stale committed bank must not turn a failed deletion into a ghost.
// Cleanup failure after a successful deletion may leave *chunks*, not headers.
static void test_stale_bank_deletion(void) {
  fresh_store();
  assert(jot_store_input("original"));
  assert(jot_store_flush());
  const int slot = jot_store_active_slot();
  assert(jot_store_input(" updated"));
  // Flush clears all 17 keys in the target bank, then garbage-collects the
  // previous bank. Fail precisely that GC header deletion.
  s_fail_after_deletes = RECORD_KEYS / 2;
  assert(jot_store_flush());
  const int base = slot * RECORD_KEYS;
  assert(s_entries[base].exists);
  assert(s_entries[base + RECORD_KEYS / 2].exists);

  // The stale header deletion succeeds; the active-header deletion fails.
  // Deletion must be reported as failed and the current note must survive.
  s_fail_after_deletes = 1;
  assert(!jot_store_delete(slot));
  jot_store_init();
  assert(jot_store_count() == 1);
  expect_note(slot, "original updated");

  // This time deletion commits, but first-chunk cleanup fails. No header
  // remains, so even an immediate restart cannot resurrect the old note.
  s_fail_after_deletes = 3;
  assert(jot_store_delete(slot));
  assert(s_entries[base + 1].exists);  // Orphaned old chunk.
  jot_store_init();
  assert(jot_store_count() == 0);

  // Reusing the slot must clean those orphaned chunks and survive reboot.
  assert(jot_store_input("replacement"));
  assert(jot_store_active_slot() == slot);
  assert(jot_store_flush());
  jot_store_init();
  assert(jot_store_count() == 1);
  expect_note(slot, "replacement");
}

// 8. Recover a mixed-generation slot made by older buggy releases: a stale
// higher revision must never hide a newer serial's first revision.
static void test_mixed_generation_recovery(void) {
  fresh_store();
  assert(jot_store_input("old"));
  assert(jot_store_flush());
  const int slot = jot_store_active_slot();
  assert(jot_store_input(" revision"));
  assert(jot_store_flush());  // Old note: revision 2 in bank 1.
  const int old_header = slot * RECORD_KEYS + RECORD_KEYS / 2;
  const FakeEntry saved_header = s_entries[old_header];
  const FakeEntry saved_chunk = s_entries[old_header + 1];
  assert(saved_header.exists && saved_chunk.exists);

  assert(jot_store_delete(slot));
  assert(jot_store_input("new"));
  assert(jot_store_active_slot() == slot);
  assert(jot_store_flush());  // New serial: revision 1 in bank 0.

  // Recreate the leftover old bank that an older release could have left.
  assert(!s_entries[old_header].exists);
  assert(!s_entries[old_header + 1].exists);
  s_entries[old_header] = saved_header;
  s_entries[old_header + 1] = saved_chunk;
  s_bytes_used += saved_header.size + saved_chunk.size;
  jot_store_init();
  assert(jot_store_count() == 1);
  expect_note(slot, "new");
}

// 9. Edits must respect whole UTF-8 codepoints, including line breaks.
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

// Tab storage is U+0009, never a display-converted space.
static void test_tab_editing_and_persistence(void) {
  fresh_store();
  assert(jot_store_input("a\té\n\tZ"));
  assert(jot_store_length() == strlen("a\té\n\tZ"));
  assert(jot_store_set_cursor(1)); // Immediately before tab.
  assert(jot_store_move_cursor(JOT_CURSOR_RIGHT));
  assert(jot_store_cursor() == 2);
  assert(jot_store_move_cursor(JOT_CURSOR_LEFT));
  assert(jot_store_delete_forward()); // Remove tab, not é.
  assert(strcmp(jot_store_text(), "aé\n\tZ") == 0);
  assert(jot_store_input("\t"));
  assert(jot_store_input("\b")); // Remove tab in one backspace.
  assert(strcmp(jot_store_text(), "aé\n\tZ") == 0);
  assert(jot_store_input("\t"));
  assert(jot_store_set_cursor(strlen("a\té\n")));
  assert(jot_store_input("\b")); // Delete LF, keep two tabs.
  assert(strcmp(jot_store_text(), "a\té\tZ") == 0);
  assert(jot_store_input("\n"));
  assert(jot_store_flush());
  const int slot = jot_store_active_slot();
  jot_store_init();
  expect_note(slot, "a\té\n\tZ");
  assert(jot_store_open(slot));
  assert(strcmp(jot_store_text(), "a\té\n\tZ") == 0);
}

// 10. The 256-slot and 1 MiB app budget are strict, recoverable limits.
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
  for (int rank = 0; rank < JOT_NOTE_CAPACITY; ++rank)
    assert(jot_store_slot_at(rank) == JOT_NOTE_CAPACITY - rank - 1);
  assert(jot_store_slot_at(JOT_NOTE_CAPACITY) == -1);

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


static bool apply_compose(void) {
  JotComposeEdit edit;
  if (!jot_compose_at(jot_store_text(), jot_store_cursor(), &edit))
    return false;
  return jot_store_replace_before_cursor(
      edit.codepoints_before_cursor, edit.replacement);
}

// 11. Every Compose edit is a single UTF-8 safe, crash-recoverable
// mutation; a failed replacement never removes the original bytes.
static void test_compose_undo_and_capacity(void) {
  fresh_store();
  assert(jot_store_input("hello oe world"));
  assert(jot_store_set_cursor(8)); // After "oe", before " world".
  assert(apply_compose());
  assert(strcmp(jot_store_text(), "hello œ world") == 0);
  assert(jot_store_cursor() == strlen("hello œ"));
  // With the cursor moved, we cycle a character entered earlier.
  assert(jot_store_set_cursor(strlen("hello œ")));
  assert(apply_compose());
  assert(strcmp(jot_store_text(), "hello ¤ world") == 0);
  assert(jot_store_cursor() == strlen("hello ¤"));
  // A transformation on the prefix never damages the UTF-8 suffix.
  assert(jot_store_flush());
  const int slot = jot_store_active_slot();
  jot_store_init();
  expect_note(slot, "hello ¤ world");

  assert(jot_store_new());
  assert(jot_store_input("s"));
  static const char *const cycle[] = {"š", "ş", "ŝ", "ß", "∫", "s"};
  for (size_t i = 0; i < sizeof cycle / sizeof cycle[0]; ++i) {
    assert(apply_compose());
    assert(strcmp(jot_store_text(), cycle[i]) == 0);
    assert(jot_store_cursor() == strlen(cycle[i]));
  }
  assert(jot_store_flush());
  const int cycle_slot = jot_store_active_slot();
  jot_store_init();
  expect_note(cycle_slot, "s");

  // A full note cannot be expanded, but its bytes, cursor, dirty flag
  // and suffix remain intact. The rejected Compose is not autosaved.
  assert(jot_store_new());
  char full[JOT_NOTE_TEXT_BYTES];
  memset(full, 'q', JOT_NOTE_TEXT_BYTES - 2);
  full[JOT_NOTE_TEXT_BYTES - 2] = 's';
  full[JOT_NOTE_TEXT_BYTES - 1] = '\0';
  assert(jot_store_input(full));
  assert(jot_store_flush());
  assert(!jot_store_dirty());
  assert(jot_store_cursor() == JOT_NOTE_TEXT_BYTES - 1);
  assert(!apply_compose()); // s → š would grow the note by one byte.
  assert(!jot_store_dirty());
  assert(jot_store_length() == JOT_NOTE_TEXT_BYTES - 1);
  assert(jot_store_cursor() == JOT_NOTE_TEXT_BYTES - 1);
  assert(strcmp(jot_store_text(), full) == 0);
  assert(!jot_store_replace_before_cursor(2, NULL));
  assert(!jot_store_replace_before_cursor(0, "x"));
  assert(!jot_store_replace_before_cursor(3, "x"));
  assert(!jot_store_dirty());
}

// Every phone-visible note identity must survive delete-all and restart.
static void test_serial_high_water_survives_delete_all(void) {
  fresh_store();
  assert(jot_store_input("first"));
  assert(jot_store_flush());
  const int first = jot_store_active_slot();
  uint32_t a = 0, rev = 0;
  assert(jot_store_identity(first, &a, &rev) && a > 0 && rev == 1);
  assert(jot_store_delete(first));
  jot_store_init();
  assert(jot_store_count() == 0);
  assert(jot_store_input("second"));
  assert(jot_store_flush());
  uint32_t b = 0;
  assert(jot_store_identity(jot_store_active_slot(), &b, NULL));
  assert(b > a);
}

// Imported phone notes are idempotent across restart, edit and lost receipts.
static void test_import_receipts_and_isolation(void) {
  fresh_store();
  const char *text = "Phone to watch 📝\nsecond line";
  JotImportReceipt a = {0}, b = {0};
  assert(jot_store_import_new(text, strlen(text), 0xA1234567u,
                              0x11223344u, &a) == JOT_IMPORT_SAVED);
  assert(a.serial != 0 && a.revision == 1 && jot_store_count() == 1);
  expect_note(jot_store_slot_at(0), text);
  assert(jot_store_import_new(text, strlen(text), 0xA1234567u,
                              0x11223344u, &b) == JOT_IMPORT_ALREADY);
  assert(a.serial == b.serial && a.created == b.created);
  assert(jot_store_count() == 1);
  jot_store_init();
  assert(jot_store_import_lookup(0xA1234567u, 0x11223344u, &b));
  assert(b.serial == a.serial && b.created == a.created);
  assert(jot_store_import_new(text, strlen(text), 0xA1234567u,
                              0x11223344u, &b) == JOT_IMPORT_ALREADY);
  assert(jot_store_count() == 1);

  const int slot = jot_store_slot_at(0);
  assert(jot_store_open(slot));
  assert(jot_store_input("!"));
  assert(jot_store_flush());
  jot_store_init();
  assert(jot_store_import_lookup(0xA1234567u, 0x11223344u, &b));
  assert(b.serial == a.serial && b.revision == 2);
  assert(jot_store_count() == 1);

  assert(jot_store_new());
  assert(jot_store_input("unsaved draft"));
  assert(jot_store_dirty());
  assert(jot_store_import_new("new", 3, 0x123u, 0x456u, &b) ==
         JOT_IMPORT_BUSY);
  assert(strcmp(jot_store_text(), "unsaved draft") == 0);
  assert(jot_store_dirty());
  assert(jot_store_flush());

  const int prior = jot_store_count();
  s_fail_after_writes = 3; // Fail the final header after preflight/data.
  assert(jot_store_import_new("safely retried", strlen("safely retried"),
                              0xB00u, 0xC00u, &b) == JOT_IMPORT_FAILED);
  assert(jot_store_count() == prior);
  jot_store_init();
  assert(!jot_store_import_lookup(0xB00u, 0xC00u, &b));
  assert(jot_store_import_new("safely retried", strlen("safely retried"),
                              0xB00u, 0xC00u, &b) == JOT_IMPORT_SAVED);
  assert(jot_store_count() == prior + 1);

  assert(jot_store_import_new("", 0, 1, 2, &b) == JOT_IMPORT_INVALID);
  assert(jot_store_import_new("\xC0\xAF", 2, 1, 2, &b) == JOT_IMPORT_INVALID);
  assert(jot_store_import_new("\xED\xA0\x80", 3, 1, 2, &b) ==
         JOT_IMPORT_INVALID);
  assert(jot_store_import_new("\xF4\x90\x80\x80", 4, 1, 2, &b) ==
         JOT_IMPORT_INVALID);
  char full[JOT_NOTE_TEXT_BYTES];
  memset(full, 'a', sizeof full - 1);
  full[sizeof full - 1] = '\0';
  assert(jot_store_import_new(full, sizeof full - 1, 33, 44, &b) ==
         JOT_IMPORT_SAVED);
  assert(jot_store_import_new(full, sizeof full, 66, 77, &b) ==
         JOT_IMPORT_INVALID);
}

int main(void) {
  test_import_receipts_and_isolation();
  test_serial_high_water_survives_delete_all();
  test_committed_round_trip();
  test_ranked_slot_lifecycle();
  test_directory_recovery();
  test_failed_saves();
  test_interrupted_commit();
  test_delete_isolation();
  test_stale_bank_deletion();
  test_mixed_generation_recovery();
  test_utf8_editing();
  test_tab_editing_and_persistence();
  test_compose_undo_and_capacity();
  test_capacity_and_quota();
  jot_store_deinit();
  puts("Storage safety scenarios (including durable sync identity) passed");
  return 0;
}

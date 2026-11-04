#include <pebble.h>
#include <stdlib.h>
#include "jot/store.h"

// JOT2 uses two banks per slot: a complete new version is written into
// the inactive bank before its header is committed. No legacy migration:
// this release intentionally resets the QA watch's JOT1 records.
#define JOT_MAGIC 0x4A4F5432u
#define JOT_IMPORT_MAGIC 0x4A4F5433u // v3 header: durable phone transfer ID
#define JOT_FIRST_KEY 5000u
#define JOT_LEGACY_FIRST_KEY 4100u
// Small directory stored outside all JOT1/JOT2 note-key ranges.
// It is a CONSERVATIVE candidate-slot hint, never authoritative note data.
#define JOT_DIRECTORY_KEY 4999u
// Keep serial generations monotonic even after the LAST note is deleted.
// This key lies outside both JOT1/JOT2 note banks and the directory.
#define JOT_SERIAL_HIGH_WATER_KEY 4998u
#define JOT_DIRECTORY_MAGIC 0x4A4F5449u // 'JOTI' v1

enum { JOT_LEGACY_CAPACITY = 16, JOT_PART_BYTES = 256,
       JOT_PART_COUNT = JOT_NOTE_TEXT_BYTES / JOT_PART_BYTES,
       JOT_BANK_KEYS = JOT_PART_COUNT + 1 };

typedef struct {
  uint32_t magic;
  uint32_t serial;
  uint32_t timestamp;
  uint32_t revision;
  uint16_t length;
  uint16_t checksum;
  uint32_t import_hi;
  uint32_t import_lo;
} NoteHeader;

// Put wide fields first: 16 bytes per index rather than 20 on ARM.
typedef struct {
  uint32_t serial;
  uint32_t timestamp;
  uint32_t revision;
  uint16_t length;
  int8_t bank;   // -1 until the first successful flush.
  bool used;
} NoteIndex;

// 40 bytes in one atomic persistence value. Each set bit means that the
// slot MAY contain a committed bank, not that a note definitely exists.
// This deliberately admits stale bits after interrupted saves/deletions.
// A valid but incomplete bitmask would be dangerous; every first commit
// writes its bit BEFORE publishing the new note header.
typedef struct {
  uint32_t magic;
  uint8_t candidates[JOT_NOTE_CAPACITY / 8];
  uint32_t checksum;
} NoteDirectory;

static NoteDirectory s_directory;
static bool s_directory_valid;
static NoteIndex s_index[JOT_NOTE_CAPACITY];
// Newest-first RAM-only slot order. Slot IDs are 0..255, so each costs 1 byte.
// It is rebuilt from the recovered index at launch and never written to flash.
static uint8_t s_ranked_slots[JOT_NOTE_CAPACITY];
static int s_ranked_count;
static char *s_read_text; // Allocated lazily for list/open reads.
static uint32_t s_next_serial;
static int s_active_slot;
static time_t s_active_time;
static char s_text[JOT_NOTE_TEXT_BYTES];
static size_t s_length;
static size_t s_cursor;
static size_t s_vertical_column;
static bool s_keep_vertical_column;
static bool s_dirty;

static bool prv_continuation(unsigned char byte) {
  return (byte & 0xC0) == 0x80;
}

static size_t prv_previous(size_t pos) {
  if (pos == 0) return 0;
  do { --pos; }
  while (pos > 0 && prv_continuation((unsigned char)s_text[pos]));
  return pos;
}

static size_t prv_next(size_t pos) {
  if (pos >= s_length) return s_length;
  ++pos;
  while (pos < s_length && prv_continuation((unsigned char)s_text[pos]))
    ++pos;
  return pos;
}

static size_t prv_line_start(size_t pos) {
  while (pos > 0 && s_text[pos - 1] != '\n') --pos;
  return pos;
}

static size_t prv_line_end(size_t pos) {
  while (pos < s_length && s_text[pos] != '\n') pos = prv_next(pos);
  return pos;
}

static size_t prv_column(size_t line_start, size_t pos) {
  size_t column = 0;
  for (size_t at = line_start; at < pos; at = prv_next(at)) ++column;
  return column;
}

static size_t prv_to_column(size_t start, size_t end, size_t col) {
  size_t at = start;
  while (col && at < end) {
    at = prv_next(at);
    --col;
  }
  return at;
}

size_t jot_store_cursor(void) { return s_cursor; }

bool jot_store_set_cursor(size_t offset) {
  if (offset > s_length ||
      (offset < s_length &&
       prv_continuation((unsigned char)s_text[offset]))) return false;
  s_cursor = offset;
  s_keep_vertical_column = false;
  return true;
}

bool jot_store_move_cursor(JotCursorMove movement) {
  size_t target = s_cursor;
  switch (movement) {
    case JOT_CURSOR_LEFT:
      target = prv_previous(s_cursor);
      break;
    case JOT_CURSOR_RIGHT:
      target = prv_next(s_cursor);
      break;
    case JOT_CURSOR_HOME:
      target = 0;
      break;
    case JOT_CURSOR_END:
      target = s_length;
      break;
    case JOT_CURSOR_UP: {
      const size_t start = prv_line_start(s_cursor);
      if (start == 0) return false;
      if (!s_keep_vertical_column)
        s_vertical_column = prv_column(start, s_cursor);
      const size_t previous_end = start - 1; // The preceding newline.
      target = prv_to_column(prv_line_start(previous_end),
                             previous_end, s_vertical_column);
      s_keep_vertical_column = true;
      s_cursor = target;
      return true;
    }
    case JOT_CURSOR_DOWN: {
      const size_t end = prv_line_end(s_cursor);
      if (end == s_length) return false;
      if (!s_keep_vertical_column)
        s_vertical_column = prv_column(prv_line_start(s_cursor), s_cursor);
      const size_t next_start = end + 1;
      target = prv_to_column(next_start, prv_line_end(next_start),
                             s_vertical_column);
      s_keep_vertical_column = true;
      s_cursor = target;
      return true;
    }
    default:
      return false;
  }
  s_keep_vertical_column = false;
  if (target == s_cursor) return false;
  s_cursor = target;
  return true;
}

static uint32_t prv_key(int slot, int bank, int part) {
  return JOT_FIRST_KEY + (uint32_t)(slot * 2 + bank) * JOT_BANK_KEYS + part;
}

static uint16_t prv_checksum(const char *text, size_t length) {
  uint16_t value = 0xA5A5u;
  for (size_t i = 0; i < length; ++i)
    value = (uint16_t)((value * 33u) ^ (unsigned char)text[i]);
  return value;
}

static uint32_t prv_directory_checksum(const NoteDirectory *directory) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < sizeof directory->candidates; ++i) {
    hash ^= directory->candidates[i];
    hash *= 16777619u;
  }
  return hash ^ JOT_DIRECTORY_MAGIC;
}

static bool prv_directory_has(int slot) {
  return (s_directory.candidates[slot / 8] &
          (uint8_t)(1u << (slot % 8))) != 0;
}

static bool prv_directory_load(void) {
  if (persist_read_data(JOT_DIRECTORY_KEY, &s_directory,
                        sizeof s_directory) != (int)sizeof s_directory)
    return false;
  return s_directory.magic == JOT_DIRECTORY_MAGIC &&
         s_directory.checksum == prv_directory_checksum(&s_directory);
}

// Only commit a changed in-memory directory after its flash write succeeds.
// A failed directory write must never authorize a new note's header.
static bool prv_directory_save(const NoteDirectory *candidate) {
  NoteDirectory checked = *candidate;
  checked.magic = JOT_DIRECTORY_MAGIC;
  checked.checksum = prv_directory_checksum(&checked);
  if (persist_write_data(JOT_DIRECTORY_KEY, &checked,
                         sizeof checked) != (int)sizeof checked)
    return false;
  s_directory = checked;
  s_directory_valid = true;
  return true;
}

static bool prv_directory_set(int slot) {
  if (s_directory_valid && prv_directory_has(slot)) return true;
  NoteDirectory next = s_directory;
  next.candidates[slot / 8] |= (uint8_t)(1u << (slot % 8));
  return prv_directory_save(&next);
}

static void prv_directory_clear(int slot) {
  if (!s_directory_valid || !prv_directory_has(slot)) return;
  NoteDirectory next = s_directory;
  next.candidates[slot / 8] &= (uint8_t)~(1u << (slot % 8));
  // Failure is safe: a stale candidate incurs two extra reads at launch.
  (void)prv_directory_save(&next);
}

static bool prv_header(int slot, int bank, NoteHeader *header) {
  if (slot < 0 || slot >= JOT_NOTE_CAPACITY || bank < 0 || bank > 1)
    return false;
  memset(header, 0, sizeof *header);
  const int bytes = persist_read_data(prv_key(slot, bank, 0),
                                      header, sizeof *header);
  const bool v2 = bytes == (int)offsetof(NoteHeader, import_hi) &&
                  header->magic == JOT_MAGIC;
  const bool v3 = bytes == (int)sizeof *header &&
                  header->magic == JOT_IMPORT_MAGIC;
  return (v2 || v3) && header->serial != 0 &&
         header->revision != 0 && header->length < JOT_NOTE_TEXT_BYTES;
}

static bool prv_ensure_read_buffer(void) {
  if (!s_read_text) s_read_text = malloc(JOT_NOTE_TEXT_BYTES);
  return s_read_text != NULL;
}

static bool prv_read(int slot, char *out, size_t capacity) {
  if (slot < 0 || slot >= JOT_NOTE_CAPACITY || !s_index[slot].used ||
      s_index[slot].bank < 0) return false;
  const NoteIndex *index = &s_index[slot];
  NoteHeader header;
  if (!prv_header(slot, index->bank, &header) ||
      header.serial != index->serial || header.revision != index->revision ||
      (size_t)header.length + 1 > capacity) return false;
  const size_t bytes = (size_t)header.length + 1;
  for (size_t offset = 0, part = 1; offset < bytes; ++part) {
    const size_t remaining = bytes - offset;
    const size_t chunk = remaining < JOT_PART_BYTES ? remaining : JOT_PART_BYTES;
    if (persist_read_data(prv_key(slot, index->bank, (int)part),
                          out + offset, chunk) != (int)chunk) return false;
    offset += chunk;
  }
  return out[header.length] == '\0' &&
         prv_checksum(out, header.length) == header.checksum;
}

// Purge stale banks before reuse. A failure leaves the committed bank alone.
static bool prv_clear_bank(int slot, int bank) {
  status_t status = persist_delete(prv_key(slot, bank, 0));
  if (status != S_TRUE && status != E_DOES_NOT_EXIST) return false;
  for (int part = 1; part <= JOT_PART_COUNT; ++part) {
    status = persist_delete(prv_key(slot, bank, part));
    if (status != S_TRUE && status != E_DOES_NOT_EXIST) return false;
  }
  return true;
}

// Rare mutations do linear work; every draw/navigation rank lookup is O(1).
static void prv_ranked_insert(int slot) {
  const uint32_t serial = s_index[slot].serial;
  int at = 0;
  while (at < s_ranked_count &&
         s_index[s_ranked_slots[at]].serial > serial) ++at;
  memmove(&s_ranked_slots[at + 1], &s_ranked_slots[at],
          (size_t)(s_ranked_count - at));
  s_ranked_slots[at] = (uint8_t)slot;
  ++s_ranked_count;
}

static void prv_ranked_remove(int slot) {
  for (int at = 0; at < s_ranked_count; ++at) {
    if (s_ranked_slots[at] != slot) continue;
    --s_ranked_count;
    memmove(&s_ranked_slots[at], &s_ranked_slots[at + 1],
            (size_t)(s_ranked_count - at));
    return;
  }
}

void jot_store_init(void) {
  memset(s_index, 0, sizeof s_index);
  s_ranked_count = 0;
  s_next_serial = 1;
  s_active_slot = -1;
  s_length = 0;
  s_cursor = 0;
  s_keep_vertical_column = false;
  s_text[0] = '\0';
  s_dirty = false;
  // Warm boot: one 40-byte directory read plus two header reads for each
  // candidate slot, rather than 512 reads + 16 deletes on EVERY launch.
  // Missing, truncated or corrupt directories trigger a complete legacy
  // scan and rebuild, including pre-directory 1.9-1.15 installations.
  memset(&s_directory, 0, sizeof s_directory);
  s_directory_valid = prv_directory_load();
  if (!s_directory_valid) {
    memset(&s_directory, 0, sizeof s_directory);
    // JOT1 cleanup is only needed during an initial/recovery full scan.
    for (int i = 0; i < JOT_LEGACY_CAPACITY; ++i)
      persist_delete(JOT_LEGACY_FIRST_KEY + i);
  }

  NoteDirectory rebuilt = {0};
  for (int i = 0; i < JOT_NOTE_CAPACITY; ++i) {
    if (s_directory_valid && !prv_directory_has(i)) continue;
    NoteHeader a, b;
    const bool has_a = prv_header(i, 0, &a);
    const bool has_b = prv_header(i, 1, &b);
    if (!has_a && !has_b) continue;
    if (!s_directory_valid)
      rebuilt.candidates[i / 8] |= (uint8_t)(1u << (i % 8));
    // A slot can contain two *different* notes after interrupted cleanup.
    // Serial identifies the note generation; revision only orders its edits.
    const int bank = has_b &&
        (!has_a || b.serial > a.serial ||
         (b.serial == a.serial && b.revision > a.revision)) ? 1 : 0;
    const NoteHeader *header = bank ? &b : &a;
    s_index[i] = (NoteIndex) {
      .used = true, .bank = (int8_t)bank, .serial = header->serial,
      .timestamp = header->timestamp, .revision = header->revision,
      .length = header->length
    };
    prv_ranked_insert(i);
    // Include both generations so a still-present stale header cannot be
    // mistaken for a newer note after slot reuse.
    if (has_a && a.serial >= s_next_serial) s_next_serial = a.serial + 1;
    if (has_b && b.serial >= s_next_serial) s_next_serial = b.serial + 1;
  }
  if (!s_directory_valid) {
    // A directory write failure retains the full-scan recovery path on
    // the next launch. Never treat an uncommitted bitmap as trustworthy.
    (void)prv_directory_save(&rebuilt);
  }
  // A clean restart with zero notes must not recycle an identity that the
  // phone has already seen. A failed read is conservative: use scanned
  // committed notes; write failure on a new note prevents its commit.
  uint32_t high_water = 0;
  if (persist_read_data(JOT_SERIAL_HIGH_WATER_KEY, &high_water,
                        sizeof high_water) == (int)sizeof high_water &&
      high_water < UINT32_MAX && high_water >= s_next_serial)
    s_next_serial = high_water + 1;
  if (!s_next_serial) s_next_serial = 1;
}

void jot_store_deinit(void) {
  free(s_read_text);
  s_read_text = NULL;
}

const char *jot_store_text(void) { return s_text; }
size_t jot_store_length(void) { return s_length; }
bool jot_store_dirty(void) { return s_dirty; }

int jot_store_active_slot(void) { return s_active_slot; }

int jot_store_count(void) { return s_ranked_count; }

static int prv_free_slot(void) {
  for (int i = 0; i < JOT_NOTE_CAPACITY; ++i)
    if (!s_index[i].used) return i;
  return -1;
}

bool jot_store_input(const char *binding) {
  if (!binding || !*binding) return true;
  if (binding[0] == '\b') {
    if (!s_cursor) return true;
    const size_t previous = prv_previous(s_cursor);
    const size_t removed = s_cursor - previous;
    // Include the NUL terminator so deletion works at any point.
    memmove(s_text + previous, s_text + s_cursor,
            s_length - s_cursor + 1);
    s_length -= removed;
    s_cursor = previous;
  } else {
    const size_t added = strlen(binding);
    if (s_length + added >= sizeof s_text) return false;
    if (s_active_slot < 0 && prv_free_slot() < 0) return false;
    // Move the suffix and NUL before copying the inserted characters.
    memmove(s_text + s_cursor + added, s_text + s_cursor,
            s_length - s_cursor + 1);
    memcpy(s_text + s_cursor, binding, added);
    s_length += added;
    s_cursor += added;
  }
  s_keep_vertical_column = false;
  if (s_active_slot < 0 && s_length) {
    s_active_slot = prv_free_slot();
    s_index[s_active_slot] = (NoteIndex) {
      .used = true, .bank = -1, .serial = s_next_serial++, .timestamp = (uint32_t)time(NULL)
    };
    s_active_time = (time_t)s_index[s_active_slot].timestamp;
    prv_ranked_insert(s_active_slot);
  }
  if (s_active_slot >= 0) s_dirty = true;
  return true;
}

bool jot_store_replace_before_cursor(size_t codepoints, const char *replacement) {
  if (!replacement || codepoints < 1 || codepoints > 2) return false;
  size_t start = s_cursor;
  for (size_t n = 0; n < codepoints; ++n) {
    if (!start) return false;
    start = prv_previous(start);
  }
  const size_t removed = s_cursor - start;
  const size_t added = strlen(replacement);
  if (added == removed && memcmp(s_text + start, replacement, added) == 0)
    return false;
  if (s_length - removed + added >= sizeof s_text) return false;
  if (s_active_slot < 0 && prv_free_slot() < 0) return false;

  // The note is unchanged unless the *entire* replacement fits.
  memmove(s_text + start + added, s_text + s_cursor,
          s_length - s_cursor + 1); // Including the NUL.
  memcpy(s_text + start, replacement, added);
  s_length = s_length - removed + added;
  s_cursor = start + added;
  s_keep_vertical_column = false;
  if (s_active_slot < 0 && s_length) {
    s_active_slot = prv_free_slot();
    s_index[s_active_slot] = (NoteIndex){
      .used = true, .bank = -1, .serial = s_next_serial++,
      .timestamp = (uint32_t)time(NULL)
    };
    s_active_time = (time_t)s_index[s_active_slot].timestamp;
    prv_ranked_insert(s_active_slot);
  }
  if (s_active_slot >= 0) s_dirty = true;
  return true;
}

bool jot_store_delete_forward(void) {
  if (s_cursor >= s_length) return false;
  // Advance by one whole UTF-8 codepoint, including newlines.
  const size_t next = prv_next(s_cursor);
  memmove(s_text + s_cursor, s_text + next, s_length - next + 1);
  s_length -= next - s_cursor;
  s_keep_vertical_column = false;
  // A no-op at end never changes the note or marks it dirty.
  if (s_active_slot >= 0) s_dirty = true;
  return true;
}

bool jot_store_flush(void) {
  if (!s_dirty) return true;
  if (s_active_slot < 0) return false;
  NoteIndex *index = &s_index[s_active_slot];
  const int previous_bank = index->bank;
  const int target_bank = previous_bank == 0 ? 1 : 0;
  // The serial high-water mark MUST be durable before the first header.
  // Otherwise deletion of the last note followed by restart can recycle a
  // serial already synced to the independent phone notebook.
  if (previous_bank < 0 &&
      persist_write_data(JOT_SERIAL_HIGH_WATER_KEY, &index->serial,
                         sizeof index->serial) != (int)sizeof index->serial)
    return false;
  // Editing must preserve a phone import receipt already committed in the
  // active bank, without allocating 8 extra bytes in every note index.
  NoteHeader old_header = {0};
  if (previous_bank >= 0 &&
      !prv_header(s_active_slot, previous_bank, &old_header)) return false;
  // Candidate-slot bit MUST be durable before the first bank header.
  // On failure, retain the entire dirty draft and do not publish a note.
  // Existing notes are already represented, so their next bank can
  // commit without a redundant directory write.
  if (!prv_directory_set(s_active_slot)) return false;
  if (!prv_clear_bank(s_active_slot, target_bank)) return false;
  const size_t bytes = s_length + 1;
  for (size_t offset = 0, part = 1; offset < bytes; ++part) {
    const size_t remaining = bytes - offset;
    const size_t chunk = remaining < JOT_PART_BYTES ? remaining : JOT_PART_BYTES;
    const int written = persist_write_data(
        prv_key(s_active_slot, target_bank, (int)part), s_text + offset, chunk);
    if (written != (int)chunk) return false;
    offset += chunk;
  }
  NoteHeader header = {
    .magic = JOT_IMPORT_MAGIC, .serial = index->serial,
    .timestamp = (uint32_t)s_active_time,
    .revision = index->revision + 1,
    .length = (uint16_t)s_length,
    .checksum = prv_checksum(s_text, s_length),
    .import_hi = old_header.import_hi, .import_lo = old_header.import_lo
  };
  if (!header.revision) header.revision = 1;
  if (persist_write_data(prv_key(s_active_slot, target_bank, 0),
                         &header, sizeof header) != (int)sizeof header)
    return false;
  index->bank = (int8_t)target_bank;
  index->revision = header.revision;
  index->length = header.length;
  s_dirty = false;
  if (previous_bank >= 0)
    prv_clear_bank(s_active_slot, previous_bank); // Best-effort GC.
  return true;
}

bool jot_store_new(void) {
  if (!jot_store_flush()) return false;
  s_active_slot = -1;
  s_length = 0;
  s_cursor = 0;
  s_keep_vertical_column = false;
  s_text[0] = '\0';
  return true;
}

bool jot_store_open(int slot) {
  if (slot < 0 || slot >= JOT_NOTE_CAPACITY || !s_index[slot].used) return false;
  if (!jot_store_flush()) return false;
  if (!prv_ensure_read_buffer() ||
      !prv_read(slot, s_read_text, JOT_NOTE_TEXT_BYTES)) return false;
  s_active_slot = slot;
  s_active_time = (time_t)s_index[slot].timestamp;
  s_length = s_index[slot].length;
  s_cursor = s_length;
  s_keep_vertical_column = false;
  memcpy(s_text, s_read_text, s_length + 1);
  s_dirty = false;
  return true;
}

bool jot_store_delete(int slot) {
  if (slot < 0 || slot >= JOT_NOTE_CAPACITY || !s_index[slot].used) return false;
  if (!jot_store_flush()) return false;
  // The inactive bank may still contain an older committed revision if
  // best-effort GC failed during a previous save. Invalidate that header
  // FIRST; only removing the active header can then commit the deletion.
  // This gives recovery the same guarantee as a tombstone without another
  // persistent record: a successfully deleted slot has no valid headers.
  const int bank = s_index[slot].bank;
  if (bank < 0) return false;
  const int other_bank = 1 - bank;
  const status_t stale_status = persist_delete(prv_key(slot, other_bank, 0));
  if (stale_status != S_TRUE && stale_status != E_DOES_NOT_EXIST)
    return false;
  if (persist_delete(prv_key(slot, bank, 0)) != S_TRUE)
    return false;
  prv_ranked_remove(slot);
  s_index[slot] = (NoteIndex) {0};
  const bool cleared_other = prv_clear_bank(slot, other_bank);
  const bool cleared_active = prv_clear_bank(slot, bank);
  // Only remove the candidate when BOTH banks are certainly gone.
  // Any cleanup error leaves a conservative bit and thus full recovery.
  if (cleared_other && cleared_active) prv_directory_clear(slot);
  if (s_active_slot == slot) {
    s_active_slot = -1;
    s_length = 0;
    s_cursor = 0;
    s_keep_vertical_column = false;
    s_text[0] = '\0';
    s_dirty = false;
  }
  return true;
}

int jot_store_slot_at(int rank) {
  return rank < 0 || rank >= s_ranked_count ? -1 : s_ranked_slots[rank];
}

bool jot_store_preview(int slot, char *out, size_t capacity, time_t *created) {
  if (!out || capacity == 0 || slot < 0 || slot >= JOT_NOTE_CAPACITY ||
      !s_index[slot].used) return false;
  const char *text = s_text;
  if (slot != s_active_slot) {
    if (!prv_ensure_read_buffer() ||
        !prv_read(slot, s_read_text, JOT_NOTE_TEXT_BYTES)) return false;
    text = s_read_text;
  }
  size_t n = 0;
  while (text[n] && n + 1 < capacity) ++n;
  while (n && (((unsigned char)text[n] & 0xC0) == 0x80)) --n;
  memcpy(out, text, n);
  out[n] = '\0';
  if (created) *created = (time_t)s_index[slot].timestamp;
  return true;
}

bool jot_store_identity(int slot, uint32_t *serial, uint32_t *revision) {
  if (slot < 0 || slot >= JOT_NOTE_CAPACITY || !s_index[slot].used)
    return false;
  if (serial) *serial = s_index[slot].serial;
  if (revision) *revision = s_index[slot].revision;
  return true;
}

bool jot_store_export(int slot, char *out, size_t capacity, time_t *created) {
  if (!out || capacity == 0 || slot < 0 || slot >= JOT_NOTE_CAPACITY ||
      !s_index[slot].used) return false;
  const size_t length = slot == s_active_slot ? s_length : s_index[slot].length;
  if (length + 1 > capacity) return false;
  if (slot == s_active_slot) memcpy(out, s_text, length + 1);
  else if (!prv_read(slot, out, capacity)) return false;
  if (created) *created = (time_t)s_index[slot].timestamp;
  return true;
}

// Full scalar validation: no overlong sequences, invalid continuation
// bytes, surrogates, embedded NUL or >U+10FFFF.
static bool prv_valid_import_utf8(const char *s, size_t n) {
  if (!s || !n || n >= JOT_NOTE_TEXT_BYTES) return false;
  for (size_t i = 0; i < n;) {
    const uint8_t a = (uint8_t)s[i++];
    if (a >= 1 && a < 0x80) continue;
    if (a < 0xC2 || a > 0xF4) return false;
    const int extras = a < 0xE0 ? 1 : a < 0xF0 ? 2 : 3;
    if (n - i < (size_t)extras) return false;
    for (int j = 0; j < extras; ++j) {
      const uint8_t b = (uint8_t)s[i + (size_t)j];
      if ((b & 0xC0) != 0x80) return false;
      if (j == 0 &&
          ((a == 0xE0 && b < 0xA0) ||
           (a == 0xED && b >= 0xA0) ||
           (a == 0xF0 && b < 0x90) ||
           (a == 0xF4 && b > 0x8F))) return false;
    }
    i += (size_t)extras;
  }
  return true;
}

bool jot_store_import_lookup(uint32_t hi, uint32_t lo,
                             JotImportReceipt *receipt) {
  if ((!hi && !lo) || !receipt) return false;
  for (int i = 0; i < JOT_NOTE_CAPACITY; ++i) {
    const NoteIndex *n = &s_index[i];
    if (!n->used || n->bank < 0) continue;
    NoteHeader header;
    if (!prv_header(i, n->bank, &header) ||
        header.serial != n->serial || header.revision != n->revision ||
        header.import_hi != hi || header.import_lo != lo) continue;
    *receipt = (JotImportReceipt){
      .serial = n->serial, .created = n->timestamp, .revision = n->revision
    };
    return true;
  }
  return false;
}

// Write an independent new record without touching editor state. The final
// bank header holds BOTH content checksum and import ID atomically.
JotImportResult jot_store_import_new(const char *text, size_t length,
                                     uint32_t hi, uint32_t lo,
                                     JotImportReceipt *receipt) {
  if (!receipt || (!hi && !lo) || !prv_valid_import_utf8(text, length) ||
      text[length] != '\0') return JOT_IMPORT_INVALID;
  if (jot_store_import_lookup(hi, lo, receipt)) return JOT_IMPORT_ALREADY;
  if (s_dirty) return JOT_IMPORT_BUSY;
  const int slot = prv_free_slot();
  if (slot < 0 || !s_next_serial || s_next_serial == UINT32_MAX)
    return JOT_IMPORT_FULL;
  const uint32_t serial = s_next_serial;
  if (persist_write_data(JOT_SERIAL_HIGH_WATER_KEY, &serial,
                         sizeof serial) != (int)sizeof serial)
    return JOT_IMPORT_FAILED;
  ++s_next_serial; // Burn the serial even on an interrupted import.
  if (!prv_directory_set(slot) ||
      !prv_clear_bank(slot, 0) || !prv_clear_bank(slot, 1))
    return JOT_IMPORT_FAILED;
  for (size_t offset = 0, part = 1; offset < length + 1; ++part) {
    const size_t remaining = length + 1 - offset;
    const size_t chunk = remaining < JOT_PART_BYTES ?
                         remaining : JOT_PART_BYTES;
    if (persist_write_data(prv_key(slot, 0, (int)part),
                           text + offset, chunk) != (int)chunk)
      return JOT_IMPORT_FAILED;
    offset += chunk;
  }
  const uint32_t created = (uint32_t)time(NULL);
  const NoteHeader header = {
    .magic = JOT_IMPORT_MAGIC, .serial = serial, .timestamp = created,
    .revision = 1, .length = (uint16_t)length,
    .checksum = prv_checksum(text, length),
    .import_hi = hi, .import_lo = lo
  };
  if (persist_write_data(prv_key(slot, 0, 0), &header,
                         sizeof header) != (int)sizeof header)
    return JOT_IMPORT_FAILED;
  s_index[slot] = (NoteIndex) {
    .used = true, .bank = 0, .serial = serial,
    .timestamp = created, .revision = 1, .length = (uint16_t)length
  };
  prv_ranked_insert(slot);
  *receipt = (JotImportReceipt){
    .serial = serial, .created = created, .revision = 1
  };
  return JOT_IMPORT_SAVED;
}

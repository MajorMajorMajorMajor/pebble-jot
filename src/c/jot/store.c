#include <pebble.h>
#include "jot/store.h"

// JOT2 uses two banks per slot: a complete new version is written into
// the inactive bank before its header is committed. No legacy migration:
// this release intentionally resets the QA watch's JOT1 records.
#define JOT_MAGIC 0x4A4F5432u
#define JOT_FIRST_KEY 5000u
#define JOT_LEGACY_FIRST_KEY 4100u
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
} NoteHeader;

typedef struct {
  bool used;
  int8_t bank;   // -1 until the first successful flush.
  uint32_t serial;
  uint32_t timestamp;
  uint32_t revision;
  uint16_t length;
} NoteIndex;

static NoteIndex s_index[JOT_NOTE_CAPACITY];
static char s_read_text[JOT_NOTE_TEXT_BYTES];
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

static bool prv_header(int slot, int bank, NoteHeader *header) {
  if (slot < 0 || slot >= JOT_NOTE_CAPACITY || bank < 0 || bank > 1)
    return false;
  if (persist_read_data(prv_key(slot, bank, 0), header, sizeof *header) !=
      (int)sizeof *header) return false;
  return header->magic == JOT_MAGIC && header->serial != 0 &&
         header->revision != 0 && header->length < JOT_NOTE_TEXT_BYTES;
}

static bool prv_read(int slot, char *out, size_t capacity) {
  if (slot < 0 || slot >= JOT_NOTE_CAPACITY || !s_index[slot].used ||
      s_index[slot].bank < 0) return false;
  const NoteIndex *index = &s_index[slot];
  NoteHeader header;
  if (!prv_header(slot, index->bank, &header) ||
      header.serial != index->serial || header.revision != index->revision ||
      header.length + 1 > capacity) return false;
  const size_t bytes = (size_t)header.length + 1;
  for (size_t offset = 0, part = 1; offset < bytes; ++part) {
    const size_t remaining = bytes - offset;
    const size_t chunk = remaining < JOT_PART_BYTES ? remaining : JOT_PART_BYTES;
    if (persist_read_data(prv_key(slot, index->bank, (int)part),
                          out + offset, chunk) != (int)chunk) return false;
    offset += chunk;
  }
  return out[header.length] == '\\0' &&
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

void jot_store_init(void) {
  memset(s_index, 0, sizeof s_index);
  s_next_serial = 1;
  s_active_slot = -1;
  s_length = 0;
  s_cursor = 0;
  s_keep_vertical_column = false;
  s_text[0] = '\\0';
  s_dirty = false;
  // JOT1 keys are not readable by JOT2. Reclaim them on the QA watch.
  for (int i = 0; i < JOT_LEGACY_CAPACITY; ++i)
    persist_delete(JOT_LEGACY_FIRST_KEY + i);
  for (int i = 0; i < JOT_NOTE_CAPACITY; ++i) {
    NoteHeader a, b;
    const bool has_a = prv_header(i, 0, &a);
    const bool has_b = prv_header(i, 1, &b);
    if (!has_a && !has_b) continue;
    const int bank = has_b && (!has_a || b.revision > a.revision) ? 1 : 0;
    const NoteHeader *header = bank ? &b : &a;
    s_index[i] = (NoteIndex) {
      .used = true, .bank = (int8_t)bank, .serial = header->serial,
      .timestamp = header->timestamp, .revision = header->revision,
      .length = header->length
    };
    if (header->serial >= s_next_serial) s_next_serial = header->serial + 1;
  }
  if (!s_next_serial) s_next_serial = 1;
}

const char *jot_store_text(void) { return s_text; }
size_t jot_store_length(void) { return s_length; }
bool jot_store_dirty(void) { return s_dirty; }

int jot_store_active_slot(void) { return s_active_slot; }

int jot_store_count(void) {
  int n = 0;
  for (int i = 0; i < JOT_NOTE_CAPACITY; ++i) if (s_index[i].used) ++n;
  return n;
}

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
    .magic = JOT_MAGIC, .serial = index->serial,
    .timestamp = (uint32_t)s_active_time,
    .revision = index->revision + 1,
    .length = (uint16_t)s_length,
    .checksum = prv_checksum(s_text, s_length)
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
  if (!prv_read(slot, s_read_text, sizeof s_read_text)) return false;
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
  // Commit logical deletion by removing the active header first. A
  // failed delete never changes the index; chunk GC is best-effort.
  const int bank = s_index[slot].bank;
  if (bank < 0 || persist_delete(prv_key(slot, bank, 0)) != S_TRUE)
    return false;
  s_index[slot] = (NoteIndex) {0};
  prv_clear_bank(slot, 1 - bank);
  prv_clear_bank(slot, bank);
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
  if (rank < 0 || rank >= jot_store_count()) return -1;
  uint32_t cutoff = UINT32_MAX;
  for (int step = 0; step <= rank; ++step) {
    int best = -1;
    uint32_t highest = 0;
    for (int i = 0; i < JOT_NOTE_CAPACITY; ++i) {
      uint32_t serial = s_index[i].serial;
      if (s_index[i].used && serial < cutoff && serial >= highest) {
        highest = serial;
        best = i;
      }
    }
    if (best < 0) return -1;
    if (step == rank) return best;
    cutoff = highest;
  }
  return -1;
}

bool jot_store_preview(int slot, char *out, size_t capacity, time_t *created) {
  if (!out || capacity == 0 || slot < 0 || slot >= JOT_NOTE_CAPACITY ||
      !s_index[slot].used) return false;
  const char *text = s_text;
  if (slot != s_active_slot) {
    if (!prv_read(slot, s_read_text, sizeof s_read_text)) return false;
    text = s_read_text;
  }
  size_t n = 0;
  while (text[n] && n + 1 < capacity) ++n;
  while (n && (((unsigned char)text[n] & 0xC0) == 0x80)) --n;
  memcpy(out, text, n);
  out[n] = '\\0';
  if (created) *created = (time_t)s_index[slot].timestamp;
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

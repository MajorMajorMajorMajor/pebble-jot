#include <pebble.h>
#include "jot/store.h"

#define JOT_MAGIC 0x4A4F5431u  // "JOT1"
#define JOT_FIRST_KEY 4100u

typedef struct {
  uint32_t magic;
  uint32_t serial;
  uint32_t timestamp;
  uint16_t length;
  char text[JOT_NOTE_TEXT_BYTES];
} NoteRecord;

typedef struct {
  bool used;
  uint32_t serial;
  uint32_t timestamp;
} NoteIndex;

static NoteIndex s_index[JOT_NOTE_CAPACITY];
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

static bool prv_read(int slot, NoteRecord *record) {
  if (slot < 0 || slot >= JOT_NOTE_CAPACITY) return false;
  if (persist_read_data(JOT_FIRST_KEY + slot, record, sizeof *record) !=
      (int)sizeof *record) return false;
  return record->magic == JOT_MAGIC && record->serial != 0 &&
         record->length < JOT_NOTE_TEXT_BYTES &&
         record->text[record->length] == '\0';
}

void jot_store_init(void) {
  memset(s_index, 0, sizeof s_index);
  s_next_serial = 1;
  s_active_slot = -1;
  s_length = 0;
  s_cursor = 0;
  s_keep_vertical_column = false;
  s_text[0] = '\0';
  s_dirty = false;
  for (int i = 0; i < JOT_NOTE_CAPACITY; ++i) {
    NoteRecord record;
    if (!prv_read(i, &record)) continue;
    s_index[i] = (NoteIndex) {
      .used = true, .serial = record.serial, .timestamp = record.timestamp
    };
    if (record.serial >= s_next_serial) s_next_serial = record.serial + 1;
  }
  if (!s_next_serial) s_next_serial = 1; // Handle counter rollover.
}

const char *jot_store_text(void) { return s_text; }
size_t jot_store_length(void) { return s_length; }
bool jot_store_dirty(void) { return s_dirty; }

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
      .used = true, .serial = s_next_serial++, .timestamp = (uint32_t)time(NULL)
    };
    s_active_time = (time_t)s_index[s_active_slot].timestamp;
  }
  if (s_active_slot >= 0) s_dirty = true;
  return true;
}

bool jot_store_flush(void) {
  if (!s_dirty) return true;
  if (s_active_slot < 0) return false;
  NoteRecord record = {
    .magic = JOT_MAGIC,
    .serial = s_index[s_active_slot].serial,
    .timestamp = (uint32_t)s_active_time,
    .length = (uint16_t)s_length,
  };
  memcpy(record.text, s_text, s_length + 1);
  const int written = persist_write_data(JOT_FIRST_KEY + s_active_slot,
                                         &record, sizeof record);
  if (written != (int)sizeof record) return false;
  s_dirty = false;
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
  NoteRecord record;
  if (!prv_read(slot, &record)) return false;
  s_active_slot = slot;
  s_active_time = (time_t)record.timestamp;
  s_length = record.length;
  s_cursor = s_length;
  s_keep_vertical_column = false;
  memcpy(s_text, record.text, s_length + 1);
  s_dirty = false;
  return true;
}

bool jot_store_delete(int slot) {
  if (slot < 0 || slot >= JOT_NOTE_CAPACITY || !s_index[slot].used) return false;
  if (!jot_store_flush()) return false;
  // Pebble's persist_delete() returns S_TRUE (not S_SUCCESS) on success.
  // An incorrect check leaves a ghost index entry after the watch has
  // already deleted the record, and can poison subsequent editing.
  if (persist_delete(JOT_FIRST_KEY + slot) != S_TRUE) return false;
  s_index[slot] = (NoteIndex) {0};
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
  if (capacity == 0 || slot < 0 || slot >= JOT_NOTE_CAPACITY ||
      !s_index[slot].used) return false;
  const char *text;
  NoteRecord record;
  if (slot == s_active_slot) text = s_text;
  else {
    if (!prv_read(slot, &record)) return false;
    text = record.text;
  }
  size_t n = 0;
  while (text[n] && text[n] != '\n' && n + 1 < capacity) ++n;
  // Never end a string in the middle of a UTF-8 codepoint.
  while (n && (((unsigned char)text[n] & 0xC0) == 0x80)) --n;
  memcpy(out, text, n);
  out[n] = '\0';
  if (created) *created = (time_t)s_index[slot].timestamp;
  return true;
}

bool jot_store_export(int slot, char *out, size_t capacity, time_t *created) {
  if (!out || capacity == 0 || slot < 0 || slot >= JOT_NOTE_CAPACITY ||
      !s_index[slot].used) return false;

  NoteRecord record;
  const char *text = s_text;
  size_t length = s_length;
  if (slot != s_active_slot) {
    if (!prv_read(slot, &record)) return false;
    text = record.text;
    length = record.length;
  }

  if (length + 1 > capacity) return false;
  memcpy(out, text, length + 1);
  if (created) *created = (time_t)s_index[slot].timestamp;
  return true;
}

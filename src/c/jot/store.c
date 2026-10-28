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
static bool s_dirty;

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
    if (!s_length) return true;
    do { --s_length; } while (s_length &&
      (((unsigned char)s_text[s_length] & 0xC0) == 0x80));
    s_text[s_length] = '\0';
  } else {
    size_t added = strlen(binding);
    if (s_length + added >= sizeof s_text) return false;
    if (s_active_slot < 0 && prv_free_slot() < 0) return false;
    memcpy(s_text + s_length, binding, added + 1);
    s_length += added;
  }
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
  memcpy(s_text, record.text, s_length + 1);
  s_dirty = false;
  return true;
}

bool jot_store_delete(int slot) {
  if (slot < 0 || slot >= JOT_NOTE_CAPACITY || !s_index[slot].used) return false;
  if (!jot_store_flush()) return false;
  if (persist_delete(JOT_FIRST_KEY + slot) != S_SUCCESS) return false;
  s_index[slot] = (NoteIndex) {0};
  if (s_active_slot == slot) {
    s_active_slot = -1;
    s_length = 0;
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

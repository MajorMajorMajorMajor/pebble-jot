#include <pebble.h>
#include <stdlib.h>
#include "jot/backup.h"
#include "jot/jot.h"
#include "jot/store.h"
#include "message_keys.auto.h"

enum {
  BACKUP_BEGIN = 1, BACKUP_NOTE = 2, BACKUP_END = 3,
  BACKUP_MAX_ATTEMPTS = 4, BACKUP_ACK_TIMEOUT_MS = 4000,
  // 240 bytes leaves room for AppMessage dictionary headers in a 384B outbox.
  BACKUP_CHUNK_BYTES = 240,
  IMPORT_BEGIN = 1, IMPORT_CHUNK = 2, IMPORT_COMMIT = 3,
  IMPORT_CHUNK_BYTES = 160,
  IMPORT_COMMIT_ACK = -2,
  BACKUP_FULL = 0, BACKUP_ONE = 1, BACKUP_DELETED = 2
};

static bool s_available;
static bool s_active;
static bool s_connected;
static bool s_user_started;
static bool s_auto_full;
static uint8_t s_auto_queued_kind; // NONE=0, ONE=1, DELETED=2.
static int s_auto_queued_slot;
static uint32_t s_auto_queued_serial;
static AppTimer *s_auto_timer;
static uint8_t s_mode;
static int s_auto_slot;
static uint32_t s_deleted_serial;
static uint32_t s_session;
static int s_total;
static int s_phase;  // 0=BEGIN, 1..total=NOTE, total+1=END.
static uint8_t s_attempts;
static AppTimer *s_retry_timer;
static char s_status[36];
static char *s_note; // Allocated for the duration of a backup only.
static size_t s_note_length;
static time_t s_created;
static uint32_t s_serial;
static uint32_t s_revision;
static size_t s_offset;
static size_t s_chunk_length;
static int16_t s_part;
static bool s_last_chunk;

// Incoming phone-to-watch transfer. Never modifies s_text or the editor.
static char *s_import_text; // Heap-owned only during an active transfer.
static uint32_t s_import_hi, s_import_lo;
static uint16_t s_import_total, s_import_offset, s_import_previous_offset;
static uint16_t s_import_previous_length;
static int16_t s_import_part;
static bool s_import_ready;

static void prv_cancel_timer(void) {
  if (s_retry_timer) {
    app_timer_cancel(s_retry_timer);
    s_retry_timer = NULL;
  }
}

static void prv_status(const char *status) {
  snprintf(s_status, sizeof s_status, "%s", status);
  jot_backup_ui_changed();
}

static void prv_fail(const char *reason) {
  prv_cancel_timer();
  s_active = false;
  free(s_note);
  s_note = NULL;
  s_auto_full = true; // Ambiguous outcome requires a complete reconciliation.
  s_auto_queued_kind = 0;
  if (s_user_started) prv_status(reason);
}

static void prv_send_current(void *context);
static void prv_auto_kick(void *context);

static void prv_auto_schedule(uint32_t delay_ms) {
  if (!s_available || !s_connected || s_active ||
      (!s_auto_full && !s_auto_queued_kind)) return;
  if (s_auto_timer) app_timer_cancel(s_auto_timer);
  s_auto_timer = app_timer_register(delay_ms, prv_auto_kick, NULL);
}

void jot_backup_note_committed(int slot) {
  if (slot < 0 || slot >= JOT_NOTE_CAPACITY) return;
  if (!s_available || !s_connected || s_active ||
      s_auto_queued_kind == BACKUP_DELETED)
    s_auto_full = true;
  else if (s_auto_queued_kind == BACKUP_ONE &&
           s_auto_queued_slot != slot)
    s_auto_full = true;
  else if (!s_auto_full) {
    s_auto_queued_kind = BACKUP_ONE;
    s_auto_queued_slot = slot;
  }
  prv_auto_schedule(850); // Coalesce saves off the editor hot path.
}

void jot_backup_note_deleted(uint32_t serial) {
  if (!serial) return;
  if (!s_available || !s_connected || s_active || s_auto_queued_kind)
    s_auto_full = true;
  else if (!s_auto_full) {
    s_auto_queued_kind = BACKUP_DELETED;
    s_auto_queued_serial = serial;
  }
  prv_auto_schedule(850);
}

static void prv_schedule(uint32_t delay_ms) {
  prv_cancel_timer();
  s_retry_timer = app_timer_register(delay_ms, prv_send_current, NULL);
  if (!s_retry_timer) prv_fail("Backup timer failed");
}

static bool prv_load_note(void) {
  const int slot = s_mode == BACKUP_ONE ?
      s_auto_slot : jot_store_slot_at(s_phase - 1);
  if (slot < 0 || !jot_store_export(slot, s_note, JOT_NOTE_TEXT_BYTES, &s_created) ||
      !jot_store_identity(slot, &s_serial, &s_revision))
    return false;
  s_note_length = strlen(s_note);
  return true;
}

static void prv_send_current(void *context) {
  (void)context;
  s_retry_timer = NULL;
  if (!s_active) return;
  if (s_attempts >= BACKUP_MAX_ATTEMPTS) {
    prv_fail("Phone not responding");
    return;
  }
  ++s_attempts;

  DictionaryIterator *iter = NULL;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK || !iter) {
    prv_schedule(500);
    return;
  }

  const int16_t index = (int16_t)(s_phase - 1);
  const int8_t kind = s_phase == 0 ? BACKUP_BEGIN :
                      s_phase == s_total + 1 ? BACKUP_END : BACKUP_NOTE;
  DictionaryResult written = dict_write_int8(iter, MESSAGE_KEY_BackupKind, kind);
  if (written == DICT_OK)
    written = dict_write_uint32(iter, MESSAGE_KEY_BackupSession, s_session);
  if (written == DICT_OK)
    written = dict_write_int16(iter, MESSAGE_KEY_BackupIndex, index);
  if (written == DICT_OK)
    written = dict_write_int16(iter, MESSAGE_KEY_BackupPart, s_part);

  if (kind == BACKUP_BEGIN && written == DICT_OK) {
    written = dict_write_int16(iter, MESSAGE_KEY_BackupTotal, (int16_t)s_total);
    if (written == DICT_OK)
      written = dict_write_uint8(iter, MESSAGE_KEY_BackupMode, s_mode);
    if (written == DICT_OK && s_mode == BACKUP_DELETED)
      written = dict_write_uint32(iter, MESSAGE_KEY_BackupSerial,
                                  s_deleted_serial);
  } else if (kind == BACKUP_NOTE && written == DICT_OK) {
    if (s_part == 0 && s_offset == 0 && !prv_load_note()) {
      prv_fail("Cannot read note");
      return;
    }
    s_chunk_length = s_note_length - s_offset;
    if (s_chunk_length > BACKUP_CHUNK_BYTES)
      s_chunk_length = BACKUP_CHUNK_BYTES;
    // Each packet is a valid UTF-8 string; no truncated codepoint.
    while (s_chunk_length && s_offset + s_chunk_length < s_note_length &&
           ((unsigned char)s_note[s_offset + s_chunk_length] & 0xC0) == 0x80)
      --s_chunk_length;
    s_last_chunk = s_offset + s_chunk_length == s_note_length;
    char chunk[BACKUP_CHUNK_BYTES + 1];
    memcpy(chunk, s_note + s_offset, s_chunk_length);
    chunk[s_chunk_length] = '\0';
    written = dict_write_uint32(iter, MESSAGE_KEY_BackupTimestamp,
                                (uint32_t)s_created);
    if (written == DICT_OK)
      written = dict_write_uint32(iter, MESSAGE_KEY_BackupSerial, s_serial);
    if (written == DICT_OK)
      written = dict_write_uint32(iter, MESSAGE_KEY_BackupRevision, s_revision);
    if (written == DICT_OK)
      written = dict_write_int8(iter, MESSAGE_KEY_BackupFinal,
                                s_last_chunk ? 1 : 0);
    if (written == DICT_OK)
      written = dict_write_cstring(iter, MESSAGE_KEY_BackupText, chunk);
  }
  if (written != DICT_OK) {
    prv_fail("Message too large");
    return;
  }

  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    prv_schedule(500);
    return;
  }
  prv_schedule(BACKUP_ACK_TIMEOUT_MS);
}


static void prv_import_ack(uint32_t hi, uint32_t lo, int16_t part,
                           int8_t status, const JotImportReceipt *saved) {
  DictionaryIterator *iter = NULL;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK || !iter) return;
  DictionaryResult d = dict_write_uint32(iter, MESSAGE_KEY_ImportTokenHi, hi);
  if (d == DICT_OK) d = dict_write_uint32(iter, MESSAGE_KEY_ImportTokenLo, lo);
  if (d == DICT_OK) d = dict_write_int16(iter, MESSAGE_KEY_ImportAck, part);
  if (d == DICT_OK) d = dict_write_int8(iter, MESSAGE_KEY_ImportStatus, status);
  if (d == DICT_OK && saved) {
    d = dict_write_uint32(iter, MESSAGE_KEY_ImportSerial, saved->serial);
    if (d == DICT_OK)
      d = dict_write_uint32(iter, MESSAGE_KEY_ImportCreated, saved->created);
    if (d == DICT_OK)
      d = dict_write_uint32(iter, MESSAGE_KEY_ImportRevision, saved->revision);
  }
  if (d == DICT_OK) (void)app_message_outbox_send();
}

static void prv_import_receive(DictionaryIterator *iter) {
  Tuple *kind = dict_find(iter, MESSAGE_KEY_ImportKind);
  Tuple *high = dict_find(iter, MESSAGE_KEY_ImportTokenHi);
  Tuple *low = dict_find(iter, MESSAGE_KEY_ImportTokenLo);
  Tuple *part = dict_find(iter, MESSAGE_KEY_ImportPart);
  if (!kind || !high || !low || !part) return;
  const uint32_t hi = high->value->uint32, lo = low->value->uint32;
  const int16_t seq = (int16_t)part->value->int32;
  const int which = kind->value->int32;
  if (!hi && !lo) return;
  JotImportReceipt receipt;
  // An already-persisted transfer is acknowledged before any buffer check.
  // This includes retries after a lost commit acknowledgment or app restart.
  // Only look up the persisted receipt at boundaries or for a stale
  // packet. Normal sequential chunks already belong to this RAM session;
  // avoid hundreds of flash-header reads for every 160-byte packet.
  if ((which != IMPORT_CHUNK || !s_import_ready ||
       hi != s_import_hi || lo != s_import_lo) &&
      jot_store_import_lookup(hi, lo, &receipt)) {
    prv_import_ack(hi, lo, seq, JOT_IMPORT_ALREADY, &receipt);
    return;
  }
  if (s_active) {
    prv_import_ack(hi, lo, seq, JOT_IMPORT_BUSY, NULL);
    return;
  }
  if (which == IMPORT_BEGIN) {
    Tuple *total = dict_find(iter, MESSAGE_KEY_ImportTotal);
    const int len = total ? total->value->int32 : -1;
    if (seq != -1 || len <= 0 || len >= JOT_NOTE_TEXT_BYTES) {
      prv_import_ack(hi, lo, seq, JOT_IMPORT_INVALID, NULL);
      return;
    }
    if (jot_store_dirty()) {
      prv_import_ack(hi, lo, seq, JOT_IMPORT_BUSY, NULL);
      return;
    }
    if (jot_store_count() >= JOT_NOTE_CAPACITY) {
      prv_import_ack(hi, lo, seq, JOT_IMPORT_FULL, NULL);
      return;
    }
    if (!s_import_text) s_import_text = malloc(JOT_NOTE_TEXT_BYTES);
    if (!s_import_text) {
      prv_import_ack(hi, lo, seq, JOT_IMPORT_FAILED, NULL);
      return;
    }
    s_import_hi = hi;
    s_import_lo = lo;
    s_import_total = (uint16_t)len;
    s_import_offset = 0;
    s_import_part = 0;
    s_import_previous_offset = 0;
    s_import_previous_length = 0;
    s_import_text[0] = '\0';
    s_import_ready = true;
    prv_import_ack(hi, lo, seq, JOT_IMPORT_SAVED, NULL);
    return;
  }
  if (!s_import_ready || !s_import_text ||
      hi != s_import_hi || lo != s_import_lo) {
    prv_import_ack(hi, lo, seq, JOT_IMPORT_INVALID, NULL);
    return;
  }
  if (which == IMPORT_CHUNK) {
    Tuple *text = dict_find(iter, MESSAGE_KEY_ImportText);
    if (!text || !text->length ||
        text->length > IMPORT_CHUNK_BYTES + 1 ||
        text->value->cstring[text->length - 1] != '\0') {
      prv_import_ack(hi, lo, seq, JOT_IMPORT_INVALID, NULL);
      return;
    }
    const size_t len = strlen(text->value->cstring);
    if (len == 0 || len + 1 != text->length) {
      prv_import_ack(hi, lo, seq, JOT_IMPORT_INVALID, NULL);
      return;
    }
    if (seq == s_import_part - 1 &&
        len == s_import_previous_length &&
        memcmp(s_import_text + s_import_previous_offset,
               text->value->cstring, len) == 0) {
      prv_import_ack(hi, lo, seq, JOT_IMPORT_SAVED, NULL);
      return;
    }
    if (seq != s_import_part || s_import_offset + len > s_import_total) {
      prv_import_ack(hi, lo, seq, JOT_IMPORT_INVALID, NULL);
      return;
    }
    s_import_previous_offset = s_import_offset;
    s_import_previous_length = (uint16_t)len;
    memcpy(s_import_text + s_import_offset, text->value->cstring, len);
    s_import_offset += (uint16_t)len;
    s_import_text[s_import_offset] = '\0';
    ++s_import_part;
    prv_import_ack(hi, lo, seq, JOT_IMPORT_SAVED, NULL);
    return;
  }
  if (which == IMPORT_COMMIT && seq == IMPORT_COMMIT_ACK) {
    if (s_import_offset != s_import_total) {
      prv_import_ack(hi, lo, seq, JOT_IMPORT_INVALID, NULL);
      return;
    }
    // This is a persistent write, not a Bluetooth transport acknowledgment.
    const JotImportResult status =
        jot_store_import_new(s_import_text, s_import_offset, hi, lo, &receipt);
    if (status == JOT_IMPORT_SAVED || status == JOT_IMPORT_ALREADY) {
      s_import_ready = false;
      free(s_import_text);
      s_import_text = NULL;
      jot_backup_ui_changed();
      prv_import_ack(hi, lo, seq, status, &receipt);
    } else {
      prv_import_ack(hi, lo, seq, status, NULL);
    }
    return;
  }
  prv_import_ack(hi, lo, seq, JOT_IMPORT_INVALID, NULL);
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  (void)context;
  if (dict_find(iter, MESSAGE_KEY_ImportKind)) {
    prv_import_receive(iter);
    return;
  }
  if (!s_active) return;
  Tuple *session = dict_find(iter, MESSAGE_KEY_BackupSession);
  Tuple *ack = dict_find(iter, MESSAGE_KEY_BackupAck);
  Tuple *part = dict_find(iter, MESSAGE_KEY_BackupPart);
  Tuple *status = dict_find(iter, MESSAGE_KEY_BackupStatus);
  if (!session || !ack || !part || !status ||
      session->value->uint32 != s_session ||
      ack->value->int32 != s_phase - 1 ||
      part->value->int32 != s_part) return;

  prv_cancel_timer();
  if (status->value->int32 != 1) {
    prv_fail("Phone save failed");
    return;
  }
  if (s_phase == s_total + 1) {
    s_active = false;
    free(s_note);
    s_note = NULL;
    prv_status("Phone copy saved");
    return;
  }
  if (s_phase > 0 && !s_last_chunk) {
    s_offset += s_chunk_length;
    ++s_part;
  } else {
    ++s_phase;
    s_offset = 0;
    s_part = 0;
  }
  s_attempts = 0;
  if (s_phase <= s_total) {
    snprintf(s_status, sizeof s_status, "Exporting %d/%d",
             s_phase, s_total);
  } else {
    snprintf(s_status, sizeof s_status, "Saving on phone...");
  }
  jot_backup_ui_changed();
  prv_schedule(40);
}

static void prv_outbox_failed(DictionaryIterator *iter,
                              AppMessageResult reason, void *context) {
  (void)iter; (void)reason; (void)context;
  if (!s_active) return;
  prv_schedule(500);
}

void jot_backup_init(void) {
  s_status[0] = '\0';
  s_active = false;
  s_import_ready = false;
  s_session = (uint32_t)time(NULL);
  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_outbox_failed(prv_outbox_failed);
  s_available = app_message_open(256, 384) == APP_MSG_OK;
  if (!s_available) prv_status("Phone link unavailable");
}

void jot_backup_deinit(void) {
  jot_backup_cancel();
  free(s_import_text);
  s_import_text = NULL;
  s_import_ready = false;
  if (s_available) app_message_deregister_callbacks();
  s_available = false;
}

void jot_backup_start(void) {
  if (s_active) return;
  if (!s_available) { prv_status("Phone link unavailable"); return; }
  if (!jot_store_flush()) { prv_status("Save note before backup"); return; }
  s_total = jot_store_count();
  if (s_total == 0) { prv_status("No notes to export"); return; }
  s_note = malloc(JOT_NOTE_TEXT_BYTES);
  if (!s_note) { prv_status("Not enough memory"); return; }
  ++s_session;
  if (!s_session) ++s_session;
  s_phase = 0;
  s_part = 0;
  s_offset = 0;
  s_attempts = 0;
  s_active = true;
  prv_status("Connecting to phone...");
  prv_schedule(80);
}

bool jot_backup_active(void) { return s_active; }
const char *jot_backup_status(void) { return s_status; }

void jot_backup_cancel(void) {
  bool was_active = s_active;
  prv_cancel_timer();
  s_active = false;
  free(s_note);
  s_note = NULL;
  if (was_active) prv_status("Export canceled");
}

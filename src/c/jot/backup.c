#include <pebble.h>
#include "jot/backup.h"
#include "jot/jot.h"
#include "jot/store.h"
#include "message_keys.auto.h"

enum {
  BACKUP_BEGIN = 1, BACKUP_NOTE = 2, BACKUP_END = 3,
  BACKUP_MAX_ATTEMPTS = 4, BACKUP_ACK_TIMEOUT_MS = 4000
};

static bool s_available;
static bool s_active;
static uint32_t s_session;
static int s_total;
static int s_phase;  // 0=BEGIN, 1..total=NOTE, total+1=END
static uint8_t s_attempts;
static AppTimer *s_retry_timer;
static char s_status[36];

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
  prv_status(reason);
}

static void prv_send_current(void *context);

static void prv_schedule(uint32_t delay_ms) {
  prv_cancel_timer();
  s_retry_timer = app_timer_register(delay_ms, prv_send_current, NULL);
  if (!s_retry_timer) prv_fail("Backup timer failed");
}

static void prv_send_current(void *context) {
  s_retry_timer = NULL;
  if (!s_active) return;
  if (s_attempts >= BACKUP_MAX_ATTEMPTS) {
    prv_fail("Phone not responding");
    return;
  }

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

  if (kind == BACKUP_BEGIN && written == DICT_OK) {
    written = dict_write_int16(iter, MESSAGE_KEY_BackupTotal, (int16_t)s_total);
  } else if (kind == BACKUP_NOTE && written == DICT_OK) {
    const int slot = jot_store_slot_at(index);
    char note[JOT_NOTE_TEXT_BYTES];
    time_t created;
    if (slot < 0 || !jot_store_export(slot, note, sizeof note, &created)) {
      prv_fail("Cannot read note");
      return;
    }
    written = dict_write_uint32(iter, MESSAGE_KEY_BackupTimestamp,
                                (uint32_t)created);
    if (written == DICT_OK)
      written = dict_write_cstring(iter, MESSAGE_KEY_BackupText, note);
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
  ++s_attempts;
  prv_schedule(BACKUP_ACK_TIMEOUT_MS);
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  if (!s_active) return;
  Tuple *session = dict_find(iter, MESSAGE_KEY_BackupSession);
  Tuple *ack = dict_find(iter, MESSAGE_KEY_BackupAck);
  Tuple *status = dict_find(iter, MESSAGE_KEY_BackupStatus);
  if (!session || !ack || !status ||
      session->value->uint32 != s_session ||
      ack->value->int32 != s_phase - 1) return;

  prv_cancel_timer();
  if (status->value->int32 != 1) {
    prv_fail("Phone save failed");
    return;
  }

  if (s_phase == s_total + 1) {
    s_active = false;
    prv_status("Phone copy saved");
    return;
  }

  ++s_phase;
  s_attempts = 0;
  if (s_phase <= s_total) {
    snprintf(s_status, sizeof s_status, "Exporting %d/%d",
             s_phase, s_total);
  } else {
    snprintf(s_status, sizeof s_status, "Saving on phone...");
  }
  jot_backup_ui_changed();
  prv_schedule(80);
}

static void prv_outbox_failed(DictionaryIterator *iter,
                              AppMessageResult reason, void *context) {
  if (!s_active) return;
  prv_schedule(500);
}

void jot_backup_init(void) {
  s_status[0] = '\0';
  s_active = false;
  s_session = 0;
  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_outbox_failed(prv_outbox_failed);
  s_available = app_message_open(128, 384) == APP_MSG_OK;
  if (!s_available) prv_status("Phone link unavailable");
}

void jot_backup_deinit(void) {
  jot_backup_cancel();
  if (s_available) app_message_deregister_callbacks();
  s_available = false;
}

void jot_backup_start(void) {
  if (s_active) return;
  if (!s_available) { prv_status("Phone link unavailable"); return; }
  s_total = jot_store_count();
  if (s_total == 0) { prv_status("No notes to export"); return; }
  ++s_session;
  if (!s_session) ++s_session;
  s_phase = 0;
  s_attempts = 0;
  s_active = true;
  prv_status("Connecting to phone...");
  prv_schedule(80);
}

bool jot_backup_active(void) { return s_active; }
const char *jot_backup_status(void) { return s_status; }

void jot_backup_cancel(void) {
  prv_cancel_timer();
  s_active = false;
}

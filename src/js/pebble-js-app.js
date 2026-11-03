/* Jot phone companion — explicit, offline-only snapshot backup.
 * Notes arrive in UTF-8-safe chunks, acknowledged with note + part indices.
 */
var STORAGE_KEY = 'jot.backup.v1';
var pending = null;

function reply(session, index, part, ok) {
  Pebble.sendAppMessage({
    BackupSession: session,
    BackupAck: index,
    BackupPart: part,
    BackupStatus: ok ? 1 : 0
  }, function () {}, function (error) {
    console.log('Jot receipt not delivered: ' + JSON.stringify(error));
  });
}

Pebble.addEventListener('ready', function () {
  console.log('Jot phone companion ready.');
});

Pebble.addEventListener('appmessage', function (event) {
  var p = event.payload || {};
  if (typeof p.BackupKind === 'undefined' ||
      typeof p.BackupSession === 'undefined' ||
      typeof p.BackupIndex === 'undefined') return;
  var kind = Number(p.BackupKind);
  var session = Number(p.BackupSession);
  var index = Number(p.BackupIndex);
  var part = Number(typeof p.BackupPart === 'undefined' ? 0 : p.BackupPart);
  if (!isFinite(session) || !isFinite(index) || !isFinite(part) ||
      part < 0 || part !== Math.floor(part)) return;

  if (kind === 1) {
    var total = Number(p.BackupTotal);
    if (!isFinite(total) || total < 0 || total > 256 ||
        total !== Math.floor(total) || index !== -1 || part !== 0) {
      reply(session, index, part, false);
      return;
    }
    if (!pending || pending.session !== session) {
      pending = {session: session, total: total, notes: new Array(total)};
    }
    reply(session, index, part, true);
  } else if (kind === 2) {
    var finalPart = Number(typeof p.BackupFinal === 'undefined'
      ? 1 : p.BackupFinal);
    if (!pending || pending.session !== session ||
        index < 0 || index >= pending.total || index !== Math.floor(index) ||
        typeof p.BackupText !== 'string' ||
        typeof p.BackupTimestamp === 'undefined' ||
        (finalPart !== 0 && finalPart !== 1)) {
      reply(session, index, part, false);
      return;
    }
    var note = pending.notes[index];
    if (!note && part === 0) {
      note = {created: Number(p.BackupTimestamp), text: '',
              nextPart: 0, complete: false};
      pending.notes[index] = note;
    }
    if (!note) { reply(session, index, part, false); return; }
    if (part === note.nextPart - 1) {
      // Lost acknowledgment: do not duplicate a chunk.
      reply(session, index, part, true);
      return;
    }
    if (note.complete || part !== note.nextPart ||
        Number(p.BackupTimestamp) !== note.created) {
      reply(session, index, part, false);
      return;
    }
    note.text += p.BackupText;
    note.nextPart++;
    if (finalPart === 1) note.complete = true;
    reply(session, index, part, true);
  } else if (kind === 3) {
    if (!pending || pending.session !== session ||
        index !== pending.total || part !== 0) {
      try {
        var old = JSON.parse(localStorage.getItem(STORAGE_KEY) || 'null');
        reply(session, index, part, !!old && old.session === session);
      } catch (e) { reply(session, index, part, false); }
      return;
    }
    for (var i = 0; i < pending.total; i++) {
      if (!pending.notes[i] || !pending.notes[i].complete) {
        reply(session, index, part, false);
        return;
      }
    }
    try {
      var snapshot = {
        format: 'jot-backup-v1',
        session: session,
        savedAt: new Date().toISOString(),
        notes: pending.notes.map(function (n) {
          return {created: n.created, text: n.text};
        })
      };
      // An interrupted or failed transfer never replaces the old snapshot.
      localStorage.setItem(STORAGE_KEY, JSON.stringify(snapshot));
      pending = null;
      reply(session, index, part, true);
    } catch (e) {
      console.log('Jot local backup write failed: ' + e);
      reply(session, index, part, false);
    }
  }
});


// Experimental bundled configuration page (#39). Read only; no note writes.
// The phone companion's existing atomic backup snapshot remains authoritative.
var configBackup = null;
var CONFIG_CHUNK = 1024;
function configChunkEnd(s, start) {
  var end = Math.min(s.length, start + CONFIG_CHUNK);
  // Keep each UTF-16 surrogate pair intact across WebView messages.
  if (end > start && end < s.length &&
      s.charCodeAt(end - 1) >= 0xD800 && s.charCodeAt(end - 1) <= 0xDBFF &&
      s.charCodeAt(end) >= 0xDC00 && s.charCodeAt(end) <= 0xDFFF) end--;
  return end;
}
Pebble.addEventListener('configmessage', function (event) {
  var request = event.data || {};
  if (request.type === 'jotBackupInfo') {
    try {
      var raw = localStorage.getItem(STORAGE_KEY);
      configBackup = raw ? JSON.parse(raw) : null;
      if (configBackup && (configBackup.format !== 'jot-backup-v1' ||
          !Array.isArray(configBackup.notes) ||
          configBackup.notes.length > 256)) throw new Error('Invalid backup');
      if (configBackup) for (var k = 0; k < configBackup.notes.length; k++)
        if (!configBackup.notes[k] ||
            typeof configBackup.notes[k].text !== 'string' ||
            !isFinite(Number(configBackup.notes[k].created)))
          throw new Error('Invalid note');
      event.respond(configBackup ?
        {ok: true, found: true, count: configBackup.notes.length,
         session: configBackup.session, savedAt: configBackup.savedAt} :
        {ok: true, found: false});
    } catch (e) {
      configBackup = null;
      event.respond({ok: false, error: 'Could not read local backup'});
    }
    return;
  }
  if (request.type === 'jotBackupNote') {
    var index = Number(request.index), part = Number(request.part);
    if (!configBackup || !isFinite(index) || !isFinite(part) ||
        index < 0 || index !== Math.floor(index) ||
        part < 0 || part !== Math.floor(part) ||
        index >= configBackup.notes.length || part > 8192) {
      event.respond({ok: false, error: 'Invalid note request'});
      return;
    }
    var note = configBackup.notes[index], start = 0, step = 0;
    while (step < part && start < note.text.length) {
      start = configChunkEnd(note.text, start);
      step++;
    }
    if (step !== part || (part > 0 && start >= note.text.length)) {
      event.respond({ok: false, error: 'Invalid note part'});
      return;
    }
    var end = configChunkEnd(note.text, start);
    event.respond({ok: true, index: index, part: part,
      created: note.created, text: note.text.slice(start, end),
      done: end === note.text.length});
    return;
  }
  event.respond({ok: false, error: 'Unknown configuration request'});
});

// Base64-encode UTF-8 without requiring btoa/TextEncoder in PebbleKit JS.
function toBase64Utf8(text) {
  var binary = encodeURIComponent(text).replace(/%([0-9A-F]{2})/g,
    function (match, hex) { return String.fromCharCode(parseInt(hex, 16)); });
  var chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  var result = '';
  for (var i = 0; i < binary.length; i += 3) {
    var a = binary.charCodeAt(i);
    var hasB = i + 1 < binary.length;
    var hasC = i + 2 < binary.length;
    var b = hasB ? binary.charCodeAt(i + 1) : 0;
    var c = hasC ? binary.charCodeAt(i + 2) : 0;
    result += chars.charAt(a >> 2);
    result += chars.charAt(((a & 3) << 4) | (b >> 4));
    result += hasB ? chars.charAt(((b & 15) << 2) | (c >> 6)) : '=';
    result += hasC ? chars.charAt(c & 63) : '=';
  }
  return result;
}

Pebble.addEventListener('showConfiguration', function () {
  var url = 'https://majormajormajormajor.github.io/pebble-jot/export.html';
  try {
    var snapshot = localStorage.getItem(STORAGE_KEY);
    if (snapshot) url += '#' + toBase64Utf8(snapshot);
  } catch (e) {
    console.log('Cannot load Jot backup for viewing: ' + e);
  }
  Pebble.openURL(url);
});

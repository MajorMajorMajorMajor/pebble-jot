/* Jot phone companion — no cloud service and no automatic uploads.
 * Watch -> AppMessage -> complete snapshot -> PebbleKit JS localStorage.
 * The configuration page receives a copy in the URL fragment, which is
 * processed locally by the static export page, not sent in HTTP requests.
 */
var STORAGE_KEY = 'jot.backup.v1';
var pending = null;

function reply(session, index, ok) {
  Pebble.sendAppMessage({
    BackupSession: session,
    BackupAck: index,
    BackupStatus: ok ? 1 : 0
  }, function () {}, function (error) {
    console.log('Jot receipt not delivered: ' + JSON.stringify(error));
    // The watch retransmits the packet when it does not receive a receipt.
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
  if (!isFinite(session) || !isFinite(index)) return;

  if (kind === 1) {
    var total = Number(p.BackupTotal);
    if (!isFinite(total) || total < 0 || total > 16 || index !== -1) {
      reply(session, index, false);
      return;
    }
    if (!pending || pending.session !== session) {
      pending = {session: session, total: total, notes: new Array(total)};
    }
    reply(session, index, true);
  } else if (kind === 2) {
    if (!pending || pending.session !== session ||
        index < 0 || index >= pending.total ||
        typeof p.BackupText !== 'string' ||
        typeof p.BackupTimestamp === 'undefined') {
      reply(session, index, false);
      return;
    }
    pending.notes[index] = {
      created: Number(p.BackupTimestamp),
      text: p.BackupText
    };
    reply(session, index, true);
  } else if (kind === 3) {
    if (!pending || pending.session !== session ||
        index !== pending.total) {
      // If the completion receipt was lost, the watch may repeat END.
      try {
        var old = JSON.parse(localStorage.getItem(STORAGE_KEY) || 'null');
        reply(session, index, !!old && old.session === session);
      } catch (e) { reply(session, index, false); }
      return;
    }
    for (var i = 0; i < pending.total; i++) {
      if (!pending.notes[i]) { reply(session, index, false); return; }
    }
    try {
      var snapshot = {
        format: 'jot-backup-v1',
        session: session,
        savedAt: new Date().toISOString(),
        notes: pending.notes
      };
      // Only replace the previous backup after every note has arrived.
      localStorage.setItem(STORAGE_KEY, JSON.stringify(snapshot));
      pending = null;
      reply(session, index, true);
    } catch (e) {
      console.log('Jot local backup write failed: ' + e);
      reply(session, index, false);
    }
  }
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

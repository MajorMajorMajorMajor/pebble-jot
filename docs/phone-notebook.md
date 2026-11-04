# Jot 1.19.8 — Independent phone notebook (#70)

Branch: feature/issue-70-phone-notebook
Status: phase 1, pending real Pebble/Android QA, not on main.

## Working foundation

The phone now stores jot.phone.v1, its own collection independent of
watch slots. Watch exports include note generation serial and revision.
Repeated backups merge; deleting a watch note does not remove it from
the phone (#51). Phone edits are protected from later watch imports;
conflicting edits are flagged rather than silently overwritten.

The bundled page now offers New, Edit, Save, Cancel, two-step Delete,
browsing, Copy and TXT/JSON export. Phone deletion is an explicit
operation and never deletes from the watch. Tombstones prevent a
phone-deleted watch record from being reimported inadvertently.
Local edits are committed in Unicode-safe chunks with one atomic write.
No remote hosting or note-bearing URL is involved.

Watch note generation serials retain a persistent high-water mark,
even if every note is deleted before a reboot.

## Still ahead in #70

Safe phone-to-watch note creation, propagation of phone edits to
the corresponding watch note, watch-side persistent acknowledgements,
and resolving simultaneous changes. Phone edits currently show
"Phone changes not on watch" instead of claiming to have synced.

No migration of previous fake jot.backup.v1 snapshot data is done,
per the tester's approval to discard development notes.
The bundled configPage API remains preview-only in mobile 1.16.0.5.

## Build on Emery

    git fetch origin
    git switch --track origin/feature/issue-70-phone-notebook
    sh scripts/build-jot.sh --clean

The wrapper builds Jot, bundles config.html into the PBW, verifies
its manifest and HTML contents, then installs it. This deliberately
prevents a common false success: after "pebble clean" and "pebble
build", the watch installs but Settings does nothing because the
separate PBW packaging step was accidentally omitted.

For SDK build-only verification (e.g. CI), use:

    sh scripts/build-jot.sh --build-only

The clean step matters when switching from #39 to #70 in the same worktree.
Pebble's cached build/include/message_keys.auto.h may still describe
#39's message keys even though package.json already defines
BackupSerial and BackupRevision. A clean build regenerates the SDK's
message-key header. The enableMultiJS warning is unrelated to the C
header and does not require changing Jot's JavaScript module style.

Open Jot Settings in the Pebble Android app with Plugins Preview enabled.

## Device checks with disposable notes

1. Write two notes on watch, send both to phone: both should appear.
2. Delete one on watch and send backup again: both remain on phone;
   the missing watch copy now says "Phone only".
3. Create a phone-only note; edit it; reopen Settings; text persists.
4. Edit an on-watch note on phone, back up from watch again; phone edits
   survive, labelled "Phone changes not on watch".
5. Edit same note on watch too and backup: see "Conflicting edits".
6. Delete a note on PHONE (two taps), repeat watch backup:
   deleted phone entry does not reappear; watch copy stays intact.
7. Verify Copy, TXT, and JSON with newlines, accents and emojis.
8. Restart the mobile companion, then reopen Settings and recheck.

This phase is deliberately isolated from main until physical testing.

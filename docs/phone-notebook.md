# Jot 1.20.0 — Independent two-device notebook (#70)

Branch: feature/issue-70-phone-notebook
Status: phase 1 watch→phone has passed hardware QA; phase 2 phone→watch
is built and awaiting real Pebble Time 2 QA. Not on main.

## Working foundation

The phone now stores jot.phone.v1, its own collection independent of
watch slots. Watch exports include note generation serial and revision.
Repeated backups merge; deleting a watch note does not remove it from
the phone (#51). Phone edits are protected from later watch imports;
conflicting edits are flagged rather than silently overwritten.

The bundled page now offers New, Edit, Save, Cancel, two-step Delete,
browsing, Copy and TXT/JSON export. Phone deletion is an explicit
operation and never deletes from the watch. If that note remains on
the watch, the next explicit watch backup restores its phone copy.
Earlier development tombstones are ignored and removed automatically.
Local edits are committed in Unicode-safe chunks with one atomic write.
No remote hosting or note-bearing URL is involved.

Watch note generation serials retain a persistent high-water mark,
even if every note is deleted before a reboot.

## Phone-to-watch phase 2 — hardware acceptance pending

For phone-only notes, the Settings page offers Send to Watch and shows a
4095-byte UTF-8 watch budget. Pressing the button is the sole confirmation.
The companion transmits bounded 160-byte Unicode-safe AppMessage chunks and
retries unacknowledged packets. The watch validates the entire note,
rejects a dirty editor or a full notebook, writes to an actually vacant
persistence slot, and acknowledges **only after a committed header**.

A randomly generated 64-bit transfer token is saved with the phone note
before sending. The watch commits it in the same persistent header as
the note. Lost acknowledgments/reboots therefore cannot create duplicate
notes on re-sending. Earlier v2 watch headers remain readable. If the
watch saved the note but phone localStorage failed, the next Send to Watch
recovers the original watch receipt.

This release does not send phone edits to existing watch notes. A phone
note that was edited while a send was unresolved resumes the ORIGINAL
snapshot on retry; the later phone edit is preserved as an unsynced change.

Hardware QA: create a short multiline/emoji phone note, Send to Watch,
verify it in watch list, repeat backup to check single identity. Try a
near-limit note, disconnect/reconnect after sending and retry, and test
sending when the watch editor is dirty. Use disposable notes.

## Still ahead in #70

Safe phone-to-watch note creation, propagation of phone edits to
the corresponding watch note, watch-side persistent acknowledgements,
and resolving simultaneous changes. Phone edits currently show
"Phone changes not on watch" instead of claiming to have synced.

No migration of previous fake jot.backup.v1 snapshot data is done,
per the tester's approval to discard development notes.
The bundled configPage API remains preview-only in mobile 1.16.0.5.

There is now a local Settings fallback: if the phone does not recognize
the PBW's configPage metadata (e.g. stale locker entry), the old
showConfiguration event opens the **same static bundled HTML** as a
`data:text/html` URL. This carries NO note contents in the URL and uses
no web hosting. The fallback and bundled file are kept byte-for-byte
identical by scripts/sync-settings-fallback.py. The page still requires
Plugins Preview for its real-time message bridge.

## Build on Emery

**Important:** Pebble's Plugins Preview bundling and sideloading guide
requires **pebble-tool 5.1.0 or later**. A local tool at 5.0.40 can
build/install the watch app but does not meet that documented
requirement for bundled configuration pages. Upgrade it before
troubleshooting a Settings button that does nothing:

    uv tool upgrade pebble-tool
    pebble --version

Run the version command from the actual shell/environment used for
installing (including `nix develop`). Confirm that it resolves to
5.1.0 or newer; if Nix shadows the upgraded executable, fix your
shell PATH before continuing. Make sure Pebble mobile app 1.16.0.5 has
Settings → Plugins → Plugins / permissions preview enabled.


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
6. Delete a note on PHONE (two taps): it disappears from the phone,
   but the watch copy remains. Back up from WATCH again: the deleted
   phone entry reappears. Repeat backup: no duplicate is created.
7. Verify Copy, TXT, and JSON with newlines, accents and emojis.
8. Restart the mobile companion, then reopen Settings and recheck.

If the PBW definitely contains `config.html` but Settings is inert,
verify the `pebble-tool` version as above, then fully restart the
Pebble phone app and try Settings. To collect evidence after retrying:

    pebble logs --cloudpebble

The preview API normally opens `configPage` without a legacy
`showConfiguration` handler; do not restore the GitHub Pages URL.

This phase is deliberately isolated from main until physical testing.

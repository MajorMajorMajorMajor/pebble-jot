# Jot 1.21.0 — Seamless watch-to-phone sync preview (#76)

Branch: feature/issue-76-seamless-sync
Status: automated tests pending full Emery QA, not merged to main.

The watch app uses ConnectionService to reconcile on launch and when the
Pebble phone application reconnects. After **durable** note saves and deletes,
it coalesces events into small AppMessage transactions. If it misses an event,
cannot obtain a durable phone acknowledgment, or disconnects, it marks a
full reconciliation necessary. No note text or index is removed on the
phone just because it disappeared from the watch.

Hold Up on the watch note list still forces a full transfer, including
when the watch has zero notes. Background transfers do not block list
navigation or display manual-transfer progress. Phone-initiated Send to
Watch preempts the background transfer; it never overwrites a dirty draft.

If the same shared note was changed independently on both sides, the
phone accepts the watch version under its established watch identity,
**preserves the phone edition as an ordinary phone-only copy**, and
commits both in one localStorage write. Repeat sync does not create
another fork. There is deliberately no conflict-dialog user interface.

Only actual differing watch text is treated as a divergent change;
merely incrementing its revision while text is identical to the
common base does not create copies. Watch deletion is still local;
phone deletion is still local, and a future watch backup can re-add
the note, as already approved in #70.

IMPORTANT: this phase makes watch-to-phone changes automatic **while
the Jot watchapp is running**. A dormant app cannot continuously send
messages in the background without an additional always-on service.
Opening the app while connected or reconnecting triggers reconciliation.

## Build on Termux / Emery

    git fetch origin
    git switch --track origin/feature/issue-76-seamless-sync
    sh scripts/build-jot.sh --clean

## First wrist tests (all notes are disposable QA data)

1. Start Jot with the phone connected; create and save a note. Check that
   it reaches the phone without holding Up on the list.
2. Edit and save that note; verify the phone version updates without
   duplicating it. Delete it on watch; phone copy should remain, now
   labelled Phone only.
3. Disconnect Bluetooth, add two notes, then reconnect. Check both
   arrive following a full reconciliation, without duplicates.
4. Edit the SAME note on phone and watch while offline, then reconnect.
   Check that the watch's text remains under its original identity
   and the phone's text becomes an independent Phone only copy.
5. Repeat a full sync twice: no extra conflict copies. Hold Up should
   still force a full sync, even after deleting all watch notes.
6. Repeat a phone-to-watch Send while auto sync is running. The
   explicit send takes priority, with no dirty draft lost.
7. Check instant launch, normal list scrolling and keyboard response;
   sync traffic should not monopolize the editor UI.

No auto phone-to-watch propagation of shared-note edits yet. That
remains the next #70 milestone and must use revision checks.

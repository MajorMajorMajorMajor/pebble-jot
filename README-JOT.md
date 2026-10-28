# Jot 0.1 — Pebble quick capture

First Pebble on-watch note-taking app built on the `mobile-gestures` keyboard.
Gesture recognition and bindings are carried over, including CW digits 0–9.

## Controls

* Launch **Jot**: immediately opens a blank writing surface. No launch menu.
* Type with existing taps, swipes, eight drag-and-returns, and CW/CCW circles.
* **Select (short press)**: save the current note and start a new one.
* **Up or Down**: open the notes list; **KB corner tap** also opens it.
* In the list: Up/Down browse, Select edits a selected note, Back returns.
* Hold Select on a list entry to request deletion. Select confirms, Back cancels.
* **Back from editor**: exits; saves pending note on orderly app shutdown.
* Hold Select in the editor for ~1.2 s to access the old diagnostic menu
  (Typing / Trace), if you need to continue gesture experiments.

## Persistence

Notes are stored in the watch app's Pebble persistent storage. The app
has a deliberately bounded capacity of **16 notes**, each up to **159 UTF-8
bytes** (not characters). At capacity it refuses to create another note;
**it does not overwrite an old one**. Notes are written after ~1.8 seconds
without new typing, and also when entering the list, starting a new note,
or leaving the app normally. A `*` after JOT indicates an unsaved change.
`SAVE ERROR` indicates Pebble storage has not accepted a write.

This app has a **different UUID** from the typing-test harness. It can be
installed alongside the original app, but **its notes are stored separately**.
Uninstalling Jot removes those local notes. Phone sync/export is not yet
implemented; don't store irreplaceable information in this experimental build.

## Apply

From the repository's `mobile-gestures` branch, after fetching the patch:

```sh
git apply --check /storage/emulated/0/Download/jot-v0.1.patch
git apply /storage/emulated/0/Download/jot-v0.1.patch
pebble build
```

The patch was generated against the uploaded `typing-source.tar.gz` plus
`typing-mode-integrated`, `gesture-classifier-calibrated`,
`trace-harness-responsive`, `trace-export-status`, and
`circle-tuning-numbers` changes. If the branch has diverged, please use
`git apply --check` before modifying anything. Do not force-apply.

## First device test

1. Launch Jot from the app menu or assigned quick-launch button.
2. Write a short phrase with at least one CW digit, wait for `*` to disappear.
3. Press Select, write a second phrase, then press Up.
4. Open the earlier note, edit it, exit the app, relaunch and verify it remains.
5. Confirm delete requires a second action, and the original typing app
   remains installed separately.

Build / runtime status: host storage tests passed; Pebble SDK compilation and
on-device testing have **not** yet been performed.

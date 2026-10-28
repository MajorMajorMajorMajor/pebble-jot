# Jot — detailed behavior and developer notes

Jot is an early on-watch note-taking app for Pebble Time 2, built on the MessagEase-inspired touchscreen gesture keyboard. The main [README](README.md) is the best starting point.

## Current controls

- **Launch:** go directly to a blank editor.
- **Editor Select:** save this note and start a new one.
- **Editor Up / Down or KB touch target:** show saved notes.
- **Editor Back:** exit, saving pending changes during normal shutdown.
- **Editor long Select (~1.2 s):** open the inherited typing/trace diagnostic menu.
- **Notes list Up / Down:** browse saved notes.
- **Notes list Select:** open a note for editing.
- **Notes list long Select:** begin a two-step delete; Select confirms and Back cancels.
- **Notes list Back:** return to the editor.

The keyboard supports taps, directional swipes, directional drag-and-returns, and CW/CCW circles; clockwise circles on the letter keys enter digits 1–9, with 0 on the Space key.

## Persistence and limitations

The app stores notes in Pebble's per-app persistent storage, currently capped at **16 notes** of **159 UTF-8 bytes** each. A note's byte count may differ from its character count. The app refuses to allocate a seventeenth note instead of silently overwriting existing notes.

A pending change is indicated by `JOT *`. Autosave fires approximately 1.8 seconds after the last edit, and saves are also attempted at navigation boundaries. `SAVE ERROR` means a write was not accepted; do not assume the affected note is safely stored. Note order is newest-first.

**No backup/export or phone synchronization exists yet.** The watch stores notes locally, and uninstalling Jot may delete them. Avoid irreplaceable notes during development.

## Development and verification

The initial Jot app has been compiled, installed, and photographed working on real Pebble hardware, including the writing screen and notes list. This confirms those visible screens and text entry run on-device, **not** that all storage edge cases or gesture classes have been thoroughly hardware-tested.

From the repository root:

```sh
pebble build
pebble install --emulator emery
```

Use your configured watch-install procedure for hardware tests. The primary platform in `package.json` is currently `emery`.

Relevant modules:

- `src/c/jot/jot.c` — editor/list interface, autosave, navigation.
- `src/c/jot/store.c` and `store.h` — local note persistence.
- `src/c/keyboard/` — gesture bindings and keyboard state.
- `src/c/ui/touch.c` — gesture recording and recognition.

## Roadmap

1. A robust phone-side export/backup path before treating notes as dependable storage.
2. Device testing of note persistence, full capacity, relaunch, and deletion.
3. Refinement of Jot's visual identity without reducing keyboard touch targets.

The earlier `jot-v0.1.patch` instructions are historical; this repository now contains the Jot source directly. Simply pull and build the branch.

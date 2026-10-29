# Jot — detailed behavior and developer notes

Jot is an early on-watch note-taking app for Pebble Time 2, built on the MessagEase-inspired touchscreen gesture keyboard. The main [README](README.md) is the best starting point.

## Current controls

- **Launch:** go directly to a blank editor.
- **Editor Select:** save this note and start a new one.
- **Editor text strip tap:** peek at the full current draft without saving or switching notes.
- **Editor Up / Down or KB touch target:** show saved notes.
- **Editor Back:** exit, saving pending changes during normal shutdown.
- **Editor long Select (~1.2 s):** open the inherited typing/trace diagnostic menu.
- **Notes list Up / Down:** browse saved notes.
- **Notes list Select:** open a saved note in the new full-screen, read-only paper reader.
- **Notes list long Select:** begin a two-step delete; Select confirms and Back cancels.
- **Notes list Back:** return to the editor.
- **Reader Up / Down:** scroll the note vertically, 60 pixels per press (when it exceeds the screen).
- **Reader Select:** open that note in the existing keyboard editor.
- **Reader Back:** return to the notes list without modifying the note.
- **Draft peek tap, Select or Back:** return to the same keyboard editor.

The keyboard supports taps, directional swipes, directional drag-and-returns, and CW/CCW circles; clockwise circles on the letter keys enter digits 1–9, with 0 on the Space key.

## Maintenance fixes (v1.3.1)

- Note deletion now follows Pebble's `persist_delete()` API contract: a
  successful delete returns `S_TRUE`. Deleting a just-created note or an
  older note updates both on-device storage and the in-memory list. The editor
  is refreshed when an active note is deleted. A native host regression test
  covers these cases and subsequent note entry.
- Up/Down in the notebook list moves the selection by **one note**, and the
  visible card window now slides one item at a time instead of jumping by
  groups of three.
- Notes from **today** display the local time (`HH:MM`). Older notes display
  the local month/day (`MM/DD`) in the narrow timestamp column. The
  full-screen reader still shows the full note timestamp.

All saved-note keys and record structures are unchanged. Issues #1, #8, and
#13 were verified on the watch and closed.

## Persistence and limitations

The app stores notes in Pebble's per-app persistent storage, currently capped at **16 notes** of **159 UTF-8 bytes** each. A note's byte count may differ from its character count. The app refuses to allocate a seventeenth note instead of silently overwriting existing notes.

A pending edit is indicated by a small `*` at the top right of the input strip. Autosave fires approximately 1.8 seconds after the last edit, and saves are also attempted at navigation boundaries. `SAVE ERROR` means a write was not accepted; do not assume the affected note is safely stored. `FULL` means a character was rejected because the note is at capacity. Note order is newest-first.

**Manual phone export is available and has been demonstrated on real hardware.** The watch keeps its local notes; the phone companion keeps one complete snapshot in its local storage. It is not automatic synchronization. Copy or save a separate file before relying on a backup. Uninstalling Jot may delete the on-watch notes.

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

## Full-screen reading mode (v1.2)

Selecting a saved note now opens its **full text** on a cream paper screen with ruled lines, a timestamp, a yellow Jot header, an ink navigation footer and a note-position badge. This is a read-only view: it does not alter the editor's current draft, note contents or storage. Up and Down scroll the text when necessary.

Press **Select** to open that note for editing with the existing keyboard, or **Back** to return to the list at the same selection. If a note cannot be read or opened, Jot remains on the list or reader and shows a failure indicator. Touchscreen typing is ignored while the reader is on screen.

This installment **does not** change the 159-byte note limit, storage format, keyboard geometry or gesture classifier. The reader has now been demonstrated on real Pebble hardware; the enlarged body font and draft peek are the next on-watch test.

## Multiline editing preview (v1.4)

The 40-pixel editor strip now uses both rows for the **real note text**, not
a `JOT` heading followed by a flattened tail. It preserves actual newline
characters, uses Pebble's own word wrapping, and automatically scrolls the
text behind a clipped 40-pixel viewport to keep the end visible. A `|` caret
marks where the *next* character will be appended. If the most recent
character is a newline, the caret appears on the next line. Backspace
deletes the newline and returns it to the preceding line.

At this stage, the caret is **end-of-note only**; positioning and inserting
mid-note will follow as part of [cursor editing issue #3](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/3).
The strip continues to open a read-only preview on tap. The small `*`
marker appears while dirty; `SAVE ERROR` and `FULL` are prominent status
badges. The normal editor no longer spends an entire line displaying `JOT`.

The editor display is now a clipping Layer with a word-wrapped TextLayer
child; the surrounding keyboard and its 40-pixel touch boundary have not
changed. The note storage format, save schedule, gesture bindings and limits
remain unchanged. The multiline viewport awaits on-watch visual testing.

### Draft peek and typography (v1.3)

**Tap the strip above the keyboard** to open the full-screen paper reader
with a `DRAFT` badge and read-only snapshot of the current buffer. Up / Down
scroll; tap the reader, Select, or Back to return to the same editor.
Peeking does **not** flush, allocate, edit, or switch notes. Autosave runs
independently as before.

The full-screen reader uses Gothic 24 Bold for legibility, with Gothic 18
Bold metadata. Its large text wraps naturally; it is unaffected by the
editor's smaller two-line viewport.

## Watch notes list appearance

The browser uses a notebook-style yellow header, cream paper, three note-preview cards per page, a yellow selection card, and a charcoal status footer. **Up/Down, Select, long Select, Back and long Up** retain their previous actions; the keyboard geometry and gesture thresholds are unchanged. This visual redesign is awaiting an on-watch appearance check.

## Phone backup — experimental (v0.2)

The watch-to-phone transfer is **manual and non-destructive**. Jot retains its notes on the watch.

1. Launch Jot, press **Up** or **Down** to open the notes list.
2. **Hold Up for about one second**. Watch for `Exporting X/Y` and then **`Phone copy saved`**. That final message means the Pebble phone companion has confirmed writing the entire snapshot to its own `localStorage`; it does **not** mean you've saved a file elsewhere on the phone.
3. In the Pebble phone app, find **Jot → Settings** (the gear icon). The read-only Jot backup page shows your most recent transferred notes.
4. Use **Copy notes** or **Select all** and paste into a permanent notes app or file. **Save .txt** and **Save .json** are also offered, though some in-app webviews may not support downloads.

If the watch says `Phone not responding` or `Phone save failed`, the previous complete phone snapshot is kept. Retry when connected. If you cancel midway, no partial snapshot overwrites the previous one. You may need to close and reopen the Jot settings page to see newly transferred notes.

### One-time GitHub Pages setup

The companion's read-only export viewer is at
`https://majormajormajormajor.github.io/pebble-jot/export.html`.

For that link to work, the repository owner must enable **GitHub Pages** in
**Settings → Pages → Build and deployment → Deploy from a branch**, choose
**main** and **/docs**, then save. This is one-time hosting of the *empty
static viewer*, not hosting any note data. Until Pages is enabled and
published, the watch-to-phone transfer can work but the viewer won't open.

### Privacy / durability

- No Jot backend receives the notes. The Pebble phone companion stores a single latest complete snapshot in its own local storage. It can be erased by resetting the companion app or its storage.
- When opening the viewer, the snapshot is carried in the URL **fragment** (after `#`), which is not included in HTTP requests for the page. The viewer clears the fragment after reading it. Nonetheless, a copied URL, screenshots, or browser history could expose the notes. **Do not share the export URL.**
- The static page has no trackers or external scripts; the page's HTML is public but your notes are not committed to GitHub.
- This is a **manual snapshot**, not ongoing background sync. Re-export after editing notes, and copy to a separate permanent location before relying on it as a backup.
- An end-to-end export of one note has been demonstrated on the real watch and Android phone, including the `Phone copy saved` indicator and the rendered viewer. Multi-note transfers, disconnect recovery, and full-capacity exports still require broader device testing.

## Roadmap

1. Verify end-to-end phone backup and export on real hardware (including Unicode, disconnects and failure recovery).
2. Device testing of note persistence, full capacity, relaunch, and deletion.
3. Verify the 1.4.0 multiline editor display on real Emery, then implement safe cursor editing in stages.

The earlier `jot-v0.1.patch` instructions are historical; this repository now contains the Jot source directly. Simply pull and build the branch.

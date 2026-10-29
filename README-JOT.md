# Jot — detailed behavior and developer notes

Jot is an early on-watch note-taking app for Pebble Time 2, built on the MessagEase-inspired touchscreen gesture keyboard. The main [README](README.md) is the best starting point.

## Current controls

- **Launch:** go directly to a blank editor.
- **Editor Select:** save this note and start a new one.
- **Editor text strip tap:** peek at the full current draft without saving or switching notes.
- **Editor Space swipe left / right:** move the insertion cursor by one UTF-8 codepoint.
- **Editor I swipe up / S swipe down:** move between explicit newline-separated text lines, retaining a preferred column.
- **Editor I up-and-return / S down-and-return:** jump to the beginning / end of the active note.
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

## Hairline insertion caret (v1.5.1 — watch verification pending)

The Jot 1.5 editor represented the cursor by temporarily inserting a literal
`|` into its display buffer. This never modified the saved note, but it
shifted nearby characters as the cursor moved and made the caret
indistinguishable from a genuine pipe character.

The new editor uses a **separate transparent Layer** above the unchanged
word-wrapped TextLayer. It draws a **1px charcoal vertical caret** at the
UTF-8 insertion boundary, with height based on the font's line spacing.
Pebble's native text-layout measurements locate the caret in the current
line and keep that line visible as the note scrolls. The display no longer
inserts or deletes any character to represent the cursor.

This change affects **only the editor display**. Note text, cursor gestures,
saved-note records, the 40px editor/keyboard boundary, touch targets, and
the full-screen reader are unchanged. GitHub's regression suite now includes
a guard against accidentally returning to printable-glyph caret rendering.

**On-watch findings (v1.5.1):** The independent hairline caret works
well, and a typed `|` is visually distinct. Hardware screenshots in
[issue #16](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/16#issuecomment-6503088514)
also exposed three polish needs: when moving left from a line start the
caret sometimes jumped to blank space; its stroke stopped above descenders;
and the last row could dip below the keyboard edge.

**v1.5.2 follow-up:** A cursor positioned immediately before an LF now uses
the preceding line's measured text without adding a fake probe character
(which could force a spurious wrap). Trailing spaces at that hard break are
excluded from *visual* placement, but remain completely unchanged in the
note. For empty lines after LF, the measurement probe is still used so their
line is counted. The caret extends two pixels past the line step, with a
five-pixel scroll clearance for descenders along the 40px editor boundary.

These are **visual-only fixes**, not a change to cursor movement, saved text,
or keyboard geometry. They pass CI's source-level layout checks, but await
another real-watch visual inspection; issue #16 remains open until confirmed.

## Cursor editing (v1.5 — watch verified)

The original `messagease keys.xlsx` design map is now preserved in
[readable Markdown](docs/gesture-map.md) and
[complete CSV](docs/gesture-map.csv). These include both **implemented** and
**planned** bindings. In particular, the six navigation controls above were
*already reserved* by the spreadsheet, even though the C binding table did not
implement them. Blank C entries are not unassigned design space.

The note store now tracks a **transient UTF-8 byte offset** for the cursor.
Insertion and backspace operate at that position without splitting a UTF-8
codepoint, and undoing a newline merges lines. Moving the cursor does not
mark a clean note dirty. Opening a saved note starts at its end; starting a
new note resets the cursor. The caret is drawn in the 40-pixel keyboard
viewport at the correct insertion point, which scrolls to follow it.

**Known limitations:** Up/Down navigates explicit LF-delimited lines, not
individual *visually wrapped* rows yet. The editor still uses tap-to-peek
rather than tap-to-position, and word navigation/forward delete are reserved
by the design map but not implemented. The original gesture recognizer,
keyboard geometry, 16-note capacity, and on-disk `JOT1` records are unchanged.

Native host tests cover cursor movement, insertion/deletion in the middle
of Unicode text, vertical-column memory, full-buffer rejection, persistence,
and reopening saved notes. GitHub CI checks the original gesture map
against the six implemented cursor bindings. All six navigation gestures
were successfully tested on a physical Emery; issue #3 is closed.
Follow-ups #16–#18 track caret rendering, touch positioning, and
visual-line navigation separately.

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

As of v1.5 the caret can move and mid-note editing is supported through
the original keyboard navigation gestures. See the cursor editing section
above for details and current limitations.
The strip continues to open a read-only preview on tap. The small `*`
marker appears while dirty; `SAVE ERROR` and `FULL` are prominent status
badges. The normal editor no longer spends an entire line displaying `JOT`.

The editor uses a clipping Layer with a word-wrapped TextLayer and,
since v1.5.1, a separate hairline-caret overlay. The keyboard's 40px touch
boundary has not changed. The note storage format, save schedule, gesture bindings and limits
remain unchanged. The multiline viewport was successfully tested on real Emery hardware.

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
3. Verify the 1.5.2 caret-placement refinements on real Emery, then implement forward delete and navigation polish.

The earlier `jot-v0.1.patch` instructions are historical; this repository now contains the Jot source directly. Simply pull and build the branch.

# Jot — a pocket notebook for Pebble

**A thought, one button away.** Jot is a lightweight, offline note-taking app for the Pebble Time 2. It opens directly into a writing surface and uses a compact touchscreen gesture keyboard for entering notes without reaching for your phone.

Jot grew out of the [MessagEase-inspired gesture keyboard experiments](https://github.com/MajorMajorMajorMajor/messagease-test). Its keyboard supports taps, eight-direction swipes, eight directional drag-and-return gestures, and clockwise/counterclockwise circles. Clockwise circles on the letter grid enter digits 1–9 in calculator order; clockwise on Space enters 0.

## Status

**Early working prototype.** Jot has been built and photographed running on a real Pebble, with live text entry and a notes list visible on-device. This does **not** yet establish comprehensive verification of persistence, deletion, or every gesture on all hardware. The app currently targets **emery (Pebble Time 2)**.

Notes remain on the watch. Jot v0.2 adds an **experimental manual phone backup** over Bluetooth. This is not continuous sync, and the copy in the Pebble phone companion is not yet a durable external backup; **do not store irreplaceable information** until you've copied the notes to another location. Uninstalling Jot may remove its local notes.

## Using Jot

Launch Jot from your watch or an assigned quick-launch button. It opens straight into the editor.

| Location | Action | Result |
| --- | --- | --- |
| Editor | Type on touchscreen | Enter text with gestures |
| Editor | Backspace tap / NW / N / SW / W | Delete character before caret |
| Editor | Backspace swipe NE / E / SE / S | Delete character after caret (forward delete) |
| Editor | Select | Save a new note and open the list; when editing a saved note, save and return to its reader |
| Editor | Tap the text strip above keyboard | Peek at the entire current note without changing it |
| Editor | Space swipe left / right | Move the insertion cursor one Unicode character |
| Editor | I swipe up / S swipe down | Move cursor to previous / next explicit text line |
| Editor | I up-and-return / S down-and-return | Move cursor to start / end of note |
| Editor | Up / Down | Open the notes list |
| Editor | Tap **KB** | Open the notes list |
| Editor | Back | Save and show notes list; while editing an existing note, save and return to its reader |
| Notes list | Up / Down | Move one note at a time; **Up** from newest focuses **+ NEW** |
| Notes list | Select | Open the highlighted note, or create a note when **+ NEW** is focused |
| Notes list | Long Select | Request deletion; Select confirms, Back cancels |
| Notes list | Tap a note card | Open that note in the full-screen reader |
| Notes list | Drag vertically on cards | Smoothly scroll through notes; the selected note follows |
| Notes list | Tap **+ NEW** in the header | Open a fresh blank editor (touch remains optional) |
| Notes list | Back | From older notes or **+ NEW**, return to newest; from newest, exit Jot |
| Reader | Up / Down | Scroll the full note |
| Reader | Select | Open the note in the keyboard editor |
| Reader | Back | Return to the notes list |
| Draft peek | Tap, Select, or Back | Return to the same editor |
| Editor | Long Select (~1.2 s) | Open the legacy typing / trace diagnostics |

The editor uses a small `*` in its upper-right corner to indicate unsaved edits. Jot automatically saves after about **1.8 seconds** without typing and at several navigation boundaries. `SAVE ERROR` means the storage write did not succeed; `FULL` means the attempted character did not fit.

## Screen navigation (v1.6 — watch test pending)

The editor now remembers when it was opened from a saved-note reader. **Select and Back after editing** first commit the draft, then reopen the *same reader* using the newly persisted text instead of an outdated snapshot. **Select on a quick-capture note** saves it, resets the editor to an empty note and opens the notes list with the saved note selected. **Back on a quick-capture note** saves and opens the list but keeps the current editor buffer; only pressing Select finishes that capture.

**Jot 1.6.1** makes the header's **+ NEW** pill button-selectable as an item immediately above the newest note. Press **Up** from the newest note to focus it, then **Select** to begin writing; **Down** returns to the newest note. The former `x / 16` badge has been removed for a larger, clearer button; tapping the pill still works. If the list is empty, **+ NEW** starts selected automatically. Pressing Back while viewing an older list position jumps to the top first; pressing Back from the top exits to PebbleOS. The full-screen reader's Back still returns to the notes list. Tap-to-peek remains read-only and returns to the same draft. If a save fails, Jot keeps the editor and text on screen, showing **SAVE ERROR** rather than discarding any writing.

The keyboard geometry, recognizer, caret rendering, note storage format, and existing backup flow are unchanged. These stack transitions still need a physical Pebble Time 2 check.

## Touch the notebook (v1.7 — watch test pending)

The saved-notes list now responds to **taps and vertical drags** as well as the
physical buttons. Tap an actual note card to open it directly in the reader,
or slide a finger up/down across the cards to scroll with pixel-level
movement. The yellow header (including **+ NEW**) and charcoal backup footer
stay fixed. Swipes don't accidentally open notes: after a gesture crosses a
10-pixel movement threshold, release is never treated as a tap. Blank paper,
the spaces between cards, and header/footer areas are not note-card targets.

The highlighted note follows the visible list while dragging, and physical
Up/Down/Select/Back continue to work afterward. The original **+ NEW**
button-and-touch interaction, deletion confirmation, and backup controls
are preserved. Scrolling currently follows the finger directly rather than
continuing with kinetic/inertial momentum after release; that's optional
future polish depending on real-watch feedback.

The new pure-C geometry tests cover card edges, partially scrolled cards,
scroll bounds and empty lists. Pixel interaction still needs checking on
Pebble Time 2.

## Read your notes full-screen (v1.2)

Select a note in the notes list to open a **read-only paper page**, with its timestamp and complete text. Scroll with Up / Down. Select opens that note for editing using the unchanged gesture keyboard; Back returns to the same place in the list. Browsing a page does not overwrite or resave your current draft.

Jot 1.3 adds **tap-to-peek** from the text strip above the keyboard. The preview copies the current draft without saving or switching notes; tap again (or press Select/Back) to return to typing. Reader text uses a larger, bold font for legibility.

**Jot 1.4 introduces a true multiline editing preview.** The 40-pixel text strip now displays the actual note with word wrapping and line breaks, scrolling automatically to keep the last line and its `|` caret in view. A newline moves that caret onto a fresh line; backspace removes the newline as usual. The status badge appears only when necessary. This first pass enabled multiline writing; **Jot 1.5 introduces cursor editing** through six controls already assigned in the original gesture-map spreadsheet: Space left/right moves by one UTF-8 character; I up / S down moves between explicit newline-separated lines; I up-and-return / S down-and-return jumps to the start or end of the note. Text insertion and backspace now operate at the cursor, and the tiny editor viewport follows it. **Jot 1.5.1 replaces the printable `|` caret with a separate, single-pixel overlay:** moving the cursor no longer pushes the surrounding characters sideways, and a literal `|` typed into a note remains distinct. Pebble's font-layout measurements position the caret without changing note contents. **Jot 1.5.2** extended the caret to the font's descenders and added bottom-edge clearance. **Jot 1.5.3** fixes a separate issue: the caret could land far to the right of text that had wrapped onto the next line. Its new visual-line layout helper follows actual word breaks from the beginning instead of measuring a backward-fitting suffix. **Jot 1.5.4 adds forward delete:** swipe NE, E, SE or S on Backspace to erase the character *after* the caret. Other Backspace gestures still erase backward. Word-wrapped Up/Down *cursor navigation* and touch-to-place-cursor remain future refinements.

The keyboard's layout, recognizer thresholds, note storage format and 159-byte limit remain unchanged. See the [complete readable gesture map](docs/gesture-map.md) and its [CSV source](docs/gesture-map.csv). This map records original design intent—including commands not yet implemented—not a list of features currently working on a watch.

## Export notes to your phone (v0.2)

In the notes list, **hold Up** for about one second. Wait for **Phone copy saved** on the watch. This indicates the entire note snapshot was received and stored in the Pebble phone companion. The watch retains its notes.

In the Pebble phone app, open **Jot → Settings** (gear icon) to view your latest backup and **Copy notes** or save a `.txt`/`.json` file. Browser/webview download support may vary; copying is the fallback.

**One-time setup:** The repo's empty static export viewer must be published with GitHub Pages: **Settings → Pages → Deploy from a branch → main → /docs**. The page doesn't host your note data; the phone passes its local snapshot in the URL fragment, which the page decodes locally. The fragment can reveal private notes to anyone who receives that URL, so don't share it.

See [README-JOT.md](README-JOT.md) for detailed limits and failure handling. This feature needs on-watch/phone verification.

## Storage limits

The current storage model is intentionally bounded:

- Up to **16 notes**, each containing **159 UTF-8 bytes** of text (not necessarily 159 characters).
- Notes are timestamped and displayed newest first.
- When full, Jot refuses new notes rather than overwriting existing ones.
- Notes live in Pebble's local persistent storage; phone backup is manual and does not delete on-watch notes.

## Building

Install the Pebble SDK, then from this repository:

```sh
pebble build
pebble install --emulator emery
```

For a physical watch, use the installation method supported by your connected Pebble setup (for example, `pebble install --cloudpebble` if configured). Set up the SDK and device connection first.

## Code layout

| Path | Responsibility |
| --- | --- |
| `src/c/jot/jot.c` | Jot editor, notes list, autosave, navigation |
| `src/c/jot/store.c` | Bounded on-watch note persistence |
| `src/c/keyboard/` | Keyboard, key layouts, state and bindings |
| `src/c/ui/` | Touch classifier, drawing, input and diagnostics |
| `src/c/main.c` | App startup and integration |
| `package.json` | Pebble app metadata and supported platforms |

See [README-JOT.md](README-JOT.md) for detailed controls and limits, and [TOUCH_TRACES.md](TOUCH_TRACES.md) for gesture tracing.

## Next steps

The initial cursor controls and hairline-caret concept have been verified on Emery. The immediate priority is **on-watch verification of Jot 1.5.3's wrap-aware caret positioning**, then forward deletion, smoother screen navigation, typography, touch-to-caret positioning, dependable backups, and storage expansion. The keyboard's touch targets remain unchanged.

Pebble SDK documentation: <https://developer.repebble.com>

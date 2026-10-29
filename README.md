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
| Editor | Select | Save the current note and start a new one |
| Editor | Tap the text strip above keyboard | Peek at the entire current note without changing it |
| Editor | Space swipe left / right | Move the insertion cursor one Unicode character |
| Editor | I swipe up / S swipe down | Move cursor to previous / next explicit text line |
| Editor | I up-and-return / S down-and-return | Move cursor to start / end of note |
| Editor | Up / Down | Open the notes list |
| Editor | Tap **KB** | Open the notes list |
| Editor | Back | Exit and save on normal app shutdown |
| Notes list | Up / Down | Move through saved notes |
| Notes list | Select | Read the highlighted note full-screen |
| Notes list | Long Select | Request deletion; Select confirms, Back cancels |
| Notes list | Back | Return to editor |
| Reader | Up / Down | Scroll the full note |
| Reader | Select | Open the note in the keyboard editor |
| Reader | Back | Return to the notes list |
| Draft peek | Tap, Select, or Back | Return to the same editor |
| Editor | Long Select (~1.2 s) | Open the legacy typing / trace diagnostics |

The editor uses a small `*` in its upper-right corner to indicate unsaved edits. Jot automatically saves after about **1.8 seconds** without typing and at several navigation boundaries. `SAVE ERROR` means the storage write did not succeed; `FULL` means the attempted character did not fit.

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

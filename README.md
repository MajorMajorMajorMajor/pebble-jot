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
| Editor | Type on touchscreen | Enter or delete text with gestures |
| Editor | Select | Save the current note and start a new one |
| Editor | Up / Down | Open the notes list |
| Editor | Tap **KB** | Open the notes list |
| Editor | Back | Exit and save on normal app shutdown |
| Notes list | Up / Down | Move through saved notes |
| Notes list | Select | Open the highlighted note for editing |
| Notes list | Long Select | Request deletion; Select confirms, Back cancels |
| Notes list | Back | Return to editor |
| Editor | Long Select (~1.2 s) | Open the legacy typing / trace diagnostics |

The `JOT *` heading means there are unsaved edits. Jot automatically saves after about **1.8 seconds** without typing and at several navigation boundaries. `SAVE ERROR` indicates the storage write did not succeed.

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

The immediate priorities are hardware verification of the v0.2 manual export, a more durable backup destination, real-watch testing of persistence, and UI polish that keeps the keyboard touch targets intact.

Pebble SDK documentation: <https://developer.repebble.com>

## Jot 1.17.0 — Shift and Shift Lock (#33; Emery QA pending)

On the **ABC** keyboard, swipe **R up** to cycle through three modes:
**normal → uppercase next letter → uppercase lock → normal**.
Swipe **R down** to cancel immediately from either shifted mode.
The R key shows an upward triangle in normal mode, a brighter upward
triangle plus a downward cancel arrow in one-shot mode, and an extra
horizontal bar under the bright triangle in locked mode, following
the Android reference. Both the **nine large central letters and all
alphabetic peripheral legends** change to uppercase in either shifted
mode. Punctuation and control icons never change case.

One-shot Shift is consumed by one alphabetic character entered using
an ABC gesture, not by moving the cursor, punctuation, Space,
Backspace, Save, Return, or switching briefly to 123. Caps Lock stays
enabled until R-down or the third R-up. The numeric face retains its
own unchanged bindings and labels. Finishing a note and entering the
list/reader clears Shift so it cannot unexpectedly carry to another
note. Shift is transient and never persisted at startup.

No new key layers, icons, geometry, touch hitboxes or note storage
changes. Pure C tests cover all three modes and ASCII/UTF-8 casing;
the SDK build and real Ember wrist usability remain to be verified.

# Jot — controls and developer guide

This is the **living** reference for how Jot works today on **Pebble Time 2 (Emery)**. Start with the shorter [README](README.md) if you're new to the project.

For release-by-release decisions, see the [changelog](CHANGELOG.md). The old, much longer developer notes have been [archived verbatim](docs/history/README-JOT-legacy.md) rather than discarded.

## Editor and keyboard

Launch Jot to open the note editor. The small ruled notebook strip shows wrapped text and a hairline caret; tap it for a full-screen, read-only **draft Peek**, then tap, Select, or Back to resume.

The MessagEase-inspired keyboard supports taps, eight straight swipes, eight outward-and-return gestures, and clockwise/counterclockwise circles. The **ABC** face shows the central `a n i h o r t e s` and all 17 remaining alphabetic letters as small directional legends. The **123** face offers digits and punctuation. Some control gestures described in the canonical workbook are deliberately not implemented.

| Gesture or button | Editor behavior |
| --- | --- |
| Tap/swipe letter or numeric keys | Insert the mapped character |
| Space swipe W/E | Move the caret back/forward one Unicode codepoint |
| I swipe N / S swipe S | Move caret across explicit newline-separated lines |
| I return N / S return S | Go to beginning/end of the note |
| Backspace tap, NW, N, SW, W | Delete preceding character |
| Backspace swipe NE, E, SE, S | Delete following character |
| Hold a supported cursor/delete gesture | Start key repeat after ~550 ms; repeat every ~135 ms until liftoff, even with finger drift |
| Tap the SAVE key | Commit the note and open its reader; a blank draft does nothing |
| Circle on SAVE | Cycle the four editor/keyboard orientations; a plain tap still saves |
| Physical Select | Save a new note and enter list, or save an edit and return to its reader |
| Physical Back | Save and return to list/previous reader |
| Physical Up/Down | Open the notes list |
| Long physical Select | Open the typing/gesture diagnostics |

**Four orientations:** clockwise circles on SAVE cycle left controls/top strip → right controls/top strip → right controls/bottom strip → left controls/bottom strip → left/top. Counterclockwise circles reverse the sequence. The setting persists separately from your notes; the gesture map and actual key hitboxes follow the displayed layout.

An asterisk (`*`) marks pending changes. Jot attempts autosave approximately **1.8 seconds** after edits and at navigation boundaries. `SAVE ERROR` means the write failed; the note must not be assumed safely committed. `FULL` means the attempted insertion exceeded the available bounds.

**Do not add hidden Spacebar label toggles.** Primary alphabetic labels are visible by default, and secondary visibility belongs to future settings work. [Tab](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/31), [Compose](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/32), [Shift](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/33), and [secondary labels](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/34) have separate issue histories; consult their current status before planning changes.

## Notes list

The newest notes appear first. `+ NEW` provides a dedicated entry point for a blank note.

| Action | Behavior |
| --- | --- |
| Up/Down | Change selection (Up from newest focuses `+ NEW`) |
| Select | Open highlighted saved note or `+ NEW` |
| Tap a note card | Open that note |
| Touch drag/flick | Scroll through cards with momentum; a new touch stops movement |
| Long Select | Begin a two-step list deletion; Select confirms and Back cancels |
| Back | Return toward newest selection, then exit from newest/empty list |
| Long Up (~900 ms) | Start manual watch-to-phone export |

## Full-screen reader and triage

The reader displays the saved note on paper with a timestamp, word-wrapped text, and a note-position indicator. It does not silently edit or save the note.

| Action | Behavior |
| --- | --- |
| Vertical drag/flick on body | Scroll long text with bounded kinetic momentum |
| Up/Down with action bar closed | Scroll the current note, 60px per press |
| Tap visible saved-note text | Edit the current note directly |
| Select | Open the native reader action bar |
| Up/Down with action bar open | Browse newer/older saved notes without returning to the list |
| Select with action bar in Edit mode | Edit the displayed note |
| Long Select | Open the action bar's persistent Delete mode |
| Select twice in Delete mode | First arm, then confirm removal |
| Up/Down or Back while delete is armed | Cancel the pending deletion without removing the note |
| Back while action bar is open | Close it; a further Back returns to the list |
| Back with no action bar | Return to notes list |

A successful deletion selects an adjacent surviving note and retains Delete mode. The final deletion returns to the list. The action bar is intentionally unavailable in **draft Peek**; that view always returns to the same draft. Scrolling and navigation should never cause accidental editing.

## Storage and durability

The **JOT2** format supports **256 logical note slots**, up to **4,095 UTF-8 bytes per note**, subject to an **up-to-1 MiB per-app persistence budget**. Character count is not byte count. Because all notes, headers, indexes, and temporary save copies share the budget, 256 maximum-sized notes cannot be assumed to fit simultaneously.

On-watch storage is transactional: an edit writes chunks to the inactive bank and commits a new header last. On restart, Jot chooses a valid committed version; a failed write should leave the older committed note recoverable. Each Pebble persisted value is limited to 256 bytes, so long notes use chunks. A small conservative directory speeds launch by avoiding unnecessary empty-slot reads; missing/damaged hints trigger a full rescan rather than replacing authoritative note data.

The old JOT1 layout (16 × 159 bytes) was retired in the 1.9 QA storage reset; **there is no automatic JOT1 migration**. Anyone updating from a very old installation should have an independent backup first.

Long-note hardware tests, interruption recovery, and full-budget exhaustion tests remain worth repeating before relying on Jot for irreplaceable writing.

## Phone backup and privacy

Phone backup is **manual**, not continuous synchronization, and it does not remove on-watch notes.

1. Open the notes list and **hold Up** for about a second.
2. Let the watch finish transferring the snapshot. `Phone copy saved` means the Pebble companion accepted the full transfer into its local storage, **not** that an independent durable file exists.
3. Open **Jot → Settings** in the Pebble phone companion. Use **Copy notes**, **Save .txt**, or **Save .json** to preserve a separate copy. Downloads may not work in every embedded webview; copy/paste is a fallback.

If the watch shows `Phone not responding` or `Phone save failed`, retry once connected. An incomplete transfer should not replace a previous complete companion snapshot. Close and reopen the settings page if it shows an older snapshot.

The optional read-only export viewer lives at `docs/export.html`. Repository owners can publish it using GitHub Pages (**Settings → Pages → Deploy from a branch → main /docs**). It receives serialized notes in a URL fragment, decodes them locally, and attempts to clear the fragment afterward; no Jot backend hosts note content. **A copied link, browser history, or screenshot may still reveal private notes; do not share an export link.**

Large snapshots may exceed mobile URL/webview limits, and end-to-end restore of a full 1 MiB archive is **not established**. Re-export after editing and copy important notes outside both watch and phone companion. Uninstalling Jot or resetting companion storage may erase the respective copies.

## Build and test

The app currently targets `emery` in `package.json`. With the [Pebble SDK](https://developer.repebble.com/) installed:

```sh
pebble build
pebble install --emulator emery

# Or, on a configured connected watch:
pebble install --cloudpebble
```

The CI suite is organized by **four user behaviors**, not by internal
implementation helper: **keeping notes** (seven recovery scenarios),
**reading notes** (navigation, deliberate deletion, tap/drag),
**writing notes** (wrapping, caret, editing hold/repeat), and **backing
up notes** (atomic transfers, retries, export). Each file has a few
named behavior scenarios; individual assertions are checkpoints inside
them, not independent test cases.

The four entry points live under `tests/behavior/` and are run by
`.github/workflows/behavior-tests.yml`. To run them locally:

```sh
node tests/behavior/backup.js
cc -std=c11 -Wall -Wextra -Werror -I tests/fake -I src/c \
  tests/behavior/notes.c src/c/jot/store.c -o /tmp/jot-notes
/tmp/jot-notes
cc -std=c11 -Wall -Wextra -Werror -I src/c \
  tests/behavior/reading.c src/c/jot/reader_review.c \
  src/c/jot/reader_touch.c -o /tmp/jot-reading
/tmp/jot-reading
cc -std=c11 -Wall -Wextra -Werror -I src/c \
  tests/behavior/writing.c src/c/ui/caret_layout.c \
  src/c/ui/key_repeat.c -o /tmp/jot-writing
/tmp/jot-writing
```

The tests deliberately retain focused failure-mode coverage rather
than exhaustively enumerating every adjacent pixel or state transition.
The storage suite uses simulated Pebble flash, not real hardware.
Visual feedback, optical alignment, handedness, list gestures, and
kinetic animations are left to on-watch QA.

Run **on-watch QA** for changes to touch recognition, layer lifetimes, optical alignment, firmware RAM, storage, or animation. Host tests cannot prove real wrist ergonomics or a successful SDK metadata-injection build.

## Architecture and authoritative references

| Path | Responsibility |
| --- | --- |
| `src/c/jot/jot.c` | Editor, note list, navigation, and save paths |
| `src/c/jot/store.c` | JOT2 two-bank note storage |
| `src/c/jot/reader.c` | Saved-note reader, Peek and triage controls |
| `src/c/keyboard/` | Letter/numeric mappings and keyboard state |
| `src/c/ui/touch.c` | Gesture recognition, taps, repeats and flick handling |
| `src/c/ui/legend_geometry.c` | **One** compass layout and all ABC/numeric optical offsets |
| `tests/` | Regression tests and fake SDK fixtures |
| `docs/export.html` | Optional static phone backup viewer |

- [Canonical gesture design reference](docs/gesture-map.md) and [CSV](docs/gesture-map.csv). These preserve *intended* actions, including not-yet-implemented gestures; confirm the runtime binding before documenting a shortcut as working.
- [Numeric legend tuning guide](docs/legend-tuning.md), with key and compass indices. Preserve the alphabet's approved offsets when calibrating numeric symbols.
- [Changelog](CHANGELOG.md) for historical releases, [historical developer archive](docs/history/README-JOT-legacy.md) for older decisions, and [GitHub Issues](https://github.com/MajorMajorMajorMajor/pebble-jot/issues) for the live roadmap.

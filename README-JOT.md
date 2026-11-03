## Jot 1.18.3 — Compose corner pixel polish (Emery QA pending)

The Compose inlay is now **truly flush** with the A key's upper and
left face edges. Its outer northwest corner matches the parent key
radius, while its inner lower-right corner is deliberately square.
The cream C is nudged 1 px downward for optical balance. The existing
outer ink border and 1px bevel still paint on top for an uninterrupted
outline. No changes to glyph mappings, touch geometry, note storage,
Compose behavior, or Shift.

## Jot 1.18.2 — flush corner Compose inlay (Emery wrist QA pending)

The small inverted Compose `C` on the A key now uses an inset
rounded blue corner, matching the key's outer upper-left curve and
rounding the inner lower-right corner like the approved *Before* design.
It sits flush inside the existing one-pixel key outline; the outer
outline and highlight are drawn over the inlay for a continuous edge.
The calibrated cream `C` stays in exactly the same northwest text slot.
No changes to note data, Unicode Compose, Shift, input hitboxes, or layout.

## Jot 1.18.1 — Inverted Compose legend (#32; Emery QA pending)

The tiny `C` at A↖ is now cream lettering on a filled blue
rectangle, visually distinguishing the Compose *action* from the
alphabetic swipe letters. The background aligns to the same calibrated
16×15 northwest compass slot without changing key dimensions,
gesture mapping or typography. No new Pebble layers or assets.
Compose and all existing editor functions are unchanged.

## Jot 1.18.0 — Retroactive Compose (#32; wrist QA pending)

Swipe **A up-left** on the ABC face to transform text immediately to
the **left of the insertion cursor**, even when navigating inside a
previously saved note. The canonical MessagEase pairs include
`o`+`e` → `œ`, `a`+`´` → `á`, `T`+`M` → `™`, and Greek/math
symbols. For a recognized two-codepoint sequence, Compose replaces
both with the resulting Unicode character. If no pair matches, it
cycles the nearest preceding character through supported alternatives.
The `s` cycle is deliberately `s → š → ş → ŝ → ß → ∫ → s`,
including when any variant was inserted directly by a numeric gesture.
Uppercase has a companion `S → Š → Ş → Ŝ → S` cycle. A cursor moved
through existing text works identically to freshly typed input.

The full #32 reference (223 entries, 217 distinct pairs, first
mapping wins on duplicates) is compiled as static UTF-8 data.
An inverted cream-on-blue `C` in the A-key northwest corner makes the
previously reserved gesture discoverable without a new layer. Note text
is changed only by an **atomic, UTF-8-boundary-safe replacement** that
preserves cursor and suffix and refuses changes that exceed the 4095-byte
note limit. Unknown input, an empty prefix and unusable cursor positions
are no-ops; no Compose mode, queue, extra layers or startup-time work.
The active Shift mode is unchanged by Compose. The watch's stock fonts
may lack glyphs for some rare symbols; they remain safely stored UTF-8.
The full Emery build and real hardware display/cursor verification must
pass before closing #32.

## Jot 1.17.2 — stronger one-shot Shift icon (Emery QA pending)

The one-shot Shift **blue upward outline on R** now has a second
1-pixel inset outline, creating a clearer double-weight arrow at
Pebble scale. Its 13×8 outer silhouette does **not** grow. Normal Shift
stays the original thin **teal outline**; Shift Lock remains the
**solid blue triangle**. The downward teal cancellation arrow,
alphabet case switching, controls and all keyboard gestures are
unchanged.

The second outline uses only Pebble-supported 1px graphics lines
and is drawn on the same R-key layer with no extra allocation.
Visual contract tests guard the inset geometry and state restriction.
Emery contrast/appearance still needs a wrist test.

## Jot 1.17.1 — distinct R-key Shift indicators (Emery QA pending)

Refine the R-up glyph within its **original 13×8 pixel footprint**:
- **No Shift:** outlined upward triangle in the normal teal accent
  (RGB 0,170,170).
- **Shift once:** the same outline in the dark blue already used for
  keyboard peripheral legends (RGB 0,85,170).
- **Shift Lock:** a **solid filled blue triangle**, with no extra bar.

The down-pointing teal cancel triangle remains visible in both shifted
states. This three-way icon treatment matches the approved visual mockup
while solving the pale-on-cream contrast and insufficient space for a
separate lock bar. It uses only supported Pebble line primitives on
the existing R key layer, without allocations or new assets.

The actual R-up/R-down gestures, uppercase center/peripheral legends,
one-shot behavior, Shift Lock, keyboard flips, text editor and storage
are unchanged. New visual contracts in the writing behavior tests keep
outline/fill, colours, geometry and the cancellation arrow intentional.
The Nix Emery build runs in CI; final contrast/legibility needs wrist QA.

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
the SDK build and real Emery wrist usability remain to be verified.

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

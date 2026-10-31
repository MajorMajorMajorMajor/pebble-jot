# Jot — a pocket notebook for Pebble

**A thought, one button away.**

Jot is a small, offline-first notebook for **Pebble Time 2 (Emery)**. Launch it from your watch, write with a compact MessagEase-inspired gesture keyboard, and browse your notes on a paper-inspired screen—without pulling out your phone.

Jot is actively developed and tested on real Pebble hardware. This repository contains the application and its developer tools; it is not a promise of a finished, fully stress-tested public release.

## What it does

- **Write on your wrist.** Tap, swipe, return-swipe, or circle on a gesture keyboard with the full alphabet visible. Switch between alphabetic and numeric faces without changing your draft.
- **Edit longer thoughts.** Use a multiline writing strip, a hairline caret, cursor gestures, backward/forward delete, and hold-to-repeat editing.
- **Pick your grip.** Circle on the editor's SAVE key to cycle four keyboard/editor orientations: left/right control column × notebook strip at top/bottom.
- **Read and triage.** View complete notes on ruled paper, touch-scroll with momentum, or open the reader action bar to browse, edit, and deliberately delete notes.
- **Keep notes on the watch.** Jot uses bounded, transactional local storage, with an optional **manual** phone snapshot/export workflow.

## Get started

Install the [Pebble SDK](https://developer.repebble.com/), then build from this repository:

```sh
pebble build
pebble install --emulator emery
```

For a connected physical Pebble Time 2, use your configured watch-install method, for example:

```sh
pebble build && pebble install --cloudpebble
```

Jot currently targets **Emery only** (`package.json`).

## Using Jot

Jot opens directly into the editor. The most useful shortcuts are:

| Where | Action | Result |
| --- | --- | --- |
| Editor | Tap or swipe a key | Enter its displayed character or assigned command |
| Editor | Tap the ruled text strip | Peek at the full draft without changing it |
| Editor | Swipe left/right on Space | Move the caret one Unicode character |
| Editor | Swipe up on I / down on S | Move the caret between explicit text lines |
| Editor | Backspace tap or reverse-direction gestures | Delete before the caret |
| Editor | Backspace NE / E / SE / S | Delete after the caret |
| Editor | Tap SAVE | Save and open the note in the reader |
| Editor | Draw a circle on SAVE | Cycle the four keyboard/editor orientations |
| Notes list | Touch-scroll, tap a card, or use Up/Down and Select | Find and open a note |
| Reader | Drag/flick the page | Read a long note smoothly |
| Reader | Select | Open the action bar for browsing/editing |
| Reader | Long Select | Enter deliberate, confirmed deletion mode |

The alphabetic peripheral legends are always visible; there is **no gesture to hide them**. The numeric legends are being fine-tuned separately.

For full button behavior, note deletion safeguards, keyboard gestures, and on-watch diagnostics, read the [detailed controls and developer guide](README-JOT.md) and the [gesture reference](docs/gesture-map.md). The gesture reference includes *planned* commands that are not necessarily implemented.

## Notes, storage, and backup

Jot supports up to **256 note slots** and **4,095 UTF-8 bytes per note**, subject to the watch's shared **up-to-1 MiB** per-app persistence budget. You cannot assume 256 maximum-size notes will fit together.

The watch stores notes locally. Phone export is **manual**, not live sync: from the notes list, **hold Up** to transfer a snapshot to the Pebble companion, then open Jot's settings in the companion to copy or save it. A phone snapshot is not the same as an independently saved backup. **Copy important notes to durable storage before relying on them**; removing the app or clearing companion storage can remove data.

See [backup details and safety notes](README-JOT.md#phone-backup-and-privacy).

## Project resources

| Resource | Purpose |
| --- | --- |
| [Detailed Jot guide](README-JOT.md) | Current controls, data handling, testing, and architecture |
| [Gesture reference](docs/gesture-map.md) · [CSV](docs/gesture-map.csv) | Canonical design mappings, including not-yet-wired commands |
| [Numeric legend tuning](docs/legend-tuning.md) | Pixel-level placement work on the 123 keyboard |
| [Changelog](CHANGELOG.md) | Release history, design iterations, and past QA findings |
| [Historical developer notes](docs/history/README-JOT-legacy.md) | Archived detailed release notes |
| [GitHub Issues](https://github.com/MajorMajorMajorMajor/pebble-jot/issues) | Bugs, feature planning, and current priorities |

## For contributors

The core code is in `src/c/jot/` (notes, reader, persistence), `src/c/keyboard/` (layouts and mappings), and `src/c/ui/` (touch and drawing). Tests live in `tests/`, with regression checks in GitHub Actions.

Run the relevant host tests before committing a change, and build/install on Emery for visual and interaction changes. Actual watch behavior takes precedence over a simulator or screenshot mockup.

**Keep this README about how Jot works today.** Put release-by-release details in [CHANGELOG.md](CHANGELOG.md), and enduring implementation knowledge in the [developer guide](README-JOT.md).

# Jot — development guidance

Jot is an offline-first **Pebble Time 2 (Emery)** notes **watchapp**, not a
watchface. It has a gesture keyboard, editor, notes list, reader, local
transactional storage, and a manual phone-export feature.

## Target and source of truth

- `package.json` is authoritative for build configuration: Pebble SDK 3,
  `targetPlatforms: ["emery"]`, `watchface: false`. Do not add other watch
  families or watchface-specific behavior without an explicit requirement.
- `README-JOT.md` documents current controls, data handling, testing, and
  architecture. `README.md` is the shorter user-facing overview.
- `docs/gesture-map.md` / `docs/gesture-map.csv` include *planned* gestures:
  verify a mapping in the runtime code before claiming it exists.
- `CHANGELOG.md` is release history; `docs/history/` is historical, not a
  description of the current application.

## Actual source layout

- `src/c/main.c`: Pebble entry point and keyboard-window/event-loop setup.
- `src/c/jot/jot.c`: editor, notes list, save/navigation flows.
- `src/c/jot/store.c`: JOT2 persistence, indexing, recovery and deletion.
- `src/c/jot/reader.c`: saved-note reader, draft Peek, action-bar triage.
- `src/c/keyboard/`: keyboard layout, mappings and faces.
- `src/c/ui/`: window/layer drawing, touch gestures, scrolling, feedback,
  caret layout and controls. `legend_geometry.c` owns shared ABC/123
  peripheral-legend geometry and optical offsets.
- `src/js/pebble-js-app.js`: companion-side PebbleKit JS for backup/config.
- `resources/icons/`: bundled action-bar bitmaps (declared in `package.json`).
- `docs/export.html`: optional static companion export viewer.
- `tests/behavior/` and `tests/fake/`: focused host-side behavior tests and
  simulated Pebble persistence/SDK fixtures.

Jot is **not** a generic one-window TextLayer sample. Check existing ownership,
navigation, callbacks and layer lifetimes before changing a screen.

## Build and verification

Install the Pebble SDK or use the pinned Nix environment in
`flake.nix`/`flake.lock`:

```sh
pebble build
# Or, without separately installing the SDK:
nix develop --command pebble build

# With a configured Emery emulator or connected watch:
pebble install --emulator emery
pebble install --cloudpebble
```

For emulator commands on headless Linux, use the SDK's `--vnc` option when
needed. Do not substitute Basalt or another platform for Emery.

Run the focused host behavior checks locally:

```sh
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
node tests/behavior/backup.js
```

`.github/workflows/behavior-tests.yml` runs these fast host suites **and** a
separate full Emery SDK build that compiles, links and validates the packaged
`.pbw`. Passing host tests alone does not prove SDK compatibility; a
successful build does not prove on-watch ergonomics or firmware behavior.

## Hardware and performance constraints

- JOT2 supports **256 note slots**, at most **4,095 UTF-8 bytes per note**,
  individual Pebble persistence values of at most **256 bytes**, and an
  **up-to-1 MiB per-app persistence budget**. The shared budget cannot hold
  256 maximum-size notes simultaneously.
- Storage uses two banks per slot, chunked text, header-last commits and
  startup recovery. The directory is a conservative acceleration hint, not
  authoritative note data. Preserve write-failure safety, restart recovery,
  deletion semantics and slot reuse when touching persistence.
- CPU and RAM are limited. Avoid repeated full-table scans, flash reads or
  allocations in drawing, touch-move and scrolling hot paths. Newest-first
  note-rank lookups are cached in RAM rather than re-sorting on each redraw.
- Preserve UTF-8 cursor boundaries and existing touch/gesture ergonomics.
  For peripheral legends, keep shared geometry centralized and use
  `docs/legend-tuning.md` for per-symbol optical calibration; changing 123
  legends must not unintentionally shift approved ABC positions.
- Phone backup is a **manual export**, not continuous synchronization. Do not
  describe an accepted companion snapshot as an independently durable backup.

Use hardware or Emery-emulator checks for changes to screen layout, touch
hitboxes, scrolling, animation or window lifetimes. Prefer real-watch QA for
pixel alignment, wrist ergonomics, RAM pressure and persistence behavior.
Do not require screenshots for unrelated documentation or pure logic changes.
Keep new tests focused on distinct user-visible or durability behaviors rather
than accumulating redundant low-level assertions.

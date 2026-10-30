# Jot — detailed behavior and developer notes

Jot is an early on-watch note-taking app for Pebble Time 2, built on the MessagEase-inspired touchscreen gesture keyboard. The main [README](README.md) is the best starting point.

## Current controls

- **Launch:** go directly to a blank editor.
- **Editor Select:** save a quick-capture note and show it in the list, or save an existing-note edit and return to its reader.
- **Editor text strip tap:** peek at the full current draft without saving or switching notes.
- **Editor Space swipe left / right:** move the insertion cursor by one UTF-8 codepoint.
- **Editor Backspace NE / E / SE / S:** delete the following UTF-8 character; tapping Backspace or swiping NW / N / SW / W deletes backward.
- **Editor I swipe up / S swipe down:** move between explicit newline-separated text lines, retaining a preferred column.
- **Editor I up-and-return / S down-and-return:** jump to the beginning / end of the active note.
- **Editor Up / Down:** show saved notes.
- **Editor save icon (top-left key, formerly SAVE/KB):** commit the note and open its saved full-screen reader; a blank draft does nothing. The physical Select key still saves new notes to the list.
- **Editor Back:** save and show the notes list from quick capture; while editing a saved note, save and return to that note's reader.
- **Editor long Select (~1.2 s):** open the inherited typing/trace diagnostic menu.
- **Notes list Up / Down:** browse saved notes. Up from the newest note focuses **+ NEW**, Down from **+ NEW** returns to the newest note.
- **Notes list Select:** open the selected saved note in the full-screen reader, or choose **+ NEW** to start a blank note.
- **Notes list tap a note card:** open that exact note directly in the reader.
- **Notes list vertical drag:** scroll the cards smoothly with finger motion, while keeping header, footer, and button focus consistent.
- **Notes list long Select:** begin a two-step delete; Select confirms and Back cancels.
- **Notes list tap + NEW:** open a new blank editor (only a stationary tap activates it; buttons work too).
- **Notes list Back:** from older notes or **+ NEW**, return to the newest note; at the newest note, exit Jot. From an empty list, Back exits.
- **Reader Up / Down:** scroll the note vertically, 60 pixels per press (when it exceeds the screen).
- **Reader / draft Peek vertical drag:** scroll long text smoothly, pixel for pixel, in the page body. Header, timestamp and footer stay fixed; ordinary taps keep their existing Back/edit/return behavior.
- **Reader Select or brief tap on visible note text:** open that saved note in the keyboard editor; its origin is remembered until the edit is safely saved.
- **Reader Back or tap the upper-left ← NOTES key:** return to the notes list without modifying the note.
- **Draft peek short tap anywhere, Select, Back, or upper-left ← EDIT:** return to the same unsaved keyboard draft.

The keyboard supports taps, directional swipes, directional drag-and-returns, and CW/CCW circles; clockwise circles on the letter keys enter digits 1–9, with 0 on the Space key.

## Native Pebble action bar refinement (v1.10.1 — Emery QA pending)

Native Pebble SDK `ActionBarLayer` now replaces the custom dark drawn
panel. It owns action icons, spacing and physical button highlights.
The Up/Down and confirm/cancel assets are reused from open-source
PebbleOS; 18px Jot-specific white pencil and trash icons complete the
set. The normal background is dark, while Delete mode makes the native
bar red.

**Armed Delete display:** the reader's status/footer displays exactly
`Confirm delete?`; Up is blank (still cancels if pressed), Select shows
the checkmark, and Down shows the X. Back also cancels as before.
The collapsed nub is a plain, fully filled black circular slice with no
interior gold mark. It uses public SDK `graphics_fill_circle`; the
`graphics_fill_oval` implementation used internally by PebbleOS is not
available to third-party app builds.
The note viewport itself is not reflowed.

Native `ActionBarLayer` is added to the window when Select opens the
review bar and removed when Back closes it; the ordinary reader click
provider is then restored so physical Up/Down continue scrolling.
The overlay touch hitbox uses platform `ACTION_BAR_WIDTH` instead of
the old handmade 74px panel. Draft Peek remains action-bar free.
The JOT2 storage format and keyboard gestures are untouched.

## Persistent action-bar triage (v1.10.0 — Emery wrist test pending)

Issue #10 uses a slim black-and-gold overlay on saved reader pages, with
a small black semicircle protruding from the right edge as a cue.
A single physical Select opens the panel; the next Select opens Edit.
While the panel is visible, Up/Down browse newer/older notes (by newest-
first rank), stopping at both ends with FIRST/LAST feedback. Pages open
at the beginning; the panel persists and Back from it closes the panel
instead of leaving the reader. A second Back returns to the list at
the currently displayed note, with the list focus/scroll synchronized.

Holding physical Select (800ms) toggles to a persistent DELETE panel.
First short Select arms confirmation. Second short Select deletes the
current saved note. Up, Down or Back cancels an armed delete without
navigating. On success, the next available saved note is shown at the
top; Delete mode remains visible. Removing the final note returns to
the empty list. Failed delete keeps the current note visible and reports
an error; a failed browse keeps the previous snapshot unchanged.
The overlay is not a text tap target and cannot trigger accidental
editing. Touch-drag scrolling is unchanged and remains possible in
the visible text body, alongside physical Up/Down in ordinary mode.

Draft Peek never shows the action bar and can neither browse nor delete.
The #10 action panel does not change the JOT2 persistence format, the
44px keyboard/editor boundary, or gesture mapping. Host tests exercise
mode transitions, double Select safety, cancel/back, rank edges,
buffer-safe browsing and list focus wiring. Hardware validation pending.

## Long-note reader touch scrolling (v1.9.2 — verified on Emery)

Issue #11 adds a pixel-by-pixel vertical drag to both the saved-note
reader and the temporary unsaved-draft Peek. The finger may start anywhere
inside the clipped text viewport (including blank paper); it must move
more than 10px predominantly vertically before it can scroll. A direct
finger-up move increases the page scroll position. Unlike a swipe-to-page,
the content follows every position update without animation or inertia.
The first drag update includes the whole displacement, later updates use
only incremental deltas, and lift-off does not generate a jump.

The pure `reader_touch.c` helpers validate the start inside the body
(not header, date, footer or offscreen), require a scrollable note, and
clamp the page position from zero to the measured viewport maximum.
The editor/list gesture recognizers are untouched. Existing short taps
on saved note text still edit, while stationary taps in draft Peek return
to the unchanged editor. A dragged gesture remains ineligible for either
tap action, even if it ends where it began. Up/Down physical buttons
still scroll by 60 pixels. Footer hints on long notes say DRAG.

Tests cover long/short/invalid viewport layouts, edge coordinates,
1px movement, reversed direction and both scroll limits, together with
sticky tap cancellation and navigation wiring. Watch testing should
check repeated up/down drags on a long note, release and retouch,
tap-to-edit after scrolling, blank draft Peek, and physical buttons.

## JOT2 1 MiB storage architecture (v1.9.0 — QA)

Current Pebble firmware supports up to 1 MiB of per-app persistent data,
but each `persist_write_data` value remains capped at 256 bytes. Jot
allocates 256 logical note slots of up to 4095 UTF-8 bytes each. These
limits share one physical budget; 256 maximally long notes exceed it
after metadata/transactional overhead. A failed write retains the dirty
editor and shows SAVE ERROR; it never silently overwrites another note.

Each slot has two versions of 16 × 256-byte data chunks and an independent
20-byte commit header, using JOT2 magic and a checksum. New chunks are
written to the inactive bank, then the header is written last and the
previous bank is garbage-collected. At launch, the newer valid header
chooses which bank to read. This is a deliberately **breaking QA reset**:
the old 4100–4115 JOT1 keys are purged; there is no data migration.
Host storage tests verify short and 4095-byte notes, edit/reopen,
deletion, write failures, and all 256 slots.

The existing phone companion now transfers text in UTF-8-safe chunks of
at most 240 bytes, with part indices, a final-part marker and idempotent
acknowledgments. An END packet commits the snapshot only after each note
is complete. The existing browser exporter carries serialized backup
JSON in the URL fragment; full-megabyte snapshots may exceed mobile
WebView URL limits. This is not yet an established reliable restore path
for a full archive.

The on-watch reader measurement was widened for long note text. The editor
uses a full-note buffer and preserves its current caret overlay and
44px geometry. On-hardware stress and firmware-capacity testing are
still required.

## Fine key bevels and saturated Space (v1.8.4 — hardware pending)

A real Emery photo of the 1.8.3 editor confirmed the red notebook rule,
text inset, paper frame, and glyph placement. It also showed the top/left
highlights were visually too thick. Version 1.8.4 draws the rounded keys
with a 1px charcoal outline and a continuous 1px inset edge highlight,
running up the left edge, around the top-left corner, across the top
edge and a little around the top-right corner. The contour hugs the
outline rather than floating inside the key. The dark controls use
a subtle slate highlight; light letters and Space also get a 1px
lower-edge bevel following the bottom corners. Space is filled
with brighter `GColorFromRGB(255,196,0)` gold to counter Emery's
washed-out rendition of earlier beige/yellow.

Drawing changes only. The 44px editor, text/caret inset, keyboard
hitboxes, gestures, note storage and save/navigation paths are unchanged.
Watch testing should confirm the left-edge contours, bottom bevels
and saturated Space under real lighting.

## High-contrast notebook paper and key bevels (v1.8.3 — verified on Emery)

On-device v1.8.2 photos confirm the 44px writing strip and Gothic 18 Bold
font feel well-proportioned. This follow-up strengthens the paper
treatment without increasing the editor height: filled charcoal border,
visible horizontal blue-gray rules, a 2px red rule at x=10, and left
padding of 18px (right padding 5px). The placeholder loses two leading
spaces now that the text begins past the rule. The text viewport, caret
geometry, and layout measurement use the exact same inset width to avoid
incorrect wrapping or cursor positions.

Key faces gain 3px in height inside their original 4x4 cells; shadows
below the faces are removed and replaced with 2px top/left bevel
highlights. Cream is slightly warmer for contrast with white highlights;
dark action controls receive slate highlights, Space pale-gold. Center
text labels rise 2px, with unchanged font weights and peripheral hints.

No changes to the 44px editor/keyboard boundary, hit tests, gesture
recognition, saving, or persistence. Source-level tests guard the
insets, color/stroke choices, absence of old drop shadows and fixed
touch geometry. Real Pebble Time 2 display testing remains necessary.

## Editor typography trial (v1.8.2 — verified on Emery)

The real-watch v1.8.1 photo confirmed that a 44px writing strip is more
comfortable, with typing still working. The next controlled visual
experiment switches the *editor note text* from system Gothic 14 Bold
to system Gothic 18 Bold (the same size/weight family as note-list
previews). The code reuses `prv_font()` for TextLayer drawing, text
measurement, word-wrap and insertion-caret layout. The save-status
badge retains 14 Bold. No changes to the 44px boundary, touch mapping,
gesture recognizer, keyboard faces, note data, or navigation.

Verify an empty placeholder, ordinary one-/two-line notes, text
near a line break, long-note auto-scroll, and saved-note editing on
Emery before approving the font size.

## Pocket Notebook writing-strip height trial (v1.8.1 — verified on Emery)

The initial v1.8.0 notebook styling worked on real Emery but the 40px text
area appeared visually cramped alongside the newly raised keys.
This low-risk refinement increases the editor viewport to 44px and reduces
the 188px keyboard to 184px, one pixel per row. The new shared
`JOT_EDITOR_CONTENT_HEIGHT` constant controls both window layout and all
editor-versus-keyboard touch decisions (key lookup and draft Peek).
Text remains in the same caret-following view, and no new input commands or
storage changes are introduced. Judge the actual height and touch alignment
on hardware before accepting #21. The real-watch test confirmed that
44px feels more spacious and typing remains reliable.

## Pocket Notebook editor styling (v1.8.0 — verified on Emery)

Keyboard colors and icons now match the drawn-paper direction chosen for
the notes list: light cream letter keys, dark charcoal action keys,
gold Space, soft ruled-paper underlays, 6px key rounding, and raised
ink-like bevel/outline shapes. The editor strip has subtle paper
rules, a narrow red left-margin mark, and no app header. SAVE, Backspace,
Return, and Space receive compact vector glyphs rather than words; 123
remains text. All labels and key IDs in the underlying keyboard map
are retained. This avoids changing the keyboard's touch bindings, cell
dimensions, diagnostic gesture modes, note persistence, or cursor logic.

This is a *Pebble C rendering change*, not an embedded bitmap texture:
low-resolution hardware draws the ruled paper and simple icons natively.
The new source checks guard the exact 4x4 input geometry and all four
visual control glyphs. Real watch testing is required to evaluate balance,
contrast, ink shadow weight, label placement, and the small caret area.
#17 (touch-to-position cursor) remains deferred.

## Touch SAVE → committed reader (v1.7.4 — verified on Emery)

The old **KB** key at keyboard row 1, column 1 became **SAVE** in v1.7.4.
In v1.8.0 its centered label is represented by a small vector save icon.
The 50px-wide cell and all touch coordinate geometry remain unchanged. The original keyboard gesture mapping remains stored
in `docs/gesture-map.csv`, including the menu/hand key's other design
assignments. Only a stationary tap on key index 13 in Jot mode is
repurposed, and the diagnostic typing/trace binding table is untouched.

**A deliberate distinction between touch and buttons:**

- **Quick-capture editor, SAVE touch:** commit → notes list pushed
  under the reader → that exact **saved note's full-screen reader** is
  displayed. The editor is cleared only after the reader opens.
- **Quick-capture editor, physical Select:** unchanged; commit and
  open the **notes list**, highlighting the just-saved note.
- **Existing saved-note edit, SAVE touch:** reuse the existing
  `prv_finish_saved_edit()` flow; save and return to the refreshed
  reader for that exact record.
- **Existing saved-note edit, physical Select/Back:** unchanged; save
  and return to that note's refreshed reader.
- **Saved reader ← NOTES:** returns to the notes list with the correct
  selected note, and the list's **+ NEW** starts another blank note.
  Touching saved reader text reopens editing, as verified in #23.
- **Quick-capture empty draft, SAVE:** no-op; no phantom note is
  allocated and no unrelated saved reader is shown.

**Persistence and navigation safety:** The new
`jot_editor_touch_save()` checks its editing origin, requires the list
window to exist and the editor to be the active screen, then calls
`prv_flush()` **before any window change**. If persist_write_data
fails, it leaves the editor buffer and UTF-8 cursor intact with the
existing SAVE ERROR indication. On success it uses
`jot_store_active_slot()` and `prv_rank_of_slot(slot)`, rather than
assuming the edited note is always newest. It pushes the list first,
then calls `jot_reader_show(slot, rank, count)`. If either window
cannot be opened, Jot pops the list and returns to the current
editor *without clearing its text*. Only after the reader opens does
`jot_store_new()` reset the underlying keyboard. Autosave may
already have persisted the record; the final SAVE tap reuses the
same slot and creates no duplicate.

The host regression suite now checks the SAVE tap dispatch, label/font,
save-before-window-stack ordering, rollback paths, no-op on empty
drafts, editor reset only after reader success, and preservation of
existing physical-button controls. Store-side tests still simulate
write failures and check that drafts survive. The SAVE key's legibility and direct saved-reader transition were verified
on physical Emery, and
[issue #24](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/24)
is closed. Host regressions cover the failure and rollback paths.

## Note-list two-line previews (v1.7.5 — awaiting watch verification)

The notes list previously cut a preview at its first explicit LF even when the
card had space for another line. `jot_store_preview()` now keeps embedded
newlines, so the existing two-line card text box can display the next line.
The 64-byte preview buffer still bounds reading; a truncation never ends
inside a UTF-8 codepoint. Extra lines or long paragraphs remain clipped/
ellipsized in the existing text rectangle. Drawing dimensions, button focus,
touch-card hit testing, persistence format, and keyboard handling stay as-is.
Host storage regression tests cover multiline and Unicode truncation.

## Touch saved-note text to edit (v1.7.3 — verified on Emery)

From the normal saved-note reader, a **short, stationary tap inside
visible note text** now opens its editor. The action is routed through
`jot_reader_edit_current()`—the **same** function physical Select calls.
That function validates the current reader and note slot, calls the
existing `jot_reader_edit(slot)` to reopen the actual saved record, and
only pops the reader after a successful transition. On failure, the reader
remains visible and displays its existing open-error status.

**Touch coordinates:** The `JotReaderTouchLayout` passed from `reader.c`
to `reader_touch.c` carries the real screen dimensions, measured text
height, and current vertical scroll. The text-only target is clipped to
the viewport from y=75 through the pixel before the footer, plus the
TextLayer's 2px top inset and measured height. Horizontal taps must be
between the 12px left and right text margins. Date metadata, header,
footer, gutters, and unused paper below a short note are not editing
targets. Both Touchdown and Liftoff must be inside this area.

**Gesture protection:** A touch lasts less than 700ms and must stay
within the same sticky 10px tap-slop limits as the working reader Back
control. A finger that swipes away and returns is **not** a tap, even
if it lifts in the same location. When the user later scrolls long
reader pages by touch (#11), the drag will not accidentally launch
editing on release.

**Peek remains different:** In a temporary `DRAFT` Peek, the header
Back is `← EDIT`, and a short tap elsewhere still dismisses the peek
to the same *unsaved* draft without opening any saved note. The normal
reader keeps `← NOTES` to return to its original list position.
Reader Select, physical Back and Up/Down scrolling remain intact.

The compiled host touch tests cover the exact visible text edges,
timestamp and footer exclusions, short-note blank paper, scrolled
long notes, empty/unavailable viewports, drag cancellation and Peek.
The source-level CI test enforces the shared Select and touch editing
path and failure-safe stack transitions. The feature was thoroughly
verified on Emery; [issue #23](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/23)
is completed.

## Contextual reader Back key (v1.7.2 — verified on Emery)

The reader's decorative pencil in the yellow upper-left header has been
replaced with a visible, cream-paper **Back arrow** outlined in ink, with
a contextual destination label beside it:

- **Saved note:** `← NOTES` takes you back to the notes list; list scroll
  offset and note selection remain where they were.
- **Draft Peek:** `← EDIT` returns to the *same unsaved editor draft*,
  without flushing, allocating a note or entering saved-note edit mode.

Both header-tap and hardware Back invoke `jot_reader_back()`, which
closes only the current reader window. The Back hit target is the
**42 × 42px top-left corner**; its visible cream key is inset within that
area, for a finger-friendly target without crowding the note header.
Only a short stationary tap counts. In `src/c/jot/reader_touch.c`,
either a sticky gesture-movement flag, a movement of more than 10px, or a
duration of 700ms or longer cancels activation. A gesture must **start
and end within the Back target**, so swiping into it won't navigate.
A tap on the remaining header, date, or footer of a normal saved reader
does *not* enter editing. As of v1.7.3, **the visible saved-note text area**
is a distinct, carefully bounded edit target tracked in
[#23](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/23).

The temporary Peek retains its earlier **short tap anywhere to dismiss**
behavior, now with the same stationary-gesture safeguard. Long presses and
scroll swipes are not interpreted as Back. The reader's physical Select
remains immediate edit, and Up/Down remain note scrolling.

A dependency-free host C test covers exact Back rectangle boundaries,
peek-versus-saved behavior, tap timing, movement and sticky swipe
cancellation; the source-level navigation test ensures the two Back paths
stay unified. The contextual header controls were verified on physical
Emery hardware and
[issue #12](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/12)
is complete.

## Touch-responsive note list (v1.7 tested; v1.7.1 focus refinement awaiting watch test)

The note cards now scroll in their own **clipped Pebble Layer** beneath
the fixed `+ NEW` header and backup/status footer. Instead of advancing
one card for each swipe, the cards follow vertical finger motion in actual
pixel increments. The viewport is bounded to the note content: it cannot
drag past the first or last card. On release it stays in place (no inertial
fling yet).

**Tap:** a brief tap within an actual drawn card opens the corresponding
read-only reader, preserving its rank and allowing Back to return to that
position. The tap hit test shares the painter's exact card width, row
height, margins and current pixel offset. A tap in a gap, on unused paper,
or in the fixed footer is ignored. The `+ NEW` header tap still works.

**Drag:** after more than 10 pixels of vertical movement, Jot changes to
a scrolling gesture and stops treating that stroke as a possible tap.
Once the stroke has moved far enough, returning to the starting point
cannot accidentally open a note.

**Jot 1.7.1 focus policy (#22):** As soon as dragging begins, the yellow
selection highlight disappears (including any focus on `+ NEW`). The list
keeps its precise pixel scroll position when the gesture ends, with no
highlight. A separate `s_list_focus_valid` flag distinguishes this state
from actual `+ NEW` focus (`JOT_LIST_FOCUS_NEW == -1`).

The **first** physical **Up or Down** after a drag performs *the same action*:
highlight the **topmost completely visible note card**. This uses the
actual painted card's 3px vertical inset, not merely the first logical row,
so a card whose margin is clipped can still count as fully visible.
That first button press deliberately does **not** move the viewport, avoiding
a jump. Subsequent button presses restore the usual one-note-at-a-time
movement and positioning; Up from the newest note can still select `+ NEW`.

While touch browsing has no selection, physical Select or long Select first
restores focus and never acts on an invisible, stale note. Back returns focus
to the newest note at the top (rather than accidentally exiting); the next
Back exits Jot as before. Tapping any card continues to open it directly,
reinstating focus on that exact note so Reader Back returns to the same
scroll position. Empty lists restore `+ NEW` on button press.

Only gestures beginning within the card viewport can scroll it. Touch
scrolling and card taps are suppressed during delete confirmation and
active phone backup. NEW focus, Back-to-top, long Select to delete, long
Up to back up, and the original keyboard gesture recognizer are not
changed.

New `src/c/jot/list_touch.c` is a dependency-free, host-tested source
of truth for scroll bounds and card hitboxes. Automated tests cover
the empty list, 1–16 notes, card gaps, clipped top/bottom edges,
fractional scrolling and scroll-position-dependent hits. The original
tap/drag behavior was verified on Emery and
[issue #7](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/7)
is complete. New Jot 1.7.1 host cases test full-card focus after a partial
scroll, and [issue #22](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/22)
tracks the on-watch focus refinement.
A kinetic fling remains optional future polish.

## Context-aware screen navigation (v1.6 — Emery testing pending)

Jot still launches into a **blank, immediate editor**. The existing reader
and keyboard are reused; no new screen type or note record is introduced.

- **Quick capture, Select:** flush pending text. If it is a real note, reset
  the keyboard to a blank draft and open the notes list with that exact note
  highlighted. If nothing was typed, just open the list without allocating
  a phantom note. Opening the list must not overwrite the saved text.
- **Quick capture, Back:** flush and show the list, but leave the underlying
  editor buffer available until the next deliberate fresh-note action.
- **From a saved-note reader, Select to edit:** load that note into the
  existing keyboard and remember its persistent slot. When editor **Select
  or Back** is pressed, flush the changes, reset the underlying editor to a
  fresh draft, open the list on the same note, then reopen its reader using
  the *committed* record. The new reader cannot display the stale snapshot
  from before the edit.
- **Reader Back:** return to the list, preserving its selection.
- **List + NEW pill (v1.6.1):** this is now a selectable header action,
  not a touch-only control. From the newest note, press **Up** to focus it,
  then **Select** to open a blank editor. Press **Down** to return to the
  newest note. A stationary tap is still an alternative. The `x / 16`
  capacity badge was removed to provide room for a clearly labeled
  **+ NEW** pill; the focused pill is drawn with stronger contrast.
  From an empty notes list, this action is the default selection.
  The shared action flushes any pending draft, resets the editor, then
  pops the notes list so typing can begin immediately. Gestures or swipes
  do not activate it.
- **List Back:** if the selected note is below the first, or **+ NEW** is
  focused while notes exist, jump to the newest note. From the newest note,
  exit Jot with `window_stack_pop_all(true)`. When the list is empty,
  Back from **+ NEW** exits directly.
  If delete confirmation is active, Back *cancels* it instead; active backup
  is cancelled via the existing flow.

**No lost writing:** Every navigation step that could abandon or reset a
draft checks `jot_store_flush()` first. When a persistent write fails,
`SAVE ERROR` is displayed and the editor stays open with its original
buffer and cursor; there is no silent discard or forced page transition.
The note's old saved version is never replaced by a failed write.

Host regressions cover the active note's slot and a simulated save failure
across `jot_store_new()`. v1.6.1 adds a pure-C focus navigator with host tests for
the `note → + NEW → note` button sequence, empty lists, Back/exit behavior
and removing the last note. Source-level checks ensure Select and touch
share the same save-safe new-note action and the capacity badge is gone. CI also checks the button/event wiring and stack
transition sequence. Actual windows, touch hitboxes, and the Back behavior
must still be tested on Emery. See
[issue #14](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/14).

## Forward deletion (v1.5.4 — awaiting watch verification)

The original English-LH keyboard map assigns Backspace **NE/E/SE/S** to delete the Unicode codepoint immediately *after* the cursor. This is now implemented in Jot mode, without changing the diagnostic keyboard's legacy binding table. The existing backward-delete bindings are unchanged. Forward delete preserves the caret offset, joins lines when deleting a newline, and is a no-op at end of note (no new dirty state or autosave timer). It is safe to delete a character in a full note and then insert another (the limit is now 4,095 bytes). Host regressions cover UTF-8, line joins, persistence, full notes and end-of-text.

The keyboard layout, touch thresholds, stored-note records, and reader are unchanged. Real Emery gesture testing is still needed before [issue #15](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/15) is closed.

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

**On-watch findings (v1.5.2):** The user confirmed that the caret height and
bottom-row clearance are now right. However, the horizontal caret position
still jumps far to the right at soft wraps, including after moving left
across LF following a wrapped line. Screenshots are now available directly
in the development conversation as well as the issue.

**v1.5.3 follow-up — visual-line layout:** The previous caret routine
searched backward for the first suffix that *could* fit on a line. When a
short word wrapped, that suffix could contain words from the preceding line,
yielding a caret position near the right edge even though the word rendered
on the left. The replacement `src/c/ui/caret_layout.c` walks forward through
whole words, uses the same native font-width measurements as the TextLayer,
and explicitly tracks the starts of visible rows. Overlong words are split
on UTF-8 boundaries; hard LF breaks and trailing spaces are handled
separately. The 1px caret stroke, font, bottom clearance and keyboard
geometry confirmed in 1.5.2 remain unchanged.

This layout helper is compiled and exercised in CI with regression cases
modeled on the watch screenshots, including short words wrapping after long
lines, crossing explicit line breaks, empty lines, trailing spaces and
UTF-8 codepoints. The source-level overlay safeguards also remain.
It is still a model of Pebble's word-wrapping behavior, so physical Emery
testing is necessary before closing issue #16.

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

The app stores notes in Pebble's per-app persistent storage. As of v1.9.0 it supports **256 note slots** and **4,095 UTF-8 bytes per note**, shared within the firmware's **up to 1 MB** storage budget. A note's byte count may differ from its character count. It refuses to allocate a 257th note rather than silently overwriting existing notes. The earlier JOT1 format allowed only 16 × 159 bytes.

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

At the time, this installment **did not** change the then-159-byte note limit, storage format, keyboard geometry or gesture classifier. The storage format and limit were subsequently expanded in v1.9.0. The reader has now been demonstrated on real Pebble hardware; the enlarged body font and draft peek are the next on-watch test.

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
3. Verify the 1.7.4 touch SAVE → reader flow on real Emery, then tackle #17 tap-to-place cursor, reader gesture scrolling, and storage durability.

The earlier `jot-v0.1.patch` instructions are historical; this repository now contains the Jot source directly. Simply pull and build the branch.

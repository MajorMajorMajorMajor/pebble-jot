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
| Editor | Tap the **save icon** (upper-left key) | Commit and open the saved note directly in its reader |
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
| Reader | Back or tap **← NOTES** in top-left header | Return to the notes list |
| Reader | Tap visible note text | Open that saved note in the keyboard editor (same as Select) |
| Draft peek | Tap anywhere briefly, Select, Back, or tap **← EDIT** | Return to the same draft editor |
| Editor | Long Select (~1.2 s) | Open the legacy typing / trace diagnostics |

The editor uses a small `*` in its upper-right corner to indicate unsaved edits. Jot automatically saves after about **1.8 seconds** without typing and at several navigation boundaries. `SAVE ERROR` means the storage write did not succeed; `FULL` means the attempted character did not fit.

## Fine-line key bevels and bolder gold (v1.8.4 — wrist test pending)

The raised-key bevels of 1.8.3 looked pleasing on real Emery but too
heavy, particularly on the four charcoal controls. In 1.8.4 each key
uses a **1px ink outline** with a **continuous, 1px inset highlight**
along the left side and top, following the top-left rounded corner
and wrapping slightly around the upper-right corner. Light letter
keys and the gold Space key also receive a discreet 1px darker
bottom-edge bevel that wraps the lower corners. The dark action
keys retain only the softer top/left highlight. The Space fill
is now a punchier saturated yellow-gold (`255,196,0`), chosen for
better contrast on Emery's screen.

This change affects only key drawing: no changes to the editor inset,
44px writing strip, text/caret, key cell sizes, hit geometry, labels,
gestures, save flow, note storage, or reader/list navigation. Physical
display verification is pending.

## Pocket Notebook contrast and bevel polish (v1.8.3 — verified on Emery)

Emery photos of 1.8.2 confirmed that 44px and Gothic 18 Bold work together,
but the notebook's red margin and light horizontal rules were difficult to
see. The editor now draws a stronger 2px ink frame, visible blue-gray paper
rules, and a saturated 2px red margin line at x=10. Note text begins at
x=18 with 5px of right clearance. The placeholder no longer adds its own
invisible leading spaces. The same left inset and narrower content width
drive note wrapping, caret layout, scrolling, and the TextLayer, so no
saved text or cursor offsets are changed.

The keyboard now uses **inset top/left highlights rather than downward
drop shadows**. Rounded key faces are 3px taller within the existing
4x4 rectangles, and central text labels move up 2px for visual balance.
The pale parchment fill was adjusted to keep white highlights visible on
Pebble's limited color palette. Charcoal controls and the gold Space key
retain their identities. This is a *painting and text-inset* revision;
44px editor height, touch targets, gestures, note storage, save behavior,
and reader/list navigation are untouched. Visual verification on Emery
is still required before issue #21 can close.

## Larger editor writing (v1.8.2 — verified on Emery)

After real-world testing confirmed the 44px text strip feels comfortable
and typing remains accurate, the note editor now tries Pebble's native
**Gothic 18 Bold** font instead of Gothic 14 Bold. The font is used by
the actual TextLayer *and* the existing caret/wrap measurement path, so
cursor placement and automatic following use identical metrics. The
44px area and keyboard touch boundaries do not move. Long notes can show
fewer words at a time, so physical testing should check scrolling,
the text caret at line breaks, and the empty-draft placeholder. This is
a reversible typography experiment; note storage and gestures are unchanged.

## Editor height tune-up (v1.8.1 — verified on Emery)

The v1.8.0 Pocket Notebook styling made the writing strip feel more
compressed on real hardware even though its height was still 40px.
For the v1.8.1 wrist experiment, the writing strip gains 4px
(**44px total**) and the keyboard becomes 184px instead of 188px
at Emery's 228px display height. Each of the four keyboard rows loses
only 1px. A single `JOT_EDITOR_CONTENT_HEIGHT` constant drives both
the rendered layout and the touch classifier, including short-tap
draft Peek, so there is no dead or mismatched touch band. The
keyboard's column widths, key identities, gesture bindings, note
storage and navigation remain unchanged. If hardware feedback does
not favor the result, revert the one shared constant.

## The Pocket Notebook editor (v1.8.0 — verified on Emery; height refined in 1.8.1)

Jot now gives the quick editor the same paper-and-ink character as the list:
a warm ruled writing strip with a red notebook margin, cream letter keys
with rounded charcoal outlines and inset highlights, charcoal control
keys, and a gold Space key. The upper-left SAVE action, Backspace, Return,
and Space use drawn vector icons rather than text; the familiar 123 key
retains its clear label. The original A-N-I / H-O-R / T-E-S keyboard and
small peripheral letter hints are unchanged.

This is a **visual-only change**: the text strip originally used 40px
(subsequently expanded to 44px in v1.8.1). The keyboard grid stays four
columns and four rows, and every key retains its *full rectangular* hit area.
No new header, bitmap resource, gesture behavior, save route, cursor
placement, note storage, or list/reader navigation is introduced. The
source-level theme test checks these constraints. Emulator/physical Pebble
Time 2 presentation requires verification before #21 closes.

## Touch SAVE → read the finished note (v1.7.4 — verified on Emery)

The upper-left keyboard key was labeled **SAVE** in v1.7.4 and is drawn as a save icon starting in v1.8.0. A stationary
tap in Jot mode **saves your current text and opens that exact note
directly in its full-screen reader**, so you can launch Jot, write a note,
and see the result without pressing a physical button or visiting the list
first.

The reader sits above the notes list in the window stack: tapping
**← NOTES** goes to the list with the saved note highlighted; **+ NEW**
takes you back to a blank editor. Tapping the saved note's text enters
editing as usual. From a saved-note edit, keyboard SAVE behaves like
physical Select: save and return to the updated reader. **Physical Select
has not changed:** new notes still save to the list, and saved-note edits
return to their reader. Physical Back also retains its tested flow.

SAVE first commits the note, then checks its persistent slot and rank.
Only after the list and reader are successfully open does it reset the
underlying editor. A failed persistent write stays in the editor with the
draft and cursor intact; a reader/list setup error rolls back to the
existing editor buffer. Tapping SAVE on an empty quick-capture draft is
a harmless no-op, not an invitation to open an unrelated note or create
a phantom record. Repeated SAVE taps on an already auto-saved draft
reuse its record rather than duplicating it.

The keyboard cell dimensions, other gesture bindings, storage format
and backup flow are unchanged. Host regression checks cover save
ordering, error handling, the existing button behavior and the new
SAVE key label. The SAVE-to-reader interaction was subsequently verified on a physical Pebble Time 2; failure paths remain covered by host tests.

## Two-line note card previews (v1.7.5 — verified on Emery)

Saved-note previews now preserve explicit newlines instead of cutting off
everything after the first line. The notes list renders up to two visible
lines within each existing card, with long text ellipsized. Previews are
bounded and truncated only at valid UTF-8 boundaries. The card size,
selection, touch hitboxes, gestures, stored note records, and full reader
are unchanged. Host tests verify the second line and Unicode truncation.
Physical Emery testing confirmed readable two-line cards and unchanged tap navigation.

## Screen navigation (v1.6 — verified on Emery)

The editor now remembers when it was opened from a saved-note reader. **Select and Back after editing** first commit the draft, then reopen the *same reader* using the newly persisted text instead of an outdated snapshot. **Select on a quick-capture note** saves it, resets the editor to an empty note and opens the notes list with the saved note selected. **Back on a quick-capture note** saves and opens the list but keeps the current editor buffer; only pressing Select finishes that capture.

**Jot 1.6.1** makes the header's **+ NEW** pill button-selectable as an item immediately above the newest note. Press **Up** from the newest note to focus it, then **Select** to begin writing; **Down** returns to the newest note. The former `x / 16` badge has been removed for a larger, clearer button; tapping the pill still works. If the list is empty, **+ NEW** starts selected automatically. Pressing Back while viewing an older list position jumps to the top first; pressing Back from the top exits to PebbleOS. The full-screen reader's Back still returns to the notes list. Tap-to-peek remains read-only and returns to the same draft. If a save fails, Jot keeps the editor and text on screen, showing **SAVE ERROR** rather than discarding any writing.

The keyboard geometry, recognizer, caret rendering, note storage format, and existing backup flow are unchanged. The navigation transitions were verified on physical Pebble Time 2 hardware.

## Touch the notebook (v1.7–v1.7.1 — verified on Emery)

The saved-notes list now responds to **taps and vertical drags** as well as the
physical buttons. Tap an actual note card to open it directly in the reader,
or slide a finger up/down across the cards to scroll with pixel-level
movement. The yellow header (including **+ NEW**) and charcoal backup footer
stay fixed. Swipes don't accidentally open notes: after a gesture crosses a
10-pixel movement threshold, release is never treated as a tap. Blank paper,
the spaces between cards, and header/footer areas are not note-card targets.

**Jot 1.7.1:** The yellow highlight now **disappears during a drag**
instead of chasing the finger. When you release, the list stays where you
left it, with no card selected. The first physical **Up or Down** press
highlights the **topmost completely visible note card**, without moving the
scroll position; further presses navigate a note at a time. Select and
long-Select cannot open or delete a stale, unhighlighted note. Back from
touch browsing restores the newest note at the top, preserving the
existing Back-to-top flow.

The original **+ NEW** button-and-touch action, delete confirmation, and
backup controls remain unchanged. Scrolling still follows the finger
directly without kinetic momentum. The v1.7 touch/drag/tap behavior has been
tested on Emery; the v1.7.1 focus-state transition was also verified on
hardware. Host tests cover card edges, partial scroll, exact full-card visibility,
scroll bounds and empty lists.

## Contextual reader Back (v1.7.2 — verified on Emery)

The upper-left yellow reader header now contains a **real Back control**,
drawn as a small charcoal arrow on a cream-paper key. Its label describes
the destination: **← NOTES** on an ordinary saved-note page, and
**← EDIT** on the temporary unsaved-draft Peek. Both the on-screen
control and physical Back use the very same window-stack exit path.

A short, stationary tap inside the header's 42 × 42px Back target exits.
A gesture that moves more than 10 pixels, a long hold, or a touch that
starts outside the target cannot accidentally navigate. Tapping saved-note
text is still intentionally inactive pending #23; ordinary brief taps
anywhere in draft Peek continue to return to the same unsaved editor.
Existing note data, screen scrolling, physical buttons and list touch
selection remain unchanged.

Host tests cover the Back target's boundaries and tap-versus-swipe behavior.
The visual appearance and touch behavior were verified on Emery.

## Touch to edit a saved note (v1.7.3 — verified on Emery)

A brief tap on the **visible text of a saved note** opens that exact note
in the keyboard editor—the same transition as physical Select. The edit
origin is remembered, so finishing the edit with Back or Select still
saves the revision and returns to an up-to-date reader.

The tap target follows the TextLayer's measured visible height and current
scroll offset, clipped between the date strip and the bottom footer.
Tapping the date, header, blank page below a short note, gutters, or footer
does not open the editor. A swipe, long press or finger movement exceeding
10 pixels cannot accidentally activate editing, even after returning to
the starting point. The existing **← NOTES** header Back action retains
priority. Tapping a temporary **DRAFT Peek** still returns to the same
unsaved editor buffer, never to saved-note editing.

Host tests cover these targets, gestures and the shared Select/edit path.
The content tap was thoroughly verified on physical Emery hardware and #23 is closed.

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

The editor gestures, hairline caret, notes-list touch scrolling, button navigation, saved-reader tap-to-edit, and v1.7.4 touch SAVE → reader flow have been verified on Emery. Next: verify v1.8.4's 1px key contours and saturated gold Space on Emery, then prioritize safer/larger storage and backup/restore before the beta. Touch-to-position-caret (#17) remains deliberately deferred; key gestures are unchanged.

Pebble SDK documentation: <https://developer.repebble.com>

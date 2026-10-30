## Jot 1.15.2 — final alphabet legend optical nudges (Emery QA pending)

Following the second real-watch screenshot, adjust just three ABC
peripheral glyphs: **l** and **d** move down 1 pixel; **f** moves
left 1 pixel and down 1 pixel. All other glyphs retain their
1.15.1 positions. The global compass rows, numeric labels, big central
letters, gestures, press animation, colours and hitboxes do not move.
The individual optical offsets are host-tested so future glyph changes
cannot accidentally disturb the rest of the alphabet.

## Jot 1.15.1 — Centred ABC legend ink (Emery QA pending)

The first Jot 1.15 wrist screenshot confirmed ample room for all eight
O-key letters, but also showed the small blue lettering consistently
sitting too low within its compass positions. Pebble Gothic 14 Bold
draws visible ink below the top of the text frame. The primary ABC
renderer now raises all three legend rows by **4 pixels**, balancing
their visible top, middle and bottom positions and bringing the bottom
letters away from the key outlines. Left/centre/right alignment,
the full alphabet, font, palette, big key labels, 123 legends and all
gesture bindings are unchanged. Visual QA is still needed on Emery.

## Jot 1.15.0 — The alphabet reveals itself (Emery QA pending)

The **ABC** face now paints all 20 implemented primary peripheral
characters around the existing nine central keys. The eight compass
positions match the swipes exactly: A v; N l; I x; H k; O q/u/p/b/j/d/g/c;
R m; T y; E w/z/:/./,; S f. The central ANIHORTES letters, 123
face, existing control icons, rounded key borders, touch hitboxes and
animation are unchanged.

The peripheral glyphs use the proven numeric face's native
Gothic 14 Bold font, 8-position placement and blue (0,85,170) accent,
drawn **within the existing key background layer**. The old five
peripheral KeyLabel child layers were removed, avoiding duplicate
labels and extra per-key layers. The O key is intentionally the most
densely labeled; real wrist QA will determine whether its centre and
corners need typographic refinement.

**Primary labels are permanently visible for now.** There are **no**
Spacebar gesture toggles: hiding an alphabet by accidental gesture
would be too alarming. Secondary labels and their future
settings-controlled visibility live in #34, with Tab #31, Compose #32,
and Shift #33 separately deferred. No new text input commands, reserved
gesture assignments, storage format or key-repeat behavior are added.

Pure C mapping tests cover 20 glyphs, every direction and missing keys;
source tests compare displayed glyphs with the runtime touch bindings.
The SDK build and legibility still require Emery hardware validation
before closing #2.

## Jot 1.14.0 — Kinetic scrolling (Emery QA pending)

The saved-note reader, draft Peek, and note list now use a shared
integer-only inertial scroll model: finger drags remain exactly 1:1,
while a sufficiently fast flick (at least 120 px/s) coasts and
gradually decelerates. Motion is sampled from recent touch updates;
waiting more than 90 ms before liftoff does not create an accidental
flick. The model caps launch velocity at 1600 px/s and retains
approximately 92% per 20 ms frame until it settles. Motion clamps at
both ends without bounce or overshoot. These parameters are isolated
for on-watch tuning; **exact PebbleOS menu friction constants could
not be verified**, so this is a native-feeling prototype, not a claim
of bit-for-bit OS physics.

A new finger contact immediately cancels ongoing momentum. That first
stationary touch only stops a moving page; a subsequent tap edits a
reader note or opens a list card. Continuing to drag after contact
still works. Physical Up/Down/Back/Select and list/reader teardown
cancel inertia too. The existing action bar, deletion confirmation,
tap-versus-drag safeguards, keyboard and note storage are unchanged.

Host tests exercise friction, flick vs pause, direction, cap,
millisecond overflow and stop/cancel behavior. A source-routing guard
protects both scroll surfaces. Pebble SDK build and Emery responsiveness
must be confirmed on real hardware before closing #30.

## Jot 1.13.1 — Repeat stays locked until liftoff (Emery QA pending)

Real-wrist testing revealed that repeat could unexpectedly stop when
the finger slid after the gesture had activated. Once the first repeat
fires, the **original editing action now continues until liftoff**,
even if the finger reverses direction, crosses another key or moves
away from its starting point. Backspace never switches to forward
delete, and the cursor continues in its original direction at 135 ms
intervals. Releasing still prevents any duplicate edit.

Before activation, gestures retain their existing recognition rules
and 550 ms dwell delay. Storage, keyboard geometry and colors remain
unchanged. Host regressions cover continued repetition after sliding,
reversal, moving off the target, timing and release cancellation.

## Jot 1.13.0 — Hold-to-repeat editing gestures (Emery QA pending)

Holding an **editing gesture** now starts repeating after a deliberate
550 ms pause, once every 135 ms until liftoff. Supported gestures are
Backspace tap/N/NW/SW/W (delete preceding character), Backspace
NE/E/SE/S (delete following character), T or numeric 1 SW (backspace),
Space E/W (cursor right/left), I or numeric 9 N (cursor up), and S or
numeric 3 S (cursor down). On the numeric keyboard the blank dedicated
Space key supports E/W repeat; the wide zero key never becomes a
cursor command.

Single taps/swipes retain their existing *release* behavior. Only a
held **stationary editing action** can repeat; movement greater than
6 px restarts the 550 ms delay, and changing finger direction after the first repeat
does not stop or alter the latched command. A return-to-origin
stroke cannot become an accidental repeating backspace tap. Once any
repeat has fired, the corresponding liftoff does **not** execute a
second action. Cursor Home/End, symbols, letters, Space insertion,
Save, Return, face toggles and CW/CCW circles are intentionally not
repeatable. Diagnostic trace mode stays untouched.

No key geometry, swipe thresholds, stored note bytes, visual palette,
or reader/list controls were changed. Host C timing/mapping tests and
JS gesture-routing guards cover the behavior; Emery touch testing
is still required before closing #19.

## Jot 1.12.0 — restrained blue numeric legends (Emery QA pending)

The 123 keyboard uses the same cream keys, charcoal controls, gold
accents, 1px outlines, bevels and press animations as the approved
ABC keyboard. **Blue is only for the small bold peripheral legends**,
which now use Gothic 14 Bold for improved wrist readability.

The bottom row consists of a normal **two-column gold 0** and a
separate **one-column gold Space key, left completely blank**.
Both have their own normal key hitbox and feedback.

As in full-size MessagEase, editor control gestures are *not*
advertised with little pictograms. On the 123 face **9↑** moves
the cursor up, **3↓** moves it down, **9↗** inserts Return,
**1↙** deletes backward, and **3↘** inserts Space. All
these commands remain enabled and are documented here instead of
drawn. Tab and Compose remain unimplemented and unlabelled.

The separate blank Space supports normal tap and left/right cursor
swipes; other printable digit and punctuation gestures follow
`docs/gesture-map.csv`. Glyphs not supported by Pebble's small font
remain hidden rather than showing empty boxes.

ABC input, note editing, storage and reader are unchanged.
Physical Emery contrast/legibility QA still required.

# Jot — a pocket notebook for Pebble

**A thought, one button away.** Jot is a lightweight, offline note-taking app for the Pebble Time 2. It opens directly into a writing surface and uses a compact touchscreen gesture keyboard for entering notes without reaching for your phone.

Jot grew out of the [MessagEase-inspired gesture keyboard experiments](https://github.com/MajorMajorMajorMajor/messagease-test). Its keyboard supports taps, eight-direction swipes, eight directional drag-and-return gestures, and clockwise/counterclockwise circles. Clockwise circles on the letter grid enter digits 1–9 in calculator order; clockwise on Space enters 0.

## Warm charcoal feedback study (v1.11.5 — wrist QA pending)

The v1.11.4 gold Spacebar and cream letter keys are **approved
unchanged**. The charcoal feedback was still a little abrupt on Emery:
neutral grey on a 64-colour Pebble screen offers only two usable
intermediate lightness values. This pass **only** adjusts charcoal
control-key feedback, introducing three distinct restrained sepia
palette values (deep olive → russet → warm clay), with a quieter
first-touch stage. The original resting charcoal, cream control
glyphs, outlines and bevels remain exactly as before.

**80ms eased in / 190ms eased out** is still shared by every key,
with the existing 60ms minimum quick-tap visibility. Only controls
(Backspace, Return, 123, Save; touch indices 10–13) start from
intensity 80 instead of 128. Cream and gold keep their original
palettes and initial intensities. No gestures, touch hitboxes, timers
or note storage are changed.

The warm tint and final cream-on-clay contrast require physical
Emery review before closing #29.

## Charcoal + Spacebar feedback correction (v1.11.4 — wrist QA complete for cream/Space)

The 1.11.3 palette-aware pulse has a common **80ms eased press-in**
and **190ms eased release-out** across *every* key, with the same
minimum 60ms visible time on a quick tap. The **cream letter-key
fill, 1px outline, labels and bevels are unchanged**.

The two non-cream palettes had on-watch problems:
- **Charcoal controls** used one identical grey at every intensity,
  so their easing was effectively invisible. They now progress from
  ink through two distinct neutral greys on the Pebble palette.
  Icon colour stays fixed rather than abruptly toggling mid-animation.
- **Gold Space** switched from darker to brighter yellow during one
  press, creating a distracting blink. It now darkens consistently
  from gold through amber to copper and returns in the reverse
  order, with the original gold and charcoal dash restored at rest.

Both have deliberately limited palette steps rather than fake RGB
interpolation. The 25ms shared timer and input recognizer are unchanged;
this requires another Emery wrist review before closing issue #29.

## Palette-aware key breathing (v1.11.3 — superseded timing, cream style retained)

The 1.11.2 comparison revealed that quick touches did not visibly fade:
its starting RGB colour quantized to the **same** 64-colour palette
entry as the idle cream fill. Jot now uses **three deliberately distinct
displayable palette colours** between cream rest and A's warm tan
pressed endpoint. Each letter key uses the preferred **A fill-only**
treatment; every 1px border, label, bevel, and touch hitbox stays still.

Touch starts at a visible warm palette step immediately, then **eases IN
over 70ms** to a darker warm pressed shade while held. Liftoff **eases
OUT over 180ms**, with a **minimum 60ms visible key state** after a quick
tap. Three intentional recovery colours replace imperceptible continuous
RGB interpolation. Charcoal controls keep legible icons; the gold
Spacebar brightens with its own fixed palette steps. A single 25ms
timer on the keyboard grid schedules all changes and is cancelled
when the grid is destroyed.

The previous A/B/C timing and styling matrix remains documented below
as an experiment, but is no longer active. Host tests cover the
palette thresholds, perceptible first frame, rising press, monotonic
falloff, and timer wiring. Evaluate actual feel on Emery before
closing issue #29.

## Key feedback comparison lab (v1.11.2 — superseded by v1.11.3)

This is an experiment for issue #29; it is **not** the final key feedback.
All key outlines remain the original 1px charcoal border and the touch
targets and recognizer timing are unchanged. One 25ms timer animates
the touched key only, with a 50ms ease-in and an eased falloff after
release (no inertia, no key motion). Quick taps still get an immediate
144/255-strength visual acknowledgement, visible on the 64-color screen. A later touch cancels the old
animation; leaving the keyboard destroys the timer safely.

Compare two dimensions independently:

| Key row | Fill / label / bevel treatment | Fade length by column |
| --- | --- | --- |
| A N I | A: gentle cream-to-tan fill only | A=100ms, N=150ms, I=200ms |
| H O R | B: stronger fill and slight boldening of centre glyph | H=100ms, O=150ms, R=200ms |
| T E S | C: warm fill and tactile bevel response | T=100ms, E=150ms, S=200ms |
| Left controls | D: charcoal brightening with fixed border | All=250ms |
| Gold Space | E: brighter gold pulse with fixed border | 250ms |

The colour transitions are quantized to the watch's available palette,
so the 25ms ticks may not all display different shades. The goal is to
identify the most pleasing actual *perceived* combination on Emery.
The exact animation and release lengths are verified by host tests, but
look/feel need watch verification before promoting a winner.

## Emery virtual-size headroom (v1.11.1 — awaiting SDK build)

The 1.11.0 build reached **65,716 bytes** of `.text + .data + .bss`,
181 bytes beyond Pebble's 65,535-byte metadata limit. Instead of shaving
just 181 bytes, v1.11.1 removes the old trace recorder's two large
**always-allocated** buffers (1,024 touch samples and 128 gesture summaries)
from static memory. They are now allocated only when an explicit
diagnostic recording starts, and freed immediately after the export queue
takes ownership of its copies. Failed allocations leave recording off
and display an error, and orderly shutdown also releases them.

This is expected to recover **well over 15 KiB of virtual-image space**,
providing meaningful growth room for the editor and future improvements.
It preserves the trace recorder's sampling capacities, 1 MB note store,
gesture mapping, key-press feedback and notebook visuals. Automated
architecture regressions protect against reintroducing fixed buffers;
the exact new `virtual_size` still requires a local Pebble SDK build.

## Gesture keyboard cleanup (v1.11.0 — wrist QA pending)

- **#28:** Clockwise loop on the wide gold Space key now inserts **0**.
  Counterclockwise loop inserts a space, as defined in the canonical
  English-LH gesture worksheet. Circle-recognition geometry and the
  nine letter-key digit mappings remain unchanged.
- **#29:** Each key now subtly lights up while touched, including
  during a swipe, with a 90ms afterglow on liftoff. Only the initiating
  key is highlighted; there is no touch-target or timing change in
  the gesture classifier. A new touchdown immediately replaces the old
  highlight, and keyboard unload/deinitialization clears the state.
  The existing paper, charcoal controls, gold Space, labels and bevels
  are preserved.

Host mapping and visual state wiring tests cover these additions.
The exact look and 90ms feedback duration need a watch review.

## Status

**Early working prototype.** Jot has been built and photographed running on a real Pebble, with live text entry and a notes list visible on-device. This does **not** yet establish comprehensive verification of persistence, deletion, or every gesture on all hardware. The app currently targets **emery (Pebble Time 2)**.

Notes remain on the watch. Jot includes an **experimental manual phone backup** over Bluetooth. This is not continuous sync, and the copy in the Pebble phone companion is not yet a durable external backup; **do not store irreplaceable information** until you've copied the notes to another location. Uninstalling Jot may remove its local notes.

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

## Reader reflow, edit-return controls and full-width Delete (v1.10.5 — Emery QA pending)

Emery QA confirmed the native ActionBar does **not** actually blend the
requested alpha background with the reader, so the bar is opaque again.
When the saved reader enters review mode, its paper and clipped viewport
reduce width by the SDK's `ACTION_BAR_WIDTH` (34px on Emery), and the
TextLayer wraps its contents inside the remaining width rather than
disappearing beneath the right-side strip. The timestamp, header note
counter and footer hint stay out of the native bar too. The width only
changes on opening/closing the bar: switching between browsing,
Delete and armed confirmation does not trigger further reflow.
Long-note touch hit detection uses the narrower viewport while active.

Delete mode now paints the **entire middle third** of the native bar
red, with the stock trash / check glyph centred on it. This is a
visual A/B experiment versus the previous small red badge; the native
top and bottom sections stay charcoal. All two-press delete/cancel
behaviour is unchanged.

A critical lifecycle bug is fixed: when the native bar is removed during
the transition into editing, PebbleOS clears the persistent reader
Window's click provider. On the next reader load/push, Jot now explicitly
re-registers its own handlers. This restores physical Up, Down, Select
and Back after returning from edit; the touch Back button continues to
close the action bar on its first tap. The header always says NOTES for
saved readers (EDIT for draft Peek), never CLOSE.

**On-watch regression:** open review on a long note, confirm wrapping
and header/count visibility, browse and scroll, enter editing, return to
reader and verify all physical controls and tap Back. Test arm/cancel,
delete, and compare the full-third red panel to the smaller red tile.

## Native action bar alpha experiment (v1.10.4 — Emery QA pending)

This is intentionally a **one-commit, hardware-testable experiment**,
layered on top of the green v1.10.3 opaque baseline. The native
`ActionBarLayer` background now receives
`GColorFromRGBA(41, 40, 43, 170)`, requesting roughly 2/3 opacity
through Pebble's quantized alpha. The SDK accepts a `GColor`, but
only the physical Emery display can establish whether the ActionBar
renderer blends the background with the note rather than drawing a
solid slab. The red destructive-action tile stays opaque and localized.

If this does not reveal underlying text satisfactorily, **revert only
the v1.10.4 alpha commit** and revisit temporarily reducing the reader
viewport width. All stock icons, slimmer nub, hidden boundary arrows,
two-press deletion and long-note touch scrolling are retained when
that experiment is rolled back.

## Action bar refinement — baseline without reader reflow (v1.10.3)

This pass deliberately keeps the full-width reader and native SDK
`ActionBarLayer`, leaving text reflow deferred. Both unavailable
navigation arrows are hidden at the newest and oldest notes, rather
than displaying FIRST NOTE / LAST NOTE in the footer. Physical buttons
still stop at either end. The black bar stays black in Delete mode:
a small red rounded tile is drawn only behind the trash/checkmark
action at Select, and the familiar two-press delete confirmation
remains. The collapsed nub is slimmer (~6px visible).

The pencil and trash use the **official Pebble developer example action
glyph artwork**, scaled slightly for legibility, rather than Jot-created
shapes. Up/Down/Check/X remain upstream
PebbleOS icon art. Pebble only exposes a single background color for
the entire native bar; per-action red emphasis therefore lives in a
small overlay child Layer on the native ActionBar.

The forthcoming alpha experiment is intentionally separate from these
baseline improvements, so the transparency change can be reverted
without giving up the new controls.

## Native ActionBarLayer and safe delete confirmation (v1.10.1 — Emery QA pending)

Jot's hand-drawn reader action panel has been replaced with Pebble's
native `ActionBarLayer`: system-managed button press feedback, icon
positioning, and the correct physical width (34 pixels on Emery).
The bar uses the official PebbleOS Up/Down triangle icons and Jot's
pencil for editing. Holding Select enters persistent Delete mode, with
a red bar and a white trash can icon.

Once deletion is armed, the footer reads **"Confirm delete?"**.
The Up icon becomes blank (but the physical Up button cancels), Select
becomes a checkmark (confirm), and Down becomes an X (cancel).
Physical Back also cancels. Only a second separate Select press can
delete. Successful deletion shows the next note while leaving Delete
mode active.

When closed, the familiar right-edge action nub is now a completely
solid black oval slice with no small speck inside. The nub uses the
same filled-circle silhouette as the open-source PebbleOS ActionBar.
It uses the public SDK's `graphics_fill_circle` (PebbleOS's internal
`graphics_fill_oval` is not public). The native action bar is removed
when closed, restoring normal
60-pixel button scrolling and preserving gesture scrolling, tap-to-edit,
and draft Peek.

Official PebbleOS action icons are attributed under
`resources/icons/ATTRIBUTION.md`. Host tests check the native SDK API
wiring and delete-confirmation icon states; physical Emery appearance
and build still require QA.

## Persistent reader action bar (v1.10.0 — awaiting Emery wrist test)

Issue #10 adds a saved-note review mode for rapidly triaging up to 256 notes.
Select once opens the dark action panel (NEWER / EDIT / OLDER); subsequent
Up/Down buttons flip through saved notes without returning to the list.
Select again edits the displayed note. The small black semicircular nub
at the page's right edge signals the available review actions.

Hold Select (800 ms) to enter persistent DELETE mode. A short Select
**arms** deletion and a second separate press confirms. A long hold
never deletes. Up/Down or Back cancel an armed deletion; otherwise Up/Down
continue browsing, and Back closes the action bar before returning to the
list. Confirmed deletion immediately displays the next existing note and
leaves Delete mode active. Deleting the last note returns to the empty
notes list. Browsing preserves note bytes and keeps the list focused on
the displayed note. Both ends stop rather than wrap.

Touch drag still scrolls long note text; tapping text still opens edit
directly. Touching the overlay never triggers hidden edit actions.
Draft Peek retains its previous controls and cannot browse/delete notes.
Reader Up/Down without the bar still scroll 60 pixels. Note storage,
keyboard gestures and Pocket Notebook theme remain untouched.
Host state-machine and navigation tests passed; wrist testing remains
necessary before closing #10.

## Smooth touch-scrolling for long notes (v1.9.2 — verified on Emery)

Reader and unsaved-draft Peek now support **direct vertical finger scrolling**
in the clipped body of the page. Dragging up moves down the note, tracking
finger movement one pixel at a time; dragging down returns toward the
beginning. There is no momentum or animated page jump. Scrolling clamps at
both ends and only starts after >10px of predominantly vertical movement,
so a stationary tap on saved note text still edits, and a stationary tap
in draft Peek still returns to typing. Drags never activate those actions,
even if the finger comes back to its starting point.

The header Back button, timestamp strip, and footer are outside the drag
target. Blank paper within the body can be used for scrolling. Existing
physical Up/Down scrolling remains at 60px per click, and the reader
footer now advertises DRAG for long notes. The editor gestures, note
storage, navigation, and Pocket Notebook visuals do not change. Pure C
geometry tests and navigation wiring regressions protect the behavior;
physical Emery smoothness remains to be checked.

## Emery virtual-memory fix (v1.9.1 — SDK and watch verification pending)

The initial 1.9.0 build failed at metadata injection: static text buffers
pushed `.text + .data + .bss` to 78,312 bytes, but Pebble's
`PebbleProcessInfo.virtual_size` is only 16-bit (maximum 65,535).
This **RAM/build limit is separate** from the new up-to-1 MiB flash
persistence allowance. The editor now renders directly from the store's
stable note buffer, rather than copying the entire 4 KB note into another
global array. Reader, phone-backup and store-read working buffers
allocate on demand and release when no longer needed. The note index is
packed to 16 bytes per entry, and the caret text-width callback uses
a small stack-independent buffer which grows only for unusually long
unbroken text. Existing disk format, 256-slot/4,095-byte bounds,
transactional saves, keyboard geometry and visual styling remain unchanged.

Host CI checks the memory structure and all existing regressions. The
Pebble SDK compilation and actual on-device memory behavior must still
be verified on Emery.

## Room for a real notebook (v1.9.0 — QA release)

Jot now uses the **up to 1 MiB per-app persistent-storage allowance** on
current Pebble firmware. Its JOT2 storage format supports **256 note slots**
and **4,095 UTF-8 bytes per note** (rather than 16 × 159). These are
independent upper bounds, **not** a guarantee that 256 maximal notes fit
simultaneously: notes share the firmware's available per-app storage pool,
which also holds keys, headers, and temporary save copies. Pebble still
limits each individual persisted value to **256 bytes**, so Jot stores notes
in chunks. The maximum byte length of a note is not the same as the number
of Unicode characters.

An edit writes a new bank of chunks and commits its header last; the
previous committed version remains available if the write fails.
`SAVE ERROR` signals insufficient persistence space or another write
failure. Editing a nearly-full store may need spare room for the temporary
bank. Each new note is allocated only after typing begins; deleting a
note reclaims its chunks. These changes leave keyboard gestures and the
pocket-notebook styling untouched.

**QA reset:** v1.9.0 intentionally discards the old JOT1 watch storage,
which has no migration path. Reinstall/update only with an external backup
of anything important. Phone backup now transfers large notes in multiple
UTF-8-safe packets with per-packet acknowledgments; the on-phone snapshot
is replaced only after every complete note arrives. Backing up near a
full megabyte over Bluetooth may take time. The existing browser export
opens backup JSON in a URL fragment; exporting very large snapshots is
**not yet verified against mobile URL-length limits**. Do not rely on that
path for restoring a full-capacity archive until hardware tests confirm it.

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

At the time of that keyboard-mapping change, its layout, recognizer thresholds, and the then-current 159-byte limit were unchanged. See the [complete readable gesture map](docs/gesture-map.md) and its [CSV source](docs/gesture-map.csv). This map records original design intent—including commands not yet implemented—not a list of features currently working on a watch.

## Export notes to your phone (v0.2)

In the notes list, **hold Up** for about one second. Wait for **Phone copy saved** on the watch. This indicates the entire note snapshot was received and stored in the Pebble phone companion. The watch retains its notes.

In the Pebble phone app, open **Jot → Settings** (gear icon) to view your latest backup and **Copy notes** or save a `.txt`/`.json` file. Browser/webview download support may vary; copying is the fallback.

**One-time setup:** The repo's empty static export viewer must be published with GitHub Pages: **Settings → Pages → Deploy from a branch → main → /docs**. The page doesn't host your note data; the phone passes its local snapshot in the URL fragment, which the page decodes locally. The fragment can reveal private notes to anyone who receives that URL, so don't share it.

See [README-JOT.md](README-JOT.md) for detailed limits and failure handling. This feature needs on-watch/phone verification.

## Storage limits

The current storage model is intentionally bounded:

- Up to **256 notes**, each containing up to **4,095 UTF-8 bytes** of text (not necessarily 4,095 characters); all notes share the available 1 MB storage budget.
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

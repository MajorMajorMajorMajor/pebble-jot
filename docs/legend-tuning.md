# Tuning keyboard labels on Emery

All key-label positions—**peripheral and centre**, on both keyboard faces—live in **one file**:

`src/c/ui/legend_geometry.c`

Both keyboard faces use the same 3-by-3 compass and text slot
(16 by 15 pixels). The ABC positions and their per-letter adjustments
were verified on wrist and should be preserved.

## Numeric face — wrist calibration

Your `stas-adjust-legends` numeric calibration commit is preserved
exactly in the code. The numeric face now shares the alphabetic
baseline: `LEGEND_BASE_FONT_OFFSET_Y = 4`, with no separate global
numeric +4 counter-offset. Your per-direction nudges are all recorded in
`s_number_optical[9][8]`, beside the alphabetical corrections.

You can then move individual symbols using
`s_number_optical[9][8]` in the same file. Entries are
`{x, y}` pixel offsets:

```c
static const JotLegendPosition s_number_optical[9][8] = {
  [1][1] = {0, 1},  // the ^ on 8 moves down 1px
  [7][6] = {-1, 0}  // the comma on 2 moves left 1px
};
```

The examples above are **illustrations only**; the actual code contains your
committed on-watch calibration offsets.

Key index order: `7 8 9 / 4 5 6 / 1 2 3` (0 through 8).
Direction index order, clockwise: `NW N NE E SE S SW W`
(0 through 7). Positive X moves right; positive Y moves down.

The approved ABC optical positions are set in
`s_abc_optical[9][8]`. They are deliberately separate from numeric
adjustments but stored beside them in the **same** geometry module.

Build and inspect on the watch:

```sh
pebble build && pebble install --cloudpebble
```

The old pixel-parity regression was intentionally removed: it froze the
pre-calibration numeric positions and would reject your intended optical
adjustments. Check the positions on the watch instead. The retained C
keyboard-binding tests validate the character mappings, not legend placement.
The numeric legend **character mappings** remain in
`src/c/keyboard/number_face.c` and should not need editing.

## Centre keys (alphabet and numbers)

The centre glyphs use the same file, `src/c/ui/legend_geometry.c`.
Look just below the peripheral tables for the new centre offset tables:

```c
static const JotLegendPosition s_abc_center_optical[9] = {
  [0] = {0, 0}
};
static const JotLegendPosition s_number_center_optical[10] = {
  [0] = {0, 0}
};
```

**Only edit `s_number_center_optical` when tuning the digits.**
For example, to move the big **8** left one pixel and down two,
add `[1] = {-1, 2},` (and remove the placeholder `[0]` if desired).
Centre indices follow the familiar numeric order:
`[0]=7, [1]=8, [2]=9 / [3]=4, [4]=5, [5]=6 / [6]=1, [7]=2, [8]=3`.
The **wide 0** is `[9]` and shares the numeric table, even though
its text frame has a special width and height.

Both alphabet and numeric centre glyphs are currently unchanged:
all per-key centre corrections are zero. The original shared centre
formula (including its -2px vertical correction), the special 0
rectangle, and even the legacy right/bottom text slots are now
calculated by `jot_key_text_frame()` in that file. The key painter
only selects fonts/colours, measures glyphs, and draws the returned
rectangle. The control pictograms are drawn as separate icon artwork,
not as text labels.

The automated label-geometry test exercises baseline positions and
face isolation without freezing specific numeric nudge values, so
you can experiment with optical offsets without rewriting expected
pixel snapshots. You still need a real-watch inspection before
accepting the adjusted positioning.

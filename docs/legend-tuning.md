# Tuning peripheral keyboard legends on Emery

All primary-label positions now live in **one file**:

`src/c/ui/legend_geometry.c`

Both keyboard faces use the same 3-by-3 compass and text slot
(16 by 15 pixels). The ABC positions and their per-letter adjustments
were verified on wrist and should be preserved.

## Numeric face — your calibration area

```c
enum {
  LEGEND_BASE_FONT_OFFSET_Y = 4,
  NUMBER_FACE_VERTICAL_ADJUST_Y = 4
};
```

The base offset of 4 pixels encodes our approved ABC alignment. The
**numeric +4** cancels that change, leaving the 123 face exactly
where it was before this refactor. For your first experiment, change
`NUMBER_FACE_VERTICAL_ADJUST_Y` to `0` and compare on the watch.
This shifts only numeric peripheral glyphs 4 pixels upward.

You can then move individual symbols using
`s_number_optical[9][8]` in the same file. Entries are
`{x, y}` pixel offsets:

```c
static const JotLegendPosition s_number_optical[9][8] = {
  [1][1] = {0, 1},  // the ^ on 8 moves down 1px
  [7][6] = {-1, 0}  // the comma on 2 moves left 1px
};
```

The examples above are **illustrations only**; the actual code
starts with all zeros until you calibrate it.

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

Run the pure C pixel-parity test after changing the geometry:

```sh
cc -std=c11 -Wall -Wextra -Werror -I src/c \
  tests/legend-geometry-test.c src/c/ui/legend_geometry.c -o /tmp/jot-geometry-test
/tmp/jot-geometry-test
```

The baseline test intentionally expects the current unadjusted numeric
positions. Once you've approved your new positions, we should update
its expected coordinates to make those adjustments permanent.
The numeric legend **character mappings** remain in
`src/c/keyboard/number_face.c` and should not need editing.

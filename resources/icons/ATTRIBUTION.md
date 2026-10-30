# PebbleOS action bar artwork

The four PNG assets `action_up.png`, `action_down.png`,
`action_check.png`, and `action_x.png` come from
[coredevices/pebbleos](https://github.com/coredevices/pebbleos):

- `resources/normal/base/images/action_bar_icon_up.png`
- `resources/normal/base/images/action_bar_icon_down.png`
- `resources/common/base/images/action_bar_icon_check.png`
- `resources/common/base/images/action_bar_icon_x.png`

The upstream PebbleOS project is licensed under
[Apache-2.0](https://github.com/coredevices/pebbleos/blob/main/LICENSE).
Jot preserves these native icon graphics unmodified. The Pencil and
Trash glyph PNGs are custom Jot resources, 18×18 pixels, drawn in the
same white-on-transparent action-bar style.

PebbleOS's `fw/applib/ui/action_bar_layer.c` uses
`graphics_fill_oval` for the circular action bar background.
Jot uses the same primitive to draw the **solid** collapsed nub without
an interior mark; Pebble SDK provides no standalone action-nub widget.

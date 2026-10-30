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
Jot preserves these four native icon graphics unmodified. The new Pencil
and Trash images are copied unmodified from the official Pebble
developer App Assets example icons (14×14 pixels):
https://developer.rebble.io/guides/app-resources/app-assets/
Their original paths in `pebble-dev/developer.rebble.io` are
`source/assets/images/guides/design-and-interaction/icons/action_bar_icon_edit.png`
and `action_bar_icon_delete.png`. The red destructive focus tile is
drawn programmatically behind these assets, not baked into new icons.

PebbleOS's `fw/applib/ui/action_bar_layer.c` uses its internal
`graphics_fill_oval` implementation for circular action bar backgrounds.
Jot draws an equivalent **solid** collapsed nub without an interior mark
using public Pebble SDK `graphics_fill_circle`. The Pebble SDK does not
expose either `graphics_fill_oval` or a standalone action-nub widget.

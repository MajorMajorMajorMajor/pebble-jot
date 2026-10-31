// Visual primary legends must match actual implemented, released gestures.
// This intentionally tests NO toggle: don't let an accidental Space gesture
// make the keyboard's alphabet mysteriously disappear.
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const read = p => fs.readFileSync(p, 'utf8');
const paint = read('src/c/ui/key_button.c');
const keys = read('src/c/keyboard/key.c');
const touch = read('src/c/ui/touch.c');
const primary = read('src/c/keyboard/primary_face.c');
const primaryHeader = read('src/c/keyboard/primary_face.h');
const window = read('src/c/ui/window.c');
const docs = read('docs/gesture-map.csv');
const bindings = [];
for(const line of touch.split('\n').filter(x=>x.includes(' // ') && /^[ ]*\{/.test(x)).slice(0,9)) {
  const values = [...line.matchAll(/"((?:\\.|[^"\\])*)"/g)].map(m=>m[1]);
  assert.equal(values.length,19);
  bindings.push(values);
}
assert.equal(bindings.length,9);
const grid = [
  // NW,N,NE,E,SE,S,SW,W gesture indices.
  [8,1,2,3,4,5,6,7],
];
const expected = [
  [null,null,null,null,'v',null,null,null],
  [null,null,null,null,null,'l',null,null],
  [null,null,null,null,null,null,'x',null],
  [null,null,null,'k',null,null,null,null],
  ['q','u','p','b','j','d','g','c'],
  [null,null,null,null,null,null,null,'m'],
  [null,null,'y',null,null,null,null,null],
  [null,'w',null,'z',':','.',',',null],
  ['f',null,null,null,null,null,null,null],
];
for(let k=0;k<9;k++) for(let d=0;d<8;d++) {
  const char = expected[k][d];
  if(!char) continue;
  assert.equal(bindings[k][grid[0][d]],char,
      'ABC runtime must type displayed character '+char);
}
assert.match(paint,/prv_draw_primary_legends\(ctx, button->key, bounds\)/);
assert.match(paint,/jot_primary_face_legend\(\(int\)index, d\)/);
assert.match(paint, /jot_legend_position/);
assert.match(paint, /JOT_LEGEND_ABC/);
assert.match(paint,/FONT_KEY_GOTHIC_14_BOLD/);
assert.match(paint,/GColorFromRGB\(0, 85, 170\)/);
assert.match(paint,/static const int8_t xs\[8\]/);
assert.match(paint,/static const int8_t ys\[8\]/);
// On the 1.15 wrist screenshot, Pebble Gothic 14's glyph ink was
// visually lower than its draw rectangle; bottom legends nearly met
// the key border. Apply the same correction to NW/N/NE, W/E, SW/S/SE
// so the compass is balanced and the centre letters remain untouched.
assert.match(paint,/PRIMARY_LEGEND_FONT_OFFSET_Y = 4/);
assert.match(paint,/2 - PRIMARY_LEGEND_FONT_OFFSET_Y/);
assert.match(paint,/\(h - 14\) \/ 2 - PRIMARY_LEGEND_FONT_OFFSET_Y/);
assert.match(paint,/h - 17 - PRIMARY_LEGEND_FONT_OFFSET_Y/);
// Number-face geometry is already watch-approved: only ABC labels move.
const numericPainter = paint.slice(paint.indexOf('static void prv_draw_number_legends('),
                                  paint.indexOf('static void prv_draw_primary_legends('));
assert.match(numericPainter,/const int row\[3\] = \{2, \(h - 14\) \/ 2, h - 17\}/);
assert.doesNotMatch(numericPainter,/PRIMARY_LEGEND_FONT_OFFSET_Y/);
assert.doesNotMatch(keys,/\.slot = KEY_LABEL_RIGHT|\.slot = KEY_LABEL_BOTTOM/);
assert.match(primary, /'q', 'u', 'p', 'b', 'j', 'd', 'g', 'c'/);
assert.match(primaryHeader, /JotPrimaryLegendOffset jot_primary_face_optical_offset/);
assert.match(primary, /symbol == 'f' \? -2 : 0/);
assert.match(primary, /symbol == 'f' \? 2/);
assert.match(primary, /symbol == 'l' \|\| symbol == 'd'/);
assert.match(docs, /\[Toggle sublabels\]/); // Workbook retains reservation.
assert.doesNotMatch(touch,/keyboard_window_toggle_primary|keyboard_window_toggle_secondary/);
assert.doesNotMatch(window,/keyboard_window_toggle_primary|keyboard_window_toggle_secondary/);
assert.match(touch,/s_touch_key == 13 && gesture_index == 0/);
assert.match(touch,/keyboard_window_toggle_number_face\(\)/);
console.log('Alphabet primary legend painting and all runtime gestures checked');

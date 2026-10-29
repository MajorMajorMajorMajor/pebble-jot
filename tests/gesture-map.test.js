// The complete, originally specified keyboard map is a versioned design input.
// Run: node tests/gesture-map.test.js
const assert = require('node:assert/strict');
const fs = require('node:fs');

const csv = fs.readFileSync('docs/gesture-map.csv', 'utf8').trimEnd();
const implementation = fs.readFileSync('src/c/ui/touch.c', 'utf8');

// Each field is quoted, including commas, quotes, backslashes and symbols.
function parseRow(line) {
  const cells = [];
  let pos = 0;
  while (pos < line.length) {
    assert.equal(line[pos], '"', 'expected opening CSV quote');
    pos += 1;
    let value = '';
    let done = false;
    while (pos < line.length) {
      const ch = line[pos++];
      if (ch === '"') {
        if (line[pos] === '"') { value += '"'; ++pos; }
        else { done = true; break; }
      } else {
        value += ch;
      }
    }
    assert.ok(done, 'unterminated CSV field');
    cells.push(value);
    if (pos === line.length) break;
    assert.equal(line[pos], ',', 'expected field separator');
    ++pos;
  }
  return cells;
}

const [header, ...records] = csv.split(/\r?\n/).map(parseRow);
assert.deepEqual(header, [
  'Key','Tap','NW','N','NE','E','SE','S','SW','W',
  'Return NW','Return N','Return NE','Return E',
  'Return SE','Return S','Return SW','Return W','CW','CCW','Hold'
]);
assert.equal(records.length, 23);
for (const cells of records) assert.equal(cells.length, header.length);
const mapping = new Map(records.map(r => [r[0], Object.fromEntries(header.map((h,i)=>[h,r[i]]))]));
assert.equal(mapping.size, records.length, 'each mapped key must be unique');

// Each cursor shortcut in touch.c is *already reserved* in the workbook.
// Catch accidental changes to either the reference or the watch implementation.
for (const [key, direction, intended, keyIndex, gestureIndex, command] of [
  ['spacebar','E','[Cursor Right]',9,3,'JOT_CURSOR_RIGHT'],
  ['spacebar','W','[Cursor Left]',9,7,'JOT_CURSOR_LEFT'],
  ['i','N','[Cursor up]',2,1,'JOT_CURSOR_UP'],
  ['s','S','[Cursor down]',8,5,'JOT_CURSOR_DOWN'],
  ['i','Return N','[Navigate to start of buffer]',2,9,'JOT_CURSOR_HOME'],
  ['s','Return S','[Navigate to end of buffer]',8,13,'JOT_CURSOR_END']
]) {
  assert.equal(mapping.get(key)[direction], intended,
    key + ' ' + direction + ': cursor binding must match workbook');
  const pattern = new RegExp(
    'key\\s*==\\s*' + keyIndex + '\\s*&&\\s*gesture\\s*==\\s*'
    + gestureIndex + '\\)\\s*move\\s*=\\s*' + command
  );
  assert.match(implementation, pattern,
    key + ' ' + direction + ': missing reserved runtime mapping');
}

assert.equal(mapping.get('spacebar')['Return E'],
  '[Cursor Forward 1 Word]', 'word movement must remain reserved');
assert.equal(mapping.get('spacebar')['Return W'],
  '[Cursor Backward 1 Word]', 'word movement must remain reserved');
// Jot implements all four original forward-delete directions, without
// remapping the other backspace gestures or the diagnostic typing mode.
for (const [direction, index] of [['NE', 2], ['E', 3], ['SE', 4], ['S', 5]]) {
  assert.equal(mapping.get('backspace')[direction], '[Delete character forward]');
  assert.match(implementation, new RegExp('gesture\\s*!=\\s*' + index),
    'missing forward-delete gesture ' + direction);
}
assert.match(implementation, /key\s*!=\s*10/);
assert.match(implementation, /jot_delete_forward\(\)/);
assert.match(implementation, /s_mode\s*==\s*MODE_JOT/);
for (const direction of ['Tap', 'NW', 'N', 'SW', 'W']) {
  assert.equal(mapping.get('backspace')[direction], '[Delete character backward]');
}
assert.equal(mapping.get('r')['N'],
  '[Cycle case]', 'case cycling remains reserved');

console.log('Original gesture map: six cursor shortcuts and four forward-delete directions OK');

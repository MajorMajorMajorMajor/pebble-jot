// A release should not turn Jot's front page back into a changelog.
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');

const read = name => fs.readFileSync(name, 'utf8');
const home = read('README.md');
const guide = read('README-JOT.md');
const history = read('CHANGELOG.md');
const archived = read('docs/history/README-JOT-legacy.md');

assert.match(home, /^# Jot — a pocket notebook for Pebble/m);
assert.match(guide, /^# Jot — controls and developer guide/m);
assert.match(history, /^# Jot changelog/m);
assert.ok(home.length < 12000, 'README should stay approachable');
assert.ok(guide.length < 18000, 'developer guide should describe current behavior');
for (const [name, text] of [['README.md', home], ['README-JOT.md', guide]]) {
  assert.ok(text.startsWith('# Jot'), name + ' must start with a product/manual heading');
  assert.doesNotMatch(text, /^##\s+(?:Jot\s+)?(?:v?\d+\.\d+)/m,
    name + ' must not have version-by-version release headings');
  assert.match(text, /CHANGELOG\.md/);
  for (const match of text.matchAll(/\]\(([^)]+)\)/g)) {
    const target = match[1].split('#')[0];
    if (!target || /^(https?:|mailto:)/.test(target)) continue;
    const resolved = path.normalize(path.join(path.dirname(name), target));
    assert.ok(fs.existsSync(resolved), name + ' has broken relative link: ' + target);
  }
}
assert.match(history, /Jot 1\.16\.2/);
assert.match(history, /Jot 1\.15\.0/);
assert.match(history, /Kinetic scrolling/);
assert.match(history, /Export notes to your phone/);
assert.match(archived, /JOT2 1 MiB storage architecture/);
assert.match(archived, /Hairline insertion caret/);
console.log('Evergreen README, current developer guide and archived release history validated');

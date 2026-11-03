// #39: read-only configmessage and bundled viewer regression tests.
'use strict';
const assert = require('node:assert/strict');
const vm = require('node:vm');
const fs = require('node:fs');
const listeners = {}, saved = {};
const Pebble = {
  addEventListener(name, callback) { listeners[name] = callback; },
  sendAppMessage(_data, done) { if (done) done(); },
  openURL() {}
};
vm.runInNewContext(fs.readFileSync('src/js/pebble-js-app.js', 'utf8'), {
  Pebble, localStorage: {
    getItem(k) { return saved[k] ?? null; },
    setItem(k,v) { saved[k] = v; }
  }, console, encodeURIComponent, decodeURIComponent
});
function ask(data) {
  let response;
  listeners.configmessage({data, respond(v){ response = v; }});
  assert.ok(response);
  return response;
}
function note(index, part) {
  return ask({type:'jotBackupNote',index,part});
}
assert.deepEqual(JSON.parse(JSON.stringify(ask({type:'jotBackupInfo'}))),
  {ok:true,found:false});
assert.equal(note(0,0).ok,false);

const long='a'+'📝'.repeat(850)+' café good-will\n'+('a'.repeat(300));
const notes=Array.from({length:256},(_,i)=>({
  created:1730000000+i,
  text:i===0?'':i===1?long:('Note '+i+' 🌈 ').repeat(280)
}));
saved['jot.backup.v1']=JSON.stringify({
  format:'jot-backup-v1',session:7,
  savedAt:'2026-11-03T10:00:00.000Z',notes
});
const original=saved['jot.backup.v1'];
assert.ok(Buffer.byteLength(original,'utf8') > 970000,
  'exercise realistic near-1MiB Unicode snapshot');
assert.equal(ask({type:'jotBackupInfo'}).count,256);
for(let index=0;index<notes.length;index++){
  let joined='',complete=false;
  for(let part=0;!complete;part++){
    assert.ok(part<8192);
    const r=note(index,part);
    assert.equal(r.ok,true);
    assert.ok(r.text.length<=1024);
    if(r.text.length){
      const last=r.text.charCodeAt(r.text.length-1);
      assert.ok(last<0xD800||last>0xDBFF,'no dangling high surrogate');
    }
    joined+=r.text;complete=r.done;
  }
  assert.equal(joined,notes[index].text);
}
assert.equal(saved['jot.backup.v1'],original,'viewer must not mutate snapshot');
for(const request of [
  {type:'jotBackupNote',index:-1,part:0},
  {type:'jotBackupNote',index:256,part:0},
  {type:'jotBackupNote',index:0,part:1},
  {type:'jotBackupNote',index:1,part:-1},
  {type:'jotBackupNote',index:1.2,part:0},
  {type:'foo'}
]) assert.equal(ask(request).ok,false);
saved['jot.backup.v1']='{"format":"bad"}';
assert.equal(ask({type:'jotBackupInfo'}).ok,false);
const page=fs.readFileSync('config.html','utf8');
assert.ok(!/https?:\/\/|<script[^>]+src=|<link[^>]+href=|fetch\(/.test(page));
assert.ok(!/location.hash|window.location.hash/.test(page));
assert.ok(page.includes("Pebble.sendMessage('pkjs'"));
assert.ok(page.includes('content.textContent=note.text'));
assert.ok(page.includes('jot-notes.json'));
assert.ok(page.includes('jot-notes.txt'));
assert.equal(JSON.parse(fs.readFileSync('package.json','utf8')).pebble.configPage,'config.html');
console.log('Bundled viewer: 256-note Unicode chunking, read-only privacy passed');


// Exercise the actual bundled-page JS against a minimal phone WebView DOM.
// A completed backup is exposed only after *every* note is received.
(async function checkPageBridge() {
  class Element {
    constructor() {
      this.children=[]; this.value=''; this.disabled=false;
      this._text=''; this.onclick=null;
    }
    set textContent(v) { this._text=String(v); this.children=[]; }
    get textContent() { return this._text; }
    appendChild(node) { this.children.push(node); return node; }
    focus() {}
    select() {}
  }
  const elements={};
  for(const id of ['status','backupText','notes','copy','select','download','json'])
    elements[id]=new Element();
  const document={
    getElementById(id) { return elements[id]; },
    createElement() { return new Element(); },
    execCommand() { return true; },
    body: new Element()
  };
  const pageEvents={};
  let interrupt=false;
  const localPebble={
    addEventListener(n,fn) { pageEvents[n]=fn; },
    sendMessage(target,request) {
      assert.equal(target,'pkjs');
      if(interrupt && request.type==='jotBackupNote' &&
         request.index===1 && request.part===1)
        return Promise.resolve({ok:false});
      return Promise.resolve(ask(request));
    }
  };
  const script=page.match(/<script>([\s\S]*?)<\/script>/)[1];
  vm.runInNewContext(script,{
    Pebble:localPebble,document,navigator:{},Date,Number,
    Blob,URL,setTimeout,console
  });
  saved['jot.backup.v1']=original;
  pageEvents.ready({target:'pkjs'});
  await new Promise(resolve=>setImmediate(resolve));
  assert.equal(elements.notes.children.length,256);
  assert.ok(elements.backupText.value.includes('good-will'));
  assert.ok(elements.backupText.value.includes('📝'));
  assert.equal(elements.copy.disabled,false);
  assert.equal(elements.json.disabled,false);
  assert.equal(elements.notes.children[1].children[1].textContent, long);

  delete saved['jot.backup.v1'];
  pageEvents.ready({target:'pkjs'});
  await new Promise(resolve=>setImmediate(resolve));
  assert.match(elements.status.textContent,/No phone backup found/);
  assert.equal(elements.copy.disabled,true);
  assert.equal(elements.notes.children.length,0);

  saved['jot.backup.v1']=original;
  interrupt=true;
  pageEvents.ready({target:'pkjs'});
  await new Promise(resolve=>setImmediate(resolve));
  assert.match(elements.status.textContent,/Could not read your notes/);
  assert.equal(elements.notes.children.length,0);
  assert.equal(elements.backupText.value,'');
  assert.equal(elements.json.disabled,true);
  console.log('Bundled page: full, empty and interrupted WebView flows passed');
})().catch(e=>{console.error(e);process.exitCode=1;});

// Jot #70: independent phone notebook, safe imports and atomic editing.
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const listeners={},storage={},acks=[];
let failNextWrite=false, openedUrl=null;
const phone={
  addEventListener(n,fn){listeners[n]=fn;},
  sendAppMessage(p,ok){acks.push(p);if(ok)ok();},
  openURL(value){openedUrl=value;}
};
vm.runInNewContext(fs.readFileSync('src/js/pebble-js-app.js','utf8'),{
  Pebble:phone,localStorage:{
    getItem(key){return storage[key]??null;},
    setItem(key,value){if(failNextWrite){failNextWrite=false;throw Error('Quota exceeded');}
                       storage[key]=value;}
  },console,Date,Number
});
// Missing locker configPage can still load the same offline page without
// exposing any note content or visiting a hosted site.
assert.equal(typeof listeners.showConfiguration, 'function');
listeners.showConfiguration();
assert.ok(openedUrl.startsWith('data:text/html;charset=utf-8,'));
assert.equal(decodeURIComponent(openedUrl.split(',').slice(1).join(',')),
  fs.readFileSync('config.html','utf8'));
assert.ok(!/https:\/\/majormajormajormajor\.github\.io/.test(openedUrl));
function packet(kind,session,index,extra={}){
  listeners.appmessage({payload:{
    BackupKind:kind,BackupSession:session,BackupIndex:index,...extra
  }});
  return acks.at(-1);
}
function begin(session,total,mode=0,deletedSerial=0){
  const fields={BackupTotal:total,BackupMode:mode};
  if(mode===2)fields.BackupSerial=deletedSerial;
  return packet(1,session,-1,fields);
}
function note(session,index,serial,revision,created,txt,part=0,final=1){
  return packet(2,session,index,{
    BackupPart:part,BackupFinal:final,BackupSerial:serial,
    BackupRevision:revision,BackupTimestamp:created,BackupText:txt
  });
}
function end(session,total){return packet(3,session,total);}
function db(){return JSON.parse(storage['jot.phone.v1']);}
function ask(data){
  let reply;
  listeners.configmessage({data,respond(value){reply=value;}});
  assert.ok(reply);
  return reply;
}
function writePhone(id,text) {
  const r=ask({type:'jotNotebookBegin',id});
  assert.equal(r.ok,true);
  let at=0,part=0;
  while(at<text.length){
    let end=Math.min(text.length,at+1024);
    if(end<text.length&&text.charCodeAt(end-1)>=0xD800&&
       text.charCodeAt(end-1)<=0xDBFF)end--;
    const a=ask({type:'jotNotebookPart',token:r.token,
      part,text:text.slice(at,end)});
    assert.equal(a.ok,true);at=end;part++;
  }
  return ask({type:'jotNotebookCommit',token:r.token});
}
function get(id){return db().notes.find(n=>n.id===id);}

// Neither incomplete transfers nor a failed write publish partial content.
assert.equal(begin(1,2).BackupStatus,1);
assert.equal(note(1,0,1,1,100,'café 📝 good-will').BackupStatus,1);
assert.equal(end(1,2).BackupStatus,0);
assert.equal(storage['jot.phone.v1'],undefined);
assert.equal(note(1,1,2,1,101,'Alice').BackupStatus,1);
assert.equal(end(1,2).BackupStatus,1);
assert.equal(db().notes.length,2);
assert.equal(get('w:1:100').text,'café 📝 good-will');
assert.equal(end(1,2).BackupStatus,1,'lost END receipt idempotent');
const persisted=storage['jot.phone.v1'];
assert.equal(begin(2,1).BackupStatus,1);
assert.equal(note(2,0,1,1,100,'x').BackupStatus,1);
failNextWrite=true;
assert.equal(end(2,1).BackupStatus,0);
assert.equal(storage['jot.phone.v1'],persisted);
assert.equal(end(2,1).BackupStatus,1,'safe retry after failed commit');

// A watch deletion DOES NOT delete the phone copy.
assert.equal(begin(3,1).BackupStatus,1);
assert.equal(note(3,0,1,1,100,'café 📝 good-will').BackupStatus,1);
assert.equal(end(3,1).BackupStatus,1);
assert.equal(db().notes.length,2);
assert.equal(get('w:2:101').onWatch,false);
assert.equal(get('w:2:101').text,'Alice');
assert.equal(begin(4,1).BackupStatus,1);
assert.equal(note(4,0,1,1,100,'café 📝 good-will').BackupStatus,1);
assert.equal(end(4,1).BackupStatus,1);
assert.equal(db().notes.length,2,'repeated backups do not duplicate notes');

// Phone-created note can be arbitrarily long (up to documented limit).
const longer='📝'.repeat(800)+'\nleft-right';
const own=writePhone('',longer);
assert.equal(own.ok,true);
assert.equal(get(own.id).text,longer);
assert.equal(get(own.id).onWatch,false);
assert.equal(db().notes.length,3);
assert.equal(ask({type:'jotBackupInfo'}).count,3);

// Phone edits to watch note must not be overwritten by a later backup.
const edit=writePhone('w:1:100','My phone version');
assert.equal(edit.ok,true);
assert.equal(get('w:1:100').phoneEdited,true);
assert.equal(begin(5,1).BackupStatus,1);
assert.equal(note(5,0,1,1,100,'café 📝 good-will').BackupStatus,1);
assert.equal(end(5,1).BackupStatus,1);
assert.equal(get('w:1:100').text,'My phone version');
assert.equal(get('w:1:100').conflict,false);
assert.equal(begin(6,1).BackupStatus,1);
assert.equal(note(6,0,1,2,100,'new edit on watch').BackupStatus,1);
assert.equal(end(6,1).BackupStatus,1);
assert.equal(get('w:1:100').text,'new edit on watch');
assert.equal(get('w:1:100').conflict,false);
assert.equal(get('w:1:100').watchText,'new edit on watch');
const fork=db().notes.find(n=>n.forkedFrom==='w:1:100');
assert.ok(fork,'offline divergent phone edit is retained as independent note');
assert.equal(fork.text,'My phone version');
assert.equal(fork.onWatch,false);
assert.ok(fork.id.startsWith('p:'));
// Replaying an identical backup cannot spawn a second fork.
assert.equal(begin(61,1).BackupStatus,1);
assert.equal(note(61,0,1,2,100,'new edit on watch').BackupStatus,1);
assert.equal(end(61,1).BackupStatus,1);
assert.equal(db().notes.filter(n=>n.forkedFrom==='w:1:100').length,1);

// User acceptance: author watch -> export -> delete on PHONE -> export
// again. The watch is still authoritative for its own live records.
assert.equal(ask({type:'jotNotebookDelete',id:'w:1:100'}).ok,true);
assert.equal(get('w:1:100'),undefined);
assert.equal(Object.hasOwn(db(),'tombstones'),false);
assert.ok(get(own.id),'unrelated phone-only note persists');

// Simulate a phone DB written by the previous #70 prototype: old deletion
// tombstones must NOT prevent the next watch backup from restoring the note.
var oldDb=db();
oldDb.tombstones=['w:1:100'];
storage['jot.phone.v1']=JSON.stringify(oldDb);
assert.equal(begin(7,1).BackupStatus,1);
assert.equal(note(7,0,1,3,100,'watch still has it').BackupStatus,1);
assert.equal(end(7,1).BackupStatus,1);
assert.equal(get('w:1:100').text,'watch still has it');
assert.equal(get('w:1:100').onWatch,true);
assert.equal(get('w:1:100').phoneEdited,false);
assert.equal(Object.hasOwn(db(),'tombstones'),false,'obsolete tombstones removed');

// A repeat watch backup must update, not duplicate, the reappeared note.
assert.equal(begin(71,1).BackupStatus,1);
assert.equal(note(71,0,1,3,100,'watch still has it').BackupStatus,1);
assert.equal(end(71,1).BackupStatus,1);
assert.equal(db().notes.filter(n=>n.id==='w:1:100').length,1);
assert.equal(ask({type:'jotNotebookDelete',id:own.id}).ok,true);
assert.equal(get(own.id),undefined);
assert.ok(get('w:1:100'),'phone-only deletion does not affect watch imports');

// Reject malformed metadata, incomplete parts and repeated corrupted segments.
assert.equal(begin(8,1).BackupStatus,1);
assert.equal(note(8,0,0,1,400,'bad').BackupStatus,0);
assert.equal(note(8,0,10,1,400,'First ',0,0).BackupStatus,1);
assert.equal(note(8,0,10,1,400,'First ',0,0).BackupStatus,1);
assert.equal(note(8,0,10,1,400,'CORRUPTED',0,0).BackupStatus,0);
assert.equal(note(8,0,10,2,400,'revised',1,1).BackupStatus,0);
assert.equal(end(8,1).BackupStatus,0);
assert.equal(note(8,0,10,1,400,'second',1,1).BackupStatus,1);
assert.equal(end(8,1).BackupStatus,1);
assert.equal(get('w:10:400').text,'First second');
// #76: one-note incremental transfer cannot mark other live notes absent.
assert.equal(begin(90,2).BackupStatus,1);
assert.equal(note(90,0,1,3,100,'watch still has it').BackupStatus,1);
assert.equal(note(90,1,10,1,400,'First second').BackupStatus,1);
assert.equal(end(90,2).BackupStatus,1);
assert.equal(get('w:1:100').onWatch,true);
assert.equal(get('w:10:400').onWatch,true);
assert.equal(begin(91,1,1).BackupStatus,1);
assert.equal(note(91,0,10,2,400,'First second (updated)').BackupStatus,1);
const beforeDelta=storage['jot.phone.v1'];
failNextWrite=true;
assert.equal(end(91,1).BackupStatus,0);
assert.equal(storage['jot.phone.v1'],beforeDelta,'delta commit atomic');
assert.equal(end(91,1).BackupStatus,1);
assert.equal(get('w:10:400').text,'First second (updated)');
assert.equal(get('w:1:100').onWatch,true,'unrelated note stays on watch');
// Presence-only deletion changes no text and no other note's status.
assert.equal(begin(92,0,2,1).BackupStatus,1);
assert.equal(end(92,0).BackupStatus,1);
assert.equal(get('w:1:100').onWatch,false);
assert.equal(get('w:1:100').text,'watch still has it');
assert.equal(get('w:10:400').onWatch,true);
assert.equal(begin(93,0).BackupStatus,1);
assert.equal(end(93,0).BackupStatus,1);
assert.equal(get('w:10:400').onWatch,false,'empty full sync clears presence');
assert.equal(get('w:10:400').text,'First second (updated)');
assert.equal(begin(94,2,1).BackupStatus,0,'one-note delta requires exactly one note');
assert.equal(begin(95,1,2,0).BackupStatus,0,'deletion must specify serial');
assert.equal(begin(9,257).BackupStatus,0);

// A failure to save edited notes never mutates the durable phone DB.
const before=storage['jot.phone.v1'];
const prepare=ask({type:'jotNotebookBegin',id:'w:10:400'});
assert.equal(prepare.ok,true);
assert.equal(ask({type:'jotNotebookPart',token:prepare.token,part:0,text:'fail save'}).ok,true);
failNextWrite=true;
assert.equal(ask({type:'jotNotebookCommit',token:prepare.token}).ok,false);
assert.equal(storage['jot.phone.v1'],before);
assert.equal(ask({type:'jotNotebookCommit',token:prepare.token}).ok,true);

// Read note text from the phone via bounded Unicode-safe messages.
const info=ask({type:'jotBackupInfo'});
assert.equal(info.found,true);
for(let index=0;index<info.count;index++){
  let restored='',part=0,r;
  do{
    r=ask({type:'jotBackupNote',index,part:part++});
    assert.equal(r.ok,true);
    assert.ok(r.text.length<=1024);
    restored+=r.text;
  }while(!r.done);
  assert.equal(restored,db().notes[index].text);
}
assert.equal(ask({type:'jotNotebookDelete',id:'invalid'}).ok,false);
assert.equal(ask({type:'bad-request'}).ok,false);
console.log('Phone notebook: atomic imports, deletion, duplicate suppression, conflicts, phone edits and Unicode passed');

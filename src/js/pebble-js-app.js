/* Jot phone notebook — independent, durable phone-side collection.
 * Manual watch backup merges records, NEVER replaces this collection.
 * No network, cloud, or remote viewer. Phone UI is bundled in the PBW.
 */
var NOTEBOOK_KEY = 'jot.phone.v1';
var pending = null;
var configBackup = null;
var configEdit = null;
var editNonce = 0;
var CONFIG_CHUNK = 1024;
var MAX_PHONE_TEXT = 100000;  // UTF-16 code units. Watch limit remains 4095 UTF-8 bytes.

function readNotebook() {
  var raw = localStorage.getItem(NOTEBOOK_KEY);
  if (!raw) return {format:'jot-phone-v1', nextId:1, notes:[], lastSession:0, savedAt:null};
  var db = JSON.parse(raw);
  if (!db || db.format !== 'jot-phone-v1' ||
      !Array.isArray(db.notes) ||
      !isFinite(db.nextId) || db.nextId < 1) throw new Error('Invalid phone notebook');
  for (var i=0;i<db.notes.length;i++) {
    var n=db.notes[i];
    if (!n || typeof n.id!=='string' || typeof n.text!=='string' ||
        !isFinite(n.created)) throw new Error('Corrupt phone note');
  }
  return db;
}
function writeNotebook(db) {
  // Older #70 test builds stored deletion tombstones. They are intentionally
  // retired: explicit watch backups are allowed to restore phone-deleted
  // notes, and every future write drops the obsolete field.
  delete db.tombstones;
  localStorage.setItem(NOTEBOOK_KEY, JSON.stringify(db));
}
function watchNoteId(note) {
  // Serial identifies an on-watch note generation, timestamp additionally
  // guards against collisions if the serial space is ever restarted.
  return 'w:'+note.serial+':'+note.created;
}
// #76: complete snapshots update presence; incremental updates only
// touch one identity; deletion deltas preserve all phone text.
function mergeWatchNotes(notes, session, mode, deletedSerial) {
  var db=readNotebook(), seen={};
  mode=mode||0;
  if(mode===0) db.notes.forEach(function(n) {
    if(n.watchSerial) n.onWatch=false;
  });
  if(mode===2) db.notes.forEach(function(n) {
    if(n.watchSerial===deletedSerial) n.onWatch=false;
  });
  for(var i=0;i<notes.length;i++) {
    var incoming=notes[i], id=watchNoteId(incoming);
    if(seen[id]) throw new Error('Duplicate watch note identity');
    seen[id]=true;
    var current=findPhoneNote(db,id);
    if(!current) {
      db.notes.push({id:id,created:incoming.created,text:incoming.text,
        watchSerial:incoming.serial,watchRevision:incoming.revision,
        watchText:incoming.text,onWatch:true,phoneEdited:false,conflict:false});
      continue;
    }
    current.onWatch=true;
    if(current.phoneEdited) {
      if(incoming.text===current.text) {
        current.phoneEdited=false;current.conflict=false;
        current.watchRevision=incoming.revision;
        current.watchText=incoming.text;
      } else if(incoming.revision>current.watchRevision ||
                incoming.text!==current.watchText) {
        // Both devices departed from their last agreed version.
        // In ONE durable write, preserve the phone copy as an independent
        // p: note, then refresh the original watch-associated note.
        db.notes.push({id:'p:'+db.nextId++,created:current.created,
          text:current.text,onWatch:false,phoneEdited:false,conflict:false,
          forkedFrom:id});
        current.text=incoming.text;
        current.watchText=incoming.text;
        current.watchRevision=incoming.revision;
        current.phoneEdited=false;
        current.conflict=false;
      }
    } else if(incoming.revision>=current.watchRevision) {
      current.text=incoming.text;current.watchText=incoming.text;
      current.watchRevision=incoming.revision;
      current.conflict=false;
    }
  }
  db.lastSession=session;
  db.savedAt=new Date().toISOString();
  writeNotebook(db);
}

/* Phone -> watch: one explicit note at a time. Every transfer has a 64-bit
 * token persisted on the PHONE before sending a single Bluetooth packet.
 * The watch commits that same token in the note header before replying.
 * A lost receipt/restarted companion resumes using the original token.
 */
var watchTx = null;
var WATCH_PACKET_BYTES = 160;
var WATCH_MAX_RETRIES = 4;
function watchUtf8Bytes(s) {
  var bytes = 0;
  for (var i=0;i<s.length;i++) {
    var ch=s.charCodeAt(i),point=ch;
    if(ch>=0xD800&&ch<=0xDBFF){
      if(i+1>=s.length)throw new Error('Unpaired UTF-16 surrogate');
      var low=s.charCodeAt(++i);
      if(low<0xDC00||low>0xDFFF)throw new Error('Unpaired UTF-16 surrogate');
      point=0x10000+((ch-0xD800)<<10)+(low-0xDC00);
    }else if(ch>=0xDC00&&ch<=0xDFFF)
      throw new Error('Unpaired UTF-16 surrogate');
    bytes+=point<0x80?1:point<0x800?2:point<0x10000?3:4;
  }
  return bytes;
}
function watchChunks(s){
  var chunks=[],start=0;
  while(start<s.length){
    var end=start,bytes=0;
    while(end<s.length){
      var ch=s.charCodeAt(end),extra=1,nb;
      if(ch>=0xD800&&ch<=0xDBFF){extra=2;nb=4;}
      else nb=ch<0x80?1:ch<0x800?2:3;
      if(bytes+nb>WATCH_PACKET_BYTES)break;
      bytes+=nb;end+=extra;
    }
    if(end===start)throw new Error('Invalid send chunk');
    chunks.push(s.slice(start,end));start=end;
  }
  return chunks;
}
function watchTxStop(){
  if(watchTx&&watchTx.timer){clearTimeout(watchTx.timer);watchTx.timer=null;}
}
function watchTxFail(message){
  if(!watchTx)return;
  watchTxStop();
  watchTx.state='failed';
  watchTx.error=message;
}
function watchTxSaved(packet){
  var tx=watchTx;
  if(!tx||tx.state!=='sending')return;
  watchTxStop();
  try{
    var serial=Number(packet.ImportSerial);
    var created=Number(packet.ImportCreated);
    var revision=Number(packet.ImportRevision);
    if(!Number.isInteger(serial)||serial<1||serial>4294967295||
       !Number.isInteger(created)||created<1||
       !Number.isInteger(revision)||revision<1)
      throw new Error('Watch did not provide a persistent receipt');
    var db=readNotebook(),n=findPhoneNote(db,tx.id);
    if(!n||n.importHi!==tx.hi||n.importLo!==tx.lo)
      throw new Error('Phone note changed during transfer');
    var id='w:'+serial+':'+created;
    var existing=findPhoneNote(db,id);
    if(existing&&existing!==n)
      throw new Error('Watch identity already present on phone');
    n.id=id;
    n.watchSerial=serial;n.watchRevision=revision;
    n.watchText=tx.text;n.onWatch=true;
    n.phoneEdited=n.text!==tx.text;n.conflict=false;
    delete n.importHi;delete n.importLo;delete n.importText;
    db.savedAt=new Date().toISOString();
    writeNotebook(db);
    configBackup=null;
    tx.state='saved';
    tx.savedId=id;
  }catch(e){watchTxFail('Watch saved it, but phone receipt failed: '+e.message+
                             '. Retry Send to Watch.');}
}
function watchTxPacket(){
  var tx=watchTx;
  if(!tx||tx.state!=='sending')return;
  var payload={ImportTokenHi:tx.hi,ImportTokenLo:tx.lo};
  var part;
  if(tx.step===0){
    part=-1;payload.ImportKind=1;
    payload.ImportPart=part;payload.ImportTotal=tx.bytes;
  }else if(tx.step<=tx.chunks.length){
    part=tx.step-1;payload.ImportKind=2;
    payload.ImportPart=part;payload.ImportText=tx.chunks[part];
  }else{
    part=-2;payload.ImportKind=3;payload.ImportPart=part;
  }
  tx.currentPart=part;
  tx.attempts++;
  // Only a matching watch acknowledgment advances the transfer. The
  // SDK's transport success callback does not mean the note was saved.
  try{Pebble.sendAppMessage(payload,function(){},function(){
    if(watchTx!==tx||tx.state!=='sending')return;
    watchTxStop();
    if(tx.attempts>=WATCH_MAX_RETRIES)watchTxFail('Cannot reach watch; retry');
    else tx.timer=setTimeout(watchTxPacket,450);
  });}catch(e){
    watchTxFail('Watch message failed: '+e.message);
    return;
  }
  if(watchTx!==tx||tx.state!=='sending')return;
  watchTxStop();
  tx.timer=setTimeout(function(){
    if(watchTx!==tx||tx.state!=='sending')return;
    if(tx.attempts>=WATCH_MAX_RETRIES)watchTxFail('Watch did not acknowledge; retry');
    else watchTxPacket();
  },3500);
}
function watchTxAck(p){
  var tx=watchTx;
  if(!tx||tx.state!=='sending'||
     Number(p.ImportTokenHi)!==tx.hi||
     Number(p.ImportTokenLo)!==tx.lo||
     Number(p.ImportAck)!==tx.currentPart)return;
  var code=Number(p.ImportStatus);
  watchTxStop();
  if(code===2){watchTxSaved(p);return;}
  if(code===1){
    if(tx.currentPart===-2){watchTxSaved(p);return;}
    tx.step++;tx.attempts=0;
    tx.timer=setTimeout(watchTxPacket,50);
    return;
  }
  watchTxFail(code===-1?'Save the open watch draft first':
              code===-2?'Watch notebook is full':
              code===-3?'Watch rejected invalid note data':
              'Watch could not save note; retry');
}
function watchTxBegin(id){
  if(watchTx&&watchTx.state==='sending')
    throw new Error('Another note is already sending');
  var db=readNotebook(),n=findPhoneNote(db,id);
  if(!n)throw new Error('Phone note not found');
  // After watch deletion an imported note remains on the phone with its
  // old w: ID. It may be sent back as a NEW watch generation. Refuse
  // only notes presently known to exist on the watch.
  if(n.onWatch)throw new Error('Note already exists on watch');
  var original=typeof n.importText==='string'?n.importText:n.text;
  var bytes=watchUtf8Bytes(original);
  if(!bytes)throw new Error('Write something before sending');
  if(bytes>4095)throw new Error('Too long for watch: '+bytes+'/4095 UTF-8 bytes');
  if(!Number.isInteger(n.importHi)||!Number.isInteger(n.importLo)||
     (!n.importHi&&!n.importLo)){
    // Random 64-bit ID rather than a recyclable note index. Persist before
    // transmission, and reuse unchanged across connection loss/restarts.
    n.importHi=(Math.floor(Math.random()*4294967296)>>>0);
    n.importLo=((Date.now() ^ Math.floor(Math.random()*4294967296))>>>0);
    if(!n.importHi&&!n.importLo)n.importLo=1;
    n.importText=original;
    writeNotebook(db);
  }
  watchTxStop();
  watchTx={id:id,hi:n.importHi,lo:n.importLo,text:original,
           bytes:bytes,chunks:watchChunks(original),step:0,attempts:0,
           currentPart:-1,state:'sending',error:'',timer:null,savedId:null};
  watchTx.timer=setTimeout(watchTxPacket,20);
  return {ok:true,state:'sending',bytes:bytes};
}

function reply(session,index,part,ok) {
  Pebble.sendAppMessage({
    BackupSession:session,BackupAck:index,BackupPart:part,BackupStatus:ok?1:0
  },function(){},function(e){
    console.log('Jot receipt not delivered: '+JSON.stringify(e));
  });
}
Pebble.addEventListener('ready',function(){
  console.log('Jot phone notebook ready.');
});
Pebble.addEventListener('appmessage',function(event){
  var p=event.payload||{};
  if(typeof p.ImportAck!=='undefined'&&
     typeof p.ImportStatus!=='undefined'){
    watchTxAck(p);return;
  }
  if(typeof p.BackupKind==='undefined'||
     typeof p.BackupSession==='undefined'||
     typeof p.BackupIndex==='undefined')return;
  var kind=Number(p.BackupKind),session=Number(p.BackupSession);
  var index=Number(p.BackupIndex),part=Number(
    typeof p.BackupPart==='undefined'?0:p.BackupPart);
  if(!isFinite(session)||!isFinite(index)||!isFinite(part)||
     index!==Math.floor(index)||part<0||part!==Math.floor(part))return;
  if(kind===1){
    var total=Number(p.BackupTotal);
    if(!isFinite(total)||total<0||total>256||
       total!==Math.floor(total)||index!==-1||part!==0){
      reply(session,index,part,false);return;
    }
    if(!pending||pending.session!==session)
      pending={session:session,total:total,notes:new Array(total)};
    reply(session,index,part,true);
  }else if(kind===2){
    var finalPart=Number(typeof p.BackupFinal==='undefined'?1:p.BackupFinal);
    var serial=Number(p.BackupSerial),revision=Number(p.BackupRevision);
    var created=Number(p.BackupTimestamp);
    if(!pending||pending.session!==session||index<0||
       index>=pending.total||typeof p.BackupText!=='string'||
       !isFinite(created)||!isFinite(serial)||!isFinite(revision)||
       serial<1||serial>4294967295||serial!==Math.floor(serial)||
       revision<1||revision!==Math.floor(revision)||
       (finalPart!==0&&finalPart!==1)){
      reply(session,index,part,false);return;
    }
    var note=pending.notes[index];
    if(!note&&part===0){
      note={created:created,serial:serial,revision:revision,text:'',
            nextPart:0,complete:false,lastChunk:null,lastFinal:null};
      pending.notes[index]=note;
    }
    if(!note){reply(session,index,part,false);return;}
    if(part===note.nextPart-1){
      // A retry must contain the EXACT previously accepted chunk.
      reply(session,index,part,
        created===note.created && serial===note.serial &&
        revision===note.revision && p.BackupText===note.lastChunk &&
        finalPart===note.lastFinal);
      return;
    }
    if(note.complete||part!==note.nextPart||
       note.created!==created||note.serial!==serial||
       note.revision!==revision||note.text.length+p.BackupText.length>MAX_PHONE_TEXT){
      reply(session,index,part,false);return;
    }
    note.text+=p.BackupText;
    note.lastChunk=p.BackupText;
    note.lastFinal=finalPart;
    note.nextPart++;
    if(finalPart===1)note.complete=true;
    reply(session,index,part,true);
  }else if(kind===3){
    if(!pending||pending.session!==session||
       index!==pending.total||part!==0){
      try{
        var previous=readNotebook();
        reply(session,index,part,previous.lastSession===session);
      }catch(e){reply(session,index,part,false);}
      return;
    }
    for(var i=0;i<pending.total;i++)
      if(!pending.notes[i]||!pending.notes[i].complete){
        reply(session,index,part,false);return;
      }
    try{
      mergeWatchNotes(pending.notes,session);
      pending=null;
      reply(session,index,part,true);
    }catch(e){
      console.log('Phone notebook import failed: '+e);
      reply(session,index,part,false);
    }
  }
});

// The configuration bridge has its own bounded reads and writes.
// A configuration-page failure must never affect the watch backup receiver.
function configChunkEnd(s,start){
  var end=Math.min(s.length,start+CONFIG_CHUNK);
  if(end>start&&end<s.length&&
     s.charCodeAt(end-1)>=0xD800&&s.charCodeAt(end-1)<=0xDBFF&&
     s.charCodeAt(end)>=0xDC00&&s.charCodeAt(end)<=0xDFFF)end--;
  return end;
}
function publicNoteStatus(n) {
  return !n.onWatch?'phone-only':n.conflict?'conflict':
    n.phoneEdited?'needs-sync':'on-watch';
}
function findPhoneNote(db,id) {
  for(var i=0;i<db.notes.length;i++)
    if(db.notes[i].id===id)return db.notes[i];
  return null;
}
function configReply(event,fn){
  try{event.respond(fn());}
  catch(e){
    console.log('Jot phone config request failed: '+e);
    event.respond({ok:false,error:e.message||'Phone storage error'});
  }
}

Pebble.addEventListener('configmessage',function(event){
  var q=event.data||{};
  if(q.type==='jotWatchBegin'){
    configReply(event,function(){return watchTxBegin(q.id);});
    return;
  }
  if(q.type==='jotWatchStatus'){
    configReply(event,function(){
      var t=watchTx;
      if(!t||t.id!==q.id)return {ok:true,state:'idle'};
      return {ok:true,state:t.state,bytes:t.bytes,
        part:t.currentPart,total:t.chunks.length,
        error:t.error,savedId:t.savedId};
    });
    return;
  }
  if(q.type==='jotBackupInfo'){
    configReply(event,function(){
      var raw=localStorage.getItem(NOTEBOOK_KEY);
      var db=readNotebook();
      configBackup=raw?{
        savedAt:db.savedAt,notes:db.notes.map(function(n){
          return {id:n.id,created:n.created,text:n.text,
                  status:publicNoteStatus(n)};
        })
      }:null;
      return configBackup?
        {ok:true,found:true,count:configBackup.notes.length,
         savedAt:configBackup.savedAt}:
        {ok:true,found:false};
    });
    return;
  }
  if(q.type==='jotBackupNote'){
    configReply(event,function(){
      var index=Number(q.index),part=Number(q.part);
      if(!configBackup||!Number.isInteger(index)||!Number.isInteger(part)||
         index<0||index>=configBackup.notes.length||part<0||part>8192)
        throw new Error('Invalid note request');
      var note=configBackup.notes[index],start=0,step=0;
      while(step<part&&start<note.text.length){
        start=configChunkEnd(note.text,start);step++;
      }
      if(step!==part||(part>0&&start>=note.text.length))
        throw new Error('Invalid note part');
      var end=configChunkEnd(note.text,start);
      return {ok:true,index:index,part:part,id:note.id,
        created:note.created,status:note.status,
        text:note.text.slice(start,end),done:end===note.text.length};
    });
    return;
  }
  if(q.type==='jotNotebookBegin'){
    configReply(event,function(){
      var db=readNotebook();
      var id=typeof q.id==='string'?q.id:'';
      if(id&&!findPhoneNote(db,id))throw new Error('Note no longer exists');
      configEdit={token:++editNonce,id:id,text:'',part:0};
      return {ok:true,token:configEdit.token};
    });
    return;
  }
  if(q.type==='jotNotebookPart'){
    configReply(event,function(){
      var token=Number(q.token),part=Number(q.part);
      if(!configEdit||token!==configEdit.token||
         !Number.isInteger(part)||part<0||typeof q.text!=='string'||
         q.text.length>CONFIG_CHUNK)throw new Error('Invalid edit chunk');
      if(part===configEdit.part-1)return {ok:true,part:part};
      if(part!==configEdit.part||
         configEdit.text.length+q.text.length>MAX_PHONE_TEXT)
        throw new Error('Edit too large or out of order');
      configEdit.text+=q.text;
      configEdit.part++;
      return {ok:true,part:part};
    });
    return;
  }
  if(q.type==='jotNotebookCommit'){
    configReply(event,function(){
      if(!configEdit||Number(q.token)!==configEdit.token)
        throw new Error('No active edit');
      var db=readNotebook(),note;
      if(configEdit.id){
        note=findPhoneNote(db,configEdit.id);
        if(!note)throw new Error('Note was deleted');
        if(note.text!==configEdit.text){
          note.text=configEdit.text;
          if(note.watchSerial)note.phoneEdited=true;
          note.conflict=false; // Future watch edits are detected at import.
        }
      }else{
        note={id:'p:'+db.nextId++,created:Math.floor(Date.now()/1000),
          text:configEdit.text,onWatch:false,phoneEdited:false,
          conflict:false};
        db.notes.unshift(note);
      }
      writeNotebook(db);
      configEdit=null;configBackup=null;
      return {ok:true,id:note.id,status:publicNoteStatus(note)};
    });
    return;
  }
  if(q.type==='jotNotebookDelete'){
    configReply(event,function(){
      var db=readNotebook(),id=q.id;
      if(typeof id!=='string')throw new Error('Invalid note');
      var note=findPhoneNote(db,id);
      if(!note)throw new Error('Note not found');
      // Delete this phone copy only. A subsequent explicit watch backup
      // will restore it if it still exists on the watch.
      db.notes=db.notes.filter(function(n){return n.id!==id;});
      writeNotebook(db);configBackup=null;
      return {ok:true};
    });
    return;
  }
  event.respond({ok:false,error:'Unknown notebook request'});
});

// JOT_EMBEDDED_SETTINGS_BEGIN (generated by scripts/sync-settings-fallback.py)
var JOT_CONFIG_PAGE_HTML = "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n<meta name=\"referrer\" content=\"no-referrer\">\n<title>Jot · Phone notebook</title>\n<style>\n:root{color-scheme:light;font-family:system-ui,-apple-system,sans-serif}\n*{box-sizing:border-box}body{margin:0;background:#f8f0d8;color:#29282b;min-height:100vh}\nmain{max-width:720px;margin:0 auto;padding:24px 18px 64px}\nheader{background:#ffcf57;padding:22px;border:3px solid #26242a;border-radius:14px;box-shadow:5px 5px 0 #29282b}\nh1{font-size:32px;margin:0 0 3px;letter-spacing:-1.5px}h2{font-size:19px;margin:24px 0 10px}\np{line-height:1.5}.sub{margin:0;font-weight:600}.muted{color:#57535b}\n.card{background:#fffaf0;border:2px solid #29282b;border-radius:10px;padding:16px;margin:16px 0}\n.row{display:flex;flex-wrap:wrap;align-items:center;gap:8px;margin:16px 0}\nbutton{background:#29282b;color:#fff;border:0;border-radius:8px;font-weight:700;font-size:15px;padding:12px 16px;cursor:pointer}\nbutton.secondary{background:#ffd35d;color:#29282b;border:2px solid #29282b;padding:10px 14px}\nbutton:disabled{opacity:.4;cursor:not-allowed}\ntextarea{width:100%;min-height:200px;background:white;color:#29282b;border:2px solid #777;border-radius:7px;padding:12px;font:14px/1.5 ui-monospace,SFMono-Regular,monospace;resize:vertical}\narticle{white-space:pre-wrap;overflow-wrap:anywhere;padding:12px 0;border-bottom:1px solid #ccc}\narticle:last-child{border:0}.date{font-size:12px;color:#555;margin-bottom:5px}.note{font-size:17px}.noteTools{display:flex;gap:8px;margin-top:10px;flex-wrap:wrap}\n.noteTools button{padding:8px 12px;font-size:13px}.statusTag{display:inline-block;font-weight:700;\nfont-size:12px;margin-left:8px;padding:3px 7px;border-radius:5px;background:#f8e6a7}\n#editPanel{background:#fffaf0;border:3px solid #29282b;border-radius:10px;padding:16px;margin:18px 0}\n#editPanel[hidden]{display:none}#noteEditor{min-height:240px}\n#status{font-weight:700}footer{margin-top:30px;font-size:13px;color:#58535b}\n</style>\n</head>\n<body>\n<main>\n<header>\n<h1>✎ Jot</h1>\n<p class=\"sub\">Your pocket notebook · phone companion</p>\n</header>\n<p id=\"status\" role=\"status\">Waiting for the local Jot companion…</p>\n<p class=\"muted\">Your phone keeps its own notes. Watch backups add and update notes, but never silently delete phone copies.</p>\n<div class=\"row\">\n<button id=\"newNote\" class=\"secondary\">+ New note</button>\n<button id=\"copy\">Copy notes</button>\n<button id=\"select\" class=\"secondary\">Select all</button>\n<button id=\"download\" class=\"secondary\">Save .txt</button>\n<button id=\"json\" class=\"secondary\">Save .json</button>\n</div>\n<label for=\"backupText\"><strong>Plain text backup</strong></label>\n<textarea id=\"backupText\" readonly spellcheck=\"false\"></textarea>\n<section id=\"editPanel\" hidden>\n  <h2 id=\"editTitle\">Edit note</h2>\n  <p class=\"muted\" id=\"editHint\">Changes are saved on your phone.</p>\n  <textarea id=\"noteEditor\" aria-label=\"Note text\" spellcheck=\"false\"></textarea>\n  <p id=\"watchBudget\" class=\"muted\">Watch capacity: 4,095 UTF-8 bytes.</p>\n  <div class=\"row\">\n    <button id=\"saveNote\">Save on phone</button>\n    <button id=\"cancelEdit\" class=\"secondary\">Cancel</button>\n  </div>\n</section>\n<h2>Phone notes</h2>\n<div id=\"notes\"></div>\n<footer>On your watch, open the notes list and hold Up to transfer notes to the phone. Your phone preserves notes even after they are deleted from the watch. Deleting on the phone removes only that copy; the next watch backup brings it back if it is still on the watch.</footer>\n</main>\n<script>\n(function() {\n  'use strict';\n  var status=document.getElementById('status');\n  var box=document.getElementById('backupText');\n  var notesRoot=document.getElementById('notes');\n  var panel=document.getElementById('editPanel');\n  var editor=document.getElementById('noteEditor');\n  var newButton=document.getElementById('newNote');\n  var saveButton=document.getElementById('saveNote');\n  var watchBudget=document.getElementById('watchBudget');\n  var controls=['copy','select','download','json'];\n  var snapshot=null, text='', editingId='', busy=false;\n  function mark(s){status.textContent=s;}\n  function enable(b){controls.forEach(function(id){document.getElementById(id).disabled=!b;});}\n  function date(t){var d=new Date(Number(t)*1000);return isNaN(d.getTime())?'Unknown date':d.toLocaleString();}\n  function send(m){return Pebble.sendMessage('pkjs',m);}\n  function partEnd(s,start){\n    var end=Math.min(s.length,start+1024);\n    if(end>start&&end<s.length&&s.charCodeAt(end-1)>=0xD800&&\n       s.charCodeAt(end-1)<=0xDBFF&&s.charCodeAt(end)>=0xDC00&&\n       s.charCodeAt(end)<=0xDFFF)end--;\n    return end;\n  }\n  function countWatchBytes(s) {\n    var count=0;\n    for(var i=0;i<s.length;i++){\n      var ch=s.charCodeAt(i);\n      if(ch>=0xD800&&ch<=0xDBFF) {\n        if(i+1>=s.length||s.charCodeAt(i+1)<0xDC00||\n           s.charCodeAt(i+1)>0xDFFF)\n          throw Error('Invalid Unicode surrogate');\n        i++;count+=4;\n      }else if(ch>=0xDC00&&ch<=0xDFFF)\n        throw Error('Invalid Unicode surrogate');\n      else count+=ch<128?1:ch<2048?2:3;\n    }\n    return count;\n  }\n  function updateBudget(){\n    try{\n      var bytes=countWatchBytes(editor.value);\n      watchBudget.textContent='Watch: '+bytes+' / 4,095 UTF-8 bytes'+\n        (bytes>4095?' — too long to send':'');\n    }catch(e){watchBudget.textContent='Invalid Unicode: cannot send to watch';}\n  }\n  editor.oninput=updateBudget;\n  function labelFor(v){\n    return v==='conflict'?'Conflicting edits':\n      v==='needs-sync'?'Phone changes not on watch':\n      v==='on-watch'?'On watch and phone':'Phone only';\n  }\n  function saveFile(name,body,mime) {\n    try {\n      var url=URL.createObjectURL(new Blob([body],{type:mime}));\n      var link=document.createElement('a');\n      link.href=url;link.download=name;document.body.appendChild(link);link.click();\n      link.remove();setTimeout(function(){URL.revokeObjectURL(url);},1000);\n      mark('Export requested. Check Downloads.');\n    } catch(e){mark('File download unavailable. Use Copy notes instead.');}\n  }\n  function beginEdit(id) {\n    if(busy)return;\n    var note=null;\n    if(snapshot&&id)for(var i=0;i<snapshot.notes.length;i++)\n      if(snapshot.notes[i].id===id){note=snapshot.notes[i];break;}\n    if(id&&!note){mark('Note not found. Reload Settings.');return;}\n    editingId=id||'';\n    document.getElementById('editTitle').textContent=id?'Edit note':'New note';\n    document.getElementById('editHint').textContent=\n      note&&note.status==='conflict'?\n      'Both devices were edited. Saving here preserves the phone version; the watch is not overwritten.':\n      'Your phone saves edits locally. Send any phone-only note to the watch; shared-note edit sync comes later.';\n    editor.value=note?note.text:'';\n    updateBudget();\n    panel.hidden=false;\n    editor.focus();\n  }\n  function closeEdit(){\n    panel.hidden=true;editingId='';editor.value='';\n  }\n  async function saveEdit() {\n    if(busy)return;\n    busy=true;saveButton.disabled=true;\n    try {\n      if(editor.value.length>100000)throw Error('Note is too large to save');\n      mark('Saving note on phone…');\n      var begin=await send({type:'jotNotebookBegin',id:editingId});\n      if(!begin||!begin.ok)throw Error(begin&&begin.error||'Cannot begin edit');\n      var part=0,at=0;\n      // Empty notes still commit atomically; no chunks necessary.\n      while(at<editor.value.length){\n        var end=partEnd(editor.value,at);\n        var reply=await send({type:'jotNotebookPart',\n          token:begin.token,part:part,text:editor.value.slice(at,end)});\n        if(!reply||!reply.ok)throw Error(reply&&reply.error||'Edit transfer interrupted');\n        at=end;part++;\n      }\n      var result=await send({type:'jotNotebookCommit',token:begin.token});\n      if(!result||!result.ok)throw Error(result&&result.error||'Phone storage failed');\n      closeEdit();\n      await load();\n      mark('Note saved on phone.');\n    }catch(e){mark('Could not save note: '+e.message);}\n    finally{busy=false;saveButton.disabled=false;}\n  }\n  async function deleteNote(note,button) {\n    if(busy)return;\n    if(button.textContent!=='Confirm delete'){\n      button.textContent='Confirm delete';\n      button.setAttribute('aria-label','Confirm deleting this note from the phone');\n      return;\n    }\n    busy=true;button.disabled=true;\n    try{\n      var result=await send({type:'jotNotebookDelete',id:note.id});\n      if(!result||!result.ok)throw Error(result&&result.error||'Deletion failed');\n      await load();\n      mark('Deleted on phone. A future watch backup can restore it.');\n    }catch(e){mark('Delete failed: '+e.message);}\n    finally{busy=false;button.disabled=false;}\n  }\n  async function sendToWatch(note,button){\n    if(busy)return;\n    busy=true;button.disabled=true;\n    try{\n      mark('Preparing note for watch…');\n      var begin=await send({type:'jotWatchBegin',id:note.id});\n      if(!begin||!begin.ok)throw Error(begin&&begin.error||'Cannot start send');\n      mark('Sending '+begin.bytes+' UTF-8 bytes to watch…');\n      var result=null;\n      for(var attempt=0;attempt<140;attempt++){\n        await new Promise(function(resolve){setTimeout(resolve,400);});\n        result=await send({type:'jotWatchStatus',id:note.id});\n        if(!result||!result.ok)throw Error('Lost watch send status');\n        if(result.state==='saved')break;\n        if(result.state==='failed')throw Error(result.error||'Watch transfer failed');\n        if(result.state!=='sending')throw Error('Transfer interrupted; retry');\n      }\n      if(!result||result.state!=='saved')\n        throw Error('Watch response timed out; retry to check receipt');\n      await load();\n      mark('Saved on watch ✓');\n    }catch(e){\n      mark('Not confirmed on watch: '+e.message);\n    }finally{busy=false;button.disabled=false;}\n  }\n  function appendNote(note){\n    var article=document.createElement('article');\n    var heading=document.createElement('div');\n    heading.className='date';\n    heading.textContent=date(note.created);\n    var badge=document.createElement('span');\n    badge.className='statusTag';\n    badge.textContent=labelFor(note.status);\n    heading.appendChild(badge);\n    var body=document.createElement('div');\n    body.className='note';body.textContent=note.text; // Untrusted text is never HTML.\n    article.appendChild(heading);article.appendChild(body);\n    var actions=document.createElement('div');actions.className='noteTools';\n    var edit=document.createElement('button');\n    edit.className='secondary';edit.textContent='Edit';\n    edit.onclick=function(){beginEdit(note.id);};\n    var del=document.createElement('button');\n    del.className='secondary';del.textContent='Delete';\n    del.onclick=function(){deleteNote(note,del);};\n    actions.appendChild(edit);\n    if(note.status==='phone-only'){\n      var sendButton=document.createElement('button');\n      sendButton.textContent='Send to Watch';\n      sendButton.onclick=function(){sendToWatch(note,sendButton);};\n      actions.appendChild(sendButton);\n    }\n    actions.appendChild(del);article.appendChild(actions);\n    notesRoot.appendChild(article);\n  }\n  async function load() {\n    enable(false);\n    notesRoot.textContent='';box.value='';snapshot=null;text='';\n    mark('Reading the phone notebook…');\n    try {\n      var info=await send({type:'jotBackupInfo'});\n      if(!info||!info.ok)throw Error(info&&info.error||'Phone notebook unavailable');\n      if(!info.found) {\n        mark('No phone notebook yet. Write a new note or send notes from your watch.');\n        box.value='No notes yet.';\n        return;\n      }\n      if(!Number.isInteger(info.count)||info.count<0||info.count>10000)\n        throw Error('Invalid note count');\n      var value={format:'jot-phone-export-v1',savedAt:info.savedAt,notes:[]};\n      var lines=['Jot phone notebook · '+(info.savedAt||'')+'\\n'];\n      for(var i=0;i<info.count;i++){\n        mark('Reading note '+(i+1)+' of '+info.count+'…');\n        var body='',created=null,id=null,noteStatus=null,done=false;\n        for(var part=0;part<8192&&!done;part++){\n          var r=await send({type:'jotBackupNote',index:i,part:part});\n          if(!r||!r.ok||r.index!==i||r.part!==part||\n             typeof r.text!=='string'||typeof r.created!=='number'||\n             typeof r.id!=='string'||(part>0&&!r.done&&!r.text.length))\n            throw Error('Transfer interrupted at note '+(i+1));\n          if(part===0){created=r.created;id=r.id;noteStatus=r.status;}\n          else if(created!==r.created||id!==r.id||noteStatus!==r.status)\n            throw Error('Note changed during transfer');\n          body+=r.text;done=r.done===true;\n        }\n        if(!done)throw Error('Note exceeded safety limit');\n        value.notes.push({id:id,created:created,text:body,status:noteStatus});\n        lines.push('['+date(created)+'] '+labelFor(noteStatus)+'\\n'+body+'\\n');\n      }\n      snapshot=value;text=lines.join('\\n---\\n\\n');box.value=text;\n      value.notes.forEach(appendNote);\n      mark(info.count+' note(s) in your phone notebook');\n      enable(true);\n    }catch(e){\n      snapshot=null;text='';box.value='';notesRoot.textContent='';enable(false);\n      mark('Could not read phone notebook: '+e.message+'. Reopen Settings to retry.');\n    }\n  }\n  enable(false);\n  newButton.onclick=function(){beginEdit('');};\n  document.getElementById('cancelEdit').onclick=closeEdit;\n  saveButton.onclick=saveEdit;\n  document.getElementById('select').onclick=function(){box.focus();box.select();};\n  document.getElementById('copy').onclick=function(){\n    box.focus();box.select();\n    if(navigator.clipboard&&navigator.clipboard.writeText)\n      navigator.clipboard.writeText(text).then(\n        function(){mark('Copied to clipboard');},\n        function(){mark('Text selected. Use Copy in the phone menu.');});\n    else if(document.execCommand&&document.execCommand('copy'))mark('Copied to clipboard');\n    else mark('Text selected. Use Copy in the phone menu.');\n  };\n  document.getElementById('download').onclick=function(){\n    if(snapshot)saveFile('jot-notes.txt',text,'text/plain;charset=utf-8');\n  };\n  document.getElementById('json').onclick=function(){\n    if(snapshot)saveFile('jot-notes.json',JSON.stringify(snapshot,null,2),\n      'application/json;charset=utf-8');\n  };\n  if(typeof Pebble==='undefined'||!Pebble.addEventListener){\n    mark('Enable Settings → Plugins → Plugins / permissions preview in the Pebble phone app.');\n    newButton.disabled=true;\n  } else {\n    Pebble.addEventListener('ready',function(event){if(event.target==='pkjs')load();});\n  }\n})();\n</script>\n</body>\n</html>\n";
// A missing/stale configPage locker entry must not silently disable Settings.
// This URL contains ONLY our static HTML, NEVER a user's notes.
Pebble.addEventListener('showConfiguration', function () {
  console.log('Jot: using self-contained Settings fallback');
  Pebble.openURL('data:text/html;charset=utf-8,' +
    encodeURIComponent(JOT_CONFIG_PAGE_HTML));
});
// JOT_EMBEDDED_SETTINGS_END

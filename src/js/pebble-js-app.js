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
  if (!raw) return {format:'jot-phone-v1', nextId:1, notes:[], tombstones:[], lastSession:0, savedAt:null};
  var db = JSON.parse(raw);
  if (!db || db.format !== 'jot-phone-v1' ||
      !Array.isArray(db.notes) || !Array.isArray(db.tombstones) ||
      !isFinite(db.nextId) || db.nextId < 1) throw new Error('Invalid phone notebook');
  for (var i=0;i<db.notes.length;i++) {
    var n=db.notes[i];
    if (!n || typeof n.id!=='string' || typeof n.text!=='string' ||
        !isFinite(n.created)) throw new Error('Corrupt phone note');
  }
  return db;
}
function writeNotebook(db) {
  localStorage.setItem(NOTEBOOK_KEY, JSON.stringify(db));
}
function watchNoteId(note) {
  // Serial identifies an on-watch note generation, timestamp additionally
  // guards against collisions if the serial space is ever restarted.
  return 'w:'+note.serial+':'+note.created;
}
function mergeWatchNotes(notes, session) {
  var db=readNotebook();
  var seen={};
  var blocked={};
  db.tombstones.forEach(function(id){ blocked[id]=true; });
  db.notes.forEach(function(note){ if (note.watchSerial) note.onWatch=false; });
  for (var i=0;i<notes.length;i++) {
    var incoming=notes[i], id=watchNoteId(incoming);
    if (seen[id]) throw new Error('Duplicate watch note identity');
    seen[id]=true;
    if(blocked[id])continue; // Explicitly deleted on the PHONE.
    var current=null;
    for(var j=0;j<db.notes.length;j++)
      if(db.notes[j].id===id){current=db.notes[j];break;}
    if(!current){
      db.notes.push({
        id:id,created:incoming.created,text:incoming.text,
        watchSerial:incoming.serial,watchRevision:incoming.revision,
        watchText:incoming.text,onWatch:true,phoneEdited:false,conflict:false
      });
      continue;
    }
    current.onWatch=true;
    // A watch edit cannot overwrite an un-synchronized phone edit.
    if(current.phoneEdited){
      if(incoming.text===current.text){
        current.phoneEdited=false;
        current.conflict=false;
        current.watchRevision=incoming.revision;
        current.watchText=incoming.text;
      } else if (incoming.revision > current.watchRevision ||
                 incoming.text!==current.watchText) {
        current.conflict=true;
        current.watchRevision=incoming.revision;
        current.watchText=incoming.text;
      }
    } else if (incoming.revision >= current.watchRevision) {
      current.text=incoming.text;
      current.watchText=incoming.text;
      current.watchRevision=incoming.revision;
      current.conflict=false;
    }
  }
  db.lastSession=session;
  db.savedAt=new Date().toISOString();
  // One atomic write. On failure all prior notes/tombstones remain durable.
  writeNotebook(db);
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
            nextPart:0,complete:false};
      pending.notes[index]=note;
    }
    if(!note){reply(session,index,part,false);return;}
    if(part===note.nextPart-1){
      // Lost acknowledgment: return success WITHOUT appending a duplicate.
      reply(session,index,part,true);return;
    }
    if(note.complete||part!==note.nextPart||
       note.created!==created||note.serial!==serial||
       note.revision!==revision||note.text.length+p.BackupText.length>MAX_PHONE_TEXT){
      reply(session,index,part,false);return;
    }
    note.text+=p.BackupText;
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
  return n.conflict?'conflict':n.phoneEdited?'needs-sync':
    n.onWatch?'on-watch':'phone-only';
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
      // A watch note deleted on the PHONE stays deleted even if the watch
      // sends another backup. On-watch deletion is a separate explicit act.
      if(note.watchSerial&&db.tombstones.indexOf(id)<0)
        db.tombstones.push(id);
      db.notes=db.notes.filter(function(n){return n.id!==id;});
      writeNotebook(db);configBackup=null;
      return {ok:true};
    });
    return;
  }
  event.respond({ok:false,error:'Unknown notebook request'});
});

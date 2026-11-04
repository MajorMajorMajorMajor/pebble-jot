// Jot #70: bundled phone notebook screen flows against a fake WebView.
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const pkjsListeners={},pageEvents={},storage={};
const Pebble={
 addEventListener(n,fn){pkjsListeners[n]=fn;},
 sendAppMessage(_data,done){if(done)done();}
};
vm.runInNewContext(fs.readFileSync('src/js/pebble-js-app.js','utf8'),{
 Pebble,localStorage:{getItem(k){return storage[k]??null;},
   setItem(k,v){storage[k]=v;}},Date,Number,console
});
function ask(data){
 let reply;
 pkjsListeners.configmessage({data,respond(v){reply=v;}});
 return reply;
}
const long='📝'.repeat(850)+' café good-will\n'+('x'.repeat(1000));
const notes=Array.from({length:256},(_,i)=>({
 id:'w:'+(i+1)+':'+(1700000000+i),created:1700000000+i,
 text:i===0?'':i===1?long:('Note '+i+' 🌈 ').repeat(280),
 watchSerial:i+1,watchRevision:1,watchText:'',
 onWatch:true,phoneEdited:false,conflict:false
}));
storage['jot.phone.v1']=JSON.stringify({
 format:'jot-phone-v1',nextId:1,notes,tombstones:[],
 lastSession:8,savedAt:'2026-11-03T15:00:00.000Z'
});
assert.ok(Buffer.byteLength(storage['jot.phone.v1'],'utf8')>950000);
class Element {
 constructor(){this.children=[];this.value='';this.disabled=false;
   this._text='';this.hidden=false;this.onclick=null;this.attributes={};}
 set textContent(v){this._text=String(v);this.children=[];}
 get textContent(){return this._text;}
 appendChild(e){this.children.push(e);return e;}
 remove(){}
 focus(){}
 select(){}
 setAttribute(k,v){this.attributes[k]=v;}
}
const elements={};
for(const id of ['status','backupText','notes','copy','select','download','json',
 'newNote','saveNote','cancelEdit','editPanel','noteEditor','editTitle','editHint','watchBudget'])
 elements[id]=new Element();
const document={
 getElementById(id){return elements[id];},
 createElement(){return new Element();},
 body:new Element(),execCommand(){return true;}
};
let interrupt=false;
const bridge={
 addEventListener(n,fn){pageEvents[n]=fn;},
 sendMessage(target,req){
   assert.equal(target,'pkjs');
   if(interrupt&&req.type==='jotBackupNote'&&req.index===1&&req.part===1)
     return Promise.resolve({ok:false});
   return Promise.resolve(ask(req));
 }
};
const page=fs.readFileSync('config.html','utf8');
assert.ok(!/https?:\/\/|<script[^>]+src=|<link[^>]+href=|fetch\(/.test(page));
assert.ok(!/location.hash|window.location.hash/.test(page));
const script=page.match(/<script>([\s\S]*?)<\/script>/)[1];
vm.runInNewContext(script,{Pebble:bridge,document,navigator:{},Date,Number,
 Blob,URL,setTimeout,console});
function spin(){return new Promise(resolve=>setImmediate(resolve));}
(async function(){
 pageEvents.ready({target:'pkjs'});
 await spin();
 assert.equal(elements.notes.children.length,256);
 assert.equal(elements.notes.children[1].children[1].textContent,long);
 assert.equal(elements.copy.disabled,false);
 assert.equal(elements.json.disabled,false);
 assert.match(elements.backupText.value,/good-will/);
 assert.equal(elements.notes.children[0].children[0].children[0].textContent,
   'On watch and phone');

 // Edit an on-watch note, confirming it becomes a pending phone edit.
 elements.notes.children[1].children[2].children[0].onclick();
 assert.equal(elements.noteEditor.value,long);
 assert.equal(elements.editPanel.hidden,false);
 elements.noteEditor.value='Edited by phone 📝';
 elements.saveNote.onclick();
 await spin();
 await spin();
 assert.equal(JSON.parse(storage['jot.phone.v1']).notes[1].text,'Edited by phone 📝');
 assert.equal(JSON.parse(storage['jot.phone.v1']).notes[1].phoneEdited,true);
 assert.equal(elements.editPanel.hidden,true);

 // Create a new phone-only note.
 elements.newNote.onclick();
 elements.noteEditor.value='Brand new phone note';
 elements.saveNote.onclick();
 await spin();await spin();
 assert.equal(JSON.parse(storage['jot.phone.v1']).notes.length,257);
 assert.equal(JSON.parse(storage['jot.phone.v1']).notes[0].onWatch,false);

 // Phone deletion requires two explicit taps; it never tombstones watch data.
 const target=elements.notes.children[1].children[2].children[1];
 target.onclick();
 assert.equal(target.textContent,'Confirm delete');
 assert.equal(JSON.parse(storage['jot.phone.v1']).notes.length,257);
 target.onclick();await spin();await spin();
 assert.equal(JSON.parse(storage['jot.phone.v1']).notes.length,256);
 assert.equal(Object.hasOwn(JSON.parse(storage['jot.phone.v1']),'tombstones'),false);

 delete storage['jot.phone.v1'];
 pageEvents.ready({target:'pkjs'});await spin();
 assert.match(elements.status.textContent,/No phone notebook yet/);
 assert.equal(elements.copy.disabled,true);
 assert.equal(elements.notes.children.length,0);
 // Even with no snapshot, New note must work.
 elements.newNote.onclick();elements.noteEditor.value='First!';
 elements.saveNote.onclick();await spin();await spin();
 assert.equal(JSON.parse(storage['jot.phone.v1']).notes.length,1);

 storage['jot.phone.v1']=JSON.stringify({
  format:'jot-phone-v1',nextId:1,notes,tombstones:[],lastSession:8,savedAt:''
 });
 interrupt=true;
 pageEvents.ready({target:'pkjs'});await spin();
 assert.match(elements.status.textContent,/Could not read phone notebook/);
 assert.equal(elements.notes.children.length,0);
 assert.equal(elements.backupText.value,'');
 assert.equal(elements.json.disabled,true);
 console.log('Phone WebView: near-1MB read, edit, new, delete, empty and interruption passed');
})().catch(e=>{console.error(e);process.exitCode=1;});

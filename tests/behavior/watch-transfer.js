// Jot #70: simulate outbound watch transfer, lost receipts and restart.
// Node-only behavior harness; watch flash idempotence is tested in notes.c.
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const source=fs.readFileSync('src/js/pebble-js-app.js','utf8');
const storage={};
const flash=new Map();
let serial=20, dropped=false, failCommitStorage=false, failBegin=false;
let sent=[],listeners=null;
function watchReply(packet,events){
  const key=packet.ImportTokenHi+':'+packet.ImportTokenLo;
  const part=packet.ImportPart;
  if(failBegin&&packet.ImportKind===1){
    failBegin=false;
    return {ImportStatus:-1,ImportAck:part,
      ImportTokenHi:packet.ImportTokenHi,ImportTokenLo:packet.ImportTokenLo};
  }
  if(flash.has(key)){
    const item=flash.get(key);
    return {ImportStatus:2,ImportAck:part,
      ImportTokenHi:packet.ImportTokenHi,ImportTokenLo:packet.ImportTokenLo,
      ImportSerial:item.serial,ImportCreated:item.created,ImportRevision:1};
  }
  if(packet.ImportKind===1){
    events.inbound={key,part:0,total:packet.ImportTotal,body:''};
  }else if(packet.ImportKind===2){
    const inb=events.inbound;
    assert.ok(inb&&inb.key===key);
    if(part===inb.part){inb.body+=packet.ImportText;inb.part++;}
    else assert.equal(part,inb.part-1,'retry must repeat latest chunk');
    assert.ok(Buffer.byteLength(inb.body,'utf8')<=inb.total);
  }else if(packet.ImportKind===3){
    const inb=events.inbound;
    assert.ok(inb&&inb.key===key);
    assert.equal(Buffer.byteLength(inb.body,'utf8'),inb.total);
    flash.set(key,{text:inb.body,serial:++serial,created:1700000000+serial});
    const receipt=flash.get(key);
    if(!dropped){
      dropped=true;return null; // Watch saved, but commit receipt was lost.
    }
    return {ImportStatus:2,ImportAck:part,
      ImportTokenHi:packet.ImportTokenHi,ImportTokenLo:packet.ImportTokenLo,
      ImportSerial:receipt.serial,ImportCreated:receipt.created,ImportRevision:1};
  }else assert.fail('unknown packet');
  return {ImportStatus:1,ImportAck:part,
    ImportTokenHi:packet.ImportTokenHi,ImportTokenLo:packet.ImportTokenLo};
}
function startPhone(){
  const events={inbound:null};
  const handlers={};
  const pebble={
    addEventListener(n,fn){handlers[n]=fn;},
    sendAppMessage(data,done){
      sent.push(data);
      const ack=watchReply(data,events);
      if(done)done();
      if(ack)setTimeout(()=>handlers.appmessage({payload:ack}),1);
    },
    openURL(){}
  };
  vm.runInNewContext(source,{
    Pebble:pebble,localStorage:{
      getItem(k){return storage[k]??null;},
      setItem(k,v){
        if(failCommitStorage&&v.includes('"watchSerial":')){
          failCommitStorage=false;throw Error('Simulated phone quota error');
        }
        storage[k]=v;
      }
    },
    console,Date,Number,Math,
    setTimeout(fn,delay){return setTimeout(fn,delay>=3500?12:1);},
    clearTimeout
  });
  listeners=handlers;
  return {handlers};
}
function ask(q){
  let reply;
  listeners.configmessage({data:q,respond(r){reply=r;}});
  return reply;
}
function phoneNote(text){
  const b=ask({type:'jotNotebookBegin',id:''});
  assert.equal(b.ok,true);
  const p=ask({type:'jotNotebookPart',token:b.token,part:0,text});
  assert.equal(p.ok,true);
  const c=ask({type:'jotNotebookCommit',token:b.token});
  assert.equal(c.ok,true);
  return c.id;
}
async function until(id,expected){
  for(let i=0;i<150;i++){
    const p=ask({type:'jotWatchStatus',id});
    if(p.state===expected)return p;
    if(p.state==='failed'&&expected!=='failed')
      throw Error('Transfer failed: '+p.error);
    await new Promise(r=>setTimeout(r,3));
  }
  throw Error('Timed out waiting for '+expected);
}
(async function(){
  startPhone();
  const original='Good morning 📝 café\\nLine two\\n\tTab';
  const id=phoneNote(original);
  const first=ask({type:'jotWatchBegin',id});
  assert.equal(first.ok,true);
  assert.equal(first.bytes,Buffer.byteLength(original,'utf8'));
  const status=await until(id,'saved');
  assert.equal(flash.size,1,'lost commit receipt must not duplicate watch note');
  assert.equal(flash.values().next().value.text,original);
  assert.equal(status.savedId,'w:21:1700000021');
  assert.equal(JSON.parse(storage['jot.phone.v1']).notes[0].id,status.savedId);
  assert.equal(JSON.parse(storage['jot.phone.v1']).notes[0].watchSerial,21);
  assert.ok(sent.some(p=>p.ImportKind===3));
  assert.ok(sent.filter(p=>p.ImportKind===3).length>=2,'commit retried');

  // Restart and ensure the successfully sent note cannot be sent again.
  startPhone();
  assert.equal(ask({type:'jotWatchBegin',id:status.savedId}).ok,false);
  assert.equal(flash.size,1);

  // Boundary: 4095 UTF-8 bytes, including a four-byte emoji; validate chunks.
  dropped=true;
  sent=[];
  const max='a'.repeat(4091)+'📝';
  const big=phoneNote(max);
  const bigStart=ask({type:'jotWatchBegin',id:big});
  assert.equal(bigStart.bytes,4095);
  await until(big,'saved');
  assert.equal(flash.size,2);
  assert.equal([...flash.values()].at(-1).text,max);
  for(const packet of sent.filter(p=>p.ImportKind===2)){
    assert.ok(Buffer.byteLength(packet.ImportText,'utf8')<=160);
    assert.ok(!packet.ImportText.includes('\uFFFD'));
  }
  const oversized=phoneNote('📝'.repeat(1024));
  assert.match(ask({type:'jotWatchBegin',id:oversized}).error,/Too long/);
  const empty=phoneNote('');
  assert.match(ask({type:'jotWatchBegin',id:empty}).error,/Write something/);
  const malformed=phoneNote('hi\uD800');
  assert.match(ask({type:'jotWatchBegin',id:malformed}).error,/surrogate/i);

  // Watch dirty editor: refusal, same durable token reusable on retry.
  const retry=phoneNote('Retry after saving draft');
  failBegin=true;
  assert.equal(ask({type:'jotWatchBegin',id:retry}).ok,true);
  const refusal=await until(retry,'failed');
  assert.match(refusal.error,/draft/i);
  const dbBefore=JSON.parse(storage['jot.phone.v1']);
  const tokenBefore=[dbBefore.notes.find(n=>n.id===retry).importHi,
                     dbBefore.notes.find(n=>n.id===retry).importLo];
  assert.equal(ask({type:'jotWatchBegin',id:retry}).ok,true);
  await until(retry,'saved');
  assert.equal(flash.size,3);
  const firstToken=sent.find(p=>p.ImportTokenHi===tokenBefore[0]&&
                                  p.ImportTokenLo===tokenBefore[1]);
  assert.ok(firstToken);

  console.log('Phone to watch: Unicode chunks, watch persistence receipt, lost ACK, retry/restart, 4095-byte limit and dirty-editor refusal passed');
})().catch(error=>{console.error(error);process.exitCode=1;});

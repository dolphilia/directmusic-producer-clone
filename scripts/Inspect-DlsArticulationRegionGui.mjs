import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const dir=path.resolve(process.argv[2]), read=p=>fs.readFileSync(p), json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,'')), hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const launch=json(dir+'/launch.json'), build=json(launch.buildSummary);
assert(build.passed&&build.sourceSnapshotUnchanged);
assert.equal(hash(launch.executable),launch.exeSha256);
assert.equal(hash(launch.buildSummary),launch.buildSummarySha256);
for(const s of build.sources){assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);assert.equal(hash(s.path),s.sha256);}
const previous='work/analysis/articulation-gui/20261004T042000Z';
const prior=json(previous+'/articulation-gui-proof.json');
assert(prior.passed);assert.equal(prior.processId,launch.processId);assert.equal(prior.exeSha256,launch.exeSha256);
assert.equal(hash(dir+'/Before.dls'),prior.redoneSha256);
assert.equal(hash(previous+'/Collection.dls'),prior.redoneSha256);
function parse(b,start=0,end=b.length){
  assert(start+8<=end);const id=b.toString('ascii',start,start+4),n=b.readUInt32LE(start+4),finish=start+8+n;
  assert(finish+(n&1)<=end);const c={id,raw:b.subarray(start,finish+(n&1)),data:b.subarray(start+8,finish)};
  if(id==='RIFF'||id==='LIST'){assert(n>=4);c.type=b.toString('ascii',start+8,start+12);c.children=[];let at=start+12;while(at<finish){const x=parse(b,at,finish);c.children.push(x);at+=x.raw.length;}assert.equal(at,finish);}
  return c;
}
const child=(c,type)=>{const a=c.children.filter(x=>x.id==='LIST'&&x.type===type);assert.equal(a.length,1);return a[0];};
const ins=c=>child(c,'lins').children.find(x=>x.type==='ins '), reg=c=>child(ins(c),'lrgn').children.find(x=>['rgn ','rgn2'].includes(x.type));
const art=c=>c.id==='LIST'&&['lart','lar2'].includes(c.type);
const blocks=c=>c.children.filter(art).flatMap(l=>l.children.filter(x=>['art1','art2'].includes(x.id)).map(x=>({listType:l.type,id:x.id,header:x.data.readUInt32LE(0),count:x.data.readUInt32LE(4),data:x.data})));
const connection=b=>{assert.equal(b.header,8);assert.equal(b.count,1);assert.equal(b.data.length,20);return [b.data.readUInt16LE(8),b.data.readUInt16LE(10),b.data.readUInt16LE(12),b.data.readUInt16LE(14),b.data.readInt32LE(16)];};
// Rebuild container lengths after removing ONLY Region articulation. Everything
// else, including Instrument articulation, GUID, pool, PCM and padding, is exact.
function stripped(c,inRegion=false){if(!c.children)return c.raw;const region=inRegion||['rgn ','rgn2'].includes(c.type);const children=c.children.filter(x=>!(region&&art(x))).map(x=>stripped(x,region));const payload=Buffer.concat([Buffer.from(c.type,'ascii'),...children]);const header=Buffer.alloc(8);header.write(c.id);header.writeUInt32LE(payload.length,4);return Buffer.concat([header,payload,...(payload.length&1?[Buffer.alloc(1)]:[])]);}
const names=['Before','Added','Undone','Redone','TwoLevels','Collection'], trees=Object.fromEntries(names.map(n=>[n,parse(read(dir+'/'+n+'.dls'))]));
assert.equal(blocks(reg(trees.Before)).length,0);
for(const n of names){assert.equal(trees[n].raw.length,read(dir+'/'+n+'.dls').length);assert(stripped(trees[n]).equals(stripped(trees.Before)),n+' unrelated bytes');assert.deepEqual(connection(blocks(ins(trees[n]))[0]),[0,0,1280,0,-65536]);}
for(const n of ['Added','Redone']){const b=blocks(reg(trees[n]));assert.equal(b.length,1);assert.equal(b[0].listType,'lart');assert.equal(b[0].id,'art1');assert.deepEqual(connection(b[0]),[0,0,1,0,-65536]);}
const undone=blocks(reg(trees.Undone));assert.equal(undone.length,1);assert.equal(undone[0].count,0);assert.equal(undone[0].data.length,8);assert.equal(undone[0].listType,'lart');assert.equal(undone[0].id,'art1');
const two=blocks(reg(trees.TwoLevels));assert.equal(two.length,2);assert.deepEqual(connection(two[0]),[0,0,1,0,-65536]);assert.equal(two[1].listType,'lar2');assert.equal(two[1].id,'art2');assert.equal(two[1].count,0);assert.equal(two[1].data.length,8);
assert(read(dir+'/Added.dls').equals(read(dir+'/Redone.dls')));assert(read(dir+'/Collection.dls').equals(read(dir+'/TwoLevels.dls')));
const state=json(dir+'/09-redone-saved-confirmed.json');assert.equal(state.window.id,launch.articulationWindowId);assert.match(state.accessibility.tree,/Owner Value: Region 1 override/);assert.match(state.accessibility.tree,/Value: -65536 ID: 24/);assert.match(state.accessibility.tree,/Saved\. Closing/);
assert.match(json(dir+'/13-independent-save-as.json').accessibility.tree,/Saved under the new name/);
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,scope:'Current retained GUI Region Level1 create/add/Undo save/Redo save and Level2 empty block; independent SaveAs exact bytes. Connection edit/remove, process reload and audio unexecuted.',processId:launch.processId,exeSha256:launch.exeSha256,buildSummarySha256:launch.buildSummarySha256,priorGuiProofSha256:hash(previous+'/articulation-gui-proof.json'),outputs:names.map(n=>({path:dir+'/'+n+'.dls',sha256:hash(dir+'/'+n+'.dls')})),unrelatedBytesPreserved:true,priorWorkingCopyRestored:true,auditorSha256:hash(new URL(import.meta.url)),fullAcceptancePassed:false};
fs.copyFileSync(new URL(import.meta.url),dir+'/region-gui-auditor.mjs');fs.writeFileSync(dir+'/region-gui-proof.json',JSON.stringify(proof,null,2)+'\n');console.log(JSON.stringify(proof,null,2));

import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';import assert from 'node:assert/strict';
const dir=path.resolve(process.argv[2]),read=p=>fs.readFileSync(p),json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,'')),hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const launch=json(dir+'/launch.json'),build=json(launch.buildSummary);assert(build.passed&&build.sourceSnapshotUnchanged);assert.equal(hash(launch.executable),launch.exeSha256);assert.equal(hash(launch.buildSummary),launch.buildSummarySha256);
for(const s of build.sources){assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);assert.equal(hash(s.path),s.sha256);}
assert.equal(hash(dir+'/Before.dls'),launch.inputSha256);
const original=read(dir+'/Before.dls');let scaleOffset;
function walk(start,end,parents=[]){assert(start+8<=end);const id=original.toString('ascii',start,start+4),n=original.readUInt32LE(start+4),finish=start+8+n;assert(finish+(n&1)<=end);
 if(id==='RIFF'||id==='LIST'){assert(n>=4);const type=original.toString('ascii',start+8,start+12);let at=start+12;while(at<finish){at+=walk(at,finish,[...parents,type]);}assert.equal(at,finish);}
 else if(id==='art1'&&parents.at(-1)==='lar2'&&parents.at(-2)==='ins '){assert.equal(scaleOffset,undefined);assert.equal(n,20);assert.equal(original.readUInt32LE(start+8),8);assert.equal(original.readUInt32LE(start+12),1);assert.equal(original.readUInt16LE(start+20),1280);assert.equal(original.readInt32LE(start+24),2147483647);scaleOffset=start+24;}
 return 8+n+(n&1);
}
assert.equal(walk(0,original.length),original.length);assert.notEqual(scaleOffset,undefined);
const expected=Buffer.from(original);expected.writeInt32LE(-78659584,scaleOffset);
for(const n of ['Noop','Undone'])assert(read(dir+'/'+n+'.dls').equals(original),n+' exact input');
for(const n of ['Edited','Redone','Collection'])assert(read(dir+'/'+n+'.dls').equals(expected),n+' only exact scale field');
for(const [n,value] of [['04-units-original','32767.9999847412109375'],['07-edited-saved','-1200.25'],['08-undone-saved','32767.9999847412109375'],['09-redone-saved','-1200.25']]){
 const s=json(dir+'/'+n+'.json');assert.equal(s.window.id,launch.articulationWindowId);assert(Date.parse(s.time)>=Date.parse(launch.startUtc));assert(s.accessibility.tree.includes('Scale (cents) Value: '+value+' ID: 24'));assert.match(s.accessibility.tree,/Scale format Value: Destination units \(16.16\)/);assert.match(s.accessibility.tree,/Saved\. Closing/);
}
const capture=json(dir+'/gui-modules.json'),provenance=json(dir+'/gui-module-provenance.json');assert.equal(capture.processId,launch.processId);assert(provenance.passed);assert.equal(provenance.captureSha256,hash(dir+'/gui-modules.json'));assert.equal(provenance.exeSha256,launch.exeSha256);
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,scope:'Current 66-source product GUI cents mode maximum exact no-op, -1200.25 edit and UndoSave/RedoSave. Decimal converter native limits/rounding/rejections passed separately. Timecents/centibels GUI, original dynamic comparison, current audio and full acceptance unexecuted.',processId:launch.processId,exeSha256:launch.exeSha256,buildSummarySha256:launch.buildSummarySha256,scaleOffset,expectedRawScale:-78659584,inputSha256:hash(dir+'/Before.dls'),editedSha256:hash(dir+'/Edited.dls'),moduleProofSha256:hash(dir+'/gui-module-provenance.json'),auditorSha256:hash(new URL(import.meta.url)),fullAcceptancePassed:false};
fs.copyFileSync(new URL(import.meta.url),dir+'/units-gui-auditor.mjs');fs.writeFileSync(dir+'/units-gui-proof.json',JSON.stringify(proof,null,2)+'\n');console.log(JSON.stringify(proof,null,2));

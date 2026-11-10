import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const read=p=>fs.readFileSync(p),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,''));
const self=fileURLToPath(import.meta.url);
function tracks(bytes){
  const found=[];
  function scan(start,end,inTracks=false){for(let p=start;p<end;){assert(p+8<=end);const size=bytes.readUInt32LE(p+4),stop=p+8+size,next=stop+(size&1);assert(next<=end);const id=bytes.toString('ascii',p,p+4);if(id==='RIFF'||id==='LIST'){assert(size>=4);const type=bytes.toString('ascii',p+8,p+12);scan(p+12,stop,inTracks||type==='trkl');}else if(id==='trkh'&&inTracks){assert(size>=32);found.push({offset:p+8,type:bytes.subarray(p+8,p+24).toString('hex'),position:bytes.readUInt32LE(p+24),group:bytes.readUInt32LE(p+28)});}p=next;}}
  assert.equal(bytes.toString('ascii',0,4),'RIFF');assert.equal(bytes.toString('ascii',8,12),'DMSG');scan(0,bytes.length);return found;
}
// Frozen dmplugin.h CLSID_DirectMusicBandTrack / CLSID_DirectMusicSeqTrack.
const band='9428acd29bb3d111870400600893b1bd',sequence='8628acd29bb3d111870400600893b1bd';
const mode=process.argv[2],dir=path.resolve(process.argv[3]);
if(mode==='prepare'){
  const input=path.resolve(process.argv[4]),bytes=read(input),headers=tracks(bytes);
  assert.equal(headers.filter(h=>h.type===band).length,3);assert.equal(headers.filter(h=>h.type===sequence).length,3);
  fs.mkdirSync(dir,{recursive:true});const variants=[];
  for(const [name,positions] of [['ascending',[1,2,3]],['descending',[3,2,1]],['permuted',[2,3,1]]]){
    const edited=Buffer.from(bytes),changes=[];for(const type of [sequence,band])headers.filter(h=>h.type===type).forEach((h,i)=>{edited.writeUInt32LE(positions[i],h.offset+16);changes.push({type,group:h.group,offset:h.offset+16,before:h.position,after:positions[i]});});
    const file=path.join(dir,name+'.sgp');fs.writeFileSync(file,edited);variants.push({name,path:file,sha256:hash(edited),changes});
  }
  const manifest={schema:1,scope:'Owned copies change only six trkh dwPosition DWORDs; no original application behavior inferred',input:{path:input,sha256:hash(bytes)},headers,variants,driverSha256:hash(read(self))};
  fs.writeFileSync(path.join(dir,'inputs.json'),JSON.stringify(manifest,null,2)+'\n');fs.copyFileSync(self,path.join(dir,'position-auditor.mjs'));console.log(JSON.stringify(manifest));
}else if(mode==='audit'){
  const manifest=json(path.join(dir,'inputs.json')),original=read(manifest.input.path);assert.equal(hash(original),manifest.input.sha256);assert.equal(hash(read(self)),manifest.driverSha256);
  const results=[];for(const runPathArg of process.argv.slice(4)){
    const runPath=path.resolve(runPathArg),run=json(runPath),variant=manifest.variants.find(v=>v.path===run.groupPlaybackInput.path);assert(variant);const input=read(variant.path);assert.equal(hash(input),variant.sha256);
    const expected=Buffer.from(original);for(const c of variant.changes){assert.equal(original.readUInt32LE(c.offset),c.before);expected.writeUInt32LE(c.after,c.offset);}assert(input.equals(expected));
    const proofPath=path.join(path.dirname(runPath),'group-playback/group-playback-proof.json'),proof=json(proofPath);assert(proof.passed);assert.equal(proof.runSha256,hash(read(runPath)));assert.equal(proof.input.sha256,variant.sha256);
    assert(input.equals(read(path.join(path.dirname(proofPath),'runtime.sgp'))));
    results.push({name:variant.name,run:runPath,runSha256:hash(read(runPath)),proof:proofPath,proofSha256:hash(read(proofPath)),build:proof.build,buildSha256:proof.buildSha256,executableSha256:proof.executableSha256,positions:tracks(input),batches:proof.batches,parameters:proof.parameters,wholeSnapshotExact:true});
  }
  assert.equal(results.length,3);assert.equal(new Set(results.map(r=>r.name)).size,3);assert.equal(new Set(results.map(r=>r.buildSha256)).size,1);assert.equal(new Set(results.map(r=>r.executableSha256)).size,1);
  const report={schema:1,passed:true,createdUtc:new Date().toISOString(),inputsSha256:hash(read(path.join(dir,'inputs.json'))),auditorSha256:hash(read(self)),results,scope:'Observed group enumeration under six changed position DWORDs; no Band event identity, audio or original Producer comparison',fullAcceptance:false};
  fs.writeFileSync(path.join(dir,'position-proof.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify({passed:true,proof:path.join(dir,'position-proof.json'),orders:results.map(r=>({name:r.name,all:r.batches.filter(b=>b.mask===0xffffffff&&[band,sequence].includes(b.type))}))}));
}else throw new Error('Use prepare DIR INPUT or audit DIR RUN1 RUN2 RUN3');

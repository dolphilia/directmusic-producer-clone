import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
const source=path.resolve(process.argv[2]),destination=path.resolve(process.argv[3]);
assert(!fs.existsSync(destination),'Use a fresh controls directory');
assert(!destination.startsWith(source+path.sep));
const read=p=>fs.readFileSync(p),json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,'')),hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const guiRoot=path.resolve(process.argv[4]);
const original=json(source+'/style-player-gui-audio-proof.json');assert(original.passed);
const retained=['output.wav','ready.json','capture.json','packets.csv','actions.json','run.json'].map(name=>({name,sha256:hash(source+'/'+name)}));
fs.mkdirSync(destination,{recursive:true});const results=[];
for(const name of ['unchanged','silent-output','stop-noise','wrong-tempo','clock-step']){
 const dir=path.join(destination,name);fs.cpSync(source,dir,{recursive:true});
 fs.renameSync(dir+'/style-player-gui-audio-proof.json',dir+'/source-proof-before-mutation.json');
 if(['silent-output','stop-noise','wrong-tempo'].includes(name)){
  const wav=read(dir+'/output.wav');let fmt,offset,bytes;
  for(let p=12;p<wav.length;){const n=wav.readUInt32LE(p+4),id=wav.toString('ascii',p,p+4);if(id==='fmt ')fmt=wav.subarray(p+8,p+8+n);if(id==='data'){offset=p+8;bytes=n;}p+=8+n+(n&1);}
  assert(fmt&&offset);const rate=fmt.readUInt32LE(4),channels=fmt.readUInt16LE(2),align=fmt.readUInt16LE(12);
  const put=(a,b)=>{for(let i=Math.round(a*rate);i<Math.round(b*rate);i++){const t=i/rate,gain=Math.max(0,Math.min(1,(t-a)/.005,(b-t)/.005));for(let c=0;c<channels;c++)wav.writeFloatLE(.02*gain*Math.sin(2*Math.PI*440*t),offset+i*align+c*4);}};
  if(name!=='stop-noise')wav.fill(0,offset,offset+bytes);
  if(name==='stop-noise')put(original.stop.after+1.3,original.restart.before-.3);
  if(name==='wrong-tempo')for(const phase of [{start:original.play,end:original.stop},{start:original.restart,end:original.finalStop}])for(let t=phase.start.after+.02;t<phase.end.before-.3;t+=.4)put(t,t+.18);
  fs.writeFileSync(dir+'/output.wav',wav);
 }
 if(name==='clock-step'){
  const capture=json(dir+'/capture.json');
  for(const k of ['endUtcBeforeFileTime','endUtcAfterFileTime'])capture[k]=(BigInt(capture[k])+1000000n).toString();
  fs.writeFileSync(dir+'/capture.json',JSON.stringify(capture,null,2)+'\n');
 }
 // Use the frozen auditor and its frozen clock module from this exact capture.
 const result=spawnSync(process.execPath,[dir+'/gui-audio-auditor.mjs',dir,guiRoot],{encoding:'utf8'});
 assert.equal(result.status,name==='unchanged'?0:1,result.stderr);
 fs.writeFileSync(dir+'/auditor.stdout.txt',result.stdout);fs.writeFileSync(dir+'/auditor.stderr.txt',result.stderr);
 let proof=null;
 if(name!=='clock-step'){
  proof=json(dir+'/style-player-gui-audio-proof.json');assert.equal(proof.passed,name==='unchanged');
  if(name==='wrong-tempo')assert(proof.runs.every(x=>!x.tempoPassed));
  if(name==='stop-noise')assert(proof.silence.find(x=>x.name==='stop-hold').rms>.0001);
  if(name==='silent-output')assert(proof.runs.every(x=>!x.pitchPassed&&x.onsets.length===0));
 }else assert.match(result.stderr,/drift|UTC/i,'Clock mutation must fail calibration');
 results.push({name,exitCode:result.status,accepted:name==='unchanged',derivedRecording:true,proofSha256:proof?hash(dir+'/style-player-gui-audio-proof.json'):null,stderrSha256:hash(dir+'/auditor.stderr.txt')});
}
for(const x of retained)assert.equal(hash(source+'/'+x.name),x.sha256);
fs.copyFileSync(new URL(import.meta.url),destination+'/test-auditor.mjs');
const report={schema:1,createdUtc:new Date().toISOString(),passed:true,scope:'Derived current GUI recording copies: unchanged, silence, Stop noise, wrong tempo, clock step; no additional product execution.',source,sourceProofSha256:hash(source+'/style-player-gui-audio-proof.json'),retained,results,testSha256:hash(new URL(import.meta.url))};
fs.writeFileSync(destination+'/controls.json',JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report,null,2));

import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';import assert from 'node:assert/strict';import {spawnSync} from 'node:child_process';
const source=path.resolve(process.argv[2]),destination=path.resolve(process.argv[3]);assert(!fs.existsSync(destination),'Use a fresh controls directory');assert(!destination.startsWith(source+path.sep));
const read=p=>fs.readFileSync(p),json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,'')),hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const original=json(path.join(source,'runtime-style-audio-proof.json'));assert(original.passed);const wavHash=hash(path.join(source,'output.wav'));
fs.mkdirSync(destination,{recursive:true});const results=[];
for(const name of ['unchanged','silent-output','stop-noise','wrong-tempo']){
 const dir=path.join(destination,name);fs.cpSync(source,dir,{recursive:true});
 if(name!=='unchanged'){
  const wav=read(path.join(dir,'output.wav'));let fmt,dataOffset,dataBytes;
  for(let p=12;p<wav.length;){const n=wav.readUInt32LE(p+4),id=wav.toString('ascii',p,p+4);if(id==='fmt ')fmt=wav.subarray(p+8,p+8+n);if(id==='data'){dataOffset=p+8;dataBytes=n;}p+=8+n+(n&1);}
  assert(fmt&&dataOffset);const rate=fmt.readUInt32LE(4),channels=fmt.readUInt16LE(2),align=fmt.readUInt16LE(12);
  if(name==='silent-output'||name==='wrong-tempo')wav.fill(0,dataOffset,dataOffset+dataBytes);
  const put=(a,b,amplitude)=>{for(let i=Math.round(a*rate);i<Math.round(b*rate);i++){const t=i/rate;const gain=Math.min(1,(t-a)/.005,(b-t)/.005);for(let c=0;c<channels;c++)wav.writeFloatLE(amplitude*Math.max(0,gain)*Math.sin(2*Math.PI*440*t),dataOffset+i*align+c*4);}};
  if(name==='stop-noise')put(original.phases['stop-return']+1.3,original.phases['restart-request']-.3,.02);
  if(name==='wrong-tempo')for(const p of [{ready:'play-ready',stop:'stop-request'},{ready:'restart-ready',stop:'final-stop-request'}]){
   const start=original.phases[p.ready]+.02;for(let t=start;t<original.phases[p.stop]-.3;t+=.5)put(t,t+.18,.02);
  }
  fs.writeFileSync(path.join(dir,'output.wav'),wav);
 }
 const result=spawnSync(process.execPath,['scripts/Inspect-RuntimeStyleAudio.mjs',dir],{encoding:'utf8'});assert.equal(result.status,name==='unchanged'?0:1,result.stderr);
 const proof=json(path.join(dir,'runtime-style-audio-proof.json'));assert.equal(proof.passed,name==='unchanged');assert.equal(json(path.join(dir,'lifecycle/lifecycle.json')).passed,true);
 if(name==='wrong-tempo')assert(proof.runs.every(r=>!r.tempoPassed),'Tempo mutation must fail the tempo check itself');
 if(name==='stop-noise')assert(proof.silence.find(p=>p.name==='stop-hold').rms>.0001);
 results.push({name,exitCode:result.status,passed:proof.passed,proofSha256:hash(path.join(dir,'runtime-style-audio-proof.json')),derivedRecording:true});
}
assert.equal(hash(path.join(source,'output.wav')),wavHash);
const report={schema:1,createdUtc:new Date().toISOString(),passed:true,scope:'Derived copies only; API success retained while silence/Stop noise/wrong tempo are rejected; no additional product run',sourceProofSha256:hash(path.join(source,'runtime-style-audio-proof.json')),results,testSha256:hash(new URL(import.meta.url))};fs.writeFileSync(path.join(destination,'controls.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report,null,2));

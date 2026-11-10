// Deliberately altered copies, never product or original evidence. Ensure API success cannot conceal absent/wrong audio.
import fs from 'node:fs';
import path from 'node:path';
import {spawnSync} from 'node:child_process';
import assert from 'node:assert/strict';
const source=path.resolve(process.argv[2]),root=path.resolve(process.argv[3]);assert(!root.toLowerCase().startsWith(source.toLowerCase()+path.sep),'Controls must be outside original evidence directory');assert(!fs.existsSync(root));fs.mkdirSync(root,{recursive:true});
const run=JSON.parse(fs.readFileSync(path.join(source,'run.json'),'utf8').replace(/^\uFEFF/,''));
assert(['notes','crud','variation-first','variation-last','motif-repeat','motif-standalone'].includes(run.profile??'notes'),'Use the dedicated tempo/lifecycle auditor for those profiles');
const originalProof=JSON.parse(fs.readFileSync(path.join(source,'audio-dls-proof.json'),'utf8'));
assert.equal(originalProof.passed,true,'A passing source recording is required');
const results=[];
for(const kind of ['unchanged-output','zero-output','wrong-middle-pitch','background-contamination']){
 const dir=path.join(root,kind);fs.cpSync(source,dir,{recursive:true});const file=path.join(dir,'output.wav'),wav=fs.readFileSync(file);let dataOffset;
 for(let p=12;p<wav.length;){const n=wav.readUInt32LE(p+4);if(wav.toString('ascii',p,p+4)==='data'){dataOffset=p+8;break;}p+=8+n+(n&1);}assert(dataOffset);
 if(kind==='zero-output')wav.fill(0,dataOffset);
 else if(kind==='wrong-middle-pitch' && run.profile==='notes'){const align=8,rate=48000;const a=dataOffset+Math.round(2.5*rate)*align,b=dataOffset+Math.round(4.5*rate)*align;wav.copy(wav,b,a,a+2*rate*align);}
 else if(kind==='wrong-middle-pitch' || kind==='background-contamination'){
  // Fixed 48kHz stereo float32 control, matching the verified source format.
  assert.equal(wav.readUInt32LE(24),48000);assert.equal(wav.readUInt16LE(22),2);assert.equal(wav.readUInt16LE(34),32);
  const start=kind==='background-contamination'?0.3:originalProof.onset+2;
  const end=kind==='background-contamination'?1.8:originalProof.onset+4;
  for(let frame=Math.round(start*48000);frame<Math.round(end*48000);frame++){
   const frequency=originalProof.expectedFrequency*2;
   const value=0.03*Math.sin(2*Math.PI*frequency*frame/48000);
   wav.writeFloatLE(value,dataOffset+frame*8);wav.writeFloatLE(value,dataOffset+frame*8+4);
  }
 }
 fs.writeFileSync(file,wav);const result=spawnSync(process.execPath,[path.resolve('scripts/Inspect-MotifDlsAudio.mjs'),dir],{encoding:'utf8'});
 fs.writeFileSync(path.join(dir,'auditor.stdout.txt'),result.stdout);fs.writeFileSync(path.join(dir,'auditor.stderr.txt'),result.stderr);
 const proof=JSON.parse(fs.readFileSync(path.join(dir,'audio-dls-proof.json'),'utf8'));const expectedPassed=kind==='unchanged-output';assert.equal(result.status,expectedPassed?0:1);assert.equal(proof.passed,expectedPassed);assert.equal(proof.notesPassed,true);results.push({kind,exitCode:result.status,expectedPassed,passed:proof.passed,rejected:!proof.passed,apiNotesStillPassed:proof.notesPassed,pitchPassed:proof.pitchPassed,baselineRms:proof.baselineRms});
}
fs.writeFileSync(path.join(root,'negative-tests.json'),JSON.stringify({source,scope:'Derived mutated copies, not recorded product runs',passed:true,results},null,2)+'\n');console.log(JSON.stringify(results));


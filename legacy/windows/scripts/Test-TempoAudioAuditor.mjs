import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
const source=path.resolve(process.argv[2]),root=path.resolve(process.argv[3]);assert(!root.toLowerCase().startsWith(source.toLowerCase()+path.sep));assert(!fs.existsSync(root));fs.mkdirSync(root,{recursive:true});
const original=JSON.parse(fs.readFileSync(path.join(source,'tempo-audio-proof.json'),'utf8'));assert(original.passed);const results=[];
for(const kind of ['missing-first-note','unchanged120-after-change']){
 const dir=path.join(root,kind);fs.cpSync(source,dir,{recursive:true});const p=path.join(dir,'output.wav'),wav=fs.readFileSync(p),saved=Buffer.from(wav);let data,rate,align;
 for(let at=12;at+8<wav.length;){const n=wav.readUInt32LE(at+4),id=wav.toString('ascii',at,at+4);if(id==='fmt '){rate=wav.readUInt32LE(at+12);align=wav.readUInt16LE(at+20);}if(id==='data')data=at+8;at+=8+n+(n&1);}assert(data&&rate&&align);
 const offset=t=>data+Math.round(t*rate)*align;
 if(kind==='missing-first-note')wav.fill(0,offset(original.onsets[0].time-.02),offset(original.onsets[1].time-.02));
 else{
  const start=original.onsets[4].time-.015;wav.fill(0,offset(start));
  for(let i=4;i<8;i++){const a=original.onsets[i].time-.015,b=i<7?original.onsets[i+1].time-.015:a+.32;const dest=start+(i-4)*.5;saved.copy(wav,offset(dest),offset(a),offset(b));}
 }
 fs.writeFileSync(p,wav);const execution=spawnSync(process.execPath,[path.resolve('scripts/Inspect-TempoAudio.mjs'),dir],{encoding:'utf8'});fs.writeFileSync(path.join(dir,'control.stdout.txt'),execution.stdout);fs.writeFileSync(path.join(dir,'control.stderr.txt'),execution.stderr);
 const proof=JSON.parse(fs.readFileSync(path.join(dir,'tempo-audio-proof.json'),'utf8'));assert.equal(execution.status,1);assert.equal(proof.passed,false);assert.equal(proof.intervalPassed,false);assert.equal(proof.notesPassed,true);results.push({kind,rejected:true,apiNotesPassed:proof.notesPassed,pitchPassed:proof.pitchPassed,onsetCount:proof.onsets.length,intervals:proof.intervals});
}
fs.writeFileSync(path.join(root,'negative-tests.json'),JSON.stringify({source,scope:'Derived WAVs only; native API success and document preserved',passed:true,results},null,2)+'\n');console.log(JSON.stringify(results));

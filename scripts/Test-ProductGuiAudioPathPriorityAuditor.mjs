import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
const source=path.resolve(process.argv[2]),root=path.resolve(process.argv[3]);
const json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
assert(!fs.existsSync(root));assert(!root.toLowerCase().startsWith(source.toLowerCase()+path.sep));
const original=json(source+'/gui-audiopath-priority-proof.json');assert(original.passed);assert.equal(original.completed.length,1);
fs.mkdirSync(root,{recursive:true});const results=[];
for(const kind of ['default-sounds','embedded-silent','missing-first-note','wrong-tempo']){
 const dir=path.join(root,kind);fs.cpSync(source,dir,{recursive:true});
 const p=dir+'/output.wav',wav=fs.readFileSync(p),saved=Buffer.from(wav);let data,rate,align;
 for(let at=12;at+8<wav.length;){const n=wav.readUInt32LE(at+4),id=wav.toString('ascii',at,at+4);if(id==='fmt '){rate=wav.readUInt32LE(at+12);align=wav.readUInt16LE(at+20);}if(id==='data')data=at+8;at+=8+n+(n&1);}
 assert(data&&rate&&align);const offset=t=>data+Math.round(t*rate)*align,play=original.completed[0],onsets=play.onsets;
 if(kind==='default-sounds')saved.copy(wav,offset(original.controlFrom+.2),offset(onsets[0].time),offset(onsets[0].time+.4));
 else if(kind==='embedded-silent')wav.fill(0,offset(play.time-.05),offset(onsets[7].time+2));
 else if(kind==='missing-first-note')wav.fill(0,offset(onsets[0].time-.02),offset(onsets[1].time-.02));
 else{const start=onsets[4].time-.015;wav.fill(0,offset(start),offset(onsets[7].time+4));for(let i=4;i<8;i++){const a=onsets[i].time-.015,b=i<7?onsets[i+1].time-.015:a+1.5;saved.copy(wav,offset(start+(i-4)*3),offset(a),offset(b));}}
 fs.writeFileSync(p,wav);
 const execution=spawnSync(process.execPath,[path.resolve('scripts/Inspect-ProductGuiAudioPathPriority.mjs'),dir],{encoding:'utf8'});
 fs.writeFileSync(dir+'/control.stdout.txt',execution.stdout);fs.writeFileSync(dir+'/control.stderr.txt',execution.stderr);
 const proof=json(dir+'/gui-audiopath-priority-proof.json');assert.equal(execution.status,1);assert.equal(proof.passed,false);
 if(kind==='default-sounds')assert.equal(proof.controlQuiet,false);else assert(proof.completed.every(p=>!p.tempoPassed));
 results.push({kind,rejected:true,controlQuiet:proof.controlQuiet,completed:proof.completed.map(p=>({tempoPassed:p.tempoPassed,pitchPassed:p.pitchPassed,onsetCount:p.onsets.length}))});
}
fs.writeFileSync(root+'/negative-tests.json',JSON.stringify({source,passed:true,scope:'Derived GUI PCM rejects default sound, embedded silence, omitted note and wrong tempo; source/build/input/actions unchanged',results},null,2)+'\n');
console.log(JSON.stringify(results));

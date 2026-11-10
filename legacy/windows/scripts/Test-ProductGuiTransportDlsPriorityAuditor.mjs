import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
const source=path.resolve(process.argv[2]),root=path.resolve(process.argv[3]);
const json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
assert(!fs.existsSync(root));assert(!root.toLowerCase().startsWith(source.toLowerCase()+path.sep));
const original=json(source+'/gui-transport-dls-priority-proof.json');assert(original.passed);assert.equal(original.completed.length,1);
fs.mkdirSync(root,{recursive:true});const results=[];const flags=['--slow',...(original.lifecycleRequired?['--require-lifecycle']:[])];
const unchanged=path.join(root,'unchanged');fs.cpSync(source,unchanged,{recursive:true});const unchangedExecution=spawnSync(process.execPath,[path.resolve('scripts/Inspect-ProductGuiTransportDlsPriorityAudio.mjs'),unchanged,...flags],{encoding:'utf8'});fs.writeFileSync(unchanged+'/control.stdout.txt',unchangedExecution.stdout);fs.writeFileSync(unchanged+'/control.stderr.txt',unchangedExecution.stderr);assert.equal(unchangedExecution.status,0);assert(json(unchanged+'/gui-transport-dls-priority-proof.json').passed);results.push({kind:'unchanged',accepted:true});
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
 const execution=spawnSync(process.execPath,[path.resolve('scripts/Inspect-ProductGuiTransportDlsPriorityAudio.mjs'),dir,...flags],{encoding:'utf8'});
 fs.writeFileSync(dir+'/control.stdout.txt',execution.stdout);fs.writeFileSync(dir+'/control.stderr.txt',execution.stderr);
 const proof=json(dir+'/gui-transport-dls-priority-proof.json');assert.equal(execution.status,1);assert.equal(proof.passed,false);
 if(kind==='default-sounds')assert.equal(proof.controlQuiet,false);else assert(proof.completed.every(p=>!p.tempoPassed));
 results.push({kind,rejected:true,controlQuiet:proof.controlQuiet,completed:proof.completed.map(p=>({tempoPassed:p.tempoPassed,pitchPassed:p.pitchPassed,onsetCount:p.onsets.length}))});
}
if(original.lifecycleRequired){for(const kind of ['unchanged-first-tempo','sound-after-stop','no-restart','missing-sustain','wrong-sustain-pitch','extra-attack']){
 const dir=path.join(root,kind);fs.cpSync(source,dir,{recursive:true});
 const file=dir+'/output.wav',wav=fs.readFileSync(file),saved=Buffer.from(wav);let data,rate,align;
 for(let p=12;p+8<wav.length;){const n=wav.readUInt32LE(p+4),id=wav.toString('ascii',p,p+4);if(id==='fmt '){rate=wav.readUInt32LE(p+12);align=wav.readUInt16LE(p+20);}if(id==='data')data=p+8;p+=8+n+(n&1);}
 assert(data&&rate&&align);const offset=t=>data+Math.round(t*rate)*align;
 for(const play of original.completed){const onsets=play.onsets;
  if(kind==='missing-sustain')for(const onset of onsets)wav.fill(0,offset(onset.time+1.5),offset(onset.time+2.5));
  if(kind==='wrong-sustain-pitch')for(const onset of onsets)for(let p=offset(onset.time+1.5);p<offset(onset.time+2.5);p+=align)for(let c=0;c<align/4;c++)wav.writeFloatLE(.04*Math.sin(2*Math.PI*880*(p-data)/align/rate),p+c*4);
  if(kind==='extra-attack'){const from=onsets[0].time+.1,to=onsets[0].time+8;saved.copy(wav,offset(to),offset(from),offset(from+1));}
  if(kind==='missing-first-note')wav.fill(0,offset(onsets[0].time-.02),offset(onsets[1].time-.02));
  if(kind==='no-restart')wav.fill(0,offset(onsets[0].time-.02),offset(onsets.at(-1).time+5));
  if(kind==='unchanged-first-tempo'){
   const start=onsets[4].time-.015,end=onsets[7].time+5;wav.fill(0,offset(start),offset(end));
   for(let i=4;i<8;i++){const a=onsets[i].time-.015,b=i<7?onsets[i+1].time-.015:a+4,dest=start+(i-4)*12;saved.copy(wav,offset(dest),offset(a),offset(b));}
  }
 }
 if(kind==='sound-after-stop'){
  const from=original.completed[0].onsets[0].time+.1,to=original.stops.find(s=>s.action==='Stop interrupted').time+3;
  saved.copy(wav,offset(to),offset(from),offset(from+1));
 }
 fs.writeFileSync(file,wav);
 const tempoMode=original.tempos[0].bpm===5?['--slow']:[];
 const execution=spawnSync(process.execPath,[path.resolve('scripts/Inspect-ProductGuiTransportDlsPriorityAudio.mjs'),dir,...flags],{encoding:'utf8'});
 fs.writeFileSync(dir+'/control.stdout.txt',execution.stdout);fs.writeFileSync(dir+'/control.stderr.txt',execution.stderr);
 const proof=json(dir+'/gui-transport-dls-priority-proof.json');assert.equal(execution.status,kind==='unchanged'?0:1);assert.equal(proof.passed,kind==='unchanged');
 if(kind==='sound-after-stop')assert.equal(proof.stopResumePassed,false);
 results.push({kind,rejected:kind!=='unchanged',sustainPassed:proof.sustainPassed,tempoPitchPassed:proof.completed.every(p=>p.tempoPassed&&p.pitchPassed),stopResumePassed:proof.stopResumePassed,quietRms:proof.quietRms});
}
}
fs.writeFileSync(root+'/negative-tests.json',JSON.stringify({source,passed:true,scope:'Derived GUI PCM rejects default sound, embedded silence, omitted note and wrong tempo; lifecycle mode additionally rejects sound after Stop, absent restart, missing/wrong sustain, extra attack and unchanged tempo; source/build/input/actions unchanged',results},null,2)+'\n');
console.log(JSON.stringify(results));

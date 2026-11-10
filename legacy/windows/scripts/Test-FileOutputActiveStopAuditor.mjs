import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';import assert from 'node:assert/strict';import {spawnSync} from 'node:child_process';
const [audioArg,unitArg,rootArg]=process.argv.slice(2);assert(rootArg,'Usage: Test-FileOutputActiveStopAuditor.mjs AUDIO UNIT NEW_CONTROLS');
const source=path.resolve(audioArg),unit=path.resolve(unitArg),root=path.resolve(rootArg),auditor=path.resolve('scripts/Inspect-FileOutputActiveStop.mjs');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,'')),write=(p,v)=>fs.writeFileSync(p,JSON.stringify(v,null,2)+'\n'),hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
assert(!fs.existsSync(root));assert(!root.toLowerCase().startsWith(source.toLowerCase()+path.sep));const proof=read(source+'/file-output-active-stop-proof.json');assert(proof.passed&&!proof.fullAcceptance);fs.mkdirSync(root,{recursive:true});const results=[];
for(const kind of ['unchanged','silent-after-record-stop','one-pitch-after-record-stop','gap-at-record-stop','sound-after-stop','missing-replay','late-record-stop','file-growth-after-stop','wrong-clock','timestamp-error','same-author-pid','bad-action-timestamp']){
 const dir=root+'/'+kind;fs.mkdirSync(dir);for(const name of ['run.json','ready.json','capture.json','gui-launch-at-capture.json','driver.ps1','endpoint.txt','packets.csv'])fs.copyFileSync(source+'/'+name,dir+'/'+name);
 const run=read(dir+'/run.json'),gui=run.guiRun,testedUnit=dir+'/unit',testedGui=dir+'/gui';fs.mkdirSync(testedUnit);fs.mkdirSync(testedGui);
 for(const name of ['saved-inputs.json','author-exit.json','recovery-states.jsonl'])fs.copyFileSync(unit+'/'+name,testedUnit+'/'+name);
 for(const name of ['exit-observer.json','actions.json','recording-final-before.json','recording-final.json','recording-after-replay.json'])fs.copyFileSync(gui+'/'+name,testedGui+'/'+name);
 fs.mkdirSync(testedGui+'/recordings');fs.copyFileSync(gui+'/recordings/file-output-multi-pcm-proof.json',testedGui+'/recordings/file-output-multi-pcm-proof.json');
 run.guiRun=testedGui;write(dir+'/run.json',run);
 const wav=fs.readFileSync(source+'/output.wav');let data,rate,align;for(let p=12;p+8<wav.length;){const id=wav.toString('ascii',p,p+4),n=wav.readUInt32LE(p+4);if(id==='fmt '){rate=wav.readUInt32LE(p+12);align=wav.readUInt16LE(p+20);}if(id==='data')data=p+8;p+=8+n+(n&1);}assert(data&&rate&&align);const at=t=>data+Math.round(t*rate)*align;
 if(kind==='silent-after-record-stop')wav.fill(0,at(proof.recordingStop+.02),at(proof.plays[1].to-.2));
 if(kind==='gap-at-record-stop')wav.fill(0,at(proof.recordingStop+.6),at(proof.recordingStop+.64));
 if(kind==='one-pitch-after-record-stop')for(let i=at(proof.recordingStop+.1);i<at(proof.recordingStop+.4);i+=align)for(let c=0;c<align/4;c++)wav.writeFloatLE(.046*Math.sin(2*Math.PI*261.6255653005986*(i-data)/align/rate),i+c*4);
 if(kind==='sound-after-stop'){const p=proof.plays[0];Buffer.from(wav.subarray(at(p.onset+.15),at(p.onset+.4))).copy(wav,at(p.to+1.1));}
 if(kind==='missing-replay'){const p=proof.plays[2];wav.fill(0,at(p.from),at(p.to));}fs.writeFileSync(dir+'/output.wav',wav);
 if(kind==='late-record-stop'||kind==='bad-action-timestamp'){const a=read(testedGui+'/actions.json'),event=a.find(a=>a.name==='record-stop');if(kind==='late-record-stop'){event.beforeUtc=new Date(Date.parse(run.captureStartUtc)+(proof.plays[1].to+1)*1000).toISOString();event.utc=event.beforeUtc;}else event.utc=new Date(Date.parse(event.beforeUtc)+600).toISOString();write(testedGui+'/actions.json',a);}
 if(kind==='file-growth-after-stop'){const f=read(testedGui+'/recording-after-replay.json');f.files[0].bytes+=88;write(testedGui+'/recording-after-replay.json',f);}
 if(kind==='wrong-clock'){run.captureStartUtc=new Date(Date.parse(run.captureStartUtc)+10000).toISOString();write(dir+'/run.json',run);}
 if(kind==='timestamp-error'){const rows=fs.readFileSync(dir+'/packets.csv','utf8').trimEnd().split(/\r?\n/),r=rows[3].split(',');r[2]=String(Number(r[2])|4);rows[3]=r.join(',');fs.writeFileSync(dir+'/packets.csv',rows.join('\n')+'\n');}
 if(kind==='same-author-pid'){const a=read(testedUnit+'/author-exit.json');a.processId=proof.processId;write(testedUnit+'/author-exit.json',a);}
 const r=spawnSync(process.execPath,[auditor,dir,testedUnit],{encoding:'utf8'});fs.writeFileSync(dir+'/stdout.txt',r.stdout);fs.writeFileSync(dir+'/stderr.txt',r.stderr);assert.equal(r.status,kind==='unchanged'?0:1,kind);
 results.push({kind,accepted:r.status===0,rejected:r.status===1,exitCode:r.status,stderrSha256:hash(dir+'/stderr.txt')});
}
const result={schema:1,passed:true,candidate:proof.candidate,sourceCapture:source,sourceUnit:unit,sourceProofSha256:hash(source+'/file-output-active-stop-proof.json'),auditorSha256:hash(auditor),driverSha256:hash(process.argv[1]),results,fullAcceptance:false};write(root+'/negative-tests.json',result);fs.copyFileSync(process.argv[1],root+'/driver.mjs');console.log(JSON.stringify(result));

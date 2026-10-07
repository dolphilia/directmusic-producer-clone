import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';import assert from 'node:assert/strict';import {spawnSync} from 'node:child_process';
const [sourceArg,unitArg,rootArg]=process.argv.slice(2);assert(rootArg,'Usage: Test-FileOutputMultiGuiAudioAuditor.mjs AUDIO UNIT NEW_CONTROLS');
const source=path.resolve(sourceArg),unit=path.resolve(unitArg),root=path.resolve(rootArg),auditor=path.resolve('scripts/Inspect-FileOutputMultiGuiAudio.mjs');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,'')),write=(p,v)=>fs.writeFileSync(p,JSON.stringify(v,null,2)+'\n'),hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
assert(!fs.existsSync(root));assert(!root.toLowerCase().startsWith(source.toLowerCase()+path.sep));const original=read(source+'/file-output-multi-gui-audio-proof.json');assert(original.passed);fs.mkdirSync(root,{recursive:true});const results=[];
for(const kind of ['unchanged','silence','one-mix-group-only','missing-replay','sound-after-stop','wrong-capture-clock','timestamp-error','same-author-pid']){
 const dir=root+'/'+kind;fs.cpSync(source,dir,{recursive:true});const wav=fs.readFileSync(dir+'/output.wav'),saved=Buffer.from(wav);let data,rate,align;
 for(let p=12;p+8<wav.length;){const id=wav.toString('ascii',p,p+4),n=wav.readUInt32LE(p+4);if(id==='fmt '){rate=wav.readUInt32LE(p+12);align=wav.readUInt16LE(p+20);}if(id==='data')data=p+8;p+=8+n+(n&1);}assert(data&&rate&&align);const at=t=>data+Math.round(t*rate)*align;
 if(kind==='silence')wav.fill(0,data);
 if(kind==='one-mix-group-only')for(const p of original.plays)for(let i=at(p.onset+.15);i<at(p.onset+.45);i+=align)for(let c=0;c<align/4;c++)wav.writeFloatLE(.046*Math.sin(2*Math.PI*(440*2**((60-69)/12))*(i-data)/align/rate),i+c*4);
 if(kind==='missing-replay'){const p=original.plays[1];wav.fill(0,at(p.from),at(p.to));}
 if(kind==='sound-after-stop'){const p=original.plays[0];saved.copy(wav,at(p.to+1.1),at(p.onset+.15),at(p.onset+.4));}
 fs.writeFileSync(dir+'/output.wav',wav);
 if(kind==='wrong-capture-clock'){const r=read(dir+'/run.json');r.captureStartUtc=new Date(Date.parse(r.captureStartUtc)+10000).toISOString();write(dir+'/run.json',r);}
 if(kind==='timestamp-error'){const lines=fs.readFileSync(dir+'/packets.csv','utf8').trimEnd().split(/\r?\n/),row=lines[3].split(',');row[2]=String(Number(row[2])|4);lines[3]=row.join(',');fs.writeFileSync(dir+'/packets.csv',lines.join('\n')+'\n');}
 let testedUnit=unit;
 if(kind==='same-author-pid'){testedUnit=dir+'/unit';fs.mkdirSync(testedUnit);for(const f of ['author-exit-observer.json','final-inputs.json'])fs.copyFileSync(unit+'/'+f,testedUnit+'/'+f);fs.mkdirSync(testedUnit+'/reload-session');fs.copyFileSync(unit+'/reload-session/exit-observer.json',testedUnit+'/reload-session/exit-observer.json');const a=read(testedUnit+'/author-exit-observer.json');a.processId=original.processId;write(testedUnit+'/author-exit-observer.json',a);}
 const execution=spawnSync(process.execPath,[auditor,dir,testedUnit],{encoding:'utf8'});fs.writeFileSync(dir+'/control.stdout.txt',execution.stdout);fs.writeFileSync(dir+'/control.stderr.txt',execution.stderr);assert.equal(execution.status,kind==='unchanged'?0:1,kind);
 if(kind==='unchanged'){const p=read(dir+'/file-output-multi-gui-audio-proof.json');assert(p.passed&&!p.fullAcceptance);assert.equal(p.activeGuiStopsPassed,original.activeGuiStopsPassed);assert.equal(p.recordingStopWhileAudibleEstablished,original.recordingStopWhileAudibleEstablished);}
 results.push({kind,accepted:execution.status===0,rejected:execution.status===1,exitCode:execution.status,stderrSha256:hash(dir+'/control.stderr.txt')});
}
const result={schema:1,passed:true,candidate:original.candidate,sourceCapture:source,sourceUnit:unit,sourceProofSha256:hash(source+'/file-output-multi-gui-audio-proof.json'),auditorSha256:hash(auditor),driverSha256:hash(process.argv[1]),results,fullAcceptance:false};write(root+'/negative-tests.json',result);fs.copyFileSync(process.argv[1],root+'/driver.mjs');console.log(JSON.stringify(result));

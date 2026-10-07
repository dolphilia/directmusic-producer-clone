import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
const [audioArg,unitArg]=process.argv.slice(2);
assert(unitArg,'Usage: Test-SourceToolAudioAuditControls.mjs AUDIO UNIT');
const audio=path.resolve(audioArg),out=path.resolve(unitArg,'audio-audit-controls');
assert(!fs.existsSync(out),'Preserve prior controls; use a fresh unit directory');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const proof=read(audio+'/source-tool-gui-audio-proof.json');assert(proof.passed);
const run=read(audio+'/run.json'),actions=read(run.guiRun+'/actions.json');
const wav=fs.readFileSync(audio+'/output.wav');let offset,format;
for(let p=12;p<wav.length;){const n=wav.readUInt32LE(p+4),id=wav.toString('ascii',p,p+4);if(id==='fmt ')format=wav.subarray(p+8,p+8+n);if(id==='data')offset=p+8;p+=8+n+(n&1);}
assert(offset&&format);const rate=format.readUInt32LE(4),align=format.readUInt16LE(12);
const at=t=>offset+Math.round(t*rate)*align;
const controlFirst=proof.control.onsets[0].time,authFirst=proof.authored.onsets[0].time;
fs.mkdirSync(out,{recursive:true});const results=[];
const auditor=path.resolve('scripts/Inspect-SourceToolGuiAudio.mjs');
function execute(name,modify){
 const dir=path.join(out,name);fs.mkdirSync(dir);
 for(const f of ['capture.json','ready.json','packets.csv'])fs.copyFileSync(audio+'/'+f,dir+'/'+f);
 const b=Buffer.from(wav);modify(b,dir);fs.writeFileSync(dir+'/output.wav',b);
 fs.writeFileSync(dir+'/actions.json',JSON.stringify(actions,null,2));
 fs.writeFileSync(dir+'/run.json',JSON.stringify({...run,guiRun:dir,derivedControl:name,fullAcceptance:false},null,2));
 const r=spawnSync(process.execPath,[auditor,dir],{encoding:'utf8'});
 fs.writeFileSync(dir+'/stdout.txt',r.stdout??'');fs.writeFileSync(dir+'/stderr.txt',r.stderr??'');
 const rejected=r.status!==0;results.push({name,exitCode:r.status,rejected,wavSha256:hash(dir+'/output.wav'),packetsSha256:hash(dir+'/packets.csv')});
 assert(rejected,'Auditor accepted invalid control '+name);
}
execute('silent',b=>b.fill(0,offset));
execute('missing-authored-note',b=>b.fill(0,at(proof.authored.onsets[1].time-.02),at(proof.authored.onsets[2].time-.02)));
execute('unchanged-gain',b=>{
 const span=3.9,source=wav.subarray(at(controlFirst-.03),at(controlFirst-.03+span));
 b.fill(0,at(authFirst-.03),at(authFirst-.03+span));source.copy(b,at(authFirst-.03));
});
execute('swapped-pitches',b=>{
 const a=at(authFirst-.015),c=at(proof.authored.onsets[1].time-.015),length=Math.round(.24*rate)*align;
 const first=Buffer.from(wav.subarray(a,a+length)),second=Buffer.from(wav.subarray(c,c+length));
 second.copy(b,a);first.copy(b,c);
});
execute('wrong-tempo-preserved-pitch',b=>{
 b.fill(0,at(authFirst-.03),at(authFirst+5));
 for(const p of proof.authored.onsets){
  const note=wav.subarray(at(p.time-.015),at(p.time+.145));
  note.copy(b,at(authFirst+(p.time-authFirst)*1.2-.015));
 }
});
execute('packet-gap',(_b,dir)=>{
 const rows=fs.readFileSync(dir+'/packets.csv','utf8').trim().split(/\r?\n/),cells=rows[2].split(',');
 cells[0]=String(Number(cells[0])+240);rows[2]=cells.join(',');fs.writeFileSync(dir+'/packets.csv',rows.join('\n')+'\n');
});
const result={schema:1,passed:results.every(r=>r.rejected),positiveProof:audio+'/source-tool-gui-audio-proof.json',positiveProofSha256:hash(audio+'/source-tool-gui-audio-proof.json'),auditorSha256:hash(auditor),scriptSha256:hash(process.argv[1]),results,scope:'Derived PCM/packet negative controls; no additional GUI playback or original parity',fullAcceptance:false};
fs.writeFileSync(out+'/proof.json',JSON.stringify(result,null,2)+'\n');
fs.copyFileSync(process.argv[1],out+'/control-driver.mjs');
console.log(JSON.stringify(result));

import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';

const [sourceArg,rootArg,mode]=process.argv.slice(2);
assert(['native','runtime'].includes(mode),'Usage: CAPTURE NEW_CONTROL_DIRECTORY native|runtime');
const source=path.resolve(sourceArg),root=path.resolve(rootArg);
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const write=(p,v)=>fs.writeFileSync(p,JSON.stringify(v,null,2)+'\n');
assert(!fs.existsSync(root));assert(!root.toLowerCase().startsWith(source.toLowerCase()+path.sep));
const original=read(source+'/dls-runtime-audio-proof.json');assert(original.passed&&original.mode===mode);
assert.equal(original.analyzerSha256,hash('scripts/Inspect-DlsRuntimeAudio.mjs'));
fs.mkdirSync(root,{recursive:true});const results=[];
const kinds=['unchanged','missing-sustain','sound-after-stop','missing-restart','wrong-pitch','competing-active-tone','missing-stop','late-stop','wrong-pid','wrong-window','same-author-pid','nonzero-exit','forced-exit','build-hash','input-hash','packet-gap','packet-error','wrong-sample-loop','wrong-tempo'];
for(const kind of kinds){
 const dir=path.join(root,kind);fs.cpSync(source,dir,{recursive:true});
 const run=read(dir+'/run.json'),launch=read(dir+'/gui-launch-at-capture.json');
 const gui=dir+'/gui';fs.mkdirSync(gui);const actions=read(run.guiRun+'/actions.json');
 const exit=read(run.guiRun+'/exit.json');write(gui+'/exit.json',exit);
 write(gui+'/actions.json',actions);run.guiRun=gui;write(dir+'/run.json',run);
 const wav=fs.readFileSync(dir+'/output.wav'),saved=Buffer.from(wav);let data,rate,align;
 for(let p=12;p+8<wav.length;){const n=wav.readUInt32LE(p+4),id=wav.toString('ascii',p,p+4);if(id==='fmt '){rate=wav.readUInt32LE(p+12);align=wav.readUInt16LE(p+20);}if(id==='data')data=p+8;p+=8+n+(n&1);}
 assert(data&&rate&&align);const at=t=>data+Math.round(t*rate)*align;
 if(kind==='missing-sustain')for(const phase of original.phases)for(const a of phase.sound)wav.fill(0,at(a.time+1.5),at(a.time+2.5));
 if(kind==='sound-after-stop'){const from=original.phases[0].sound[0].time+1.5,to=original.phases[0].to+2.1;saved.copy(wav,at(to),at(from),at(from+.5));}
 if(kind==='missing-restart'){const p=original.phases[1];wav.fill(0,at(p.from),at(p.to));}
 if(kind==='wrong-pitch'||kind==='competing-active-tone')for(const phase of original.phases)for(const a of phase.sound)for(let p=at(a.time+1.5);p<at(a.time+2.5);p+=align)for(let c=0;c<align/4;c++){
  const extra=.02*Math.sin(2*Math.PI*880*(p-data)/align/rate);wav.writeFloatLE(kind==='wrong-pitch'?extra:wav.readFloatLE(p+c*4)+extra,p+c*4);
 }
 fs.writeFileSync(dir+'/output.wav',wav);
 if(kind==='missing-stop')write(gui+'/actions.json',actions.filter(a=>a.name!=='stop-1'));
 if(kind==='late-stop'){const stop=actions.find(a=>a.name==='stop-1'),play=actions.find(a=>a.name==='play-1');stop.beforeUtc=new Date(Date.parse(play.beforeUtc)+25000).toISOString();stop.utc=new Date(Date.parse(stop.beforeUtc)+20).toISOString();write(gui+'/actions.json',actions);}
 if(kind==='wrong-pid'){actions[0].processId++;write(gui+'/actions.json',actions);}
 if(kind==='wrong-window'){actions[0].window.id++;write(gui+'/actions.json',actions);}
 if(kind==='same-author-pid'){const author=read(launch.authorExit.path);author.processId=run.processId;write(dir+'/author-exit.json',author);launch.authorExit={path:dir+'/author-exit.json',sha256:hash(dir+'/author-exit.json')};write(dir+'/gui-launch-at-capture.json',launch);}
 if(kind==='nonzero-exit'||kind==='forced-exit'){if(kind==='nonzero-exit')exit.exitCode=1;else exit.forcedTermination=true;write(gui+'/exit.json',exit);}
 if(kind==='build-hash'){run.buildSummarySha256='0'.repeat(64);write(dir+'/run.json',run);}
 if(kind==='input-hash'){run.inputs[0].sha256='0'.repeat(64);write(dir+'/run.json',run);}
 if(kind==='packet-gap'||kind==='packet-error'){const rows=fs.readFileSync(dir+'/packets.csv','utf8').trim().split(/\r?\n/);const row=rows[3].split(',');if(kind==='packet-gap')row[0]=String(Number(row[0])+rate*.1);else row[2]=String(Number(row[2])|4);rows[3]=row.join(',');fs.writeFileSync(dir+'/packets.csv',rows.join('\n')+'\n');}
 if(kind==='wrong-sample-loop'||kind==='wrong-tempo'){
  const name=kind==='wrong-sample-loop'?'LoopSource.dls':mode==='native'?'LoopSong.sgp':'LoopSong.sgt';
  const input=run.inputs.find(i=>path.basename(i.path)===name),b=fs.readFileSync(input.path);
  // Copy the input and update both identities so these controls exercise the
  // RIFF contract, rather than merely failing a content hash.
  const id=kind==='wrong-sample-loop'?'wsmp':'tetr';const offsets=[];
  function findChunk(a,z){for(let p=a;p<z;){assert(p+8<=z);const tag=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),e=p+8+n;assert(e+(n&1)<=z);if(tag===id)offsets.push(p);if(tag==='RIFF'||tag==='LIST'||tag==='seqt')findChunk(p+8+(tag==='seqt'?0:4),e);p=e+(n&1);}}
  findChunk(0,b.length);assert(offsets.length);const pos=offsets[0];
  if(kind==='wrong-sample-loop')b.writeUInt32LE(11986,pos+8+b.readUInt32LE(pos+8)+12);else b.writeDoubleLE(24,pos+20);
  const file=dir+'/'+name;fs.writeFileSync(file,b);for(const list of [run.inputs,launch.inputs])for(const i of list)if(path.basename(i.path)===name){i.path=file;i.sha256=hash(file);}
  write(dir+'/run.json',run);write(dir+'/gui-launch-at-capture.json',launch);
 }
 const args=[path.resolve('scripts/Inspect-DlsRuntimeAudio.mjs'),dir,mode];
 const execution=spawnSync(process.execPath,args,{encoding:'utf8'});
 fs.writeFileSync(dir+'/control.stdout.txt',execution.stdout);fs.writeFileSync(dir+'/control.stderr.txt',execution.stderr);
 assert.equal(execution.status,kind==='unchanged'?0:1,kind);
 if(kind==='unchanged')assert(read(dir+'/dls-runtime-audio-proof.json').passed);
 results.push({kind,accepted:execution.status===0,rejected:execution.status===1,exitCode:execution.status,command:[process.execPath,...args],stderrSha256:hash(dir+'/control.stderr.txt')});
}
const result={schema:1,passed:true,candidate:original.candidate,mode,sourceCapture:source,sourceProofSha256:hash(source+'/dls-runtime-audio-proof.json'),auditorSha256:hash('scripts/Inspect-DlsRuntimeAudio.mjs'),driverSha256:hash(process.argv[1]),results,fullAcceptance:false};
write(root+'/negative-tests.json',result);fs.copyFileSync(process.argv[1],root+'/driver.mjs');console.log(JSON.stringify(result));

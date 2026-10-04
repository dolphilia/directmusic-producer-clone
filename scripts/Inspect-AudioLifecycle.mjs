import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
const dir=path.resolve(process.argv[2]),file=n=>path.join(dir,n);
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const run=json(file('run.json')),capture=json(file('capture.json')),native=json(file('lifecycle/lifecycle.json'));
assert(['lifecycle','motif-lifecycle'].includes(run.profile));const motif=run.profile==='motif-lifecycle';assert.equal(hash(run.producer),run.producerSha256);assert.equal(hash(run.buildSummary),run.buildSummarySha256);assert.equal(hash(run.recorder),run.recorderSha256);assert.equal(hash(run.recorderBuildSummary),run.recorderBuildSummarySha256);
const build=json(run.buildSummary);assert(build.passed&&build.sourceSnapshotUnchanged);for(const s of build.sources)assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);
if(motif){assert(native.motif);assert.equal(native.calls.filter(c=>c.operation==='Get owned Motif'&&c.hresult===0).length,2);assert(!native.calls.some(c=>c.operation==='Load current DMSG'));for(const id of [1,2]){const notes=native.notes.filter(n=>n.run===id).sort((a,b)=>a.clocks-b.clocks);assert(notes.length>=4);for(const n of notes){assert.equal(n.midiValue,72);assert.equal(n.duration,384);assert.equal(n.velocity,96);assert.equal(n.channel,5);}assert.equal(notes[0].clocks,notes[0].start);for(let i=1;i<notes.length;i++)assert.equal(notes[i].clocks-notes[i-1].clocks,768);}}
for(const p of run.inputs)assert.equal(hash(p.path),p.sha256);
const dlsMode=process.argv[3]==='--dls';let measuredFrequency=null;
if(dlsMode){
 assert(!motif);const input=run.inputs.find(i=>path.basename(i.path)==='owned.dls');assert(input,'Explicit recorded DLS input required');
 assert.equal(hash(file('driver.ps1')),run.driverSha256);assert.equal(hash(run.preparation),run.preparationSha256);const prep=json(run.preparation);assert(prep.passed&&prep.inputUnchanged&&prep.buildSha256===run.buildSummarySha256);for(const f of prep.outputs)assert.equal(hash(f.path),f.sha256);
 const project=run.inputs.find(i=>path.basename(i.path)==='AuthoredDls.pro'),segment=run.inputs.find(i=>path.basename(i.path)==='Authored.sgp');assert(project&&segment);assert.equal(hash(file('lifecycle/input-project.pro')),project.sha256);assert.equal(hash(file('lifecycle/input.sgp')),segment.sha256);assert.equal(hash(file('lifecycle/runtime.sgp')),segment.sha256);
 for(const n of ['source-collection-0.dls','runtime-collection-0.dls','restart-collection-0.dls'])assert.equal(hash(file('lifecycle/'+n)),input.sha256,'Owned/restart runtime DLS bytes differ');
 const result=spawnSync(process.execPath,['scripts/Inspect-DlsSamplePitch.mjs',input.path,file('dls-sample.json'),'0','60','96'],{encoding:'utf8'});assert.equal(result.status,0,result.stderr);const sample=json(file('dls-sample.json'));assert.equal(sample.unityNote,60);assert.equal(sample.fineTuneCents,0);assert(sample.dominant.energy>.001);measuredFrequency=sample.dominant.frequency;assert(Math.abs(measuredFrequency-440)<2,'Known authored PCM must be 440 Hz');
 for(const id of [1,2]){const notes=native.notes.filter(n=>n.run===id);assert(notes.length>=4);for(const n of notes){assert.equal(n.midiValue,60);assert(n.duration>0&&n.duration<=384);assert.equal(n.velocity,96);assert.equal(n.channel,0);}}
}
if(motif){
 function chunks(b,a=0,z=b.length){const out=[];for(let p=a;p<z;){assert(p+8<=z);const n=b.readUInt32LE(p+4),e=p+8+n;assert(e<=z);const id=b.toString('ascii',p,p+4),box=id==='RIFF'||id==='LIST';out.push({id,type:box?b.toString('ascii',p+8,p+12):'',data:b.subarray(p+8,e),children:box?chunks(b,p+12,e):[]});p=e+(n&1);assert(p<=z);}return out;}
 const one=(ns,f)=>{const x=ns.filter(f);assert.equal(x.length,1);return x[0];},item=(ns,id)=>one(ns,n=>n.id===id);
 const style=run.inputs.find(i=>path.basename(i.path)==='Heartlnd.stp');assert(style);const bytes=fs.readFileSync(style.path);assert(bytes.equals(fs.readFileSync(file('lifecycle/source-style-0.stp'))));
 const root=one(chunks(bytes),n=>n.type==='DMST'),pt=one(root.children,n=>n.type==='pttn'&&(item(n.children,'ptnh').data.readUInt16LE(6)&16)!==0);
 assert.equal(item(one(pt.children,n=>n.type==='UNFO').children,'UNAM').data.toString('utf16le').replace(/\0+$/,''),'Owned Motif');
 const mtfs=item(pt.children,'mtfs').data;assert.deepEqual(Array.from({length:5},(_,i)=>mtfs.readUInt32LE(i*4)),[63,0,768,2304,1]);
 const ref=item(one(pt.children,n=>n.type==='pref').children,'prfc').data,part=one(root.children,n=>n.type==='part'&&item(n.children,'prth').data.subarray(132,148).equals(ref.subarray(0,16))),notes=item(part.children,'note').data;
 assert.equal(ref.readUInt32LE(24),5);assert.equal(notes.readUInt32LE(0),24);assert.equal(notes.length,100);for(let i=0;i<4;i++){const p=4+i*24;assert.equal(notes.readInt32LE(p)*192+notes.readInt16LE(p+12),i*768);assert.equal(notes.readUInt32LE(p+4),0xffffffff);assert.equal(notes.readInt32LE(p+8),384);assert.equal(notes.readUInt16LE(p+14),72);assert.equal(notes[p+16],96);assert.equal(notes[p+21],0);}
 assert(one(pt.children,n=>n.type==='DMBD').children.length>0);assert(fs.readFileSync(run.inputs.find(i=>i.path.endsWith('.sgp')).path).equals(fs.readFileSync(file('lifecycle/input.sgp'))));
}
const wav=fs.readFileSync(file('output.wav'));assert.equal(wav.toString('ascii',0,4),'RIFF');assert.equal(wav.readUInt32LE(4)+8,wav.length);
let fmt,data;for(let p=12;p+8<=wav.length;){const n=wav.readUInt32LE(p+4);assert(p+8+n<=wav.length);const id=wav.toString('ascii',p,p+4);if(id==='fmt ')fmt=wav.subarray(p+8,p+8+n);if(id==='data')data=wav.subarray(p+8,p+8+n);p+=8+n+(n&1);}
assert(fmt&&data);const tag=fmt.readUInt16LE(0),rate=fmt.readUInt32LE(4),channels=fmt.readUInt16LE(2),align=fmt.readUInt16LE(12);assert.equal(fmt.readUInt16LE(14),32);assert(tag===3||(tag===65534&&fmt.readUInt32LE(24)===3));
const frames=data.length/align;assert.equal(frames,rate*capture.seconds);
const mono=new Float64Array(frames);let peak=0;for(let i=0;i<frames;i++)for(let c=0;c<channels;c++){const x=data.readFloatLE(i*align+c*4);assert(Number.isFinite(x));mono[i]+=x/channels;peak=Math.max(peak,Math.abs(x));}
const packets=fs.readFileSync(file('packets.csv'),'utf8').trim().split(/\r?\n/).slice(1).map(s=>s.split(',').map(Number));assert(packets.length);
const origins=packets.slice(0,100).map(p=>p[4]-p[0]*1e7/rate).sort((a,b)=>a-b),origin=origins[Math.floor(origins.length/2)];
let maxGapFrames=0;for(let i=1;i<packets.length;i++)maxGapFrames=Math.max(maxGapFrames,packets[i][0]-packets[i-1][0]-packets[i-1][1]);
const packetIntegrity=packets.every((p,i)=>(p[2]&4)===0&&(!(p[2]&1)||i===0))&&maxGapFrames<=rate*.002;
const prepared=native.phases.some(p=>p.phase==='play-ready');
const required=prepared?['play-request','play-return','play-ready','stop-request','stop-return','restart-request','restart-return','restart-ready','final-stop-request','final-stop-return','shutdown-return']:['play-request','play-return','stop-request','stop-return','restart-request','restart-return','final-stop-request','final-stop-return','shutdown-return'];assert.deepEqual(native.phases.map(p=>p.phase),required);
const phases=Object.fromEntries(native.phases.map(p=>[p.phase,(p.qpc100ns-origin)/1e7]));
for(let i=1;i<required.length;i++)assert(phases[required[i]]>phases[required[i-1]]);
function rms(a,b){assert(a>=0&&b>a&&b<capture.seconds);let sum=0,n=0;for(let i=Math.round(a*rate);i<Math.round(b*rate);i++){sum+=mono[i]**2;n++;}return Math.sqrt(sum/n);}
function tone(a,b,midi=motif?72:60,explicitFrequency=null){const frequency=explicitFrequency??measuredFrequency??440*2**((midi-69)/12),coefficient=2*Math.cos(2*Math.PI*frequency/(rate/4));let s1=0,s2=0,n=0;const begin=Math.round(a*rate),end=Math.round(b*rate);for(let i=begin;i<end;i+=4){const s=mono[i]*(.5-.5*Math.cos(2*Math.PI*(i-begin)/(end-begin)))+coefficient*s1-s2;s2=s1;s1=s;n++;}return Math.sqrt(Math.max(0,s1*s1+s2*s2-coefficient*s1*s2))/n;}
const intervals=[
 {name:'baseline',from:.3,to:phases['play-request']-.2,sound:false},
 {name:'first-playing',from:phases[prepared?'play-ready':'play-return']+.55,to:phases['stop-request']-.15,sound:true},
 {name:'stop-hold',from:phases['stop-return']+1.2,to:phases['restart-request']-.2,sound:false},
 {name:'restarted-playing',from:phases[prepared?'restart-ready':'restart-return']+.55,to:phases['final-stop-request']-.15,sound:true},
 {name:'final-stop-hold',from:phases['final-stop-return']+1.2,to:capture.seconds-.3,sound:false}
].map(p=>({...p,rms:rms(p.from,p.to),expectedTone:p.sound?tone(p.from,p.from+.3):null,expectedMidi:motif?72:60,wrongPitch:p.sound&&(motif||dlsMode)?tone(p.from,p.from+.3,60,dlsMode?440*2**((60-69)/12):null):null}));
const phaseDurationsPassed=phases['stop-request']-phases[prepared?'play-ready':'play-return']>=1.9&&phases['restart-request']-phases['stop-return']>=2.9&&phases['final-stop-request']-phases[prepared?'restart-ready':'restart-return']>=1.9;
const passed=run.playerExitCode===0&&run.captureExitCode===0&&native.passed&&packetIntegrity&&peak<.99&&phaseDurationsPassed&&intervals.every(p=>p.sound?p.rms>.001&&p.expectedTone>.00005&&(!(motif||dlsMode)||p.expectedTone>p.wrongPitch*2):p.rms<.0001);
const report={schema:1,scope:motif?'Named Motif C5 generated notes and explicit Stop/silent hold/replay/final Stop digital output; GUI/physical speaker/exact tempo untested':'Long strings Segment; explicit early Stop/silent hold/replay/final Stop digital output only; GUI and tempo-change untested',passed,phases,intervals,peak,packetIntegrity,maxGapFrames,phaseDurationsPassed,runSha256:hash(file('run.json')),nativeSha256:hash(file('lifecycle/lifecycle.json')),wavSha256:hash(file('output.wav')),packetsSha256:hash(file('packets.csv')),analyzerSha256:hash(new URL(import.meta.url)),fullAcceptance:false};
if(dlsMode)Object.assign(report,{scope:'GUI-authored DLS with typed Region loop, owned Band/Segment, exact source/runtime/restart collection bytes; MIDI60 renders measured 440 Hz instead of GM C4; Stop/silent hold/replay digital output only',dlsMode:true,measuredFrequency,sampleProofSha256:hash(file('dls-sample.json'))});
fs.writeFileSync(file('lifecycle-audio-proof.json'),JSON.stringify(report,null,2)+'\n');fs.copyFileSync(new URL(import.meta.url),file('lifecycle-audio-auditor.mjs'));console.log(JSON.stringify(report,null,2));if(!passed)process.exitCode=1;

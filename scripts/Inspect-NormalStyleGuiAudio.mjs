import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';import assert from 'node:assert/strict';
import {captureClock} from './AudioCaptureClock.mjs';
const dir=path.resolve(process.argv[2]),read=p=>fs.readFileSync(p),json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,'')),hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const run=json(dir+'/run.json'),capture=json(dir+'/capture.json'),actions=json(dir+'/actions.json');
assert.equal(run.captureExitCode,0);assert.equal(run.state,'exited');assert(capture.passed);
for(const [p,h] of [[run.executable,run.exeSha256],[run.buildSummary,run.buildSummarySha256],[run.recorder,run.recorderSha256],[run.recorderBuildSummary,run.recorderBuildSummarySha256],[dir+'/driver.ps1',run.driverSha256],...run.inputs.map(x=>[x.path,x.sha256])])assert.equal(hash(p),h);
const build=json(run.buildSummary);assert(build.passed&&build.sourceSnapshotUnchanged);
for(const s of build.sources){assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);assert.equal(hash(path.resolve(s.path)),s.sha256);}
const launch=json(dir+'/gui-launch-at-capture.json');assert.equal(launch.processId,run.processId);assert.equal(launch.exeSha256,run.exeSha256);
const input=n=>{const x=run.inputs.find(x=>path.basename(x.path)===n);assert(x,n);return x;};
const raw=json(input('normal-style-dls-proof.json').path),gui=json(input('normal-style-gui-proof.json').path);
assert(raw.passed&&gui.passed);assert.equal(gui.exeSha256,run.exeSha256);
const reloadInput=run.inputs.find(x=>path.basename(x.path)==='normal-style-gui-reload-proof.json');
if(reloadInput){const reload=json(reloadInput.path);assert(reload.passed);assert.equal(reload.processId,run.processId);assert.equal(reload.exeSha256,run.exeSha256);assert.equal(reload.previousProcessId,gui.processId);assert.equal(reload.previousGuiProofSha256,input('normal-style-gui-proof.json').sha256);assert.equal(reload.segmentResavedSha256,gui.segmentRedoneSha256);}
else assert.equal(gui.processId,run.processId);
const fixtureInput=run.inputs.find(x=>path.basename(x.path)==='inputs.json');
let nominalSeconds=22;
if(fixtureInput){
 const fixture=json(fixtureInput.path);assert.equal(fixture.profile,'long-normal-style');assert.equal(fixture.sourceSegmentSha256,gui.segmentRedoneSha256);
 assert.equal(fixture.lengthClocks,294912);assert.equal(fixture.originalLengthClocks,49152);assert.equal(fixture.changedByteRange.bytes,4);
 nominalSeconds=2+(fixture.lengthClocks-3072)/768/3;assert.equal(fixture.nominalSeconds,nominalSeconds);
 for(const x of fixture.inputs)assert.equal(hash(x.path),x.sha256);
 for(const x of fixture.outputs){assert.equal(hash(x.path),x.sha256);assert(run.inputs.some(y=>path.resolve(y.path)===path.resolve(x.path)&&y.sha256===x.sha256));}
 const source=fixture.inputs.find(x=>x.name==='Normal.sgp'),derived=fixture.outputs.find(x=>x.name==='Normal.sgp');
 assert.equal(source.sha256,gui.segmentRedoneSha256);const normalized=Buffer.from(read(derived.path)),offset=fixture.changedByteRange.offset;
 assert.equal(normalized.toString('ascii',offset-12,offset-8),'segh');assert.equal(normalized.readUInt32LE(offset),fixture.lengthClocks);
 normalized.writeUInt32LE(fixture.originalLengthClocks,offset);assert(normalized.equals(read(source.path)));
 for(const x of fixture.outputs.filter(x=>x.name!=='Normal.sgp'))assert.equal(x.sha256,fixture.inputs.find(y=>y.name===x.name).sha256);
 for(const [name,expression] of [['21-long-playing-observed',/Playing/],['24-long-replaying-observed',/Playing/],['26-long-final-stopped-observed',/Stopped\./]]){
   const observation=json(run.guiRun+'/'+name+'.json');assert.equal(observation.window.id,launch.windowId);assert.equal(observation.window.app.toLowerCase(),('process:'+launch.executable).toLowerCase());assert.match(observation.accessibility.tree,expression);
 }
}
else assert.equal(input('Normal.sgp').sha256,gui.segmentRedoneSha256);
for(const n of ['Heartlnd.stp','owned.dls'])assert.equal(input(n).sha256,raw.outputs.find(x=>path.basename(x.path)===n).sha256);
const wav=read(dir+'/output.wav');assert.equal(wav.toString('ascii',0,4),'RIFF');assert.equal(wav.readUInt32LE(4)+8,wav.length);let fmt,data;
for(let p=12;p<wav.length;){const n=wav.readUInt32LE(p+4);assert(p+8+n<=wav.length);const id=wav.toString('ascii',p,p+4);if(id==='fmt ')fmt=wav.subarray(p+8,p+8+n);if(id==='data')data=wav.subarray(p+8,p+8+n);p+=8+n+(n&1);}
assert(fmt&&data);const rate=fmt.readUInt32LE(4),channels=fmt.readUInt16LE(2),align=fmt.readUInt16LE(12),tag=fmt.readUInt16LE(0);
assert(tag===3||(tag===65534&&fmt.readUInt32LE(24)===3));assert.equal(fmt.readUInt16LE(14),32);assert.equal(data.length/align,rate*capture.seconds);
const mono=new Float64Array(data.length/align);let peak=0;
for(let i=0;i<mono.length;i++)for(let c=0;c<channels;c++){const v=data.readFloatLE(i*align+c*4);assert(Number.isFinite(v));mono[i]+=v/channels;peak=Math.max(peak,Math.abs(v));}
const packets=read(dir+'/packets.csv').toString().trim().split(/\r?\n/).slice(1).map(s=>s.split(',').map(Number));
const ready=json(dir+'/ready.json'),clock=captureClock(ready,capture,packets,rate);
assert.equal(run.timingBasis,'recorder-start-utc-qpc');assert.equal(hash(dir+'/ready.json'),run.captureReadySha256);
const readyPollOffsetSeconds=clock.utcToSeconds(run.readyUtc);assert(readyPollOffsetSeconds>=0&&readyPollOffsetSeconds<1);
const action=name=>{const x=actions.find(x=>x.action===name);assert(x);const a={before:clock.utcToSeconds(x.beforeUtc),after:clock.utcToSeconds(x.afterUtc)};assert(a.after>=a.before);return a;};
const play=action('play'),stop=action('stop'),restart=action('restart'),finalStop=action('final-stop');
const timingPassed=play.before>2&&stop.before>play.after+5&&restart.before>stop.after+4&&finalStop.before>restart.after+5&&finalStop.after<capture.seconds-4&&stop.after-play.before<nominalSeconds-2&&finalStop.after-restart.before<nominalSeconds-2;
function rms(a,z){assert(a>=0&&z>a&&z<capture.seconds);let sum=0,n=0;for(let i=Math.round(a*rate);i<Math.round(z*rate);i++){sum+=mono[i]**2;n++;}return Math.sqrt(sum/n);}
function tone(a,z,f){const begin=Math.round(a*rate),end=Math.round(z*rate),co=2*Math.cos(2*Math.PI*f/(rate/4));let x=0,y=0,n=0;for(let i=begin;i<end;i+=4){const s=mono[i]*(.5-.5*Math.cos(2*Math.PI*(i-begin)/(end-begin)))+co*x-y;y=x;x=s;n++;}return Math.sqrt(Math.max(0,x*x+y*y-co*x*y))/n;}
const silence=[{name:'baseline',from:.3,to:play.before-.2},{name:'stop-hold',from:stop.after+1.2,to:restart.before-.2},{name:'final-stop',from:finalStop.after+1.2,to:capture.seconds-.3}].map(x=>({...x,rms:rms(x.from,x.to)}));
const baselineWindows=[];for(let a=1;a+1<play.before-.2;a++)baselineWindows.push({from:a,to:a+1,rms:rms(a,a+1),energy440:tone(a,a+1,440)});
const envelope=[];for(let i=0;i<capture.seconds*200-2;i++){const t=i*.005;envelope.push({time:t,rms:rms(t,t+.01)});}
const candidates=[];for(let i=3;i<envelope.length-1;i++){const rise=envelope[i].rms-envelope[i-2].rms,prev=envelope[i-1].rms-envelope[i-3].rms,next=envelope[i+1].rms-envelope[i-1].rms;if(rise>.001&&rise>=prev&&rise>next)candidates.push({time:envelope[i].time,rise});}
const onsets=[];for(const p of candidates){if(!onsets.length||p.time-onsets.at(-1).time>.2)onsets.push(p);else if(p.rise>onsets.at(-1).rise)onsets[onsets.length-1]=p;}
const runs=[{name:'playing',start:play,end:stop},{name:'restarted',start:restart,end:finalStop}].map(p=>{
 const heard=onsets.filter(x=>x.time>=p.start.before&&x.time<p.end.before-.15),intervals=heard.slice(1).map((x,i)=>x.time-heard[i].time),nominal=intervals.map((_,i)=>i<4?.5:1/3);
 const spectra=heard.filter(x=>x.time+.13<p.end.before).map(x=>({time:x.time,correct:tone(x.time+.04,x.time+.13,440),gm:tone(x.time+.04,x.time+.13,440*2**((60-69)/12))}));
 return {name:p.name,onsets:heard,intervals,tempoPassed:heard.length>=12&&intervals.every((v,i)=>Math.abs(v-nominal[i])<.03),pitchPassed:spectra.length>=12&&spectra.every(s=>s.correct>.00005&&s.correct>s.gm*2),spectra,rms:rms(p.start.after+.55,p.end.before-.15)};
});
let maxGapFrames=0;
for(let i=1;i<packets.length;i++)maxGapFrames=Math.max(maxGapFrames,packets[i][0]-packets[i-1][0]-packets[i-1][1]);
const packetIntegrity=packets.length>100&&packets.every((p,i)=>(p[2]&4)===0&&(!(p[2]&1)||i===0))&&maxGapFrames<=rate*.002&&capture.timestampErrors===0;
const passed=timingPassed&&packetIntegrity&&peak<.99&&silence.every(x=>x.rms<.0001)&&runs.every(x=>x.tempoPassed&&x.pitchPassed&&x.rms>.001);
const clockModule=new URL('./AudioCaptureClock.mjs',import.meta.url);
const proof={schema:2,createdUtc:new Date().toISOString(),passed,scope:'Current GUI normal Pattern/root owned DLS; waveform 440Hz and120→180 BPM; Stop/restart/final Stop with natural-end exclusion',timingPassed,nominalSeconds,play,stop,restart,finalStop,runs,silence,baselineWindows,peak,packetIntegrity,maxGapFrames,clock:clock.evidence,readyPollOffsetSeconds,processId:run.processId,exeSha256:run.exeSha256,runSha256:hash(dir+'/run.json'),readySha256:hash(dir+'/ready.json'),actionsSha256:hash(dir+'/actions.json'),wavSha256:hash(dir+'/output.wav'),packetsSha256:hash(dir+'/packets.csv'),auditorSha256:hash(new URL(import.meta.url)),clockModuleSha256:hash(clockModule),limitations:['System-wide digital endpoint, physical speaker unverified','No native MIDI callback inventory for this GUI run'],fullAcceptancePassed:false};
fs.copyFileSync(clockModule,dir+'/AudioCaptureClock.mjs');fs.copyFileSync(new URL(import.meta.url),dir+'/gui-audio-auditor.mjs');fs.writeFileSync(dir+'/normal-style-gui-audio-proof.json',JSON.stringify(proof,null,2)+'\n');
console.log(JSON.stringify({passed,timingPassed,nominalSeconds,runs:runs.map(x=>({name:x.name,onsets:x.onsets.length,tempoPassed:x.tempoPassed,pitchPassed:x.pitchPassed,rms:x.rms})),silence,packetIntegrity}));if(!passed)process.exitCode=1;

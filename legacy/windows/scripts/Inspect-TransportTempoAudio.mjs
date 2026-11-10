import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {captureClock} from './AudioCaptureClock.mjs';
const dir=path.resolve(process.argv[2]),file=n=>path.join(dir,n),json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,'')),hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const run=json(file('run-at-audio-analysis.json')),capture=json(file('capture.json')),native=json(file('native/transport-tempo.json'));
for(const [p,h] of [[run.producer,run.producerSha256],[run.buildSummary,run.buildSummarySha256],[run.recorder,run.recorderSha256],[run.recorderBuildSummary,run.recorderBuildSummarySha256]])assert.equal(hash(p),h);
assert.equal(run.profile,'transport-tempo');
function chunks(b,start=0,end=b.length){const result=[];for(let p=start;p<end;){const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),e=p+8+n;assert(e<=end);result.push({id,data:b.subarray(p+8,e)});if(id==='RIFF'||id==='LIST'||id==='seqt')result.push(...chunks(b,p+8+(id==='seqt'?0:4),e));p=e+(n&1);}return result;}
const input=fs.readFileSync(file('native/tempo.sgp')),list=chunks(input),tempo=list.filter(c=>c.id==='tetr');assert.equal(tempo.length,1);assert.equal(tempo[0].data.readUInt32LE(0),16);const tempos=[];
for(let p=4;p<tempo[0].data.length;p+=16)tempos.push({clock:tempo[0].data.readInt32LE(p),bpm:tempo[0].data.readDoubleLE(p+8)});
assert.deepEqual(tempos,[{clock:0,bpm:120},{clock:3072,bpm:120}]);
const ev=list.filter(c=>c.id==='evtl');assert.equal(ev.length,1);const stride=ev[0].data.readUInt32LE(0),notes=[];assert(stride>=20);
for(let p=4;p<ev[0].data.length;p+=stride){const b=ev[0].data;notes.push({clock:b.readInt32LE(p),duration:b.readInt32LE(p+4),channel:b.readUInt32LE(p+8),status:b[p+14],pitch:b[p+15],velocity:b[p+16]});}
assert(notes.length>100);assert(notes.every((n,i)=>n.clock===i*768&&n.duration===192&&n.channel===0&&(n.status&0xf0)===0x90&&n.pitch===60&&n.velocity===96));
const wav=fs.readFileSync(file('output.wav'));assert.equal(wav.toString('ascii',0,4),'RIFF');assert.equal(wav.readUInt32LE(4)+8,wav.length);let fmt,data;
for(let p=12;p+8<=wav.length;){const n=wav.readUInt32LE(p+4);assert(p+8+n<=wav.length);const id=wav.toString('ascii',p,p+4);if(id==='fmt ')fmt=wav.subarray(p+8,p+8+n);if(id==='data')data=wav.subarray(p+8,p+8+n);p+=8+n+(n&1);}assert(fmt&&data);
const tag=fmt.readUInt16LE(0),rate=fmt.readUInt32LE(4),channels=fmt.readUInt16LE(2),align=fmt.readUInt16LE(12);assert.equal(fmt.readUInt16LE(14),32);assert(tag===3||(tag===65534&&fmt.readUInt32LE(24)===3));
const frames=data.length/align;assert.equal(frames,rate*capture.seconds);const mono=new Float64Array(frames);let peak=0;
for(let i=0;i<frames;i++)for(let c=0;c<channels;c++){const x=data.readFloatLE(i*align+c*4);assert(Number.isFinite(x));mono[i]+=x/channels;peak=Math.max(peak,Math.abs(x));}
function rms(a,b){assert(a>=0&&b<=capture.seconds&&b>a);let sum=0,n=0;for(let i=Math.round(a*rate);i<Math.round(b*rate);i++){sum+=mono[i]**2;n++;}return Math.sqrt(sum/n);}
function tone(a,b,midi){const coef=2*Math.cos(2*Math.PI*(440*2**((midi-69)/12))/(rate/4));let s1=0,s2=0,n=0;for(let i=Math.round(a*rate);i<Math.round(b*rate);i+=4){const w=.5-.5*Math.cos(2*Math.PI*(i/rate-a)/(b-a)),s=mono[i]*w+coef*s1-s2;s2=s1;s1=s;n++;}return Math.sqrt(Math.max(0,s1*s1+s2*s2-coef*s1*s2))/n;}
const packets=fs.readFileSync(file('packets.csv'),'utf8').trim().split(/\r?\n/).slice(1).map(s=>s.split(',').map(Number));let maxGapFrames=0;for(let i=1;i<packets.length;i++)maxGapFrames=Math.max(maxGapFrames,packets[i][0]-packets[i-1][0]-packets[i-1][1]);
const packetIntegrity=packets.length>0&&packets.every((p,i)=>(p[2]&4)===0&&(!(p[2]&1)||i===0))&&maxGapFrames<=rate*.002;
const ready=json(file('ready.json')),origin=ready.startQpc100ns;assert.equal(ready.schema,2);
const clock=captureClock(ready,capture,packets,rate);
const envelope=[];for(let i=0;i<capture.seconds*200-2;i++){const t=i*.005;envelope.push({time:t,rms:rms(t,t+.01)});}
const candidates=[];for(let i=3;i<envelope.length-1;i++){const rise=envelope[i].rms-envelope[i-2].rms,previous=envelope[i-1].rms-envelope[i-3].rms,next=envelope[i+1].rms-envelope[i-1].rms;if(rise>.003&&rise>=previous&&rise>next)candidates.push({time:envelope[i].time,rise});}
const onsets=[];for(const p of candidates){if(!onsets.length||p.time-onsets.at(-1).time>.15)onsets.push(p);else if(p.rise>onsets.at(-1).rise)onsets[onsets.length-1]=p;}
const phases=native.phases.filter(p=>p.multiplier).map(p=>{
 const begin=(p.qpcBegin100ns-origin)/1e7,end=(p.qpcEnd100ns-origin)/1e7;assert(begin>0&&end<capture.seconds);
 const attacks=onsets.filter(o=>o.time>begin+.04&&o.time<end-.04),intervals=attacks.slice(1).map((o,i)=>o.time-attacks[i].time),expectedInterval=.5/p.multiplier;
 const pitches=attacks.map(o=>({time:o.time,correct:tone(o.time+.02,o.time+.12,60),lower:tone(o.time+.02,o.time+.12,59),upper:tone(o.time+.02,o.time+.12,61)}));
 return {multiplier:p.multiplier,begin,end,expectedInterval,attacks,intervals,pitches,intervalPassed:attacks.length>=2&&intervals.every(v=>Math.abs(v-expectedInterval)<.04),pitchPassed:pitches.length>=2&&pitches.every(s=>s.correct>.00005&&s.correct>s.lower*1.3&&s.correct>s.upper*1.3)};
});
assert.deepEqual(phases.map(p=>p.multiplier),[1,.5,1,2]);
const lifecycle=native.phases.at(-1),stop=(lifecycle.stopQpc100ns-origin)/1e7,replay=(lifecycle.replayQpc100ns-origin)/1e7,finalStop=(lifecycle.finalStopQpc100ns-origin)/1e7;
const silenceRms=rms(stop+.8,replay-.1),finalSilenceRms=rms(finalStop+.8,finalStop+1.7),replayRms=rms(replay+.8,finalStop-.1),baselineRms=rms(.3,1.5);
const passed=run.playerExitCode===0&&run.captureExitCode===0&&native.passed&&native.documentUnchanged&&packetIntegrity&&phases.every(p=>p.intervalPassed&&p.pitchPassed)&&silenceRms<.0001&&finalSilenceRms<.0001&&baselineRms<.0001&&replayRms>.0001&&peak<.99;
const report={schema:1,passed,scope:'Independent PCM interval/pitch and stop silence checks for runtime 1/0.5/1/2 master tempo; GUI and dynamic original parity separate',fixture:{tempos,noteCount:notes.length,pitch:60,duration:192,spacing:768,sha256:hash(file('native/tempo.sgp'))},packetIntegrity,maxGapFrames,clock:clock.evidence,phases,lifecycle:{stop,replay,finalStop,silenceRms,finalSilenceRms,replayRms,baselineRms},peak,onsets,nativeSha256:hash(file('native/transport-tempo.json')),runAtAnalysisSha256:hash(file('run-at-audio-analysis.json')),wavSha256:hash(file('output.wav')),analyzerSha256:hash(new URL(import.meta.url)),clockHelperSha256:hash(new URL('./AudioCaptureClock.mjs',import.meta.url)),fullAcceptance:false};
fs.writeFileSync(file('transport-tempo-audio-proof.json'),JSON.stringify(report,null,2)+'\n');fs.copyFileSync(new URL(import.meta.url),file('transport-tempo-auditor.mjs'));fs.copyFileSync(new URL('./AudioCaptureClock.mjs',import.meta.url),file('AudioCaptureClock.mjs'));console.log(JSON.stringify(report,null,2));if(!passed)process.exitCode=1;

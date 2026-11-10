import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {captureClock} from './AudioCaptureClock.mjs';
const dir=path.resolve(process.argv[2]),file=n=>path.join(dir,n),json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,'')),hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const run=json(file('run-at-audio-analysis.json')),capture=json(file('capture.json')),native=json(file('native/transport-position.json'));
for(const [p,h] of [[run.producer,run.producerSha256],[run.buildSummary,run.buildSummarySha256],[run.recorder,run.recorderSha256],[run.recorderBuildSummary,run.recorderBuildSummarySha256]])assert.equal(hash(p),h);
assert.equal(run.profile,'transport-position');
function chunks(b,start=0,end=b.length){const result=[];for(let p=start;p<end;){const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),e=p+8+n;assert(e<=end);result.push({id,data:b.subarray(p+8,e)});if(id==='RIFF'||id==='LIST'||id==='seqt')result.push(...chunks(b,p+8+(id==='seqt'?0:4),e));p=e+(n&1);}return result;}
const input=fs.readFileSync(file('native/position.sgp')),list=chunks(input),tempo=list.filter(c=>c.id==='tetr');assert.equal(tempo.length,1);assert.equal(tempo[0].data.readUInt32LE(0),16);const tempos=[];
for(let p=4;p<tempo[0].data.length;p+=16)tempos.push({clock:tempo[0].data.readInt32LE(p),bpm:tempo[0].data.readDoubleLE(p+8)});
assert.deepEqual(tempos,[{clock:0,bpm:120},{clock:3072,bpm:120}]);
const ev=list.filter(c=>c.id==='evtl');assert.equal(ev.length,1);const stride=ev[0].data.readUInt32LE(0),notes=[];assert(stride>=20);
for(let p=4;p<ev[0].data.length;p+=stride){const b=ev[0].data;notes.push({clock:b.readInt32LE(p),duration:b.readInt32LE(p+4),channel:b.readUInt32LE(p+8),status:b[p+14],pitch:b[p+15],velocity:b[p+16]});}
assert.equal(notes.length,170);assert(notes.every((n,i)=>n.clock===i*768&&n.duration===600&&n.channel===0&&(n.status&0xf0)===0x90&&n.pitch===(n.clock<6144?60:67)&&n.velocity===96));
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

const phase=name=>{const p=native.phases.find(p=>p.phase===name);assert(p);return {...p,seconds:(p.qpc100ns-origin)/1e7};};
const seek=phase('seek-ready'),boundary=phase('boundary-request'),ended=phase('boundary-ended'),fromStart=phase('from-start-ready'),immediate=phase('immediate-request'),returned=phase('immediate-returned');
const pitches=[{name:'cursor-6144-G4',begin:seek.seconds+.7,end:boundary.seconds-.1,midi:67,wrong:60},{name:'from-start-C4',begin:fromStart.seconds+.7,end:immediate.seconds-.1,midi:60,wrong:67}].map(p=>{const correct=tone(p.begin,p.end,p.midi),wrong=tone(p.begin,p.end,p.wrong),energy=rms(p.begin,p.end);return {...p,correct,wrong,energy,passed:correct>.00005&&correct>wrong*4&&energy>.0001};});
const boundaryWait=ended.seconds-boundary.seconds,boundaryQuiet=rms(ended.seconds+.8,ended.seconds+1.7),immediateQuiet=rms(returned.seconds+.8,returned.seconds+1.7),baseline=rms(.3,1.5),beforeImmediate=rms(immediate.seconds-.1,immediate.seconds-.02);
const expectedBoundary= native.seekStart+Math.ceil((native.boundaryRequestClocks-native.seekStart)/3072)*3072;
const boundaryClockPassed=native.boundaryRequestClocks<expectedBoundary-100&&Math.abs(native.boundaryObservedClocks-expectedBoundary)<80;
const passed=run.playerExitCode===0&&run.captureExitCode===0&&native.passed&&native.documentUnchanged&&native.seekStartPoint===6144&&packetIntegrity&&pitches.every(p=>p.passed)&&boundaryWait>.1&&boundaryWait<2.2&&boundaryClockPassed&&native.boundaryReturnMs<100&&boundaryQuiet<.0001&&immediateQuiet<.0001&&baseline<.0001&&beforeImmediate>.0001&&peak<.99;
const report={schema:1,passed,scope:'Independent raw RIFF cursor-section vs beginning pitch, actual measure-boundary stop clock, asynchronous return and Stop/quiet PCM; main GUI and original parity separate',fixture:{tempos,noteCount:notes.length,duration:600,spacing:768,firstSectionMidi:60,seekSectionMidi:67,seekClock:6144,sha256:hash(file('native/position.sgp'))},packetIntegrity,maxGapFrames,clock:clock.evidence,pitches,lifecycle:{boundaryWait,expectedBoundary,boundaryClockPassed,boundaryQuiet,immediateQuiet,baseline,beforeImmediate,peak},phases:native.phases,nativeSha256:hash(file('native/transport-position.json')),runAtAnalysisSha256:hash(file('run-at-audio-analysis.json')),wavSha256:hash(file('output.wav')),analyzerSha256:hash(new URL(import.meta.url)),clockHelperSha256:hash(new URL('./AudioCaptureClock.mjs',import.meta.url)),fullAcceptance:false};
fs.writeFileSync(file('transport-position-audio-proof.json'),JSON.stringify(report,null,2)+'\n');fs.copyFileSync(new URL(import.meta.url),file('transport-position-auditor.mjs'));fs.copyFileSync(new URL('./AudioCaptureClock.mjs',import.meta.url),file('AudioCaptureClock.mjs'));console.log(JSON.stringify(report,null,2));if(!passed)process.exitCode=1;

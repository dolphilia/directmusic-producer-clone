import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {captureClock} from './AudioCaptureClock.mjs';
const dir=path.resolve(process.argv[2]),file=n=>path.join(dir,n),json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,'')),hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const run=json(file('run-at-audio-analysis.json')),capture=json(file('capture.json'));
for(const [p,h] of [[run.producer,run.producerSha256],[run.buildSummary,run.buildSummarySha256],[run.recorder,run.recorderSha256],[run.recorderBuildSummary,run.recorderBuildSummarySha256]])assert.equal(hash(p),h);
assert.equal(run.profile,'transport-position-gui'); assert.equal(hash(run.input.path),run.input.sha256);
function chunks(b,start=0,end=b.length){const result=[];for(let p=start;p<end;){const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),e=p+8+n;assert(e<=end);result.push({id,data:b.subarray(p+8,e)});if(id==='RIFF'||id==='LIST'||id==='seqt')result.push(...chunks(b,p+8+(id==='seqt'?0:4),e));p=e+(n&1);}return result;}
const input=fs.readFileSync(file('input.sgp')),list=chunks(input),tempo=list.filter(c=>c.id==='tetr');assert.equal(tempo.length,1);assert.equal(tempo[0].data.readUInt32LE(0),16);const tempos=[];
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

const epoch=116444736000000000n,utcOrigin=(BigInt(ready.utcBeforeFileTime)+BigInt(ready.utcAfterFileTime))/2n;
const actions=json(file('actions.json')).map(a=>({...a,begin:(Number(BigInt(Date.parse(a.beginUtc))*10000n+epoch-utcOrigin))/1e7,returned:(Number(BigInt(Date.parse(a.returnedUtc))*10000n+epoch-utcOrigin))/1e7}));
const action=n=>{const a=actions.find(a=>a.action===n);assert(a);assert(a.begin>=0&&a.returned<capture.seconds&&a.returned>=a.begin);return a;};
const seek=action('cursor-play'),boundary=action('boundary-stop'),fromStart=action('from-start-play'),immediate=action('immediate-stop');
const pitches=[{name:'actual-main-cursor-6336-G4',begin:seek.returned+.7,end:boundary.begin-.1,midi:67,wrong:60},{name:'actual-main-from-start-C4',begin:fromStart.returned+.7,end:fromStart.returned+2.5,midi:60,wrong:67}].map(p=>{const correct=tone(p.begin,p.end,p.midi),wrong=tone(p.begin,p.end,p.wrong),energy=rms(p.begin,p.end);return {...p,correct,wrong,energy,passed:correct>.00005&&correct>wrong*4&&energy>.0001};});
const boundaryQuiet=rms(boundary.returned+4.5,fromStart.begin-.1),immediateQuiet=rms(immediate.returned+.8,immediate.returned+2),baseline=rms(.3,seek.begin-.5),beforeImmediate=rms(immediate.begin-.1,immediate.begin-.02),tailQuiet=rms(capture.seconds-2,capture.seconds-1);
let lastAudibleAfterBoundary=null;for(let t=boundary.returned;t+0.02<fromStart.begin-.1;t+=.02)if(rms(t,t+.02)>.00015)lastAudibleAfterBoundary=t-boundary.returned+.02;
const passed=run.productExitCode===0&&run.captureExitCode===0&&capture.passed&&packetIntegrity&&pitches.every(p=>p.passed)&&boundaryQuiet<.0001&&immediateQuiet<.0001&&baseline<.0001&&tailQuiet<.0001&&beforeImmediate>.0001&&peak<.99&&lastAudibleAfterBoundary!==null&&lastAudibleAfterBoundary<4.5;
const report={schema:1,passed,scope:'Actual main GUI keyboard operation PCM independently audited from raw RIFF and UTC/QPC mapping. Main boundary acknowledgement and retained cursor require separate UI observations. This does not prove original same-input parity, physical speaker output or a full acceptance round.',fixture:{tempos,noteCount:notes.length,duration:600,spacing:768,firstSectionMidi:60,seekSectionMidi:67,cursorClock:6336,sha256:hash(file('input.sgp')),sourceInputSha256:run.input.sha256},packetIntegrity,maxGapFrames,clock:clock.evidence,pitches,lifecycle:{boundaryQuiet,immediateQuiet,baseline,beforeImmediate,tailQuiet,peak,lastAudibleAfterBoundary},actions,runAtAnalysisSha256:hash(file('run-at-audio-analysis.json')),wavSha256:hash(file('output.wav')),analyzerSha256:hash(new URL(import.meta.url)),clockHelperSha256:hash(new URL('./AudioCaptureClock.mjs',import.meta.url)),fullAcceptance:false};
fs.writeFileSync(file('transport-position-gui-audio-proof.json'),JSON.stringify(report,null,2)+String.fromCharCode(10));fs.copyFileSync(new URL(import.meta.url),file('transport-position-gui-auditor.mjs'));fs.copyFileSync(new URL('./AudioCaptureClock.mjs',import.meta.url),file('AudioCaptureClock.mjs'));console.log(JSON.stringify(report,null,2));if(!passed)process.exitCode=1;

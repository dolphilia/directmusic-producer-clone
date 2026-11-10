import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const dir=path.resolve(process.argv[2]);
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const run=json(path.join(dir,'run.json')), capture=json(path.join(dir,'capture.json'));
assert.equal(hash(run.producer),run.producerSha256);assert.equal(hash(run.recorder),run.recorderSha256);
assert.equal(hash(run.buildSummary),run.buildSummarySha256);
const wav=fs.readFileSync(path.join(dir,'output.wav'));assert.equal(wav.toString('ascii',0,4),'RIFF');assert.equal(wav.readUInt32LE(4)+8,wav.length);
let format,data;
for(let p=12;p+8<=wav.length;){const id=wav.toString('ascii',p,p+4),n=wav.readUInt32LE(p+4);assert(p+8+n<=wav.length);if(id==='fmt ')format=wav.subarray(p+8,p+8+n);if(id==='data')data=wav.subarray(p+8,p+8+n);p+=8+n+(n&1);}
assert(format&&data);const tag=format.readUInt16LE(0),channels=format.readUInt16LE(2),rate=format.readUInt32LE(4),align=format.readUInt16LE(12),bits=format.readUInt16LE(14);
assert.equal(bits,32);assert(tag===3 || (tag===65534 && format.readUInt32LE(24)===3),'IEEE float format required');
const frames=data.length/align;assert.equal(frames,rate*capture.seconds);
const mono=new Float64Array(frames);let peak=0;
for(let i=0;i<frames;i++){for(let c=0;c<channels;c++){const v=data.readFloatLE(i*align+c*4);assert(Number.isFinite(v));mono[i]+=v/channels;peak=Math.max(peak,Math.abs(v));}}
function rms(a,b){let sum=0,count=0;for(let i=Math.round(a*rate);i<Math.min(frames,Math.round(b*rate));i++){sum+=mono[i]*mono[i];count++;}return Math.sqrt(sum/count);}
function tone(a,b,midi){const freq=440*2**((midi-69)/12),step=4,sampleRate=rate/step,coef=2*Math.cos(2*Math.PI*freq/sampleRate);let s1=0,s2=0,count=0;const begin=Math.round(a*rate),end=Math.round(b*rate);for(let i=begin;i<end;i+=step){const w=.5-.5*Math.cos(2*Math.PI*(i-begin)/(end-begin));const s=mono[i]*w+coef*s1-s2;s2=s1;s1=s;count++;}return Math.sqrt(Math.max(0,s1*s1+s2*s2-coef*s1*s2))/count;}
const windows=[];for(let i=0;i<capture.seconds*10;i++){const t=i/10;windows.push({start:t,rms:rms(t,t+.1)});}
const packets=fs.readFileSync(path.join(dir,'packets.csv'),'utf8').trim().split(/\r?\n/).slice(1).map(s=>s.split(',').map(Number));
let maxGapFrames=0;for(let i=1;i<packets.length;i++)maxGapFrames=Math.max(maxGapFrames,packets[i][0]-packets[i-1][0]-packets[i-1][1]);
const packetIntegrity=packets.length>0 && packets.every((p,i)=>(p[2]&4)===0 && (!(p[2]&1)||i===0))&&maxGapFrames<=rate*.002;



assert.equal(run.captureExitCode,0);assert(run.inputsUnchanged);assert.equal(hash(path.join(dir,'launch-at-capture.json')),run.launchSnapshotSha256);assert.equal(hash(run.recorderBuildSummary),run.recorderBuildSummarySha256);for(const p of run.inputs)assert.equal(hash(p.path),p.sha256);
const ready=json(path.join(dir,'ready.json')),actionsFile=path.join(dir,'actions-at-analysis.json'),actions=json(actionsFile);const originMs=Number(BigInt(ready.utcBeforeFileTime)/10000n)-11644473600000;const offset=(e,key='requestUtc')=>(Date.parse(e[key])-originMs)/1000;const action=n=>{const matches=actions.filter(e=>e.action==='recorded '+n);assert.equal(matches.length,1);return matches[0];};
const play=offset(action('Play')),stop=offset(action('Stop')),stopReturn=offset(action('Stop'),'returnUtc'),replay=offset(action('Replay')),finalStop=offset(action('FinalStop'));
assert(play>1&&stop>play+3&&replay>stopReturn+2&&replay<capture.seconds-2);
function onsetNear(click){for(let t=click-.1;t<Math.min(click+2,capture.seconds-.2);t+=.002)if(rms(t,t+.02)>.001)return t;return null;}
const firstOnset=onsetNear(play),replayOnset=onsetNear(replay);assert(firstOnset&&replayOnset);
const intervals=[{name:'baseline',from:.3,to:play-.3,sound:false},{name:'first-playing',from:firstOnset+.1,to:stop-.2,sound:true},{name:'stop-hold',from:stopReturn+1.2,to:replay-.3,sound:false},{name:'replay-playing',from:replayOnset+.1,to:Math.min(finalStop-.2,capture.seconds-.3),sound:true}].map(p=>({...p,rms:rms(p.from,p.to)}));
const pitches=intervals.filter(p=>p.sound).map(p=>{const active=[];for(let t=p.from;t+.08<p.to;t+=.02){const r=rms(t,t+.08);if(r>.005)active.push({from:t,rms:r,expected65:tone(t,t+.08,65),disconnected60:tone(t,t+.08,60)});}return {name:p.name,activeWindows:active.length,maximumExpected65:Math.max(...active.map(w=>w.expected65)),minimumRatio:Math.min(...active.map(w=>w.expected65/Math.max(w.disconnected60,1e-12))),passed:active.length>5&&Math.max(...active.map(w=>w.expected65))>.001&&active.every(w=>w.expected65>w.disconnected60*2)};});
const activeStopVerified=stop-firstOnset<31&&intervals[1].rms>.001&&intervals[2].rms<.0001;
const passed=packetIntegrity&&peak<.99&&activeStopVerified&&intervals.every(p=>p.sound?p.rms>.001:p.rms<.0001)&&pitches.every(p=>p.passed);
const proof={schema:1,candidate:run.candidate,passed,scope:'GUI mixed128notes native Project playback/early Stop/silent hold/replay; mapped MIDI65 only; final Stop outside recording is not verified by this capture',play,stop,stopReturn,replay,finalStop,firstOnset,replayOnset,intervals,pitches,activeStopVerified,finalStopCaptured:finalStop<capture.seconds,packetIntegrity,maxGapFrames,peak,wavSha256:hash(path.join(dir,'output.wav')),packetsSha256:hash(path.join(dir,'packets.csv')),runSha256:hash(path.join(dir,'run.json')),actionsSha256:hash(actionsFile),analyzerSha256:hash(new URL(import.meta.url)),fullAcceptance:false};fs.writeFileSync(path.join(dir,'audio-proof.json'),JSON.stringify(proof,null,2)+'\n');fs.copyFileSync(new URL(import.meta.url),path.join(dir,'auditor.mjs'));console.log(JSON.stringify(proof,null,2));if(!passed)process.exitCode=1;

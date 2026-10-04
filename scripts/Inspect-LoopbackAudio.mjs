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
for(const p of run.inputs)assert.equal(hash(p.path),p.sha256);
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
const baseline=rms(.3,1.8),tail=rms(12,15.5),active=rms(2.5,8.5);
const threshold=Math.max(.0001,baseline*15);
const sounding=windows.filter(w=>w.rms>threshold);
const onset=sounding.length?sounding[0].start:null;
const motif=['motif-repeat','motif-standalone'].includes(run.profile),expectedCount=motif?6:12;const spectra=[];if(onset!==null){for(let i=0;i<expectedCount;i++){const a=onset+i*.5+.10;const c4=tone(a,a+.22,60),c5=tone(a,a+.22,72);spectra.push({index:i,start:a,c4,c5,c4ToC5:c4/Math.max(c5,1e-12)});}}
assert(['notes','crud','variation-first','variation-last','motif-repeat','motif-standalone'].includes(run.profile));
const variation=run.profile.startsWith('variation-'),high=run.profile==='variation-last'||motif,crud=run.profile==='crud'||variation||motif;
let notesPassed=false;if(!run.silenceControl){const notes=json(path.join(dir,crud?'crud':'notes','notes.json'));const expected=crud?Array(expectedCount).fill(high?72:60):[60,60,60,60,72,72,72,72,60,60,60,60];notesPassed=notes.passed&&notes.notes.length===expectedCount&&notes.notes.every((n,i)=>n.midiValue===expected[i]&&(!crud||n.clocks-notes.start===i*768&&n.duration===384&&n.channel===5&&n.velocity===96));}
// Fixed known strings fixture: C4 fundamental present in outer blocks and absent in C5 block.
const pitchPassed=spectra.length===expectedCount&&spectra.every((s,i)=>high||(!crud&&i>=4&&i<8)?s.c5>.00005&&s.c4ToC5<.15:s.c4>.00005&&s.c4ToC5>.25);
const audioPassed=!run.silenceControl&&run.playerExitCode===0&&run.captureExitCode===0&&packetIntegrity&&capture.timestampErrors===0&&baseline<.0001&&tail<.0001&&active>.001&&onset>=2&&onset<(crud?5:4)&&peak<.99&&notesPassed&&pitchPassed;
const report={schema:1,scope:motif?'Named finite Motif C5x6; API attributes/digital endpoint, attribution requires independent source comparison; no GUI or full acceptance':variation?`Fixed eligible Variation ${high?32:1}, ${high?'C5':'C4'}x12 strings; selection attribution needs independent input comparison; no GUI or full acceptance`:crud?'New authored Pattern C4x12 strings; generated attributes and digital output; no GUI or full acceptance':'Known 120 BPM C4/C5/C4 strings fixture only; digital endpoint output, natural end; no explicit Stop/restart or tempo-change proof',passed:audioPassed,silenceControl:run.silenceControl,negativeControlQuiet:run.silenceControl?packetIntegrity&&windows.every(w=>w.rms<.0001):null,packetIntegrity,maxGapFrames,baselineRms:baseline,tailRms:tail,activeRms:active,peak,onset,notesPassed,pitchPassed,spectra,windows,runSha256:hash(path.join(dir,'run.json')),captureSha256:hash(path.join(dir,'capture.json')),wavSha256:hash(path.join(dir,'output.wav')),packetsSha256:hash(path.join(dir,'packets.csv')),analyzerSha256:hash(new URL(import.meta.url)),fullAcceptance:false};
fs.writeFileSync(path.join(dir,'audio-proof.json'),JSON.stringify(report,null,2)+'\n');fs.copyFileSync(new URL(import.meta.url),path.join(dir,'audio-auditor.mjs'));
console.log(JSON.stringify({...report,windows:undefined,spectra:report.spectra.map(s=>({index:s.index,c4:s.c4,c5:s.c5,ratio:s.c4ToC5}))},null,2));
if(run.silenceControl? !report.negativeControlQuiet: !audioPassed)process.exitCode=1;


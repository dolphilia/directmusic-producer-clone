import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const dir=path.resolve(process.argv[2]),file=n=>path.join(dir,n),hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex'),json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const run=json(file('run.json')),capture=json(file('capture.json')),native=json(file('tempo/notes.json'));
assert.equal(run.profile,'tempo');for(const [p,h] of [[run.producer,run.producerSha256],[run.buildSummary,run.buildSummarySha256],[run.recorder,run.recorderSha256],[run.recorderBuildSummary,run.recorderBuildSummarySha256],...run.inputs.map(p=>[p.path,p.sha256])])assert.equal(hash(p),h);
// Read the saved input independently of the product's model and note observer.
const input=fs.readFileSync(run.inputs[0].path);assert(input.equals(fs.readFileSync(file('tempo/input.sgp'))));
function chunks(b,start=0,end=b.length){const result=[];for(let p=start;p<end;){const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),e=p+8+n;assert(e<=end);result.push({id,data:b.subarray(p+8,e)});if(id==='RIFF'||id==='LIST'||id==='seqt')result.push(...chunks(b,p+8+(id==='seqt'?0:4),e));p=e+(n&1);}return result;}
const list=chunks(input),tempo=list.filter(p=>p.id==='tetr');assert.equal(tempo.length,1);assert.equal(tempo[0].data.readUInt32LE(0),16);
const tempos=[];for(let p=4;p<tempo[0].data.length;p+=16)tempos.push({clocks:tempo[0].data.readInt32LE(p),bpm:tempo[0].data.readDoubleLE(p+8)});assert.deepEqual(tempos,[{clocks:0,bpm:120},{clocks:3072,bpm:180}]);
const events=list.filter(p=>p.id==='evtl');assert.equal(events.length,1);const stride=events[0].data.readUInt32LE(0);assert(stride>=20);const expected=[];
for(let p=4;p<events[0].data.length;p+=stride){const b=events[0].data;if((b[p+14]&0xf0)===0x90&&b[p+16])expected.push({clocks:b.readInt32LE(p),duration:b.readInt32LE(p+4),channel:b.readUInt32LE(p+8),midiValue:b[p+15],velocity:b[p+16]});}
assert.equal(expected.length,8);assert.deepEqual(expected.map(n=>n.midiValue),[60,62,64,65,67,69,71,72]);assert(expected.every((n,i)=>n.clocks===i*768&&n.duration===384&&n.channel===0&&n.velocity===96));
const notesPassed=native.passed&&native.notes.length===8&&native.notes.every((n,i)=>n.clocks-native.start===expected[i].clocks&&n.midiValue===expected[i].midiValue&&n.duration===expected[i].duration&&n.velocity===96);
const wav=fs.readFileSync(file('output.wav'));assert.equal(wav.toString('ascii',0,4),'RIFF');assert.equal(wav.readUInt32LE(4)+8,wav.length);let fmt,data;
for(let p=12;p+8<=wav.length;){const n=wav.readUInt32LE(p+4);assert(p+8+n<=wav.length);const id=wav.toString('ascii',p,p+4);if(id==='fmt ')fmt=wav.subarray(p+8,p+8+n);if(id==='data')data=wav.subarray(p+8,p+8+n);p+=8+n+(n&1);}assert(fmt&&data);
const tag=fmt.readUInt16LE(0),rate=fmt.readUInt32LE(4),channels=fmt.readUInt16LE(2),align=fmt.readUInt16LE(12);assert.equal(fmt.readUInt16LE(14),32);assert(tag===3||(tag===65534&&fmt.readUInt32LE(24)===3));
const frames=data.length/align;assert.equal(frames,rate*capture.seconds);const mono=new Float64Array(frames);let peak=0;
for(let i=0;i<frames;i++)for(let c=0;c<channels;c++){const x=data.readFloatLE(i*align+c*4);assert(Number.isFinite(x));mono[i]+=x/channels;peak=Math.max(peak,Math.abs(x));}
function rms(a,b){let sum=0,n=0;for(let i=Math.round(a*rate);i<Math.round(b*rate);i++){sum+=mono[i]**2;n++;}return Math.sqrt(sum/n);}
function tone(a,b,midi){const coef=2*Math.cos(2*Math.PI*(440*2**((midi-69)/12))/(rate/4));let s1=0,s2=0,n=0;for(let i=Math.round(a*rate);i<Math.round(b*rate);i+=4){const w=.5-.5*Math.cos(2*Math.PI*(i/rate-a)/(b-a)),s=mono[i]*w+coef*s1-s2;s2=s1;s1=s;n++;}return Math.sqrt(Math.max(0,s1*s1+s2*s2-coef*s1*s2))/n;}
const packets=fs.readFileSync(file('packets.csv'),'utf8').trim().split(/\r?\n/).slice(1).map(s=>s.split(',').map(Number));let maxGapFrames=0;for(let i=1;i<packets.length;i++)maxGapFrames=Math.max(maxGapFrames,packets[i][0]-packets[i-1][0]-packets[i-1][1]);const packetIntegrity=packets.length>0&&packets.every((p,i)=>(p[2]&4)===0&&(!(p[2]&1)||i===0))&&maxGapFrames<=rate*.002;
// Acoustic onset detection uses only WAV amplitude growth, not native note timestamps.
const envelope=[];for(let i=0;i<capture.seconds*200-2;i++){const t=i*.005;envelope.push({time:t,rms:rms(t,t+.01)});}
const candidates=[];for(let i=2;i<envelope.length-1;i++){const rise=envelope[i].rms-envelope[i-2].rms,previous=envelope[i-1].rms-envelope[i-3]?.rms,next=envelope[i+1].rms-envelope[i-1].rms;if(rise>.003&&rise>=previous&&rise>next)candidates.push({time:envelope[i].time,rise});}
const onsets=[];for(const p of candidates){if(!onsets.length||p.time-onsets.at(-1).time>.2)onsets.push(p);else if(p.rise>onsets.at(-1).rise)onsets[onsets.length-1]=p;}
const intervals=onsets.slice(1).map((p,i)=>p.time-onsets[i].time),nominal=[.5,.5,.5,.5,1/3,1/3,1/3];
const intervalPassed=onsets.length===8&&intervals.every((v,i)=>Math.abs(v-nominal[i])<.03);
const spectra=onsets.slice(0,8).map((p,i)=>{const correct=tone(p.time+.04,p.time+.14,expected[i].midiValue),lower=tone(p.time+.04,p.time+.14,expected[i].midiValue-1),upper=tone(p.time+.04,p.time+.14,expected[i].midiValue+1);return {index:i,midi:expected[i].midiValue,correct,lower,upper};});
const pitchPassed=spectra.length===8&&spectra.every(p=>p.correct>.00005&&p.correct>p.lower*1.3&&p.correct>p.upper*1.3);
const baseline=rms(.3,1.8),tail=rms(10,15.5);
const passed=run.playerExitCode===0&&run.captureExitCode===0&&notesPassed&&packetIntegrity&&intervalPassed&&pitchPassed&&baseline<.0001&&tail<.0001&&peak<.99;
const report={schema:1,scope:'120->180 BPM GM piano8-note input; waveform onset intervals/pitches/natural end; explicit Stop and GUI not retested',passed,tempos,expected,notesPassed,packetIntegrity,maxGapFrames,firstSignalTime:envelope.find(p=>p.rms>.0001)?.time??null,candidates,onsets,intervals,nominal,intervalPassed,spectra,pitchPassed,baselineRms:baseline,tailRms:tail,peak,runSha256:hash(file('run.json')),wavSha256:hash(file('output.wav')),nativeSha256:hash(file('tempo/notes.json')),analyzerSha256:hash(new URL(import.meta.url)),fullAcceptance:false};
fs.writeFileSync(file('tempo-audio-proof.json'),JSON.stringify(report,null,2)+'\n');fs.copyFileSync(new URL(import.meta.url),file('tempo-audio-auditor.mjs'));console.log(JSON.stringify(report,null,2));if(!passed)process.exitCode=1;

import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [audioArg, unitArg] = process.argv.slice(2);
assert(unitArg, 'Usage: Inspect-FileOutputMultiGuiAudio.mjs AUDIO_CAPTURE UNIT');
const dir=path.resolve(audioArg), unit=path.resolve(unitArg);
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const run=read(dir+'/run.json'), capture=read(dir+'/capture.json'), ready=read(dir+'/ready.json');
const launch=read(dir+'/gui-launch-at-capture.json'), build=read(run.buildSummary);
const candidate=path.basename(path.dirname(run.buildSummary));
assert(build.passed&&build.sourceSnapshotUnchanged);
assert.equal(run.state,'exited');assert.equal(run.captureExitCode,0);assert(capture.passed);
assert.equal(run.timingBasis,'recorder-start-utc-qpc');assert.equal(ready.schema,2);
assert.equal(run.processId,launch.processId);assert.equal(run.exeSha256,launch.exeSha256);
for(const [p,h] of [[run.buildSummary,run.buildSummarySha256],[run.executable,run.exeSha256],
 [run.recorder,run.recorderSha256],[run.recorderBuildSummary,run.recorderBuildSummarySha256],
 [dir+'/ready.json',run.captureReadySha256],...run.inputs.map(i=>[i.path,i.sha256]),
 ...launch.inputs.map(i=>[i.path,i.sha256])])assert.equal(hash(p),h);
for(const s of build.sources)assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);
for(const o of build.outputs)assert.equal(hash(path.join(path.dirname(run.buildSummary),o.path)),o.sha256);
const exe=build.outputs.find(o=>o.path==='install/bin/Producer.exe');assert.equal(exe.sha256,run.exeSha256);
const saved=read(unit+'/final-inputs.json');assert.equal(saved.candidate,candidate);assert.equal(saved.inputs.length,5);
assert.equal(run.inputs.length,5);
for(const i of saved.inputs){assert.equal(hash(i.path),i.sha256);assert.equal(hash(i.sourcePath),i.sha256);
 assert(run.inputs.some(r=>path.resolve(r.path)===path.resolve(i.path)&&r.sha256===i.sha256));}
const author=read(unit+'/author-exit-observer.json'), reload=read(unit+'/reload-session/exit-observer.json');
assert.notEqual(author.processId,reload.processId);assert.equal(run.processId,reload.processId);
for(const e of [author,reload]){assert.equal(e.state,'exited');assert.equal(e.exitCode,0);
 assert.equal(e.forcedTermination,false);assert.equal(e.exeSha256,exe.sha256);assert.equal(hash(e.executable),exe.sha256);
 assert.equal(e.driverSha256,hash('scripts/Watch-ProductGuiExit.ps1'));}
assert(Date.parse(author.exitUtc)<Date.parse(reload.startUtc));
const observations=read(unit+'/observations/states.json');
const apRestore=observations.find(o=>o.label==='Reload restored AudioPath FileOutput count two');
assert(apRestore&&/FileOutput effects: 2/.test(apRestore.accessibility.tree));
const actions=read(run.guiRun+'/actions.json'), action=name=>{const a=actions.filter(a=>a.action===name);assert.equal(a.length,1,name);return a[0];};
const normalize=p=>p.replaceAll('\\','/').toLowerCase();
for(const name of ['Play first','Stop first','Play replay','Stop recording','Stop replay']){
 const a=action(name);assert.equal(a.window,launch.window?.id);
 assert(Date.parse(a.utc)>=Date.parse(a.beforeUtc)&&Date.parse(a.utc)-Date.parse(a.beforeUtc)<500);
 assert(observations.some(o=>o.window.id===a.window&&normalize(o.window.app)==='process:'+normalize(run.executable)
  &&Date.parse(o.utc)<=Date.parse(a.beforeUtc)));
}
const start=Date.parse(run.captureStartUtc);
const fileTime=BigInt(ready.utcBeforeFileTime)+(BigInt(ready.utcAfterFileTime)-BigInt(ready.utcBeforeFileTime))/2n;
const calibrated=Number((fileTime-116444736000000000n)/10000n);
assert(Math.abs(start-calibrated)<=1,'Capture UTC must be derived from recorder clock');
assert(BigInt(ready.utcAfterFileTime)>=BigInt(ready.utcBeforeFileTime));
assert(BigInt(ready.qpcFrequency)>0n&&BigInt(ready.startQpcTicks)>0n);
const time=a=>(Date.parse(a.beforeUtc)-start)/1000;
const recordingStart=time(action('Start recording')), recordingStop=time(action('Stop recording'));
assert(recordingStart>1&&recordingStop>recordingStart&&recordingStop<capture.seconds-2);
const outputProofPath=unit+'/recordings/file-output-multi-pcm-proof.json', outputProof=read(outputProofPath);
assert(outputProof.passed&&!outputProof.fullAcceptance);assert.equal(outputProof.candidate,candidate);
assert.equal(outputProof.buildSummarySha256,run.buildSummarySha256);
assert.equal(outputProof.auditorSha256,hash('scripts/Inspect-FileOutputMultiPcm.mjs'));
for(const f of outputProof.recordings)assert.equal(hash(f.path),f.sha256);

function chunks(b,a=0,z=b.length){const cs=[];for(let p=a;p<z;){assert(p+8<=z);const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),e=p+8+n;assert(e+(n&1)<=z);cs.push({id,data:b.subarray(p+8,e)});p=e+(n&1);}return cs;}
const one=(cs,id)=>{const a=cs.filter(c=>c.id===id);assert.equal(a.length,1);return a[0].data;};
const wav=fs.readFileSync(dir+'/output.wav');assert.equal(wav.toString('ascii',0,4),'RIFF');
assert.equal(wav.toString('ascii',8,12),'WAVE');assert.equal(wav.readUInt32LE(4)+8,wav.length);
const wc=chunks(wav,12),fmt=one(wc,'fmt '),data=one(wc,'data');
assert.equal(fmt.readUInt16LE(14),32);
assert(fmt.readUInt16LE(0)===3||(fmt.readUInt16LE(0)===65534&&fmt.length>=40&&fmt.readUInt32LE(24)===3));
const rate=fmt.readUInt32LE(4),channels=fmt.readUInt16LE(2),align=fmt.readUInt16LE(12);
assert.equal(align,channels*4);assert.equal(rate,ready.sampleRate);assert.equal(channels,ready.channels);
assert.equal(data.length%align,0);const frames=data.length/align,mono=new Float64Array(frames);
assert.equal(frames,rate*capture.seconds);let peak=0;
for(let i=0;i<frames;i++)for(let c=0;c<channels;c++){const x=data.readFloatLE(i*align+c*4);assert(Number.isFinite(x));mono[i]+=x/channels;peak=Math.max(peak,Math.abs(x));}
function rms(a,z){assert(a>=0&&z<=capture.seconds&&z>a);let sum=0,n=0;for(let i=Math.round(a*rate);i<Math.round(z*rate);i++){sum+=mono[i]**2;n++;}return Math.sqrt(sum/n);}
function tone(a,z,midi){const coeff=2*Math.cos(2*Math.PI*(440*2**((midi-69)/12))/(rate/4));let s1=0,s2=0,n=0;for(let i=Math.round(a*rate);i<Math.round(z*rate);i+=4){const w=.5-.5*Math.cos(2*Math.PI*(i/rate-a)/(z-a)),s=mono[i]*w+coeff*s1-s2;s2=s1;s1=s;n++;}return Math.sqrt(Math.max(0,s1*s1+s2*s2-coeff*s1*s2))/n;}
const onsets=[];let active=false,quietSince=0;
for(let t=.01;t+.01<capture.seconds;t+=.01){const level=rms(t,t+.01);if(level<.0001){if(active){quietSince=t;active=false;}}else if(level>.003&&!active){if(t-quietSince>.1)onsets.push(t);active=true;}}
assert.equal(onsets.length,2,'Two GUI Play commands must each reach loopback once');
const plays=['first','replay'].map((phase,index)=>{
 const from=time(action('Play '+phase)),to=time(action('Stop '+phase)),onset=onsets[index];
 assert(from>recordingStart&&to>from&&to<capture.seconds-2);assert(onset>=from-.04&&onset-from<3);
 const sound=[60,69].map(midi=>{const own=tone(onset+.15,onset+.45,midi),lower=tone(onset+.15,onset+.45,midi-1),upper=tone(onset+.15,onset+.45,midi+1);assert(own>.0001&&own>lower*1.3&&own>upper*1.3,'Both owned routed DLS pitches must reach loopback');return {midi,own,lower,upper};});
 const level=rms(onset+.15,onset+.45),quiet=rms(to+1,Math.min(to+2,capture.seconds));assert(level>.002&&quiet<.0001);
 const beforeStop=rms(to-.3,to-.1),activeStopEstablished=to-onset<4&&beforeStop>.002;
 // The source DMO records before the render endpoint. It has no UTC/QPC clock.
 // Compare the two onset intervals, retaining the observed offset separately.
 const fileOutputOffsets=outputProof.recordings.map(f=>({name:f.name,seconds:onset-recordingStart-f.onsets[index]}));
 return {phase,from,to,onset,level,sound,quiet,beforeStop,activeStopEstablished,fileOutputOffsets};
});
for(const f of outputProof.recordings)assert(Math.abs((plays[1].onset-plays[0].onset)-(f.onsets[1]-f.onsets[0]))<.04,
 'Independent FileOutput and loopback clocks must retain the same interval between the two GUI starts');
const growth=read(unit+'/recording-growth.json'),final=read(unit+'/recording-final.json');
assert(Date.parse(growth.before.utc)>Date.parse(action('Stop first').utc));
assert(Date.parse(growth.after.utc)<Date.parse(action('Play replay').beforeUtc));
assert(Date.parse(growth.after.utc)-Date.parse(growth.before.utc)>=400);
for(const name of ['Record.wav','Record1.wav']){
 const a=growth.before.files.find(f=>f.name===name),z=growth.after.files.find(f=>f.name===name);assert(a&&z&&z.bytes>a.bytes);
 const x=final.before.files.find(f=>f.name===name),y=final.after.files.find(f=>f.name===name),f=outputProof.recordings.find(f=>f.name===name);
 assert(x&&y&&f);assert.equal(x.bytes,y.bytes);assert.equal(x.sha256,y.sha256);assert.equal(y.sha256,f.sha256);
}
assert(Date.parse(final.before.utc)>Date.parse(action('Stop recording').utc));
assert(Date.parse(final.after.utc)-Date.parse(final.before.utc)>=400);
const duringReplay=recordingStop>=plays[1].onset&&recordingStop<plays[1].onset+4;
const recordingStopWhileAudibleEstablished=duringReplay&&rms(recordingStop+.1,recordingStop+.3)>.002;
const packets=fs.readFileSync(dir+'/packets.csv','utf8').trim().split(/\r?\n/).slice(1).map(s=>s.split(',').map(Number));let maxGapFrames=0;
assert(packets.length&&packets.every((p,i)=>p.every(Number.isFinite)&&(p[2]&4)===0&&(!(p[2]&1)||i===0)));
for(let i=1;i<packets.length;i++){assert(packets[i][0]>=packets[i-1][0]);maxGapFrames=Math.max(maxGapFrames,packets[i][0]-packets[i-1][0]-packets[i-1][1]);}assert(maxGapFrames<=rate*.002);
const baseline=rms(.3,Math.min(2,recordingStart-.5)),tail=rms(capture.seconds-3,capture.seconds-.5);assert(baseline<.0001&&tail<.0001&&peak<.99);
const proof={schema:1,passed:true,candidate,processId:run.processId,scope:'Saved/reopened main Project: two directly routed owned DLS pitches on both Play commands, two separate numbered FileOutput WAVs, musical Stop keeps files growing and recording Stop finalizes both. Active GUI interruption and tempo change remain separate.',
 plays,onsets,recordingStart,recordingStop,recordingStopWhileAudibleEstablished,activeGuiStopsPassed:plays.every(p=>p.activeStopEstablished),fileOutputClock:'Uncalibrated pre-render DMO; interval agreement and measured relative offset, not absolute UTC alignment',baselineRms:baseline,tailRms:tail,peak,maxGapFrames,
 buildSummarySha256:run.buildSummarySha256,runSha256:hash(dir+'/run.json'),wavSha256:hash(dir+'/output.wav'),actionsSha256:hash(run.guiRun+'/actions.json'),observationsSha256:hash(unit+'/observations/states.json'),fileOutputProofSha256:hash(outputProofPath),analyzerSha256:hash(process.argv[1]),fullAcceptance:false};
fs.writeFileSync(dir+'/file-output-multi-gui-audio-proof.json',JSON.stringify(proof,null,2)+'\n');fs.copyFileSync(process.argv[1],dir+'/file-output-multi-gui-audio-auditor.mjs');console.log(JSON.stringify(proof));

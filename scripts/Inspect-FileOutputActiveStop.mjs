import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [audioArg, unitArg] = process.argv.slice(2);
assert(unitArg, 'Usage: Inspect-FileOutputActiveStop.mjs AUDIO UNIT');
const dir=path.resolve(audioArg), unit=path.resolve(unitArg);
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const normalize=p=>path.resolve(p).replaceAll('\\','/').toLowerCase();
const run=read(dir+'/run.json'), ready=read(dir+'/ready.json'), capture=read(dir+'/capture.json');
const launch=read(dir+'/gui-launch-at-capture.json'), build=read(run.buildSummary);
const candidate=path.basename(path.dirname(run.buildSummary));
assert(build.passed&&build.sourceSnapshotUnchanged);
assert.equal(run.state,'exited'); assert.equal(run.captureExitCode,0); assert(capture.passed);
assert.equal(ready.schema,2); assert.equal(run.timingBasis,'recorder-start-utc-qpc');
assert.equal(run.processId,launch.processId); assert.equal(run.exeSha256,launch.exeSha256);
for(const [file,digest] of [[run.buildSummary,run.buildSummarySha256],[run.executable,run.exeSha256],
 [run.recorder,run.recorderSha256],[run.recorderBuildSummary,run.recorderBuildSummarySha256],
 [dir+'/ready.json',run.captureReadySha256],[dir+'/driver.ps1',run.driverSha256],
 ...run.inputs.map(i=>[i.path,i.sha256]),...launch.inputs.map(i=>[i.path,i.sha256])])assert.equal(hash(file),digest);
for(const s of build.sources)for(const root of [build.sourceRoot,process.cwd()])assert.equal(hash(path.join(root,s.path)),s.sha256);
for(const o of build.outputs)assert.equal(hash(path.join(path.dirname(run.buildSummary),o.path)),o.sha256);
const recorder=read(run.recorderBuildSummary);
for(const s of recorder.sources)assert.equal(hash(path.join(path.dirname(run.recorderBuildSummary),'sources',s.path)),s.sha256);
assert.equal(build.outputs.find(o=>o.path==='install/bin/Producer.exe').sha256,run.exeSha256);
const saved=read(unit+'/saved-inputs.json'); assert.equal(saved.candidate,candidate); assert.equal(saved.inputs.length,5);
assert.equal(run.inputs.length,5); assert.equal(launch.inputs.length,5);
for(const i of saved.inputs){assert.equal(hash(i.path),i.sha256);
 assert(run.inputs.some(r=>normalize(r.path)===normalize(i.path)&&r.sha256===i.sha256));}
const gui=run.guiRun, author=read(unit+'/author-exit.json'), reload=read(gui+'/exit-observer.json');
assert.notEqual(author.processId,reload.processId,'Separate process restore required'); assert.equal(reload.processId,run.processId);
for(const e of [author,reload]){assert.equal(e.state,'exited');assert.equal(e.exitCode,0);assert.equal(e.forcedTermination,false);
 assert.equal(e.exeSha256,run.exeSha256);assert.equal(hash(e.executable),run.exeSha256);
 assert.equal(e.driverSha256,hash('scripts/Watch-ProductGuiExit.ps1'));}
assert(Date.parse(author.exitUtc)<Date.parse(reload.startUtc));
const observations=fs.readFileSync(unit+'/recovery-states.jsonl','utf8').trim().split(/\r?\n/).map(JSON.parse);
for(const [label,pattern] of [['native-project-reopened',/Duration \(clocks\) Value: 122880/],
 ['restore-path-file-menu',/MIDI pitch Value: 60[\s\S]*Duration \(clocks\) Value: 122880[\s\S]*PChannel.*Value: 9/],
 ['restored-audiopath-two-effects',/FileOutput effects: 2/],
 ['record-stopped-music-continues',/Buffer recording stopped and WAV finalized/],
 ['reload-clean-before-exit',/Stopped\. Applied groups: 1; Tempo track 1; Time signature track 1\. Notes: 2/]]){
 const o=observations.find(o=>o.label===label&&pattern.test(o.accessibility.tree));assert(o,label);
 assert.equal(o.window.app.replaceAll('\\','/').toLowerCase(),'process:'+normalize(run.executable));
 assert(Date.parse(o.utc)>=Date.parse(reload.startUtc)&&Date.parse(o.utc)<Date.parse(reload.exitUtc));
}
const actions=read(gui+'/actions.json');
const action=name=>{const a=actions.filter(a=>a.name===name);assert.equal(a.length,1,name);return a[0];};
for(const name of ['record-start','play-1','stop-1','play-2','record-stop','stop-2','play-3','stop-3']){
 const a=action(name);assert.equal(a.processId,run.processId);assert.equal(a.window.id,launch.uiWindow.id);
 assert.equal(a.window.app.replaceAll('\\','/').toLowerCase(),'process:'+normalize(run.executable));
 assert(Date.parse(a.utc)>=Date.parse(a.beforeUtc)&&Date.parse(a.utc)-Date.parse(a.beforeUtc)<500,'Bounded action timestamp');
 assert(observations.some(o=>o.window.id===a.window.id&&Date.parse(o.utc)<=Date.parse(a.beforeUtc)));
}
const start=Date.parse(run.captureStartUtc), fileTime=BigInt(ready.utcBeforeFileTime)+(BigInt(ready.utcAfterFileTime)-BigInt(ready.utcBeforeFileTime))/2n;
assert(Math.abs(start-Number((fileTime-116444736000000000n)/10000n))<=1,'Capture UTC calibration');
assert(BigInt(ready.utcAfterFileTime)>=BigInt(ready.utcBeforeFileTime));assert(BigInt(ready.qpcFrequency)>0n&&BigInt(ready.startQpcTicks)>0n);
const time=name=>(Date.parse(action(name).beforeUtc)-start)/1000;
const recordingStart=time('record-start'), recordingStop=time('record-stop');
assert(recordingStart>2&&recordingStop>recordingStart&&recordingStop<capture.seconds-2);
const outputPath=gui+'/recordings/file-output-multi-pcm-proof.json', output=read(outputPath);
assert(output.passed&&!output.fullAcceptance);assert.equal(output.candidate,candidate);
assert.equal(output.scenario,'--active-stop-long-notes');assert.equal(output.expectedDuration,122880);
assert.equal(output.buildSummarySha256,run.buildSummarySha256);assert.equal(output.auditorSha256,hash('scripts/Inspect-FileOutputMultiPcm.mjs'));
for(const i of output.inputs)assert(run.inputs.some(r=>normalize(r.path)===normalize(i.path)&&r.sha256===i.sha256));
for(const f of output.recordings)assert.equal(hash(f.path),f.sha256);
const before=read(gui+'/recording-final-before.json'), final=read(gui+'/recording-final.json'), after=read(gui+'/recording-after-replay.json');
assert.deepEqual(final.before,before);
assert(Date.parse(before.utc)>Date.parse(action('record-stop').utc));
assert(Date.parse(final.after.utc)>Date.parse(action('stop-2').utc));
assert(Date.parse(after.utc)>Date.parse(action('stop-3').utc));
for(const f of output.recordings){for(const s of [before,final.after,after]){
 const v=s.files.find(v=>v.name===f.name);assert(v&&v.bytes>44);assert.equal(v.sha256,f.sha256);assert.equal(v.bytes,fs.statSync(f.path).size);
}}
function chunks(b,a=0,z=b.length){const cs=[];for(let p=a;p<z;){assert(p+8<=z);const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),e=p+8+n;assert(e+(n&1)<=z);cs.push({id,data:b.subarray(p+8,e)});p=e+(n&1);}return cs;}
const one=(cs,id)=>{const a=cs.filter(c=>c.id===id);assert.equal(a.length,1);return a[0].data;};
function pcm(file){const wav=fs.readFileSync(file);assert.equal(wav.toString('ascii',0,4),'RIFF');assert.equal(wav.toString('ascii',8,12),'WAVE');assert.equal(wav.readUInt32LE(4)+8,wav.length);
 const cs=chunks(wav,12),fmt=one(cs,'fmt '),data=one(cs,'data'),tag=fmt.readUInt16LE(0),bits=fmt.readUInt16LE(14),rate=fmt.readUInt32LE(4),channels=fmt.readUInt16LE(2),align=fmt.readUInt16LE(12);
 assert(tag===1||tag===3||(tag===65534&&fmt.length>=40&&fmt.readUInt32LE(24)===3));assert(bits===16||bits===32);assert.equal(align,channels*bits/8);assert.equal(data.length%align,0);
 const frames=data.length/align,seconds=frames/rate,mono=new Float64Array(frames);let peak=0;
 for(let i=0;i<frames;i++)for(let c=0;c<channels;c++){const p=i*align+c*bits/8,x=tag===1?data.readInt16LE(p)/32768:data.readFloatLE(p);assert(Number.isFinite(x));mono[i]+=x/channels;peak=Math.max(peak,Math.abs(x));}
 const rms=(a,z)=>{assert(a>=0&&z<=seconds&&z>a);let sum=0,n=0;for(let i=Math.round(a*rate);i<Math.round(z*rate);i++){sum+=mono[i]**2;n++;}return Math.sqrt(sum/n);};
 const tone=(a,z,midi)=>{const coefficient=2*Math.cos(2*Math.PI*(440*2**((midi-69)/12))/(rate/4));let s1=0,s2=0,n=0;for(let i=Math.round(a*rate);i<Math.round(z*rate);i+=4){const w=.5-.5*Math.cos(2*Math.PI*(i/rate-a)/(z-a)),s=mono[i]*w+coefficient*s1-s2;s2=s1;s1=s;n++;}return Math.sqrt(Math.max(0,s1*s1+s2*s2-coefficient*s1*s2))/n;};
 const onsets=[];let active=false,quietSince=0;for(let t=.01;t+.01<seconds;t+=.01){const level=rms(t,t+.01);if(level<.0001){if(active){quietSince=t;active=false;}}else if(level>.003&&!active){if(t-quietSince>.1)onsets.push(t);active=true;}}
 return {rate,channels,frames,seconds,peak,rms,tone,onsets,tag,bits};
}
const loop=pcm(dir+'/output.wav');assert.equal(loop.rate,ready.sampleRate);assert.equal(loop.channels,ready.channels);assert.equal(loop.frames,loop.rate*capture.seconds);assert.equal(loop.bits,32);assert.equal(loop.onsets.length,3,'Every Play, including after recording Stop, must reach loopback once');
function pitches(p,a,z,notes){assert(p.rms(a,z)>.002,'Audible sustained PCM required');return notes.map(midi=>{
 const own=p.tone(a,z,midi),lower=p.tone(a,z,midi-1),upper=p.tone(a,z,midi+1);assert(own>.0001&&own>lower*1.3&&own>upper*1.3,'Expected DLS pitch required');return {midi,own,lower,upper};});}
const plays=[1,2,3].map((phase,index)=>{const from=time('play-'+phase),to=time('stop-'+phase),onset=loop.onsets[index];
 assert(from>recordingStart&&to-from>4&&to-onset<160&&to<capture.seconds-3);assert(onset>=from-.04&&onset-from<3);
 const next=index<2?time('play-'+(phase+1)):capture.seconds-.5;assert(next-to>3);
 const onsetPitches=pitches(loop,onset+.15,onset+.45,[60,69]),beforeStop=pitches(loop,to-.4,to-.1,[60,69]);
 const quiet=loop.rms(to+1,Math.min(to+2,next-.3));assert(quiet<.0001,'Musical Stop silence required');
 for(let a=to+1;a+.25<next-.3;a+=.25)assert(loop.rms(a,a+.25)<.0001,'No sound between Stop and next Play');
 return {phase,from,to,onset,onsetPitches,beforeStop,quiet,activeStopEstablished:true};
});
assert(recordingStop>plays[1].onset+1&&recordingStop<plays[1].to-2,'Recording Stop must interrupt live second playback');
const beforeRecordingStop=pitches(loop,recordingStop-.4,recordingStop-.1,[60,69]),afterRecordingStop=pitches(loop,recordingStop+.1,recordingStop+.4,[60,69]);
let minimumTransitionRms=Infinity;for(let t=recordingStop-.5;t<recordingStop+1;t+=.01)minimumTransitionRms=Math.min(minimumTransitionRms,loop.rms(t,t+.01));assert(minimumTransitionRms>.002,'Recording Stop must preserve audible transport without a silent interruption');
const recorded=output.recordings.map(f=>{const p=pcm(f.path),note=f.name==='Record.wav'?69:60,other=note===69?60:69;
 assert.equal(p.onsets.length,2);assert(Math.abs((plays[1].onset-plays[0].onset)-(p.onsets[1]-p.onsets[0]))<.04,'DMO and endpoint interval agreement');
 assert(p.seconds-p.onsets[1]>1&&p.seconds-p.onsets[1]<160);const endPitches=pitches(p,p.seconds-.5,p.seconds-.2,[note]);assert(endPitches[0].own>p.tone(p.seconds-.5,p.seconds-.2,other)*10,'Finalized output remains exclusive while live music continues');
 return {name:f.name,seconds:p.seconds,endPitches,fileOutputOffsets:p.onsets.map((t,i)=>plays[i].onset-recordingStart-t)};
});
const packets=fs.readFileSync(dir+'/packets.csv','utf8').trim().split(/\r?\n/).slice(1).map(s=>s.split(',').map(Number));let maxGapFrames=0;
assert(packets.length&&packets.every((p,i)=>p.every(Number.isFinite)&&(p[2]&4)===0&&(!(p[2]&1)||i===0)));
for(let i=1;i<packets.length;i++){assert(packets[i][0]>=packets[i-1][0]);maxGapFrames=Math.max(maxGapFrames,packets[i][0]-packets[i-1][0]-packets[i-1][1]);}assert(maxGapFrames<=loop.rate*.002);
assert.equal(capture.timestampErrors,0);const baselineRms=loop.rms(.3,2),tailRms=loop.rms(capture.seconds-3,capture.seconds-.5);assert(baselineRms<.0001&&tailRms<.0001&&loop.peak<.99);
const proof={schema:1,passed:true,candidate,processId:run.processId,scope:'Main native long-note restore: active recording Stop preserves both routed pitches, finalized exclusive numbered WAVs stay immutable through musical Stop/replay; three active musical Stops produce silence. No tempo-change or original-dynamic/all40/all8 claim.',
 recordingStart,recordingStop,plays,beforeRecordingStop,afterRecordingStop,minimumTransitionRms,recorded,baselineRms,tailRms,peak:loop.peak,maxGapFrames,
 activeGuiStopsPassed:true,recordingStopWhileAudibleEstablished:true,finalizedFilesImmutable:true,
 fileOutputClock:'DMO lacks UTC/QPC; two-start interval agreement and measured relative offsets only',
 buildSummarySha256:run.buildSummarySha256,runSha256:hash(dir+'/run.json'),wavSha256:hash(dir+'/output.wav'),packetsSha256:hash(dir+'/packets.csv'),endpointSha256:hash(dir+'/endpoint.txt'),actionsSha256:hash(gui+'/actions.json'),observationsSha256:hash(unit+'/recovery-states.jsonl'),fileOutputProofSha256:hash(outputPath),authorExitSha256:hash(unit+'/author-exit.json'),reloadExitSha256:hash(gui+'/exit-observer.json'),analyzerSha256:hash(process.argv[1]),fullAcceptance:false};
fs.writeFileSync(dir+'/file-output-active-stop-proof.json',JSON.stringify(proof,null,2)+'\n');fs.copyFileSync(process.argv[1],dir+'/file-output-active-stop-auditor.mjs');console.log(JSON.stringify(proof));

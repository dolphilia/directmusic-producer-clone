import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {captureClock} from './AudioCaptureClock.mjs';
import {inspectWavesFixture} from './Inspect-WavesReverbFixture.mjs';

const json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const normalize=p=>path.resolve(p).toLowerCase();
export function readWavesCapture(file){
  const b=fs.readFileSync(file);assert.equal(b.toString('ascii',0,4),'RIFF');
  assert.equal(b.toString('ascii',8,12),'WAVE');assert.equal(b.readUInt32LE(4)+8,b.length);
  let fmt,data;for(let p=12;p<b.length;){assert(p+8<=b.length);const n=b.readUInt32LE(p+4),z=p+8+n;assert(z+(n&1)<=b.length);const id=b.toString('ascii',p,p+4);if(id==='fmt '){assert(!fmt);fmt=b.subarray(p+8,z);}if(id==='data'){assert(!data);data=b.subarray(p+8,z);}p=z+(n&1);}
  assert(fmt&&data);assert.equal(fmt.readUInt16LE(14),32);
  assert(fmt.readUInt16LE(0)===3||(fmt.readUInt16LE(0)===65534&&fmt.length>=40&&fmt.readUInt32LE(24)===3));
  const rate=fmt.readUInt32LE(4),channels=fmt.readUInt16LE(2),align=fmt.readUInt16LE(12);assert.equal(align,channels*4);assert.equal(data.length%align,0);
  const frames=data.length/align,mono=new Float64Array(frames),energy=new Float64Array(frames);let peak=0;
  for(let i=0;i<frames;i++)for(let c=0;c<channels;c++){const x=data.readFloatLE(i*align+c*4);assert(Number.isFinite(x));mono[i]+=x/channels;energy[i]+=x*x/channels;peak=Math.max(peak,Math.abs(x));}
  return {rate,channels,frames,mono,energy,peak};
}

// Shared numeric acceptance is also exercised by independent fabricated PCM controls.
export function analyzeWavesPcm(recorded,times,protocol){
  const {rate,frames,mono,energy,peak}=recorded,c=protocol.criteria,seconds=frames/rate;
  const rms=(a,z)=>{assert(a>=0&&z>a&&z<=seconds,'PCM window in recording');let sum=0;const from=Math.ceil(a*rate),to=Math.floor(z*rate);for(let i=from;i<to;i++)sum+=energy[i];return Math.sqrt(sum/(to-from));};
  const tone=(a,z,midi)=>{const frequency=440*2**((midi-69)/12),step=4,coefficient=2*Math.cos(2*Math.PI*frequency/(rate/step));let s1=0,s2=0,n=0;for(let i=Math.ceil(a*rate);i<Math.floor(z*rate);i+=step){const w=.5-.5*Math.cos(2*Math.PI*(i/rate-a)/(z-a)),s=mono[i]*w+coefficient*s1-s2;s2=s1;s1=s;n++;}return 2*Math.sqrt(Math.max(0,s1*s1+s2*s2-coefficient*s1*s2))/n;};
  assert(peak<c.peakMaximum,'Clipping peak');
  const baseline=rms(.5,protocol.recording.baselineSeconds);assert(baseline<=c.baselineRmsMaximum,'Baseline silence');
  assert(times.first.before>=protocol.recording.baselineSeconds);
  const firstDuration=times.stopFirst.before-times.first.after,replayDuration=times.stopReplay.before-times.replay.after;
  const within=(n,range)=>n>=range[0]&&n<=range[1];
  assert(within(firstDuration,protocol.recording.firstStopAfterPlaySeconds),'First Stop timing');
  assert(within(replayDuration,protocol.recording.replayStopAfterPlaySeconds),'Replay Stop while notes active');
  const plays=[['first',times.first,times.stopFirst],['replay',times.replay,times.stopReplay]].map(([name,play,stop])=>{
    let onset;for(let t=Math.max(0,play.before-.04);t<play.after+3;t+=.01)if(rms(t,t+.01)>=c.activeRmsMinimum){onset=t;break;}
    assert(onset!==undefined,'Play reaches render endpoint');
    const from=onset+1,to=onset+2,level=rms(from,to);assert(level>=c.activeRmsMinimum,'Active signal');
    const tones=protocol.input.expectedMidi.map(midi=>{const own=tone(from,to,midi),lower=tone(from,to,midi-1),upper=tone(from,to,midi+1);assert(own>=c.toneAmplitudeMinimum&&own>=Math.max(lower,upper)*c.expectedVsAdjacentSemitoneRatioMinimum,'Both expected DLS pitches');return {midi,own,lower,upper};});
    const quiet=rms(stop.after+protocol.recording.stopQuietDelaySeconds,stop.after+protocol.recording.stopQuietDelaySeconds+protocol.recording.stopQuietSeconds);assert(quiet<=c.stopRmsMaximum,'Stop silence');
    const beforeStop=rms(stop.before-.4,stop.before-.1);if(name==='replay')assert(beforeStop>=c.activeRmsMinimum,'Replay Stop interrupts active PCM');
    return {name,onset,activeRms:level,tones,quietRms:quiet,beforeStopRms:beforeStop};
  });
  const end=plays[0].onset+protocol.input.noteSeconds,tailWindow=protocol.criteria.naturalTailWindowAfterNoteoffSeconds;
  const naturalTailRms=rms(end+tailWindow[0],end+tailWindow[1]);
  const lateSustainRms=rms(end-2,end-1);assert(lateSustainRms>=c.activeRmsMinimum,'Input sustains through expected note duration');
  const lateTones=protocol.input.expectedMidi.map(midi=>{const own=tone(end-2,end-1,midi),adjacent=Math.max(tone(end-2,end-1,midi-1),tone(end-2,end-1,midi+1));assert(own>=c.toneAmplitudeMinimum&&own>=adjacent*c.expectedVsAdjacentSemitoneRatioMinimum,'Both pitches sustain through expected note duration');return {midi,own,adjacent};});
  const final=rms(times.close.after+2,times.close.after+4);assert(final<=c.finalRmsMaximum,'Normal close silence');
  assert(times.close.after+5<=seconds,'Close quiet interval retained');
  return {passed:true,baselineRms:baseline,finalRms:final,peak,firstDuration,replayDuration,plays,naturalTailRms,naturalNoteoffSeconds:end,lateSustainRms,lateTones};
}

export function compareWavesPcm(dry,wet,protocol){
  const amplitudes=r=>Object.fromEntries(r.plays[0].tones.map(t=>[t.midi,t.own]));
  const d=amplitudes(dry),w=amplitudes(wet),dryRatio=d[69]/d[60],wetRatio=w[69]/w[60];
  const unaffectedRelativeChange=Math.abs(w[60]/d[60]-1),relativeToneRatioChange=Math.abs(wetRatio/dryRatio-1);
  assert(unaffectedRelativeChange<=protocol.criteria.unaffectedPitchRatioTolerance,'Unaffected route stability');
  const levelDifference=relativeToneRatioChange>=protocol.criteria.relativeToneRatioChangeMinimum;
  const tailDifference=wet.naturalTailRms>=protocol.criteria.wetTailRmsMinimum&&wet.naturalTailRms>=dry.naturalTailRms*protocol.criteria.wetDryTailRatioMinimum;
  assert(levelDifference||tailDifference,'Fixed dry/wet effect difference');
  return {passed:true,dryRatio,wetRatio,relativeToneRatioChange,unaffectedRelativeChange,levelDifference,tailDifference,dryTailRms:dry.naturalTailRms,wetTailRms:wet.naturalTailRms};
}

export function inspectWavesRun(directory,variant,protocolPath){
  const dir=path.resolve(directory),protocol=json(protocolPath),run=json(path.join(dir,'run.json'));
  const launch=json(path.join(dir,'gui-launch-at-capture.json')),build=json(run.buildSummary),capture=json(path.join(dir,'capture.json')),ready=json(path.join(dir,'ready.json'));
  assert.equal(run.state,'exited');assert.equal(run.captureExitCode,0);assert(capture.passed);
  assert(build.passed&&build.sourceSnapshotUnchanged);assert.equal(run.exeSha256,protocol.producerSha256);
  assert.equal(launch.variant,variant);assert.equal(launch.candidate,protocol.candidate);
  assert.equal(launch.protocol.sha256,hash(protocolPath));assert.equal(run.processId,launch.processId);
  assert.equal(run.durationSeconds,protocol.recording.durationSeconds);assert.equal(capture.seconds,protocol.recording.durationSeconds);
  assert.equal(run.silentKeepAlive,protocol.recording.silentKeepAlive);
  for(const [p,h] of [[run.executable,run.exeSha256],[run.buildSummary,run.buildSummarySha256],[run.recorder,run.recorderSha256],[run.recorderBuildSummary,run.recorderBuildSummarySha256],[path.join(dir,'ready.json'),run.captureReadySha256],...run.inputs.map(i=>[i.path,i.sha256])])assert.equal(hash(p),h,'Capture dependency hash');
  assert.equal(run.buildSummarySha256,protocol.build.sha256);
  assert.equal(hash(path.join(dir,'driver.ps1')),run.driverSha256,'Saved capture driver');
  const recorder=json(run.recorderBuildSummary);assert(recorder.passed);assert.equal(recorder.sha256,run.recorderSha256);
  for(const s of recorder.sources)assert.equal(hash(path.join(path.dirname(run.recorderBuildSummary),'sources',s.path)),s.sha256,'Saved recorder source');
  for(const s of build.sources)assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256,'Saved source');
  for(const o of build.outputs)assert.equal(hash(path.join(path.dirname(run.buildSummary),o.path)),o.sha256,'Saved product');
  const expected=protocol.inputs.filter(i=>i.variant===variant);assert.equal(expected.length,5);assert.equal(run.inputs.length,6);
  for(const i of expected)assert(run.inputs.some(r=>normalize(r.path)===normalize(i.absolutePath)&&r.sha256===i.sha256),'Exact variant inputs');
  assert(run.inputs.some(r=>normalize(r.path)===normalize(protocolPath)&&r.sha256===hash(protocolPath)));
  const identity=json(path.join(run.guiRun,'identity.json')),exit=json(path.join(run.guiRun,'exit.json'));
  assert.equal(identity.pid,run.processId);assert.equal(identity.sha256,run.exeSha256);assert.equal(exit.processId,run.processId);
  assert.equal(exit.startUtc,identity.startUtc);assert.equal(exit.sha256,identity.sha256);assert.equal(exit.exitCode,0);
  const recorded=readWavesCapture(path.join(dir,'output.wav'));assert.equal(recorded.rate,ready.sampleRate);assert.equal(recorded.channels,ready.channels);assert.equal(recorded.frames,recorded.rate*capture.seconds);
  const packets=fs.readFileSync(path.join(dir,'packets.csv'),'utf8').trim().split(/\r?\n/).slice(1).map(l=>l.split(',').map(Number));
  const clock=captureClock(ready,capture,packets,recorded.rate);assert.equal(capture.timestampErrors,0);
  let maxGapFrames=0;for(let i=0;i<packets.length;i++){const p=packets[i];assert.equal(p[2]&4,0,'Packet timestamp');if(p[2]&1)assert(p[0]/recorded.rate<.05,'Only startup discontinuity');if(i){assert(p[0]>=packets[i-1][0]);maxGapFrames=Math.max(maxGapFrames,p[0]-packets[i-1][0]-packets[i-1][1]);}}
  assert(maxGapFrames<=recorded.rate*.002,'Packet coverage');
  const actions=json(path.join(run.guiRun,'actions.json'));
  const action=name=>{const found=actions.filter(a=>a.name===name);assert.equal(found.length,1,'Unique '+name);const a=found[0];assert.equal(normalize(a.window.app.replace(/^process:/,'')),normalize(run.executable));const before=clock.utcToSeconds(a.beforeUtc),after=clock.utcToSeconds(a.afterUtc);assert(after>=before&&after-before<2);return {before,after};};
  const times={first:action('PlayFirst'),stopFirst:action('StopFirst'),replay:action('PlayReplay'),stopReplay:action('StopReplay'),close:action('NormalClose')};
  assert(times.first.after<times.stopFirst.before&&times.stopFirst.after+4<times.replay.before&&times.replay.after<times.stopReplay.before&&times.stopReplay.after+4<times.close.before);
  assert(clock.utcToSeconds(exit.observedExitUtc)>=times.close.before);
  let defaults=null;if(variant==='wet'){
    const query=action('QueryDefaults');assert(query.before>times.first.after&&query.after<times.first.after+protocol.input.noteSeconds);
    const observed=json(path.join(run.guiRun,'QueryDefaults.json'));
    const m=observed.accessibility.tree.match(/Buffer (\d+), Waves Reverb (\d+): Input gain ([\d.-]+) dB Reverb mix ([\d.-]+) dB Reverb time ([\d.-]+) ms High-frequency time ratio ([\d.-]+)/);
    assert(m,'Actual active Waves parameter observation');assert.equal(Number(m[1]),protocol.input.effectStorageBuffer+1);assert.equal(Number(m[2]),1);
    defaults={inputGain:Number(m[3]),reverbMix:Number(m[4]),reverbTime:Number(m[5]),highFrequencyTimeRatio:Number(m[6])};assert.deepEqual(defaults,protocol.criteria.defaults);
  }
  const pcm=analyzeWavesPcm(recorded,times,protocol);
  return {variant,passed:true,pcm,defaults,clock:clock.evidence,maxGapFrames,times,bindings:{processId:run.processId,producerSha256:run.exeSha256,buildSummarySha256:run.buildSummarySha256,savedSourceCount:build.sources.length,captureDriverSha256:run.driverSha256,recorderSha256:run.recorderSha256,recorderBuildSummarySha256:run.recorderBuildSummarySha256,runSha256:hash(path.join(dir,'run.json')),actionsSha256:hash(path.join(run.guiRun,'actions.json')),pcmSha256:hash(path.join(dir,'output.wav')),packetSha256:hash(path.join(dir,'packets.csv')),endpointSha256:hash(path.join(dir,'endpoint.txt')),exitSha256:hash(path.join(run.guiRun,'exit.json')),parameterObservationSha256:variant==='wet'?hash(path.join(run.guiRun,'QueryDefaults.json')):null}};
}

if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
  const [dryDirectory,wetDirectory,protocolPath,output]=process.argv.slice(2);assert(output,'Usage: dry wet protocol output');assert(!fs.existsSync(output),'Preserve previous audit');
  const protocol=json(protocolPath),fixturePath=path.resolve(protocol.fixture.path);assert.equal(hash(fixturePath),protocol.fixture.sha256);assert.equal(hash(protocol.nativeProof.path),protocol.nativeProof.sha256);const fixture=inspectWavesFixture(path.dirname(fixturePath));
  const dry=inspectWavesRun(dryDirectory,'dry',protocolPath),wet=inspectWavesRun(wetDirectory,'wet',protocolPath);assert.notEqual(dry.bindings.processId,wet.bindings.processId);
  assert.equal(dry.bindings.endpointSha256,wet.bindings.endpointSha256,'Identical render endpoint');
  const comparison=compareWavesPcm(dry.pcm,wet.pcm,protocol);
  const report={schema:1,createdUtc:new Date().toISOString(),passed:true,candidate:protocol.candidate,dry,wet,comparison,fixture,protocolSha256:hash(protocolPath),auditorSha256:hash(process.argv[1]),scope:protocol.scope,fullAcceptance:false};
  fs.writeFileSync(output,JSON.stringify(report,null,2)+'\n',{flag:'wx'});console.log(JSON.stringify(report));
}

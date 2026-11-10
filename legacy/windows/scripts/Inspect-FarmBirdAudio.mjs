import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
import {matchSource} from './Inspect-FarmGuiAudio.mjs';
import {captureClock} from './AudioCaptureClock.mjs';
import {inspectFarmBirdScoreNotes} from './Inspect-FarmBirdScoreNotes.mjs';
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const repo=path.dirname(path.dirname(fileURLToPath(import.meta.url)));
function chunks(b,a=0,z=b.length){
  const result=[];
  for(let p=a;p<z;){
    assert(p+8<=z,'RIFF header');const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),end=p+8+n;
    assert(end+(n&1)<=z,'RIFF payload');const c={id,data:b.subarray(p+8,end)};
    if(id==='RIFF'||id==='LIST'){assert(n>=4);c.type=b.toString('ascii',p+8,p+12);c.children=chunks(b,p+12,end);}
    result.push(c);p=end+(n&1);
  }
  return result;
}
const one=(children,id,type)=>{const a=children.filter(c=>c.id===id&&(type===undefined||c.type===type));assert.equal(a.length,1,'Unique '+id+'/'+(type??''));return a[0];};
function pcm(bytes,floating){
  const r=one(chunks(bytes),'RIFF','WAVE'),f=one(r.children,'fmt ').data,d=one(r.children,'data').data;
  const tag=f.readUInt16LE(),channels=f.readUInt16LE(2),rate=f.readUInt32LE(4),align=f.readUInt16LE(12),bits=f.readUInt16LE(14);
  if(floating){assert(tag===3||(tag===65534&&f.length>=40&&f.subarray(24,40).toString('hex')==='0300000000001000800000aa00389b71'));assert.equal(bits,32);}
  else{assert.equal(tag,1);assert.equal(bits,16);assert.equal(channels,1);}
  assert.equal(align,channels*bits/8);assert.equal(d.length%align,0);const values=new Float64Array(d.length/align);let peak=0,clipped=0;
  for(let i=0;i<values.length;i++)for(let c=0;c<channels;c++){const v=floating?d.readFloatLE(i*align+c*4):d.readInt16LE(i*align+c*2)/32768;assert(Number.isFinite(v));values[i]+=v/channels;peak=Math.max(peak,Math.abs(v));if(Math.abs(v)>=.999)clipped++;}
  return{values,rate,channels,peak,clippedFraction:clipped/(values.length*channels)};
}
export function readBirdSource(dlsPath){
  const dls=one(chunks(fs.readFileSync(dlsPath)),'RIFF','DLS '),ins=one(one(dls.children,'LIST','lins').children,'LIST','ins '),insh=one(ins.children,'insh').data;
  assert.equal(insh.readUInt32LE(),1);assert.equal(insh.readUInt32LE(4),1);assert.equal(insh.readUInt32LE(8),0);
  const region=one(one(ins.children,'LIST','lrgn').children,'LIST','rgn '),rgnh=one(region.children,'rgnh').data,wsmp=one(region.children,'wsmp').data,wlnk=one(region.children,'wlnk').data;
  assert.equal(wlnk.readUInt32LE(8),0);const waves=one(dls.children,'LIST','wvpl').children;assert.equal(waves.length,1);
  const f=one(waves[0].children,'fmt ').data,d=one(waves[0].children,'data').data;assert.equal(f.readUInt16LE(),1);assert.equal(f.readUInt16LE(2),1);assert.equal(f.readUInt16LE(14),16);
  const unity=wsmp.readUInt16LE(4),fineTune=wsmp.readInt16LE(6);assert.equal(unity,85);assert.equal(fineTune,0);
  return{values:Float64Array.from({length:d.length/2},(_,i)=>d.readInt16LE(i*2)/32768),rate:f.readUInt32LE(4),unity,fineTune,low:rgnh.readUInt16LE(),high:rgnh.readUInt16LE(2),loopStart:wsmp.readUInt32LE(28),loopLength:wsmp.readUInt32LE(32),dlsSha256:hash(dlsPath)};
}
function rms(output,rate,from,to){assert(to>from&&from>=0&&to<output.length/rate);let e=0;const a=Math.ceil(from*rate),z=Math.floor(to*rate);for(let i=a;i<z;i++)e+=output[i]**2;return Math.sqrt(e/(z-a));}
// FILETIME is kept at 100 ns precision; observedFileTime is a collection
// timestamp, not a measured note onset. Only PCM matches measure onset below.
const ftIso=x=>{const t=BigInt(x)-116444736000000000n,ms=t/10000n;return new Date(Number(ms)).toISOString().replace(/\.\d{3}Z$/,'.'+(t%10000000n).toString().padStart(7,'0')+'Z');};
const csv=p=>fs.readFileSync(p,'utf8').trim().split(/\r?\n/).map(l=>l.split(','));
export function inspectFarmBirdAudio(directory,{pcmOverride}={}){
  const run=json(directory+'/run.json'),ready=json(directory+'/ready.json'),capture=json(directory+'/capture.json'),build=json(run.buildSummary);
  assert.equal(run.state,'exited');assert.equal(run.captureExitCode,0);assert(capture.passed&&build.passed);assert.equal(hash(run.buildSummary),run.buildSummarySha256);
  for(const s of build.sources)assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256,'Saved product source');
  for(const i of run.inputs)assert.equal(hash(i.path),i.sha256,'Captured input');
  const recorder=json(run.recorderBuildSummary);assert(recorder.passed);assert.equal(hash(run.recorderBuildSummary),run.recorderBuildSummarySha256);assert.equal(hash(run.recorder),run.recorderSha256);
  for(const s of recorder.sources)assert.equal(hash(path.join(path.dirname(run.recorderBuildSummary),'sources',s.path)),s.sha256,'Saved recorder source');
  const inputDirectory=path.dirname(run.inputs.find(i=>path.basename(i.path)==='FarmGame.dls').path),source=readBirdSource(inputDirectory+'/FarmGame.dls');
  const recorded=pcm(fs.readFileSync(directory+'/output.wav'),true),output=pcmOverride??recorded.values;assert.equal(output.length,recorded.values.length);assert.equal(recorded.rate,capture.sampleRate);assert.equal(recorded.channels,capture.channels);
  const packets=csv(directory+'/packets.csv').slice(1).map(r=>r.map(Number)),clock=captureClock(ready,capture,packets,recorded.rate);assert.equal(capture.timestampErrors,0);
  const discontinuities=packets.filter(p=>(p[2]&1)!==0);assert.equal(discontinuities.length,capture.discontinuities);assert(discontinuities.every(p=>p[0]/recorded.rate<.05),'No playback discontinuity');
  const rules={sourcePrefixSeconds:.13,minNormalizedCorrelation:.98,minimumRms:.0003,quietMaxRms:.00005,wrongPitchCents:[-100,100],firstOnsetWindowSeconds:.6};
  let action,notes=[],nativeActions=[],actionPath,notePath;
  if(run.observer){
    assert.equal(run.observerExitCode,0);assert.equal(run.observerNormalExit,true);assert.equal(run.captureNormalExit,true);assert.equal(run.forcedTermination,false);assert.deepEqual(run.inputsChanged,[]);
    assert.equal(hash(run.observer),run.observerSha256);assert.equal(hash(run.savedCore),run.savedCoreSha256);assert.equal(hash(run.observerProvenance),run.observerProvenanceSha256);
    const provenance=json(run.observerProvenance);assert.equal(provenance.candidate,run.candidate);assert.equal(provenance.executable.sha256,run.observerSha256);assert.equal(provenance.savedCore.sha256,run.savedCoreSha256);
    for(const s of [...provenance.sources,provenance.savedCore,provenance.savedConductor,provenance.build])assert.equal(hash(path.join(repo,s.path)),s.sha256,'Observer provenance');
    actionPath=directory+'/observations/actions.csv';notePath=directory+'/observations/notes.csv';const rows=csv(actionPath);assert.deepEqual(rows[0],['phase','beforeFileTime','afterFileTime','hresult']);
    nativeActions=rows.slice(1).map(([phase,before,after,hresult])=>({phase,from:clock.utcToSeconds(ftIso(before)),to:clock.utcToSeconds(ftIso(after)),hresult:Number(hresult)}));assert(nativeActions.every(a=>a.hresult===0));action=nativeActions.find(a=>a.phase==='Bird');assert(action);
    const nr=csv(notePath);notes=nr.slice(1).map(r=>Object.fromEntries(nr[0].map((h,i)=>[h,h==='phase'||h==='observedFileTime'?r[i]:Number(r[i])])));assert(notes.every(n=>Number.isInteger(n.midiValue)&&n.midiValue>=0&&n.midiValue<=127));
  }else{
    assert.equal(hash(run.executable),run.exeSha256);actionPath=directory+'/farm-gui-actions.json';const actions=json(actionPath);assert.equal(actions.processId,run.processId);
    const a=actions.actions.filter(a=>/Bird solo/.test(a.label));assert.equal(a.length,1,'One solo Bird action');action={phase:a[0].label,from:clock.utcToSeconds(a[0].beforeUtc),to:clock.utcToSeconds(a[0].afterUtc)};
  }
  const from=action.from,to=action.to+rules.firstOnsetWindowSeconds;
  const candidates=Array.from({length:source.high-source.low+1},(_,i)=>source.low+i).map(pitch=>({pitch,...matchSource(output,recorded.rate,source.values,source.rate,from,to,(pitch-source.unity)*100,rules.sourcePrefixSeconds)})).sort((a,b)=>b.correlation-a.correlation),correct=candidates[0];
  const nativeBird=notes.filter(n=>n.phase==='Bird');if(run.observer)assert.equal(nativeBird.length,6,'Actual solo Bird notes');
  const nativeScore=run.observer?inspectFarmBirdScoreNotes(directory):null;
  if(nativeScore)assert(nativeScore.scoreNotesPassed,'Whole native Bird score variation must match the independent input');
  const expectedMidiPitch=nativeScore?nativeScore.soloMatches[0].notes[0].expected.midiValue:null;
  const wrongPitch=rules.wrongPitchCents.map(cents=>({cents,...matchSource(output,recorded.rate,source.values,source.rate,from,to,(correct.pitch-source.unity)*100+cents,rules.sourcePrefixSeconds)}));
  const wrongSources=['Cougar','Cow','Rooster','Sheep','Wolf','Alarm'].map(name=>{const p=inputDirectory+'/Sfx'+name+'.wav',s=pcm(fs.readFileSync(p),false);return{name,sha256:hash(p),...matchSource(output,recorded.rate,s.values,s.rate,from,to,0,rules.sourcePrefixSeconds)};});
  const baseline={from:Math.max(0,from-.3),to:from-.025};baseline.rms=rms(output,recorded.rate,baseline.from,baseline.to);baseline.passed=baseline.rms<=rules.quietMaxRms;
  const naturalEnd={from:action.to+5,to:action.to+5.5};naturalEnd.rms=rms(output,recorded.rate,naturalEnd.from,naturalEnd.to);naturalEnd.passed=naturalEnd.rms<=rules.quietMaxRms;
  const firstPrefixPassed=correct.correlation>=rules.minNormalizedCorrelation&&correct.gain>0&&correct.rms>=rules.minimumRms&&candidates.slice(1).every(c=>c.correlation<rules.minNormalizedCorrelation)&&wrongPitch.every(c=>c.correlation<rules.minNormalizedCorrelation)&&wrongSources.every(c=>c.correlation<rules.minNormalizedCorrelation)&&(expectedMidiPitch===null||correct.pitch===expectedMidiPitch)&&baseline.passed&&naturalEnd.passed&&recorded.clippedFraction<=.0001;
  const nativeNoteDiagnostics=notes.filter(n=>n.phase==='Bird'||(n.phase==='AfterBackgroundStop'&&n.channel===16)).map(n=>{const observed=clock.utcToSeconds(ftIso(n.observedFileTime)),match=matchSource(output,recorded.rate,source.values,source.rate,Math.max(0,observed-.05),observed+.3,(n.midiValue-source.unity)*100,rules.sourcePrefixSeconds);return{phase:n.phase,clocks:n.clocks,midiValue:n.midiValue,duration:n.duration,collectionTime:observed,match,prefixPassed:match.correlation>=rules.minNormalizedCorrelation&&match.gain>0&&match.rms>=rules.minimumRms};});
  const solo=nativeNoteDiagnostics.filter(n=>n.phase==='Bird'),intervals=solo.slice(1).map((n,i)=>({clocks:n.clocks-solo[i].clocks,expected100BpmSeconds:(n.clocks-solo[i].clocks)*60/(100*768),measuredSeconds:n.match.time-solo[i].match.time}));
  const strictSoloTempoPassed=solo.length===6&&solo.every(n=>n.prefixPassed)&&intervals.every(i=>Math.abs(i.measuredSeconds-i.expected100BpmSeconds)<=.025);
  const postStop=nativeNoteDiagnostics.filter(n=>n.phase==='AfterBackgroundStop'),stop=nativeActions.find(a=>a.phase==='StopBackgroundOnly');
  const postStopPrefixPassed=Boolean(stop&&postStop.some(n=>n.prefixPassed&&n.match.time>stop.to+.15));
  return{schema:1,createdUtc:new Date().toISOString(),candidate:run.candidate??path.basename(path.dirname(run.buildSummary)),rules,firstPrefixPassed,firstPrefix:{correct,expectedMidiPitch,scorePitchOracleVerified:Boolean(nativeScore),candidates,wrongPitch,wrongSources},nativeScoreNotes:nativeScore?{passed:nativeScore.scoreNotesPassed,soloVariation:nativeScore.soloMatches[0].variationMask,soloNotes:nativeScore.soloMatches[0].notes.length,coexistenceVariation:nativeScore.coexistenceMatches[0].variationMask,coexistenceNotes:nativeScore.coexistenceMatches[0].notes.length,bindings:nativeScore.bindings}:null,baseline,naturalEnd,nativeNoteDiagnostics,intervals,strictSoloTempoPassed,postStopPrefixPassed,wholeFarmPassed:false,clock:clock.evidence,peak:recorded.peak,clippedFraction:recorded.clippedFraction,bindings:{processId:run.processId??run.observerProcessId,producerSha256:run.exeSha256??null,observerSha256:run.observerSha256??null,buildSummarySha256:run.buildSummarySha256,recorderSha256:run.recorderSha256,sourceDlsSha256:source.dlsSha256,runSha256:hash(directory+'/run.json'),wavSha256:hash(directory+'/output.wav'),actionsSha256:hash(actionPath),notesSha256:notePath?hash(notePath):null,readySha256:hash(directory+'/ready.json'),captureSha256:hash(directory+'/capture.json'),packetsSha256:hash(directory+'/packets.csv'),endpointSha256:hash(directory+'/endpoint.txt'),auditorSha256:hash(fileURLToPath(import.meta.url)),matcherSha256:hash(fileURLToPath(new URL('./Inspect-FarmGuiAudio.mjs',import.meta.url))),clockAuditorSha256:hash(fileURLToPath(new URL('./AudioCaptureClock.mjs',import.meta.url))),scoreAuditorSha256:hash(fileURLToPath(new URL('./Inspect-FarmBirdScoreNotes.mjs',import.meta.url)))},scope:'Isolated first Bird attack DLS source and measured pitch; native whole score variation independently matched before using input-derived first MIDI pitch. Wrong pitch/source rejection, adjacent baseline and natural-end silence. Full PCM score/tempo, main score pitch, Night source, mixed separation and whole Farm acceptance remain incomplete.',fullAcceptance:false};
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
  const[directory,output]=process.argv.slice(2);assert(directory&&output,'Capture directory and fresh report required');assert(!fs.existsSync(output),'Preserve previous report');const r=inspectFarmBirdAudio(path.resolve(directory));fs.writeFileSync(output,JSON.stringify(r,null,2)+'\n',{flag:'wx'});console.log(JSON.stringify({firstPrefixPassed:r.firstPrefixPassed,first:r.firstPrefix.correct,expectedMidiPitch:r.firstPrefix.expectedMidiPitch,baseline:r.baseline,naturalEnd:r.naturalEnd,strictSoloTempoPassed:r.strictSoloTempoPassed,postStopPrefixPassed:r.postStopPrefixPassed,wholeFarmPassed:false,fullAcceptance:false}));if(!r.firstPrefixPassed)process.exitCode=1;
}

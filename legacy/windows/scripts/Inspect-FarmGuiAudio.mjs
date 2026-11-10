import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {captureClock} from './AudioCaptureClock.mjs';

const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const hash=p=>sha(fs.readFileSync(p));
const json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
function chunks(b,a=0,z=b.length){
  const result=[];
  for(let p=a;p<z;){
    assert(p+8<=z,'RIFF header');
    const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),end=p+8+n;
    assert(end+(n&1)<=z,'RIFF payload');
    const c={id,data:b.subarray(p+8,end)};
    if(id==='RIFF'||id==='LIST'||id==='seqt'){
      const skip=id==='seqt'?0:4;assert(n>=skip);
      c.type=skip?b.toString('ascii',p+8,p+12):'';
      c.children=chunks(b,p+8+skip,end);
    }
    result.push(c);p=end+(n&1);
  }
  return result;
}
const flat=c=>[c,...(c.children??[]).flatMap(flat)];
function one(children,id){const c=children.filter(c=>c.id===id);assert.equal(c.length,1,'Unique '+id);return c[0];}
function pcm(bytes,floating){
  const roots=chunks(bytes);assert.equal(roots.length,1);const root=roots[0];assert.equal(root.type,'WAVE');
  const f=one(root.children,'fmt ').data,d=one(root.children,'data').data;
  const tag=f.readUInt16LE(),channels=f.readUInt16LE(2),rate=f.readUInt32LE(4),align=f.readUInt16LE(12),bits=f.readUInt16LE(14);
  if(floating){assert(tag===3||(tag===65534&&f.length>=40&&f.subarray(24,40).toString('hex')==='0300000000001000800000aa00389b71'));assert.equal(bits,32);}
  else {assert.equal(tag,1);assert.equal(bits,16);assert.equal(channels,1);}
  assert.equal(align,channels*bits/8);assert.equal(d.length%align,0);
  const values=new Float64Array(d.length/align);let clipped=0,peak=0;
  for(let i=0;i<values.length;i++){
    let v=0;
    for(let c=0;c<channels;c++){
      const sample=floating?d.readFloatLE(i*align+c*4):d.readInt16LE(i*align+c*2)/32768;
      assert(Number.isFinite(sample),'Finite PCM');v+=sample/channels;peak=Math.max(peak,Math.abs(sample));if(Math.abs(sample)>=.999)clipped++;
    }
    values[i]=v;
  }
  return {values,rate,channels,peak,clippedFraction:clipped/(values.length*channels)};
}
function rms(values,rate,from,to){assert(to>from&&from>=0&&to<=values.length/rate);let e=0;const a=Math.ceil(from*rate),z=Math.floor(to*rate);for(let i=a;i<z;i++)e+=values[i]**2;return Math.sqrt(e/(z-a));}

// Resample the input PCM, never the product's output, to construct the expected
// zero-cent source prefix. Coarse candidates are refined at single-frame steps.
export function matchSource(output,rate,source,sourceRate,from,to,cents=0,seconds=.13){
  const stride=8,step=sourceRate/rate*2**(cents/1200),template=[];let energy=0;
  for(let i=0;i<Math.round(rate*seconds);i+=stride){
    const p=i*step,a=Math.floor(p);assert(a+1<source.length);const v=source[a]*(1-(p-a))+source[a+1]*(p-a);template.push(v);energy+=v*v;
  }
  assert(energy>0,'Nonempty source template');
  const start=Math.max(0,Math.floor(from*rate)),end=Math.min(Math.ceil(to*rate),output.length-Math.round(rate*seconds));
  assert(end>start,'Template search interval');
  function at(frame){let dot=0,e=0;for(let j=0;j<template.length;j++){const v=output[frame+j*stride];dot+=template[j]*v;e+=v*v;}return {frame,time:frame/rate,correlation:e>0?dot/Math.sqrt(e*energy):0,gain:dot/energy,rms:Math.sqrt(e/template.length)};}
  let candidates=[];
  for(let i=start;i<end;i+=8){const v=at(i);if(candidates.length<32||v.correlation>candidates.at(-1).correlation){candidates.push(v);candidates.sort((a,b)=>b.correlation-a.correlation);candidates.length=Math.min(32,candidates.length);}}
  let best=candidates[0];
  for(const c of candidates)for(let i=Math.max(start,c.frame-7);i<=Math.min(end-1,c.frame+7);i++){const v=at(i);if(v.correlation>best.correlation)best=v;}
  return best;
}
export function inspectFarmGuiAudio(directory,{pcmOverride}={}){
  const run=json(path.join(directory,'run.json')),ready=json(path.join(directory,'ready.json')),capture=json(path.join(directory,'capture.json')),actions=json(path.join(directory,'actions.json'));
  assert.equal(run.state,'exited');assert.equal(run.captureExitCode,0);assert(capture.passed);
  const build=json(run.buildSummary);assert(build.passed);assert.equal(hash(run.buildSummary),run.buildSummarySha256);assert.equal(hash(run.executable),run.exeSha256);
  for(const s of build.sources)assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256,'Saved product source');
  for(const i of run.inputs)assert.equal(hash(i.path),i.sha256,'Capture input');
  const recorder=json(run.recorderBuildSummary);assert.equal(hash(run.recorderBuildSummary),run.recorderBuildSummarySha256);assert.equal(hash(run.recorder),run.recorderSha256);assert(recorder.passed);
  for(const s of recorder.sources)assert.equal(hash(path.join(path.dirname(run.recorderBuildSummary),'sources',s.path)),s.sha256,'Saved recorder source');
  const protocolPath=path.join(run.guiRun,'pcm-protocol.json'),protocol=json(protocolPath),rules=protocol.criteria;
  assert.equal(rules.minNormalizedCorrelation,.98);assert.equal(rules.sourcePrefixSeconds,.13);assert.equal(rules.pitchCents,0);assert.deepEqual(rules.wrongPitchCents,[-100,100]);assert.equal(rules.minimumRms,.0003);assert.equal(rules.baselineMaxRms,.00005);
  const recorded=pcm(fs.readFileSync(path.join(directory,'output.wav')),true);assert.equal(recorded.rate,capture.sampleRate);assert.equal(recorded.channels,capture.channels);
  const output=pcmOverride??recorded.values;assert.equal(output.length,recorded.values.length);
  const packets=fs.readFileSync(path.join(directory,'packets.csv'),'utf8').trim().split(/\r?\n/).slice(1).map(l=>l.split(',').map(Number));
  const clock=captureClock(ready,capture,packets,recorded.rate);
  assert.equal(capture.timestampErrors,0);assert.equal(packets.filter(p=>(p[2]&1)!==0).length,capture.discontinuities);
  assert(packets.filter(p=>(p[2]&1)!==0).every(p=>p[0]/recorded.rate<.05),'No discontinuity during playback');
  const inputDirectory=path.dirname(run.inputs.find(i=>path.basename(i.path)==='FarmOwned.spp').path),names=['Cougar','Cow','Rooster','Sheep','Wolf','Alarm'];
  const sources=names.map(name=>{const sourcePath=path.join(inputDirectory,'Sfx'+name+'.wav'),segmentPath=path.join(inputDirectory,'Sfx'+name+'.sgt'),source=pcm(fs.readFileSync(sourcePath),false),items=flat(chunks(fs.readFileSync(segmentPath))[0]).filter(x=>x.id==='waih');assert.equal(items.length,1);const h=items[0].data;assert.equal(h.length,64);assert.equal(h.readInt32LE(4),0);assert.equal(h.readBigInt64LE(16),0n);assert.equal(h.readBigInt64LE(24),0n);assert.equal(h.readUInt32LE(60),0);return {name,source,sourcePath,segmentPath,sourceSha256:hash(sourcePath),segmentSha256:hash(segmentPath)};});
  const results=sources.map(s=>{
    const action=actions.find(a=>a.name===s.name);assert(action,'Required action '+s.name);
    const from=clock.utcToSeconds(action.beforeUtc),to=clock.utcToSeconds(action.afterUtc)+rules.onsetWindowSeconds[1];assert(from>=0&&to<capture.seconds);
    const correct=matchSource(output,recorded.rate,s.source.values,s.source.rate,from,to,0,rules.sourcePrefixSeconds);
    const wrongPitch=rules.wrongPitchCents.map(cents=>({cents,...matchSource(output,recorded.rate,s.source.values,s.source.rate,from,to,cents,rules.sourcePrefixSeconds)}));
    const wrongSources=sources.filter(x=>x.name!==s.name).map(x=>({name:x.name,...matchSource(output,recorded.rate,x.source.values,x.source.rate,from,to,0,rules.sourcePrefixSeconds)}));
    const passed=correct.correlation>=rules.minNormalizedCorrelation&&correct.gain>0&&correct.rms>=rules.minimumRms&&[...wrongPitch,...wrongSources].every(x=>x.correlation<rules.minNormalizedCorrelation);
    return {name:s.name,from,to,correct,wrongPitch,wrongSources,sourceSha256:s.sourceSha256,segmentSha256:s.segmentSha256,passed};
  });
  const first=clock.utcToSeconds(actions[0].beforeUtc),baseline={from:1,to:first-.5};assert(baseline.to>baseline.from);baseline.rms=rms(output,recorded.rate,baseline.from,baseline.to);baseline.passed=baseline.rms<=rules.baselineMaxRms;
  const stop=actions.find(a=>a.name==='AllStop'),stopFrom=clock.utcToSeconds(stop.afterUtc)+rules.stopSettlingSeconds;
  const stopped={from:stopFrom,to:stopFrom+rules.stopSilenceWindowSeconds};stopped.rms=rms(output,recorded.rate,stopped.from,stopped.to);stopped.passed=stopped.rms<=rules.baselineMaxRms;
  const night=actions.find(a=>a.name==='Night'),nightFrom=clock.utcToSeconds(night.afterUtc)+1,nightTo=clock.utcToSeconds(stop.beforeUtc)-.5;
  const backgroundDiagnostic={from:nightFrom,to:nightTo,rms:rms(output,recorded.rate,nightFrom,nightTo),sourceIdentityVerified:false};
  const isolatedSfxPassed=baseline.passed&&stopped.passed&&results.every(r=>r.passed)&&recorded.clippedFraction<=rules.clippedFrameFractionMax;
  return {schema:1,createdUtc:new Date().toISOString(),isolatedSfxPassed,results,baseline,stopped,backgroundDiagnostic,peak:recorded.peak,clippedFraction:recorded.clippedFraction,clock:clock.evidence,bindings:{processId:run.processId,producerSha256:run.exeSha256,buildSummarySha256:run.buildSummarySha256,recorderSha256:run.recorderSha256,runSha256:hash(path.join(directory,'run.json')),actionsSha256:hash(path.join(directory,'actions.json')),wavSha256:hash(path.join(directory,'output.wav')),packetSha256:hash(path.join(directory,'packets.csv')),readySha256:hash(path.join(directory,'ready.json')),captureSha256:hash(path.join(directory,'capture.json')),endpointSha256:hash(path.join(directory,'endpoint.txt')),protocolSha256:hash(protocolPath),auditorSha256:hash(fileURLToPath(import.meta.url)),clockAuditorSha256:hash(fileURLToPath(new URL('./AudioCaptureClock.mjs',import.meta.url)))},scope:'Six isolated zero-cent native Wave source prefixes, wrong-source/wrong-pitch rejection, baseline and AllStop silence. Background identity/tempo, restart and secondary coexistence are not accepted by this analysis.',fullAcceptance:false};
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
  const [directory,output]=process.argv.slice(2);assert(directory&&output,'Usage: node Inspect-FarmGuiAudio.mjs <capture directory> <fresh output JSON>');assert(!fs.existsSync(output),'Preserve previous analysis');
  const result=inspectFarmGuiAudio(path.resolve(directory));fs.writeFileSync(output,JSON.stringify(result,null,2)+'\n',{flag:'wx'});
  console.log(JSON.stringify({isolatedSfxPassed:result.isolatedSfxPassed,results:result.results.map(x=>({name:x.name,passed:x.passed,correlation:x.correct.correlation,gain:x.correct.gain})),baseline:result.baseline,stopped:result.stopped,backgroundDiagnostic:result.backgroundDiagnostic,fullAcceptance:false}));if(!result.isolatedSfxPassed)process.exitCode=1;
}

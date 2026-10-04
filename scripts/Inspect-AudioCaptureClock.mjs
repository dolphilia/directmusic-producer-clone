import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';import assert from 'node:assert/strict';
import {captureClock} from './AudioCaptureClock.mjs';
const dir=path.resolve(process.argv[2]),read=p=>fs.readFileSync(p),json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,'')),hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const run=json(dir+'/run.json'),ready=json(dir+'/ready.json'),capture=json(dir+'/capture.json');
const summaryPath=run.recorderBuildSummary||run.buildSummary,build=json(summaryPath);
assert.equal(run.captureExitCode,0);assert(capture.passed&&build.passed);
assert.equal(hash(summaryPath),run.recorderBuildSummarySha256||run.buildSummarySha256);assert.equal(hash(run.recorder),run.recorderSha256);assert.equal(run.recorderSha256,build.sha256);assert.equal(hash(dir+'/driver.ps1'),run.driverSha256);
for(const source of build.sources){assert.equal(hash(path.join(path.dirname(summaryPath),'sources',source.path)),source.sha256);assert.equal(hash(path.resolve('tests/audio',source.path)),source.sha256);}
const packets=read(dir+'/packets.csv').toString().trim().split(/\r?\n/).slice(1).map(x=>x.split(',').map(Number));
const clock=captureClock(ready,capture,packets,capture.sampleRate);
const readyPollOffsetSeconds=clock.utcToSeconds(run.readyObservedUtc||run.readyUtc);assert(readyPollOffsetSeconds>=0&&readyPollOffsetSeconds<1);
if(run.timingBasis){assert.equal(run.timingBasis,'recorder-start-utc-qpc');assert.equal(hash(dir+'/ready.json'),run.captureReadySha256);assert(Math.abs(clock.utcToSeconds(run.captureStartUtc))<.000001);}
const wav=read(dir+'/output.wav');assert.equal(wav.toString('ascii',0,4),'RIFF');assert.equal(wav.readUInt32LE(4)+8,wav.length);let fmt,data;
for(let p=12;p<wav.length;){const n=wav.readUInt32LE(p+4);assert(p+8+n<=wav.length);const id=wav.toString('ascii',p,p+4);if(id==='fmt ')fmt=wav.subarray(p+8,p+8+n);if(id==='data')data=wav.subarray(p+8,p+8+n);p+=8+n+(n&1);}
assert(fmt&&data);assert.equal(fmt.readUInt16LE(14),32);assert(fmt.readUInt16LE(0)===3||(fmt.readUInt16LE(0)===65534&&fmt.readUInt32LE(24)===3));
const rate=fmt.readUInt32LE(4),channels=fmt.readUInt16LE(2),align=fmt.readUInt16LE(12);assert.equal(rate,capture.sampleRate);assert.equal(data.length/align,rate*capture.seconds);
let sum=0,peak=0,count=0;for(let i=Math.round(.3*rate);i<Math.round((capture.seconds-.3)*rate);i++){let mono=0;for(let c=0;c<channels;c++){const v=data.readFloatLE(i*align+c*4);assert(Number.isFinite(v));mono+=v/channels;peak=Math.max(peak,Math.abs(v));}sum+=mono*mono;count++;}
const baseline={from:.3,to:capture.seconds-.3,rms:Math.sqrt(sum/count),peak};baseline.quiet=baseline.rms<.0001&&peak<.0005;
const controls=[];
function rejected(name,mutate){const r=structuredClone(ready),c=structuredClone(capture),p=structuredClone(packets);mutate(r,c,p);let error;try{captureClock(r,c,p,capture.sampleRate);}catch(e){error=e.message;}assert(error,name+' must reject');controls.push({name,rejected:true,error});}
rejected('QPC100ns and raw ticks disagree',r=>r.startQpc100ns+=1000000);
rejected('one packet shifted by50ms',(_r,_c,p)=>p[0][0]+=capture.sampleRate*.05);
rejected('end UTC stepped100ms',(_r,c)=>{c.endUtcBeforeFileTime=(BigInt(c.endUtcBeforeFileTime)+1000000n).toString();c.endUtcAfterFileTime=(BigInt(c.endUtcAfterFileTime)+1000000n).toString();});
rejected('UTC bracket widened100ms',r=>r.utcAfterFileTime=(BigInt(r.utcBeforeFileTime)+1000000n).toString());
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,scope:'Recorder start/end UTC-QPC correlation and packet origin, not Producer playback acceptance; quiet measurement is a separate observation',clock:clock.evidence,readyPollOffsetSeconds,baseline,controls,recorderSha256:run.recorderSha256,runSha256:hash(dir+'/run.json'),readySha256:hash(dir+'/ready.json'),captureSha256:hash(dir+'/capture.json'),packetsSha256:hash(dir+'/packets.csv'),wavSha256:hash(dir+'/output.wav'),auditorSha256:hash(new URL(import.meta.url)),clockModuleSha256:hash(new URL('./AudioCaptureClock.mjs',import.meta.url)),fullAcceptancePassed:false};
fs.copyFileSync(new URL(import.meta.url),dir+'/clock-auditor.mjs');fs.copyFileSync(new URL('./AudioCaptureClock.mjs',import.meta.url),dir+'/AudioCaptureClock.mjs');fs.writeFileSync(dir+'/clock-proof.json',JSON.stringify(proof,null,2)+'\n');console.log(JSON.stringify(proof,null,2));

import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {captureClock} from './AudioCaptureClock.mjs';
import {inspectFarmGuiAudio,matchSource} from './Inspect-FarmGuiAudio.mjs';

const [firstDirectory,secondDirectory,outputPath]=process.argv.slice(2);assert(firstDirectory&&secondDirectory&&outputPath);assert(!fs.existsSync(outputPath),'Preserve prior analysis');
const dir=path.resolve(secondDirectory),sha=b=>crypto.createHash('sha256').update(b).digest('hex'),hash=p=>sha(fs.readFileSync(p)),json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const first=inspectFarmGuiAudio(path.resolve(firstDirectory));assert(first.isolatedSfxPassed);
const run=json(path.join(dir,'run.json')),capture=json(path.join(dir,'capture.json')),ready=json(path.join(dir,'ready.json')),actions=json(path.join(dir,'actions.json'));
assert.equal(run.state,'exited');assert.equal(run.captureExitCode,0);assert(capture.passed);assert.equal(run.exeSha256,first.bindings.producerSha256);assert.equal(run.processId,first.bindings.processId);
for(const i of run.inputs)assert.equal(hash(i.path),i.sha256,'Recorded input binding');
assert.equal(hash(run.executable),run.exeSha256);assert.equal(hash(run.buildSummary),run.buildSummarySha256);assert.equal(hash(run.recorder),run.recorderSha256);
const exitPath=path.join(run.guiRun,'exit.json'),exit=json(exitPath),identity=json(path.join(run.guiRun,'identity.json'));
assert.equal(exit.processId,run.processId);assert.equal(exit.sha256,run.exeSha256);assert.equal(exit.startUtc,identity.startUtc);assert.equal(exit.exitCode,0);
function wave(file,floating){const b=fs.readFileSync(file);let data,rate,channels,align;assert.equal(b.toString('ascii',0,4),'RIFF');assert.equal(b.toString('ascii',8,12),'WAVE');for(let p=12;p<b.length;){const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4);assert(p+8+n<=b.length);if(id==='fmt '){channels=b.readUInt16LE(p+10);rate=b.readUInt32LE(p+12);align=b.readUInt16LE(p+20);assert.equal(b.readUInt16LE(p+22),floating?32:16);}if(id==='data')data=b.subarray(p+8,p+8+n);p+=8+n+(n&1);}const values=new Float64Array(data.length/align);for(let i=0;i<values.length;i++)for(let c=0;c<channels;c++)values[i]+=(floating?data.readFloatLE(i*align+c*4):data.readInt16LE(i*align+c*2)/32768)/channels;return {values,rate};}
const recorded=wave(path.join(dir,'output.wav'),true),packets=fs.readFileSync(path.join(dir,'packets.csv'),'utf8').trim().split(/\r?\n/).slice(1).map(l=>l.split(',').map(Number));
const clock=captureClock(ready,capture,packets,recorded.rate);assert.equal(capture.timestampErrors,0);assert(packets.filter(p=>(p[2]&1)!==0).every(p=>p[0]/recorded.rate<.05));
const action=name=>{const a=actions.find(x=>x.name===name);assert(a,'Required '+name);return {before:clock.utcToSeconds(a.beforeUtc),after:clock.utcToSeconds(a.afterUtc)};};
function energy(name,from,to,silent){assert(from>=0&&to>from&&to<=recorded.values.length/recorded.rate);let sum=0;const a=Math.ceil(from*recorded.rate),z=Math.floor(to*recorded.rate);for(let i=a;i<z;i++)sum+=recorded.values[i]**2;const rms=Math.sqrt(sum/(z-a));return {name,from,to,rms,passed:silent?rms<=.00005:rms>=.0003};}
const night=action('Night'),stop=action('AllStop'),restart=action('NightRestart'),cougar=action('CougarOverlay'),bird=action('BirdOverlay'),stop2=action('AllStopAfterOverlay'),close=action('FarmClose');
const phases=[energy('baseline',1,night.before-.5,true),energy('active-before-stop',night.after+1,stop.before-.5,false),energy('stop-silence',stop.after+2,stop.after+4,true),energy('restart-before-secondary',restart.after+1,cougar.before-.5,false),energy('primary-after-Cougar-end',cougar.after+3,bird.before-.5,false),energy('active-before-second-stop',bird.after+5,stop2.before-.5,false),energy('second-stop-silence',stop2.after+2,stop2.after+4,true),energy('private-window-close-silence',close.after+2,close.after+4,true)];
const inputDir=path.dirname(run.inputs.find(x=>path.basename(x.path)==='FarmOwned.spp').path),nightBytes=fs.readFileSync(path.join(inputDir,'BGNight.sgt'));
assert.equal(nightBytes.toString('ascii',12,16),'segh');assert.equal(nightBytes.readUInt32LE(20),0xffffffff,'Infinite primary input loop');assert.equal(nightBytes.readUInt32LE(24),29184,'Owned primary length');
const source=wave(path.join(inputDir,'SfxCougar.wav'),false),overlay=matchSource(recorded.values,recorded.rate,source.values,source.rate,cougar.before,cougar.after+1.5);
const overlayDiagnostic={...overlay,isolatedPrefixCriterion:.98,passesIsolatedPrefixCriterion:overlay.correlation>=.98,scope:'Diagnostic only: primary music is present, so the isolated-voice model cannot accept general mixing or source identity. No threshold is lowered.'};
const activeStopSilenceRestartPassed=phases.every(p=>p.passed);
const report={schema:1,createdUtc:new Date().toISOString(),activeStopSilenceRestartPassed,phases,overlayDiagnostic,isolatedSfxProof:first.bindings,bindings:{processId:run.processId,producerSha256:run.exeSha256,runSha256:hash(path.join(dir,'run.json')),actionsSha256:hash(path.join(dir,'actions.json')),pcmSha256:hash(path.join(dir,'output.wav')),packetSha256:hash(path.join(dir,'packets.csv')),endpointSha256:hash(path.join(dir,'endpoint.txt')),exitSha256:hash(exitPath),auditorSha256:hash(process.argv[1])},remaining:['Primary source/pitch/100-BPM conformance','Bird pattern/DLS source/pitch/tempo','Input-aware simultaneous-source analysis','Predawn/Dawn/Ending audio','Original dynamic comparison'],scope:'Energy lifecycle of unchanged infinite-loop primary and separately accepted isolated SFX; source identity or musical tempo is not inferred from RMS.',fullAcceptance:false};
fs.writeFileSync(outputPath,JSON.stringify(report,null,2)+'\n',{flag:'wx'});console.log(JSON.stringify({activeStopSilenceRestartPassed,phases,overlayDiagnostic,fullAcceptance:false}));if(!activeStopSilenceRestartPassed)process.exitCode=1;

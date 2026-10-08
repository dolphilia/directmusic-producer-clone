import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {captureClock} from './AudioCaptureClock.mjs';
import {inspectFarmGuiAudio} from './Inspect-FarmGuiAudio.mjs';

const [captureDirectory,outputPath]=process.argv.slice(2);
assert(captureDirectory&&outputPath);assert(!fs.existsSync(outputPath),'Preserve controls');
const dir=path.resolve(captureDirectory),read=n=>JSON.parse(fs.readFileSync(path.join(dir,n),'utf8').replace(/^\uFEFF/,''));
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
function wave(file,floating){
  const b=fs.readFileSync(file);let data,rate,channels,align;
  for(let p=12;p<b.length;){const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4);assert(p+8+n<=b.length);if(id==='fmt '){channels=b.readUInt16LE(p+10);rate=b.readUInt32LE(p+12);align=b.readUInt16LE(p+20);}if(id==='data')data=b.subarray(p+8,p+8+n);p+=8+n+(n&1);}
  const values=new Float64Array(data.length/align);
  for(let i=0;i<values.length;i++)for(let c=0;c<channels;c++)values[i]+=(floating?data.readFloatLE(i*align+c*4):data.readInt16LE(i*align+c*2)/32768)/channels;
  return {values,rate};
}
const original=inspectFarmGuiAudio(dir);assert(original.isolatedSfxPassed,'Positive recorded control');
const output=wave(path.join(dir,'output.wav'),true),run=read('run.json'),actions=read('actions.json');
const packets=fs.readFileSync(path.join(dir,'packets.csv'),'utf8').trim().split(/\r?\n/).slice(1).map(l=>l.split(',').map(Number));
const clock=captureClock(read('ready.json'),read('capture.json'),packets,output.rate);
const controls=[];
function reject(name,values,extraCheck){const r=inspectFarmGuiAudio(dir,{pcmOverride:values});assert(!r.isolatedSfxPassed,name+' must reject');extraCheck?.(r);controls.push({name,rejected:true,mutatedMonoSha256:sha(Buffer.from(values.buffer)),failedSources:r.results.filter(x=>!x.passed).map(x=>x.name),baselinePassed:r.baseline.passed,stopSilencePassed:r.stopped.passed});}
reject('all-silent',new Float64Array(output.values.length),r=>assert.equal(r.results.filter(x=>!x.passed).length,6));
const cowAction=actions.find(x=>x.name==='Cow'),cowFrom=clock.utcToSeconds(cowAction.beforeUtc),cowTo=clock.utcToSeconds(cowAction.afterUtc)+5;
const missingCow=output.values.slice();missingCow.fill(0,Math.floor(cowFrom*output.rate),Math.ceil(cowTo*output.rate));
reject('missing-Cow-only',missingCow,r=>{assert(!r.results.find(x=>x.name==='Cow').passed);assert.equal(r.results.filter(x=>!x.passed).length,1);});
const inputDir=path.dirname(run.inputs.find(x=>path.basename(x.path)==='FarmOwned.spp').path),cow=wave(path.join(inputDir,'SfxCow.wav'),false);
const wrongPitch=missingCow.slice(),at=Math.round((cowFrom+.2)*output.rate),step=cow.rate/output.rate*2**(100/1200);
for(let i=0;i*step<cow.values.length-1;i++){const p=i*step,a=Math.floor(p);wrongPitch[at+i]=.44*(cow.values[a]*(1-(p-a))+cow.values[a+1]*(p-a));}
reject('Cow-plus-one-semitone',wrongPitch,r=>{const c=r.results.find(x=>x.name==='Cow');assert(!c.passed);assert(c.wrongPitch.find(x=>x.cents===100).correlation>=.98);});
const wrongSource=missingCow.slice(),wolf=wave(path.join(inputDir,'SfxWolf.wav'),false),wolfStep=wolf.rate/output.rate;
for(let i=0;i*wolfStep<wolf.values.length-1;i++){const p=i*wolfStep,a=Math.floor(p);wrongSource[at+i]=.44*(wolf.values[a]*(1-(p-a))+wolf.values[a+1]*(p-a));}
reject('Wolf-in-Cow-slot',wrongSource,r=>{const c=r.results.find(x=>x.name==='Cow');assert(!c.passed);assert(c.wrongSources.find(x=>x.name==='Wolf').correlation>=.98);});
const afterStopNoise=output.values.slice();
for(let i=Math.round(original.stopped.from*output.rate);i<Math.floor(original.stopped.to*output.rate);i++)afterStopNoise[i]=.003*Math.sin(2*Math.PI*440*i/output.rate);
reject('audible-remainder-after-AllStop',afterStopNoise,r=>assert(!r.stopped.passed));
const report={schema:1,createdUtc:new Date().toISOString(),passed:true,positiveRecordedControl:true,controls,bindings:original.bindings,testAuditorSha256:sha(fs.readFileSync(process.argv[1])),scope:'In-memory negative PCM controls using unchanged recorded timings/bindings; no product or input file mutation, and no additional product acceptance',fullAcceptance:false};
fs.writeFileSync(outputPath,JSON.stringify(report,null,2)+'\n',{flag:'wx'});console.log(JSON.stringify({passed:true,positiveControls:1,negativeControls:controls.length,fullAcceptance:false}));

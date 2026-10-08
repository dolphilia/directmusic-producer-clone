import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
import {inspectFarmBirdAudio,readBirdSource} from './Inspect-FarmBirdAudio.mjs';
const[directoryArg,outArg]=process.argv.slice(2);assert(directoryArg&&outArg,'Capture directory and fresh controls directory required');
const directory=path.resolve(directoryArg),out=path.resolve(outArg),hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');assert(!fs.existsSync(out),'Preserve existing controls');fs.mkdirSync(out,{recursive:true});
const positive=inspectFarmBirdAudio(directory);fs.writeFileSync(out+'/positive.json',JSON.stringify(positive,null,2)+'\n',{flag:'wx'});assert(positive.firstPrefixPassed,'Actual first-prefix positive required');
const wav=fs.readFileSync(directory+'/output.wav');let format,data;
for(let p=12;p<wav.length;){const id=wav.toString('ascii',p,p+4),n=wav.readUInt32LE(p+4);assert(p+8+n<=wav.length);if(id==='fmt ')format=wav.subarray(p+8,p+8+n);if(id==='data')data=wav.subarray(p+8,p+8+n);p+=8+n+(n&1);}
assert(format&&data);assert.equal(format.readUInt16LE(14),32);const channels=format.readUInt16LE(2),align=format.readUInt16LE(12),rate=format.readUInt32LE(4),base=new Float64Array(data.length/align);
for(let i=0;i<base.length;i++)for(let c=0;c<channels;c++)base[i]+=data.readFloatLE(i*align+c*4)/channels;
const report=[],editRange=(output,from,to,fn)=>{for(let i=Math.ceil(from*rate);i<Math.floor(to*rate);i++)output[i]=fn(i);};
function defect(name,modify){const output=base.slice();modify(output);const result=inspectFarmBirdAudio(directory,{pcmOverride:output});fs.writeFileSync(out+'/'+name+'.json',JSON.stringify(result,null,2)+'\n',{flag:'wx'});const rejected=!result.firstPrefixPassed;report.push({name,rejected,firstPrefixPassed:result.firstPrefixPassed,correlation:result.firstPrefix.correct.correlation,baselineRms:result.baseline.rms,naturalEndRms:result.naturalEnd.rms});assert(rejected,'Defect must reject: '+name);}
const from=positive.baseline.to+.025;
defect('silent-bird-attack',output=>editRange(output,from,from+1.2,()=>0));
const run=JSON.parse(fs.readFileSync(directory+'/run.json','utf8').replace(/^\uFEFF/,'')),cowPath=run.inputs.find(i=>path.basename(i.path)==='SfxCow.wav').path,cow=fs.readFileSync(cowPath);let cowFmt,cowData;
for(let p=12;p<cow.length;){const n=cow.readUInt32LE(p+4),id=cow.toString('ascii',p,p+4);if(id==='fmt ')cowFmt=cow.subarray(p+8,p+8+n);if(id==='data')cowData=cow.subarray(p+8,p+8+n);p+=8+n+(n&1);}assert.equal(cowFmt.readUInt16LE(2),1);assert.equal(cowFmt.readUInt16LE(14),16);const cowRate=cowFmt.readUInt32LE(4);
defect('cow-instead-of-bird',output=>{editRange(output,from,from+1.2,()=>0);editRange(output,from+.15,from+1.2,i=>{const p=(i/rate-from-.15)*cowRate,a=Math.floor(p);return a+1<cowData.length/2?.2*(cowData.readInt16LE(a*2)*(1-(p-a))+cowData.readInt16LE((a+1)*2)*(p-a))/32768:0;});});
defect('background-in-baseline',output=>editRange(output,positive.baseline.from,positive.baseline.to,i=>.005*Math.sin(i*.15)));
defect('sustained-after-natural-end',output=>editRange(output,positive.naturalEnd.from,positive.naturalEnd.to,i=>.005*Math.sin(i*.15)));
for(const template of positive.firstPrefix.wrongPitch){const rejected=template.correlation<positive.rules.minNormalizedCorrelation;report.push({name:'wrong-template-'+template.cents+'-cents',rejected,correlation:template.correlation});assert(rejected);}
if(run.observer){
  const source=readBirdSource(run.inputs.find(i=>path.basename(i.path)==='FarmGame.dls').path);
  for(const cents of [-100,100])defect('wrong-rendered-pitch-'+cents,output=>{
    editRange(output,from,from+1.2,()=>0);const onset=positive.firstPrefix.correct.time,step=source.rate/rate*2**(((positive.firstPrefix.expectedMidiPitch-source.unity)*100+cents)/1200);
    editRange(output,onset,onset+.35,i=>{const p=(i-onset*rate)*step,a=Math.floor(p);return a>=0&&a+1<source.values.length?.2*(source.values[a]*(1-(p-a))+source.values[a+1]*(p-a)):0;});
  });
}
const result={schema:1,createdUtc:new Date().toISOString(),candidate:positive.candidate,passed:report.every(r=>r.rejected),positivePassed:true,defects:report,bindings:{captureRunSha256:hash(directory+'/run.json'),sourceWavSha256:hash(directory+'/output.wav'),cowSourceSha256:hash(cowPath),auditorSha256:hash(fileURLToPath(new URL('./Inspect-FarmBirdAudio.mjs',import.meta.url))),runnerSha256:hash(fileURLToPath(import.meta.url))},scope:'Actual first-attack positive; silent attack, wrong source, contaminated baseline, missing natural end and two wrong-pitch templates. Native runs additionally reject two wrong rendered pitches against observed MIDI. Main score-pitch errors and whole Farm tempo/mix acceptance are not covered.',fullAcceptance:false};
fs.writeFileSync(out+'/negative-tests.json',JSON.stringify(result,null,2)+'\n',{flag:'wx'});console.log(JSON.stringify({passed:result.passed,defects:report.length,scope:result.scope,fullAcceptance:false}));

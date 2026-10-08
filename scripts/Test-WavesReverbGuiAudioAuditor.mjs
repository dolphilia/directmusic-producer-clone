import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {analyzeWavesPcm,compareWavesPcm} from './Inspect-WavesReverbGuiAudio.mjs';

const [protocolPath,output]=process.argv.slice(2);assert(output);assert(!fs.existsSync(output),'Preserve previous controls');
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const protocol=JSON.parse(fs.readFileSync(protocolPath,'utf8'));
const times={first:{before:10,after:10.1},stopFirst:{before:190,after:190.1},replay:{before:210,after:210.1},stopReplay:{before:230,after:230.1},close:{before:250,after:250.1}};
const rate=8000,frames=rate*300,frequency=m=>440*2**((m-69)/12);
function signal({amplitude69=.015,amplitude60=.02,pitch69=69,earlyEnd=false,afterStop=false,tail=false,silent=false}={}){
  const mono=new Float64Array(frames),energy=new Float64Array(frames);let peak=0;
  const starts=[10.2,210.2];if(!silent)for(let i=0;i<frames;i++){
    const t=i/rate;let x=0;
    for(let n=0;n<starts.length;n++){const onset=starts[n],end=n===0?onset+(earlyEnd?155:160):230.05;
      if(t>=onset&&t<end)x+=amplitude69*Math.sin(2*Math.PI*frequency(pitch69)*(t-onset))+amplitude60*Math.sin(2*Math.PI*frequency(60)*(t-onset));
    }
    if(afterStop&&t>=232&&t<234)x+=.01*Math.sin(2*Math.PI*440*t);
    if(tail&&t>=170.27&&t<170.5)x+=.001*Math.sin(2*Math.PI*440*t);
    mono[i]=x;energy[i]=x*x;peak=Math.max(peak,Math.abs(x));
  }
  return {rate,channels:1,frames,mono,energy,peak};
}
const results=[],positive=(name,fn)=>{fn();results.push({name,expected:'accept',passed:true});},negative=(name,fn,message)=>{let failure;try{fn();}catch(e){failure=e;}assert(failure,'Control must be rejected: '+name);assert.match(failure.message,message);results.push({name,expected:'reject',passed:true,reason:failure.message});};
const analyze=s=>analyzeWavesPcm(s,times,protocol);
const dry=analyze(signal()),wet=analyze(signal({amplitude69:.009}));
positive('Two expected tones, natural note duration, active Stop, silence and replay with affected-route level difference',()=>compareWavesPcm(dry,wet,protocol));
positive('Equal steady level but a new effect tail',()=>compareWavesPcm(dry,analyze(signal({tail:true})),protocol));
negative('Entire recording silent',()=>analyze(signal({silent:true})),/Play reaches render endpoint/);
negative('One routed pitch missing',()=>analyze(signal({amplitude60:0})),/Both expected DLS pitches/);
negative('Affected pitch one semitone high',()=>analyze(signal({pitch69:70})),/Both expected DLS pitches/);
negative('Replay remains audible after Stop',()=>analyze(signal({afterStop:true})),/Stop silence/);
negative('Natural note ends before fixed input duration',()=>analyze(signal({earlyEnd:true})),/Input sustains through expected note duration/);
negative('No audible effect: identical dry and wet',()=>compareWavesPcm(dry,dry,protocol),/Fixed dry\/wet effect difference/);
negative('Unchanged route also changes level',()=>compareWavesPcm(dry,analyze(signal({amplitude60:.03,amplitude69:.009})),protocol),/Unaffected route stability/);
const report={schema:1,createdUtc:new Date().toISOString(),passed:true,positiveCount:2,negativeCount:7,results,recipe:{sampleRate:rate,seconds:300,firstOnset:10.2,replayOnset:210.2,firstDuration:160,replayNoteoff:230.05,dryAmplitudes:{69:.015,60:.02},wetAmplitude69:.009,times},protocolSha256:hash(protocolPath),auditorSha256:hash('scripts/Inspect-WavesReverbGuiAudio.mjs'),controlDriverSha256:hash(process.argv[1]),scope:'Independent fabricated numeric PCM acceptance controls; not product execution or substitute for recorded audio.',fullAcceptance:false};
fs.writeFileSync(output,JSON.stringify(report,null,2)+'\n',{flag:'wx'});console.log(JSON.stringify(report));

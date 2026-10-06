import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const dir=path.resolve(process.argv[2]),file=n=>path.join(dir,n);
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const run=json(file('run.json')),capture=json(file('capture.json')),native=json(file('lifecycle/lifecycle.json'));
assert(['all-unmapped','mixed'].includes(run.variant));
for(const [p,h] of [[run.producer,run.producerSha256],[run.buildSummary,run.buildSummarySha256],[run.recorder,run.recorderSha256],[run.recorderBuildSummary,run.recorderBuildSummarySha256],[run.inputProof,run.inputProofSha256],[file('driver.ps1'),run.driverSha256]])assert.equal(hash(p),h);
const build=json(run.buildSummary);assert(build.passed&&build.sourceSnapshotUnchanged);
for(const s of build.sources)assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);
for(const input of run.inputs)assert.equal(hash(input.path),input.sha256);
function chunks(b,a=0,z=b.length){const out=[];for(let p=a;p<z;){assert(p+8<=z);const n=b.readUInt32LE(p+4),end=p+8+n;assert(end<=z);const id=b.toString('ascii',p,p+4),box=id==='LIST'||id==='RIFF';out.push({id,type:box?b.toString('ascii',p+8,p+12):'',data:b.subarray(p+8,end),children:box?chunks(b,p+12,end):[],pad:b.subarray(end,end+(n&1))});p=end+(n&1);assert(p<=z);}return out;}
function encode(n){const boxed=n.id==='RIFF'||n.id==='LIST',body=boxed?Buffer.concat([Buffer.from(n.type),...n.children.map(encode)]):n.data;const h=Buffer.alloc(8);h.write(n.id);h.writeUInt32LE(body.length,4);return Buffer.concat([h,body,body.length&1?(n.pad.length?n.pad:Buffer.alloc(1)):Buffer.alloc(0)]);}
const flat=ns=>ns.flatMap(n=>[n,...flat(n.children)]);
const input=run.inputs.find(i=>i.path.endsWith('.sgp')),project=run.inputs.find(i=>i.path.endsWith('.pro')),collection=run.inputs.find(i=>i.path.endsWith('.dls'));assert(input&&project&&collection);
assert.equal(hash(file('lifecycle/input.sgp')),input.sha256);assert.equal(hash(file('lifecycle/input-project.pro')),project.sha256);
for(const name of ['source-collection-0.dls','runtime-collection-0.dls','restart-collection-0.dls'])assert.equal(hash(file('lifecycle/'+name)),collection.sha256);
const source=chunks(fs.readFileSync(input.path)),runtime=chunks(fs.readFileSync(file('lifecycle/runtime.sgp')));
const seq=flat(source).filter(n=>n.id==='seqt');assert.equal(seq.length,1);const actualSeq=flat(runtime).filter(n=>n.id==='seqt');assert.equal(actualSeq.length,1);assert(seq[0].data.equals(actualSeq[0].data),'Disconnected authored notes must remain in runtime Sequence');
const events=chunks(seq[0].data).filter(n=>n.id==='evtl');assert.equal(events.length,1);const noteData=events[0].data,noteSize=noteData.readUInt32LE(0);assert.equal(noteSize,20);const sourceNotes=[];for(let p=4;p<noteData.length;p+=noteSize){assert.equal(noteData[p+14]&0xf0,0x90);sourceNotes.push({channel:noteData.readUInt32LE(p+8),pitch:noteData[p+15]});}
assert.equal(sourceNotes.length,run.variant==='mixed'?128:64);assert.equal(sourceNotes.filter(n=>n.channel===0).length,64);if(run.variant==='mixed')assert.equal(sourceNotes.filter(n=>n.channel===16).length,64);
const expected=chunks(fs.readFileSync(input.path));
function prepare(n){if(n.id==='RIFF'&&n.type==='DMBD'){const list=n.children.find(c=>c.type==='lbil');assert(list);list.children=list.children.filter(c=>c.type!=='lbin'||c.children.find(x=>x.id==='bins').data.readUInt32LE(24)===16);return list.children.some(c=>c.type==='lbin')?n:null;}n.children=n.children.map(prepare).filter(Boolean);if(n.type==='lbnd'&&!n.children.some(c=>c.type==='DMBD'))return null;if(n.type==='DMTK'){const bands=n.children.find(c=>c.type==='DMBT');if(bands&&!flat([bands]).some(c=>c.type==='lbnd'))return null;}return n;}
assert(Buffer.concat(expected.map(prepare).filter(Boolean).map(encode)).equals(fs.readFileSync(file('lifecycle/runtime.sgp'))),'Only disconnected Band download data may differ');
const wav=fs.readFileSync(file('output.wav'));assert.equal(wav.toString('ascii',0,4),'RIFF');assert.equal(wav.readUInt32LE(4)+8,wav.length);
let fmt,data;for(let p=12;p+8<=wav.length;){const n=wav.readUInt32LE(p+4);assert(p+8+n<=wav.length);const id=wav.toString('ascii',p,p+4);if(id==='fmt ')fmt=wav.subarray(p+8,p+8+n);if(id==='data')data=wav.subarray(p+8,p+8+n);p+=8+n+(n&1);}
assert(fmt&&data);const rate=fmt.readUInt32LE(4),channels=fmt.readUInt16LE(2),align=fmt.readUInt16LE(12),tag=fmt.readUInt16LE(0);assert.equal(fmt.readUInt16LE(14),32);assert(tag===3||(tag===65534&&fmt.readUInt32LE(24)===3));
const frames=data.length/align;assert.equal(frames,rate*capture.seconds);const mono=new Float64Array(frames);let peak=0;
for(let i=0;i<frames;i++)for(let c=0;c<channels;c++){const x=data.readFloatLE(i*align+c*4);assert(Number.isFinite(x));mono[i]+=x/channels;peak=Math.max(peak,Math.abs(x));}
const packets=fs.readFileSync(file('packets.csv'),'utf8').trim().split(/\r?\n/).slice(1).map(s=>s.split(',').map(Number));assert(packets.length);
const origins=packets.slice(0,100).map(p=>p[4]-p[0]*1e7/rate).sort((a,b)=>a-b),origin=origins[Math.floor(origins.length/2)];let maxGapFrames=0;for(let i=1;i<packets.length;i++)maxGapFrames=Math.max(maxGapFrames,packets[i][0]-packets[i-1][0]-packets[i-1][1]);
const packetIntegrity=packets.every((p,i)=>(p[2]&4)===0&&(!(p[2]&1)||i===0))&&maxGapFrames<=rate*.002;
const required=['play-request','play-return','play-ready','stop-request','stop-return','restart-request','restart-return','restart-ready','final-stop-request','final-stop-return','shutdown-return'];assert.deepEqual(native.phases.map(p=>p.phase),required);
const phases=Object.fromEntries(native.phases.map(p=>[p.phase,(p.qpc100ns-origin)/1e7]));for(let i=1;i<required.length;i++)assert(phases[required[i]]>phases[required[i-1]]);
function rms(a,b){assert(a>=0&&b>a&&b<capture.seconds);let sum=0,n=0;for(let i=Math.round(a*rate);i<Math.round(b*rate);i++){sum+=mono[i]**2;n++;}return Math.sqrt(sum/n);}
function tone(a,b,midi){const coef=2*Math.cos(2*Math.PI*440*2**((midi-69)/12)/(rate/4));let s1=0,s2=0,n=0;const begin=Math.round(a*rate),end=Math.round(b*rate);for(let i=begin;i<end;i+=4){const s=mono[i]*(.5-.5*Math.cos(2*Math.PI*(i-begin)/(end-begin)))+coef*s1-s2;s2=s1;s1=s;n++;}return Math.sqrt(Math.max(0,s1*s1+s2*s2-coef*s1*s2))/n;}
const intervals=[{name:'baseline',from:.3,to:phases['play-request']-.2,sound:false},{name:'first-playing',from:phases['play-ready']+.1,to:phases['stop-request']-.15,sound:run.variant==='mixed'},{name:'stop-hold',from:phases['stop-return']+1.2,to:phases['restart-request']-.2,sound:false},{name:'restarted-playing',from:phases['restart-ready']+.1,to:phases['final-stop-request']-.15,sound:run.variant==='mixed'},{name:'final-stop-hold',from:phases['final-stop-return']+1.2,to:capture.seconds-.3,sound:false}].map(p=>({...p,rms:rms(p.from,p.to)}));
const pitches=intervals.filter(p=>p.sound).map(p=>{const windows=[];for(let t=p.from;t+.1<p.to;t+=.02)windows.push({from:t,rms:rms(t,t+.1),expected65:tone(t,t+.1,65),disconnected60:tone(t,t+.1,60)});windows.sort((a,b)=>b.rms-a.rms);return {name:p.name,window:windows[0]};});
const phaseDurationsPassed=phases['stop-request']-phases['play-ready']>=1.9&&phases['restart-request']-phases['stop-return']>=2.9&&phases['final-stop-request']-phases['restart-ready']>=1.9;
const passed=run.passed&&native.passed&&packetIntegrity&&peak<.99&&phaseDurationsPassed&&intervals.every(p=>p.sound?p.rms>.001:p.rms<.0001)&&pitches.every(p=>p.window.expected65>.00005&&p.window.expected65>p.window.disconnected60*2);
const proof={schema:1,candidate:run.candidate,variant:run.variant,passed,scope:'Owned DLS, disconnected PChannel silence and mapped PChannel65 tone, early Stop/replay; source and runtime Sequence exact, private Band download change only',sourceNotes,phases,intervals,pitches,peak,packetIntegrity,maxGapFrames,phaseDurationsPassed,runSha256:hash(file('run.json')),nativeSha256:hash(file('lifecycle/lifecycle.json')),wavSha256:hash(file('output.wav')),packetsSha256:hash(file('packets.csv')),analyzerSha256:hash(new URL(import.meta.url)),fullAcceptance:false};
fs.writeFileSync(file('audio-proof.json'),JSON.stringify(proof,null,2)+'\n');fs.copyFileSync(new URL(import.meta.url),file('auditor.mjs'));console.log(JSON.stringify({candidate:run.candidate,variant:run.variant,passed,intervals,pitches},null,2));if(!passed)process.exitCode=1;

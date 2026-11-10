import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [scenarioArg, buildArg, audioArg, controlsArg] = process.argv.slice(2);
assert(controlsArg, 'Usage: Inspect-DlsSampleUnit.mjs SCENARIO BUILD AUDIO_CAPTURE CONTROLS');
const scenario=path.resolve(scenarioArg), buildPath=path.resolve(buildArg), audio=path.resolve(audioArg), controlsPath=path.resolve(controlsArg);
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const rel=p=>path.relative(process.cwd(),p).replaceAll('\\','/');
const build=read(buildPath), candidate=path.basename(path.dirname(buildPath)), start=read(scenario+'/unit-start.json');
assert(build.passed&&build.sourceSnapshotUnchanged);assert.equal(start.candidate,candidate);
assert.deepEqual([build.configureExitCode,build.buildExitCode,build.installExitCode],[0,0,0]);
for(const s of build.sources)for(const root of [build.sourceRoot,process.cwd()])assert.equal(hash(path.join(root,s.path)),s.sha256);
for(const o of build.outputs)assert.equal(hash(path.join(path.dirname(buildPath),o.path)),o.sha256);
const exe=build.outputs.find(o=>o.path==='install/bin/Producer.exe');assert(exe);
const inputs=read(scenario+'/final-inputs.json');assert.equal(inputs.candidate,candidate);assert.equal(inputs.inputs.length,4);
for(const i of inputs.inputs){assert.equal(hash(i.path),i.sha256);assert.equal(hash(scenario+'/author/SampleInheritance/'+path.basename(i.path)),i.sha256);}
const initial=read(scenario+'/author-inputs-initial.json');assert.equal(initial.inputs.length,4);
for(const i of initial.inputs)assert.equal(hash(start.carrierSource+'/'+path.basename(i.path)),i.sha256);

function chunks(b,a=0,z=b.length){const cs=[];for(let p=a;p<z;){assert(p+8<=z);const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),e=p+8+n;assert(e+(n&1)<=z);const c={id,data:b.subarray(p+8,e),raw:b.subarray(p,e+(n&1))};if(id==='RIFF'||id==='LIST'){assert(n>=4);c.type=b.toString('ascii',p+8,p+12);c.children=chunks(b,p+12,e);}cs.push(c);p=e+(n&1);}return cs;}
const one=(cs,id,type)=>{const f=cs.filter(c=>c.id===id&&(!type||c.type===type));assert.equal(f.length,1);return f[0];};
function encode(c){if(!c.children)return c.raw;const data=Buffer.concat([Buffer.from(c.type,'ascii'),...c.children.map(encode)]),h=Buffer.alloc(8);h.write(c.id,0,'ascii');h.writeUInt32LE(data.length,4);return Buffer.concat([h,data,...(data.length&1?[Buffer.alloc(1)]:[])]);}
const source=one(chunks(fs.readFileSync(start.carrierSource+'/LoopSource.dls')),'RIFF','DLS ');
const region=one(one(one(source.children,'LIST','lins').children,'LIST','ins ').children,'LIST','lrgn').children[0];
assert.equal(region.type,'rgn ');assert.equal(one(region.children,'wsmp').data.readUInt32LE(16),0);
region.children=region.children.filter(c=>c.id!=='wsmp');
const folder=scenario+'/author-saved/SampleInheritance', savedDls=fs.readFileSync(folder+'/LoopSource.dls');
assert(savedDls.equals(encode(source)),'GUI inheritance must remove only the entire Region WSMP');
for(const name of ['LoopBand.bnp','LoopSong.sgp'])assert.equal(hash(folder+'/'+name),hash(start.carrierSource+'/'+name));
const project=one(chunks(fs.readFileSync(folder+'/SampleInheritance.pro')),'RIFF','JAZP'), entries=[];
for(const c of project.children.filter(c=>c.id==='LIST'&&c.type==='file')){
 const name=one(c.children,'name').data.toString('utf16le').replace(/\0+$/,''),h=one(c.children,'filh').data,p=folder+'/'+name,b=fs.readFileSync(p),root=one(chunks(b),'RIFF');
 assert(!entries.some(e=>e.name===name));assert(h.length>=44);assert.equal(h.readUInt32LE(24),b.length);
 const guid=one(root.children,root.type==='DLS '?'dlid':'guid').data;assert.equal(guid.length,16);assert(guid.equals(h.subarray(28,44)));
 entries.push({name,form:root.type,bytes:b.length,sha256:hash(p),guid:guid.toString('hex')});
}
assert.equal(entries.length,3);assert.deepEqual(entries.map(e=>e.form).sort(),['DLS ','DMBD','DMSG']);

const author=read(scenario+'/author-exit.json'), reload=read(scenario+'/reload-exit.json');assert.notEqual(author.processId,reload.processId);
for(const r of [author,reload]){
 assert.equal(r.exitCode,0);assert.equal(r.forcedTermination,false);assert.equal(r.exeSha256,exe.sha256);assert.equal(hash(r.executable),exe.sha256);
 const basis=r.normalExitBasis, rawPath=basis.rawLaunch??basis.path, rawHash=basis.rawLaunchSha256??basis.sha256;
 assert.equal(hash(rawPath),rawHash);const raw=read(rawPath);assert.equal(raw.processId,r.processId);assert.equal(raw.exitCode,0);assert.equal(raw.timedOut,false);
 assert.equal(hash(basis.driverPath),basis.driverSha256??r.driverSha256);
}
const observations=read(scenario+'/observations/states.json'), observed=[];
for(const [label,pattern] of [
 ['Author explicit one-shot over looping Wave',/Explicit Region sample settings/],
 ['Author inherited settled',/Using Wave sample settings/],
 ['Author Undo explicit settled',/Saved\..*\n[\s\S]*Explicit Region sample settings/],
 ['Author Redo inherited settled',/Unsaved changes\..*\n[\s\S]*Using Wave sample settings/],
 ['Author saved inherited settled',/Saved\..*\n[\s\S]*Using Wave sample settings/],
 ['Author Project saved settled',/LoopSong\.sgp/],
 ['Reload saved inherited settled',/Saved\..*\n[\s\S]*Using Wave sample settings/],
 ['Reload explicit one-shot settled',/Unsaved changes\..*\n[\s\S]*Explicit Region sample settings/],
 ['Reload Undo inherited settled',/Saved\..*\n[\s\S]*Using Wave sample settings/],
 ['Final stopped before normal reload exit',/Stopped\. Applied groups: 1; Tempo track 1; Time signature track 1\. Notes: 2/]
]){const o=observations.find(o=>o.label===label&&pattern.test(o.accessibility.tree));assert(o,label);observed.push({label,utc:o.utc,window:o.window.id});}
const audioPath=audio+'/dls-sample-inheritance-proof.json', audioProof=read(audioPath), controls=read(controlsPath);
assert(audioProof.passed&&!audioProof.fullAcceptance&&controls.passed&&!controls.fullAcceptance);assert.equal(audioProof.candidate,candidate);assert.equal(audioProof.processId,reload.processId);assert.equal(controls.candidate,candidate);
assert.equal(audioProof.analyzerSha256,hash('scripts/Inspect-DlsSampleInheritanceAudio.mjs'));assert.equal(controls.auditorSha256,audioProof.analyzerSha256);assert.equal(controls.driverSha256,hash('scripts/Test-DlsSampleInheritanceAudioAuditor.mjs'));assert.equal(controls.sourceProofSha256,hash(audioPath));
assert.equal(controls.results.length,8);assert(controls.results.every(r=>r.kind==='unchanged'?r.accepted&&r.exitCode===0:r.rejected&&r.exitCode===1));
assert.equal(audioProof.observationsSha256,hash(scenario+'/observations/states.json'));
const proof={schema:1,passed:true,candidate,scope:'DLS Wave sample inheritance and explicit one-shot, private runtime WSMP resolution, native three-document author/save/normal exit/distinct reload and same-PID differential PCM; original dynamic parity and all40/all8 remain separate',buildSummary:{path:rel(buildPath),sha256:hash(buildPath),sources:build.sources.length},exeSha256:exe.sha256,processes:{author:author.processId,reload:reload.processId},project:{path:rel(folder+'/SampleInheritance.pro'),sha256:hash(folder+'/SampleInheritance.pro'),entries},observed,audio:{capture:rel(audio),proof:rel(audioPath),sha256:hash(audioPath),controls:rel(controlsPath),controlsSha256:hash(controlsPath)},auditorSha256:hash(process.argv[1]),fullAcceptance:false};
fs.writeFileSync(scenario+'/unit-proof.json',JSON.stringify(proof,null,2)+'\n');fs.copyFileSync(process.argv[1],scenario+'/unit-auditor.mjs');console.log(JSON.stringify(proof));

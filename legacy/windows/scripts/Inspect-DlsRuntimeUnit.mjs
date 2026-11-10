import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [unitArg,buildArg,nativeArg,runtimeArg]=process.argv.slice(2);
assert(runtimeArg,'Usage: UNIT BUILD NATIVE_CAPTURE RUNTIME_CAPTURE');
const unit=path.resolve(unitArg),buildPath=path.resolve(buildArg),native=path.resolve(nativeArg),runtime=path.resolve(runtimeArg);
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const rel=p=>path.relative(process.cwd(),p).replaceAll('\\','/');
const evidence=p=>({path:rel(p),sha256:hash(p)});
const build=read(buildPath),start=read(unit+'/unit-start.json'),candidate=path.basename(path.dirname(buildPath));
assert(build.passed&&build.sourceSnapshotUnchanged);assert.equal(start.candidate,candidate);
assert.deepEqual([build.configureExitCode,build.buildExitCode,build.installExitCode],[0,0,0]);
assert.equal(start.build.sha256,hash(buildPath));
for(const s of build.sources)for(const root of [build.sourceRoot,process.cwd()])assert.equal(hash(path.join(root,s.path)),s.sha256);
for(const o of build.outputs)assert.equal(hash(path.join(path.dirname(buildPath),o.path)),o.sha256);
const exe=build.outputs.find(o=>o.path==='install/bin/Producer.exe');assert(exe);

// This parser and expected tree operate on raw RIFF bytes, independent of the
// product's Chunk/DlsDocument/export implementation. Preserve opaque padding.
function chunks(b,a=0,z=b.length){const cs=[];for(let p=a;p<z;){assert(p+8<=z);const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),e=p+8+n;assert(e+(n&1)<=z);const c={id,data:b.subarray(p+8,e),raw:b.subarray(p,e+(n&1)),padding:n&1?b[e]:0};if(id==='RIFF'||id==='LIST'){assert(n>=4);c.type=b.toString('ascii',p+8,p+12);c.children=chunks(b,p+12,e);}cs.push(c);p=e+(n&1);}return cs;}
function encode(c){if(!c.children)return c.raw;const data=Buffer.concat([Buffer.from(c.type,'ascii'),...c.children.map(encode)]),h=Buffer.alloc(8);h.write(c.id,0,'ascii');h.writeUInt32LE(data.length,4);return Buffer.concat([h,data,...(data.length&1?[Buffer.from([c.padding])]:[])]);}
const one=(cs,id,type)=>{const found=cs.filter(c=>c.id===id&&(!type||c.type===type));assert.equal(found.length,1,`${id}/${type??''}`);return found[0];};
const root=p=>one(chunks(fs.readFileSync(p)),'RIFF');
const region=r=>one(one(one(r.children,'LIST','lins').children,'LIST','ins ').children,'LIST','lrgn').children[0];
const wave=r=>one(one(r.children,'LIST','wvpl').children,'LIST','wave');
const text=c=>c.data.toString('utf16le').replace(/\0+$/,'');
function stripDesign(c){if(['DLS ','rgn ','rgn2'].includes(c.type))c.children=c.children.filter(v=>v.id!=='dmpr');for(const child of c.children??[])stripDesign(child);}
const originalLaunch=read(unit+'/author-launch.json');
for(const i of originalLaunch.inputs)assert.equal(hash(i.path),i.sha256);
const carrier=path.dirname(originalLaunch.inputs.find(i=>path.basename(i.path)==='LoopSource.dls').path);
const authored=unit+'/author-saved',restored=unit+'/restored-saved';
const expectedNative=root(carrier+'/LoopSource.dls');assert.equal(one(region(expectedNative).children,'wsmp').data.readUInt32LE(16),0);
region(expectedNative).children=region(expectedNative).children.filter(c=>c.id!=='wsmp');
assert(fs.readFileSync(authored+'/LoopSource.dls').equals(encode(expectedNative)),'GUI inheritance changes only Region WSMP absence');
for(const name of ['LoopSource.dls','LoopBand.bnp','LoopSong.sgp']){
 assert.equal(hash(authored+'/'+name),hash(restored+'/'+name));
 assert.equal(hash(restored+'/'+name),hash(unit+'/author/SampleInheritance/'+name));
 if(name!=='LoopSource.dls')assert.equal(hash(authored+'/'+name),hash(carrier+'/'+name));
}
const effective=root(authored+'/LoopSource.dls');region(effective).children.push(one(wave(effective).children,'wsmp'));
stripDesign(effective);
assert(fs.readFileSync(unit+'/standalone/LoopSource.dls').equals(encode(effective)),'Main runtime output resolves Region WSMP and removes declared authoring dmpr only');
assert.equal(hash(unit+'/standalone/LoopSong.sgt'),hash(authored+'/LoopSong.sgp'),'Carrier has no authoring-only Segment records to remove');
assert.deepEqual(fs.readdirSync(unit+'/standalone').sort(),['LoopSong.sgt','LoopSource.dls']);

const project=root(restored+'/SampleInheritance.pro'),expectedProject=root(authored+'/SampleInheritance.pro');assert.equal(project.type,'JAZP');
for(const file of expectedProject.children.filter(c=>c.type==='file')){
 const name=text(one(file.children,'name'));if(!['LoopSource.dls','LoopSong.sgp'].includes(name))continue;
 const raw=Buffer.from('..\\..\\standalone\\\0','utf16le'),h=Buffer.alloc(8);h.write('rdir');h.writeUInt32LE(raw.length,4);
 one(file.children,'LIST','UNFO').children.push({id:'rdir',data:raw,raw:Buffer.concat([h,raw])});
}
assert(fs.readFileSync(restored+'/SampleInheritance.pro').equals(encode(expectedProject)),'Save As updates only two per-file runtime directories, preserving native names/identities/metadata');
assert.equal(hash(restored+'/SampleInheritance.pro'),hash(unit+'/author/SampleInheritance/SampleInheritance.pro'));
const entries=project.children.filter(c=>c.type==='file').map(file=>{
 const name=text(one(file.children,'name')),header=one(file.children,'filh').data,p=restored+'/'+name,document=root(p);
 assert(header.length>=44);assert.equal(header.readUInt32LE(24),fs.statSync(p).size);
 const guid=one(document.children,document.type==='DLS '?'dlid':'guid').data;assert.equal(guid.length,16);assert(header.subarray(28,44).equals(guid));
 return {name,form:document.type,bytes:fs.statSync(p).size,sha256:hash(p),guid:guid.toString('hex')};
});assert.equal(entries.length,3);assert.deepEqual(entries.map(e=>e.form).sort(),['DLS ','DMBD','DMSG']);

// The 44-check mode uses a richer carrier. Independently reproduce exactly
// the contract transforms, including full extensions, opaque data and padding.
const regression='work/acceptance/regression/20261007T060533532Z';
const dedicated=read(regression+'/dls-runtime-export/run.json');assert.equal(dedicated.exitCode,0);assert.equal(dedicated.checks,44);assert.equal(dedicated.timedOut,false);
const core=path.join(dedicated.evidence,'core'),extended=root(core+'/SampleInheritance/LoopSource.dls');
const sample=one(wave(extended).children,'wsmp');assert.equal(sample.data.readUInt32LE(0),22);assert.equal(sample.data.readUInt32LE(22),20);assert.equal(sample.data.at(-1),0x71);
assert.equal(one(wave(extended).children,'smpl').data.length,36);assert.equal(one(region(extended).children,'zzzz').padding,0xad);
region(extended).children=region(extended).children.filter(c=>c.id!=='wsmp');region(extended).children.push(sample);
stripDesign(extended);const exported=encode(extended);
for(const name of ['Standalone.dls','Bulk/LoopSource.dls','Configured/samples/Inherited.dls'])assert(fs.readFileSync(core+'/'+name).equals(exported),name);
const oneShot=root(core+'/OneShot.dls'),explicit=one(region(oneShot).children,'wsmp').data;
const oneShotSample=Buffer.concat([sample.data.subarray(0,22),sample.data.subarray(42)]);oneShotSample.writeUInt32LE(0,16);
assert(explicit.equals(oneShotSample),'Explicit zero-loop output retains extended header and tail, and does not reinherit a loop');
const zeroHeader=Buffer.alloc(8);zeroHeader.write('wsmp');zeroHeader.writeUInt32LE(explicit.length,4);
region(extended).children=region(extended).children.filter(c=>c.id!=='wsmp');region(extended).children.push({id:'wsmp',data:explicit,raw:Buffer.concat([zeroHeader,explicit,Buffer.from([0])])});
assert(fs.readFileSync(core+'/OneShot.dls').equals(encode(extended)),'All other OneShot chunks and padding retained');
assert(!fs.existsSync(core+'/InvalidBulk'));
const relatedIds=['dls-runtime-export','dls-sample-policy','sequence-crud','runtime-saveas-memory','runtime-file-folder','audiopath-export'];
const related=relatedIds.map(id=>{const p=regression+'/'+id+'/run.json',r=read(p);assert.equal(r.exitCode,0,id);assert.equal(r.timedOut,false);return {id,checks:r.checks,processId:r.processId,...evidence(p)};});
const relatedRound=read(regression+'/run.json');assert.equal(relatedRound.buildSummarySha256,hash(buildPath));

const exits=[unit+'/author-exit.json',unit+'/native-timed-session/exit.json',unit+'/runtime-session/exit.json'].map(p=>{const r=read(p);assert.equal(r.exitCode,0);assert.equal(r.forcedTermination,false);assert.equal(r.exeSha256,exe.sha256);assert.equal(hash(r.executable),exe.sha256);assert.equal(r.driverSha256,hash('scripts/Watch-ProductGuiExit.ps1'));return {...evidence(p),processId:r.processId,startUtc:r.startUtc,exitUtc:r.exitUtc};});
assert.equal(new Set(exits.map(e=>e.processId)).size,3);assert(Date.parse(exits[0].exitUtc)<Date.parse(exits[1].startUtc));assert(Date.parse(exits[1].exitUtc)<Date.parse(exits[2].startUtc));
const observed=read(unit+'/observations/states.json');
for(const [label,match] of [
 ['DLS Region explicit settings',/Saved\.[\s\S]*Explicit Region sample settings/],
 ['Inherited Region dirty',/Unsaved changes\.[\s\S]*Using Wave sample settings/],
 ['Undo settled explicit clean state',/Saved\.[\s\S]*Explicit Region sample settings/],
 ['Redo inherited dirty state',/Unsaved changes\.[\s\S]*Using Wave sample settings/],
 ['Inherited native DLS saved clean',/Saved\.[\s\S]*Using Wave sample settings/],
 ['native-restore-dls-inherited-saved',/Saved\.[\s\S]*Using Wave sample settings/],
 ['runtime-memory-native-project-saved',/Notes: 2/]
])assert(observed.some(o=>o.label===label&&match.test(o.accessibility?.tree)),label);
const audioDirs=[native,runtime],audioProofs=audioDirs.map((dir,i)=>{
 const p=dir+'/dls-runtime-audio-proof.json',proof=read(p),run=read(dir+'/run.json'),launch=read(dir+'/gui-launch-at-capture.json');
 assert(proof.passed&&!proof.fullAcceptance);assert.equal(proof.candidate,candidate);assert.equal(proof.mode,i?'runtime':'native');assert.equal(proof.processId,exits[i+1].processId);
 for(const [file,expected] of [[dir+'/run.json',proof.runSha256],[dir+'/output.wav',proof.wavSha256],[run.guiRun+'/actions.json',proof.actionsSha256],[run.guiRun+'/exit.json',proof.exitSha256],[launch.observationLog,proof.observationsSha256],['scripts/Inspect-DlsRuntimeAudio.mjs',proof.analyzerSha256]])assert.equal(hash(file),expected);
 const controlsPath=unit+'/'+(i?'runtime':'native')+'-controls-corrected/negative-tests.json',controls=read(controlsPath);
 assert(controls.passed&&!controls.fullAcceptance);assert.equal(controls.sourceProofSha256,hash(p));assert.equal(controls.auditorSha256,proof.analyzerSha256);assert.equal(controls.driverSha256,hash('scripts/Test-DlsRuntimeAudioAuditor.mjs'));assert.equal(controls.results.length,19);assert(controls.results.every(r=>r.kind==='unchanged'?r.accepted&&r.exitCode===0:r.rejected&&r.exitCode===1));
 return {proof,proofEvidence:evidence(p),controls:evidence(controlsPath),raw:['capture.json','ready.json','packets.csv','endpoint.txt','render-mode.json','driver.ps1','gui-launch-at-capture.json'].map(n=>evidence(dir+'/'+n)),endpoint:fs.readFileSync(dir+'/endpoint.txt','utf8').trim()};
});
assert.equal(audioProofs[0].endpoint,audioProofs[1].endpoint);assert(audioProofs[0].endpoint.length);
const comparisons=[];for(let phase=0;phase<2;phase++)for(let note=0;note<2;note++){
 const a=audioProofs[0].proof.phases[phase].sound[note],b=audioProofs[1].proof.phases[phase].sound[note];
 const relativeRms=Math.abs(a.late-b.late)/a.late,relativePitchEnergy=Math.abs(a.pitch-b.pitch)/a.pitch;
 // Both are the same fixed pure sample via the same OS renderer/endpoint.
 // A one-percent tolerance covers resampling phase; it is not original parity.
 assert(relativeRms<.01&&relativePitchEnergy<.01);comparisons.push({phase:phase+1,note:note+1,relativeRms,relativePitchEnergy});
}
const proof={schema:1,passed:true,candidate,scope:'Bounded inherited DLS standalone/bulk/configured runtime export, native edit/history/save, three normally exited main processes, independent raw RIFF and automatic native/runtime PCM comparison; original dynamic/Q2/all40/all8 unresolved',buildSummary:evidence(buildPath),sourceCount:build.sources.length,exeSha256:exe.sha256,processes:exits,project:{...evidence(restored+'/SampleInheritance.pro'),entries},runtimeDls:evidence(unit+'/standalone/LoopSource.dls'),related,relatedRound:evidence(regression+'/run.json'),extendedRuntime:{core:rel(core),files:['Standalone.dls','Bulk/LoopSource.dls','Configured/samples/Inherited.dls','OneShot.dls'].map(n=>evidence(core+'/'+n))},audio:audioProofs,comparisons,auditorSha256:hash(process.argv[1]),fullAcceptance:false};
fs.writeFileSync(unit+'/unit-proof.json',JSON.stringify(proof,null,2)+'\n');fs.copyFileSync(process.argv[1],unit+'/unit-auditor.mjs');console.log(JSON.stringify(proof));

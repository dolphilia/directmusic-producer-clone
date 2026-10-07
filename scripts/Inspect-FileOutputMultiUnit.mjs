import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';import assert from 'node:assert/strict';
const [unitArg,buildArg,nativeArg,audioArg]=process.argv.slice(2);assert(audioArg,'Usage: Inspect-FileOutputMultiUnit.mjs UNIT BUILD NATIVE_ROUND AUDIO');
const unit=path.resolve(unitArg),buildPath=path.resolve(buildArg),nativePath=path.resolve(nativeArg),audio=path.resolve(audioArg);
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,'')),hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex'),rel=p=>path.relative(process.cwd(),p).replaceAll('\\','/');
const build=read(buildPath),candidate=path.basename(path.dirname(buildPath)),native=read(nativePath);
assert(build.passed&&build.sourceSnapshotUnchanged);assert.deepEqual([build.configureExitCode,build.buildExitCode,build.installExitCode],[0,0,0]);
for(const s of build.sources)for(const root of [build.sourceRoot,process.cwd()])assert.equal(hash(path.join(root,s.path)),s.sha256);
for(const o of build.outputs)assert.equal(hash(path.join(path.dirname(buildPath),o.path)),o.sha256);
assert.equal(native.candidate,candidate);assert(native.results.find(r=>r.id==='file-output-multi'&&r.status==='合格'&&r.checks>=34));
const exe=build.outputs.find(o=>o.path==='install/bin/Producer.exe'),initial=read(unit+'/author-inputs-initial.json'),final=read(unit+'/final-inputs.json');
assert.equal(initial.candidate,candidate);assert.equal(final.candidate,candidate);assert.equal(final.inputs.length,5);assert.equal(initial.inputs.length,5);
for(const i of initial.inputs)assert.equal(hash(i.source),i.sha256);
for(const i of final.inputs){assert.equal(hash(i.path),i.sha256);assert.equal(hash(i.sourcePath),i.sha256);}
const folder=path.dirname(final.inputs.find(i=>i.name==='MultiCapture.pro').path);
for(const name of ['RouteBand.bnp','RouteSource.dls'])assert.equal(hash(folder+'/'+name),initial.inputs.find(i=>path.basename(i.path)===name).sha256);
const fixture=path.dirname(initial.inputs.find(i=>path.basename(i.path)==='MultiCapture.pro').source),core=path.dirname(fixture);
assert.equal(hash(folder+'/RoutePath.aup'),hash(core+'/runtime-input.aup'));
assert.equal(hash(folder+'/RouteSong.sgp'),hash(core+'/runtime-input.sgp'));
for(const name of ['RoutePath.aup','RouteSong.sgp'])assert.notEqual(hash(folder+'/'+name),initial.inputs.find(i=>path.basename(i.path)===name).sha256);
function tree(b,a=0,z=b.length){const cs=[];for(let p=a;p<z;){assert(p+8<=z);const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),e=p+8+n;assert(e+(n&1)<=z);const c={id,data:b.subarray(p+8,e)};if(id==='RIFF'||id==='LIST'){assert(n>=4);c.type=b.toString('ascii',p+8,p+12);c.children=tree(b,p+12,e);}cs.push(c);p=e+(n&1);}return cs;}
const one=(cs,id,type)=>{const a=cs.filter(c=>c.id===id&&(!type||c.type===type));assert.equal(a.length,1);return a[0];};
const project=one(tree(fs.readFileSync(folder+'/MultiCapture.pro')),'RIFF','JAZP'),entries=[];
for(const c of project.children.filter(c=>c.id==='LIST'&&c.type==='file')){
 const name=one(c.children,'name').data.toString('utf16le').replace(/\0+$/,''),h=one(c.children,'filh').data;
 assert.equal(path.basename(name),name);assert(!entries.some(e=>e.name===name));assert(h.length>=44);
 const file=folder+'/'+name,b=fs.readFileSync(file),root=one(tree(b),'RIFF');assert.equal(h.readUInt32LE(24),b.length);
 const guid=one(root.children,root.type==='DLS '?'dlid':'guid').data;assert.equal(guid.length,16);assert(guid.equals(h.subarray(28,44)));
 entries.push({name,form:root.type,bytes:b.length,sha256:hash(file),guid:guid.toString('hex')});
}
assert.equal(entries.length,4);assert.deepEqual(entries.map(e=>e.form).sort(),['DLS ','DMAP','DMBD','DMSG']);
const authorBinding=read(unit+'/author-ui-process.json'),reloadBinding=read(unit+'/reload-session/launch.json');
assert.equal(authorBinding.candidate,candidate);assert.equal(reloadBinding.buildSummarySha256,hash(buildPath));
const processes={};
for(const [name,file,binding] of [['author','author-exit-observer.json',authorBinding],['reload','reload-session/exit-observer.json',reloadBinding]]){
 const e=read(unit+'/'+file);assert.equal(e.state,'exited');assert.equal(e.exitCode,0);assert.equal(e.forcedTermination,false);
 assert.equal(e.exeSha256,exe.sha256);assert.equal(hash(e.executable),exe.sha256);assert.equal(e.processId,binding.processId);
 assert.equal(e.driverSha256,hash('scripts/Watch-ProductGuiExit.ps1'));processes[name]={processId:e.processId,windowId:binding.windowId??binding.window.id,exitCode:0,raw:rel(unit+'/'+file),rawSha256:hash(unit+'/'+file),identityBasis:binding.identityBasis??binding.processBinding};
}
assert.notEqual(processes.author.processId,processes.reload.processId);
const author=read(unit+'/author-exit-observer.json'),reload=read(unit+'/reload-session/exit-observer.json');assert(Date.parse(author.exitUtc)<Date.parse(reload.startUtc));
const observations=read(unit+'/observations/states.json'),observed=[];
for(const [label,pattern,role] of [
 ['Author AudioPath initial one effect',/FileOutput effects: 1/,'author'],
 ['Author AudioPath added two effects settled',/FileOutput effects: 2/,'author'],
 ['Undo second FileOutput addition immediate',/FileOutput effects: 1/,'author'],
 ['Author AudioPath redo two effects settled',/FileOutput effects: 2/,'author'],
 ['AudioPath saved',/FileOutput effects: 2/,'author'],
 ['Final native Project save complete',/RouteSong\.sgp/,'author'],
 ['Saved native Project reloaded in distinct process',/RouteSong\.sgp/,'reload'],
 ['Reload restored AudioPath FileOutput count two',/FileOutput effects: 2/,'reload'],
 ['Recording started restored Project',/Buffer recording started/,'reload'],
 ['Reload clean main before normal exit',/Stopped\. Applied groups: 1; Tempo track 1; Time signature track 1\. Notes: 2/,'reload']
]){const o=observations.find(o=>o.label===label&&pattern.test(o.accessibility?.tree??''));assert(o,label);assert.equal(o.window.app.replaceAll('\\','/').toLowerCase(),'process:'+author.executable.replaceAll('\\','/').toLowerCase());assert(Date.parse(o.utc)>=Date.parse(role==='author'?author.startUtc:reload.startUtc)&&Date.parse(o.utc)<Date.parse(role==='author'?author.exitUtc:reload.exitUtc));observed.push({label,utc:o.utc,window:o.window.id});}
const audioPath=audio+'/file-output-multi-gui-audio-proof.json',audioProof=read(audioPath),audioControlsPath=unit+'/gui-audio-controls/negative-tests.json',audioControls=read(audioControlsPath),nativeControlsPath=unit+'/native-pcm-controls/negative-tests.json',nativeControls=read(nativeControlsPath);
assert(audioProof.passed&&!audioProof.fullAcceptance);assert.equal(audioProof.candidate,candidate);assert.equal(audioProof.processId,reload.processId);assert.equal(audioProof.observationsSha256,hash(unit+'/observations/states.json'));
for(const [controls,driver,auditor,total] of [[audioControls,'scripts/Test-FileOutputMultiGuiAudioAuditor.mjs','scripts/Inspect-FileOutputMultiGuiAudio.mjs',8],[nativeControls,'scripts/Test-FileOutputMultiPcmAuditor.mjs','scripts/Inspect-FileOutputMultiPcm.mjs',7]]){
 assert(controls.passed&&!controls.fullAcceptance);assert.equal(controls.candidate,candidate);assert.equal(controls.driverSha256,hash(driver));assert.equal(controls.auditorSha256,hash(auditor));assert.equal(controls.results.length,total);
 assert(controls.results.every(r=>r.kind==='unchanged'?r.accepted&&r.exitCode===0:r.rejected&&r.exitCode===1));
}
assert.equal(audioControls.sourceProofSha256,hash(audioPath));assert.equal(nativeControls.sourceProofSha256,hash(nativeControls.sourceCapture+'/file-output-multi-pcm-proof.json'));
const pcmPath=unit+'/recordings/file-output-multi-pcm-proof.json',pcm=read(pcmPath);assert(pcm.passed&&!pcm.fullAcceptance);assert.equal(pcm.candidate,candidate);assert.equal(audioProof.fileOutputProofSha256,hash(pcmPath));
for(const f of pcm.recordings)assert.equal(hash(f.path),f.sha256);
const proof={schema:1,passed:true,candidate,scope:'Two directly routed FileOutput buffers, native route ordering and atomic recording lifecycle, main add/UndoRedo/save/normal exit/distinct restore, independent numbered PCM and same-PID WASAPI. Original dynamics, active GUI interruption, tempo change, Sends and all40/all8 remain.',
 buildSummary:{path:rel(buildPath),sha256:hash(buildPath),sources:build.sources.length},exeSha256:exe.sha256,nativeRound:{path:rel(nativePath),sha256:hash(nativePath)},processes,project:{path:rel(folder+'/MultiCapture.pro'),sha256:hash(folder+'/MultiCapture.pro'),entries},observed,
 pcm:{proof:rel(pcmPath),sha256:hash(pcmPath),controls:rel(nativeControlsPath),controlsSha256:hash(nativeControlsPath)},audio:{capture:rel(audio),proof:rel(audioPath),sha256:hash(audioPath),controls:rel(audioControlsPath),controlsSha256:hash(audioControlsPath),activeGuiStopsPassed:audioProof.activeGuiStopsPassed,recordingStopWhileAudibleEstablished:audioProof.recordingStopWhileAudibleEstablished},auditorSha256:hash(process.argv[1]),fullAcceptance:false};
fs.writeFileSync(unit+'/unit-proof.json',JSON.stringify(proof,null,2)+'\n');fs.copyFileSync(process.argv[1],unit+'/unit-auditor.mjs');console.log(JSON.stringify(proof));

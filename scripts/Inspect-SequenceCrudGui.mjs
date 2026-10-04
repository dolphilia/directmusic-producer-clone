import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const self=fileURLToPath(import.meta.url),repo=path.resolve(path.dirname(self),'..');
const read=p=>fs.readFileSync(p),hash=b=>crypto.createHash('sha256').update(b).digest('hex'),json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,''));
const dir=path.resolve(process.argv[2]),statesPath=path.join(dir,'states.json'),states=json(statesPath),build=json(states.build),native=json(states.nativeRun),host=json(states.run);
assert(build.passed&&build.sourceSnapshotUnchanged); assert.equal(hash(read(states.executable)),states.executableSha256);
for(const s of build.sources){assert.equal(hash(read(path.join(build.sourceRoot,s.path))),s.sha256);assert.equal(hash(read(path.join(repo,s.path))),s.sha256);}
assert.equal(native.buildSummarySha256,hash(read(states.build)));assert(native.passed&&native.checks===26);
assert.equal(host.buildSummarySha256,native.buildSummarySha256);assert(host.cases[0].passed&&host.cases[0].sha256===states.executableSha256);
const playablePath=path.join(path.dirname(path.resolve(states.nativeRun)),'core','sequence-crud','playable.sgp'),before=read(path.join(dir,'owned.sgp')),after=read(path.join(dir,'edited.sgp'));
assert(before.equals(read(playablePath)));assert.equal(hash(before),native.files.find(f=>f.name==='playable.sgp').sha256);
function parse(b,s=0,e=b.length){const out=[];for(let p=s;p<e;){assert(p+8<=e);const size=b.readUInt32LE(p+4),stop=p+8+size,next=stop+(size&1);assert(next<=e);const id=b.toString('ascii',p,p+4),container=id==='RIFF'||id==='LIST';if(container)assert(size>=4);out.push({id,type:container?b.toString('ascii',p+8,p+12):'',start:p,raw:b.subarray(p,next),data:b.subarray(p+8,stop),children:container?parse(b,p+12,stop):[]});p=next;}return out;}
const one=(c,id,type)=>{const a=c.children.filter(x=>x.id===id&&(type===undefined||x.type===type));assert.equal(a.length,1);return a[0];};
const root=parse(before)[0];assert.equal(root.type,'DMSG');const list=one(root,'LIST','trkl');assert.equal(list.children.length,3);const seq=one(list.children[2],'seqt'),events=parse(seq.data).find(c=>c.id==='evtl');assert(events);assert.equal(events.data.readUInt32LE(0),20);assert.equal(events.data.length,144);
const records=[];for(let p=4;p<events.data.length;p+=20)records.push(Buffer.from(events.data.subarray(p,p+20)));
const target=records.shift();assert.equal(target.readInt32LE(0),0);assert.equal(target[15],65);target.writeInt32LE(5000,0);target.writeInt32LE(288,4);target[15]=72;target[16]=100;records.push(target);
const expected=Buffer.from(before),offset=seq.start+8+events.start+12;Buffer.concat(records).copy(expected,offset);assert(after.equals(expected),'Only the selected record changes/moves; all other bytes must survive');
const record=label=>{const r=states.records.find(x=>x.label===label);assert(r,`Missing GUI observation ${label}`);return r;};
for(const r of states.records){assert.equal(hash(read(path.join(dir,r.screenshot))),r.screenshotSha256);assert.equal(r.window.app,'process:'+states.executable);}
const tree=label=>record(label).accessibility.tree;
for(const label of ['moved-note-selected','edited-saved','group1-restored-settled','delete-cancelled']){assert.match(tree(label),/Sequence note Value: Note 7: 5000 clocks, pitch 72/);assert.match(tree(label),/Duration \(clocks\) Value: 288/);assert.match(tree(label),/Velocity Value: 100/);assert.match(tree(label),/Notes: 7\./);}
assert.match(tree('group2-empty-settled'),/コンボ ボックス \(disabled, settable, string\) Sequence note/);assert.match(tree('group2-empty-settled'),/Applied groups: 2;.*Notes: 0\./);
assert.match(tree('delete-confirmation'),/ダイアログ Delete Sequence Note/);assert.doesNotMatch(tree('delete-cancelled'),/ダイアログ Delete Sequence Note/);
const runtimePath=path.resolve(process.argv[3]),runtime=json(runtimePath),runtimeProofPath=path.join(path.dirname(runtimePath),'group-playback','group-playback-proof.json'),runtimeProof=json(runtimeProofPath),modulesPath=path.join(path.dirname(runtimePath),'group-playback-module-provenance.json'),modules=json(modulesPath);
assert.equal(runtime.buildSummarySha256,native.buildSummarySha256);assert.equal(runtime.groupPlaybackInput.sha256,hash(after));assert.equal(runtime.cases[0].sha256,states.executableSha256);assert(runtimeProof.passed&&runtimeProof.wholeSnapshotExact);assert.equal(runtimeProof.queries,9);assert.equal(runtimeProof.parameterCount,6);assert(modules.passed);assert(modules.modules.every(m=>m.originalHashMatches.length===0));
const guiModulesPath=path.join(dir,'gui-module-provenance.json'),guiModules=json(guiModulesPath);assert(guiModules.passed);assert(guiModules.modules.every(m=>m.originalHashMatches.length===0));
const result={schema:1,passed:true,createdUtc:new Date().toISOString(),build:states.build,buildSha256:native.buildSummarySha256,exeSha256:states.executableSha256,sourceCount:build.sources.length,nativeRun:states.nativeRun,nativeRunSha256:hash(read(states.nativeRun)),nativeChecks:26,states:statesPath,statesSha256:hash(read(statesPath)),captures:states.records.length,files:['owned.sgp','edited.sgp'].map(name=>({name,sha256:hash(read(path.join(dir,name))),exact:true})),runtime:runtimePath,runtimeSha256:hash(read(runtimePath)),runtimeProofSha256:hash(read(runtimeProofPath)),moduleProofSha256:hash(read(modulesPath)),auditorSha256:hash(read(self)),scope:'GUI move selection, empty group disable/restore, save exact record patch, delete cancellation visible state; same-build saved input runtime Play/Stop and 9 group/6 Tempo queries. Delete confirmation, Undo/Redo GUI, project reload, current audio, original comparison still pending.',fullAcceptance:false};
result.guiModules={path:guiModulesPath,sha256:hash(read(guiModulesPath)),count:guiModules.modules.length,originalMatches:0,scope:'Current GUI process at deletion-confirmation time only'};
fs.writeFileSync(path.join(dir,'sequence-crud-gui-progress-proof.json'),JSON.stringify(result,null,2)+'\n');fs.copyFileSync(self,path.join(dir,'sequence-crud-gui-progress-auditor.mjs'));console.log(JSON.stringify({passed:true,captures:result.captures,proof:path.join(dir,'sequence-crud-gui-progress-proof.json')}));


import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const dir=path.resolve(process.argv[2]);
const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const read=p=>fs.readFileSync(p);
const json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,''));
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const state=json(path.join(dir,'states.json')),run=json(state.run),build=json(state.build);
assert.equal(run.buildSummarySha256,hash(read(state.build)));
assert.equal(build.passed,true);
const host=run.cases.find(c=>c.name==='host-smoke');
assert.equal(host.passed,true);
assert.equal(hash(read(state.executable)),host.sha256);
assert(build.outputs.some(o=>path.resolve(path.dirname(state.build),o.path)===path.resolve(state.executable)&&o.sha256===host.sha256));
for(const s of build.sources){assert.equal(hash(read(path.join(build.sourceRoot,s.path))),s.sha256);assert.equal(hash(read(path.join(repo,s.path))),s.sha256);}
const coreDir=path.join(path.dirname(state.run),'core/meter-batch');
const nativeProof=json(path.join(coreDir,'meter-batch-proof.json'));
assert.equal(nativeProof.runSha256,hash(read(state.run)));
assert.equal(nativeProof.buildSha256,hash(read(state.build)));
assert.equal(nativeProof.workspaceSourcesMatched,true);
assert(nativeProof.cases.every(c=>c.wholeBytesMatch===true));
const before=read(path.join(coreDir,'indexed-before.sgp')),after=read(path.join(coreDir,'indexed-edited.sgp'));
const files=[];
for(const [name,expected] of [['edited-saved.sgp',after],['undo-saved.sgp',before],['redo-saved.sgp',after],['source.sgp',after],['resaved-after-restart.sgp',after]]){
 const bytes=read(path.join(dir,name));assert(bytes.equals(expected),name+' whole bytes');files.push({name,bytes:bytes.length,sha256:hash(bytes),expectedSha256:hash(expected),exact:true});
}
for(const r of state.records){assert.equal(hash(read(path.join(dir,r.screenshot))),r.sha256);}
const moduleProof=json(path.join(dir,'gui-module-provenance.json'));
assert.equal(moduleProof.passed,true);assert.equal(moduleProof.exeSha256,host.sha256);
const required=['selection-applied-confirmed','meter-edited','undo-save-confirmed','redo-save-confirmed','untitled-selected','document-context-restored','project-saved','separate-launch','project-reloaded','reloaded-document-list','reloaded-source-default-selection','reloaded-selection-confirmed','reloaded-document-resaved','reloaded-project-resaved'];
for(const n of required)assert(state.records.some(r=>r.name===n),n+' record');
const context=state.records.find(r=>r.name==='document-context-restored').tree;
assert.match(context,/Applied groups: 2; Tempo track 2; Time signature track 2/);
assert.match(context,/3840 clocks 150 BPM 2:1:0/);
assert.match(context,/Initial meter: 5\/4/);
const reload=state.records.find(r=>r.name==='reloaded-selection-confirmed').tree;
assert.match(reload,/Applied groups: 2; Tempo track 2; Time signature track 2/);
assert.match(reload,/3840 clocks 150 BPM 2:1:0/);
assert.match(reload,/Initial meter: 5\/4/);
assert.match(state.records.find(r=>r.name==='reloaded-source-default-selection').tree,/Applied groups: 1; Tempo track 1; Time signature track 1/);
assert.equal(state.normalCloseObserved,true);
assert.equal(state.resaveNormalCloseObserved,true);
// Decode the saved RIFF independently of the Framework implementation.
function project(name,expected){
 const bytes=read(path.join(dir,name));assert.equal(bytes.toString('ascii',0,4),'RIFF');assert.equal(bytes.toString('ascii',8,12),'DMPJ');assert.equal(bytes.readUInt32LE(4)+8,bytes.length);
 const refs=[],chunks=[];
 for(let p=12;p<bytes.length;){assert(p+8<=bytes.length);const id=bytes.toString('ascii',p,p+4),n=bytes.readUInt32LE(p+4),end=p+8+n;assert(end<=bytes.length);const data=bytes.subarray(p+8,end);chunks.push({id,data});if(id==='file'){assert.equal(n%2,0);refs.push(data.toString('utf16le').replace(/\0+$/,''));}p=end+(n&1);assert(p<=bytes.length);}
 assert.deepEqual(refs,expected);const vers=chunks.find(c=>c.id==='vers');assert(vers);assert.equal(vers.data.length,4);assert.equal(vers.data.readUInt32LE(),1);
 const references=refs.map(ref=>{const resolved=path.resolve(dir,ref);const b=read(resolved);return {reference:ref,resolved,sha256:hash(b),bytes:b.length};});
 return {name,sha256:hash(bytes),bytes:bytes.length,references,chunks};
}
const originalProject=project('project.dmpj',['empty.sgp','source.sgp']);
const resavedProject=project('resaved-project.dmpj',['empty.sgp','resaved-after-restart.sgp']);
assert.deepEqual(originalProject.chunks.filter(c=>c.id!=='file'),resavedProject.chunks.filter(c=>c.id!=='file'),'project metadata retained');
assert.equal(originalProject.references[1].sha256,hash(after));assert.equal(resavedProject.references[1].sha256,hash(after));
const projects=[originalProject,resavedProject].map(({chunks,...p})=>p);
const output={schema:1,passed:true,createdUtc:new Date().toISOString(),build:state.build,buildSha256:hash(read(state.build)),run:state.run,runSha256:hash(read(state.run)),exeSha256:host.sha256,sourceCount:build.sources.length,records:state.records.length,statesSha256:hash(read(path.join(dir,'states.json'))),files,projects,moduleCount:moduleProof.moduleCount,scope:'Current GUI group/index selection, meter edit, Undo/Redo, whole-byte saves, per-document context, project save, normal close, separate launch, reload and resave. Project reference retarget and metadata retained. Screenshots visually inspected during actions; accessibility can lag one observation. GUI exit code not measured.',fullHostAcceptance:false,reloadResave:'passed for indexed fixture',runtimeAudio:'unexecuted for this build',auditorSha256:hash(read(fileURLToPath(import.meta.url)))};
fs.writeFileSync(path.join(dir,'group-gui-proof.json'),JSON.stringify(output,null,2)+'\n');
fs.copyFileSync(fileURLToPath(import.meta.url),path.join(dir,'Inspect-TrackGroupsGui.mjs'));
console.log(JSON.stringify({passed:true,files:files.length,records:output.records,reloadResave:output.reloadResave}));

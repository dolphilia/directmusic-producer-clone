import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';

// Audit recorded GUI actions and files; this script does not operate the GUI.
assert(process.argv[2], 'Supply a GUI evidence directory');
const dir=path.resolve(process.argv[2]);
const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const read=p=>fs.readFileSync(p);
const json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/, ''));
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const fileHash=p=>hash(read(p));
const state=json(path.join(dir, 'states.json'));
const build=json(state.build), run=json(state.run), input=json(path.join(dir, 'input.json'));
assert.equal(build.passed, true);
assert.equal(run.buildSummarySha256, fileHash(state.build));
const host=run.cases.find(c=>c.name==='host-smoke');
const core=run.cases.find(c=>c.name==='core');
assert(host?.passed && core?.passed, 'Same-build native input producer must pass');
assert.equal(fileHash(state.executable), host.sha256);
assert(build.outputs.some(o=>path.resolve(path.dirname(state.build),o.path)===path.resolve(state.executable)&&o.sha256===host.sha256));
for(const s of build.sources){
    assert.equal(fileHash(path.join(build.sourceRoot,s.path)), s.sha256, 'Saved source '+s.path);
    assert.equal(fileHash(path.join(repo,s.path)), s.sha256, 'Workspace source '+s.path);
}
assert.equal(path.resolve(input.nativeRun), path.resolve(state.run));
const nativeDir=path.join(path.dirname(state.run),'core/meter-batch');
assert.equal(path.resolve(input.path), path.join(nativeDir,'indexed-before.sgp'));
assert.equal(path.resolve(input.expectedEdited), path.join(nativeDir,'indexed-edited.sgp'));
assert.equal(fileHash(input.path), input.sha256);
const native=json(path.join(nativeDir,'meter-batch-proof.json'));
assert.equal(native.runSha256, fileHash(state.run));
assert.equal(native.buildSha256, fileHash(state.build));
assert.equal(native.workspaceSourcesMatched, true);
assert.equal(native.onlyMeterBeatAndTwoClockFieldsChanged, true);
assert.equal(native.files['indexed-before.sgp'], input.sha256);
assert.equal(native.files['indexed-edited.sgp'], fileHash(input.expectedEdited));

function chunks(b,start,end){
    const result=[];
    for(let at=start;at<end;){
        assert(at+8<=end); const id=b.toString('ascii',at,at+4), n=b.readUInt32LE(at+4);
        const data=at+8, stop=data+n, next=stop+(n&1); assert(next<=end);
        const container=id==='RIFF'||id==='LIST'; assert(!container||n>=4);
        result.push({id,type:container?b.toString('ascii',data,data+4):'',data,stop,
            children:container?chunks(b,data+4,stop):[]}); at=next;
    }
    return result;
}
const find=(c,id,type)=>c.children.find(x=>x.id===id&&(!type||x.type===type));
const before=read(input.path), expected=Buffer.from(before), root=chunks(before,0,before.length);
assert.equal(root.length,1); assert.equal(root[0].type,'DMSG');
const tracks=find(root[0],'LIST','trkl').children;
assert.equal(tracks.length,5);
assert.deepEqual(tracks.map(t=>before.readUInt32LE(find(t,'trkh').data+20)),[1,2,2,2,2]);
const firstMeter=find(find(tracks[1],'LIST','TIMS'),'tims');
const secondMeter=find(find(tracks[2],'LIST','TIMS'),'tims');
assert.equal(before[firstMeter.data+8],4); assert.equal(before[secondMeter.data+8],3);
expected[secondMeter.data+8]=5;
for(const t of tracks.slice(3)){
    const tempo=find(t,'tetr'); assert.equal(before.readUInt32LE(tempo.data),16);
    assert.equal(before.readInt32LE(tempo.data+4),2304);
    assert.equal(before.readDoubleLE(tempo.data+12),150);
    expected.writeInt32LE(3840,tempo.data+4);
}
assert(expected.equals(read(input.expectedEdited)), 'Independent three-field patch');
const files=['source.sgp','resaved.sgp'].map(name=>{
    const b=read(path.join(dir,name)); assert(b.equals(expected),name+' whole bytes');
    return {name,sha256:hash(b),bytes:b.length,exact:true};
});
for(const r of state.records)assert.equal(fileHash(path.join(dir,r.screenshot)),r.sha256);
const record=name=>{const r=state.records.find(r=>r.name===name); assert(r,name); return r;};
assert.match(record('source-loaded').tree,/768 clocks 130 BPM/);
assert.match(record('applied-group2-stable').tree,/2304 clocks 150 BPM/);
assert.match(record('applied-group2-stable').tree,/Initial meter: 3\/4/);
assert.match(record('file-menu-save-edited').tree,/3840 clocks 150 BPM/);
assert.match(record('file-menu-save-edited').tree,/Initial meter: 5\/4/);
assert.match(record('reloaded-source-default').tree,/Applied groups: 1; Tempo track 1; Time signature track 1/);
assert.match(record('reloaded-source-default').tree,/Initial meter: 4\/4/);
assert.match(record('reloaded-resaved').tree,/3840 clocks 150 BPM/);
assert.match(record('reloaded-resaved').tree,/Applied groups: 2; Tempo track 2; Time signature track 2/);
assert.match(record('reloaded-resaved').tree,/Initial meter: 5\/4/);
// Capture may lag one input: the Play screen was visually checked and its
// Playing status is present in the following Stop capture's accessibility tree.
assert.match(record('reloaded-stop').tree,/Playing document snapshot/);
assert.match(record('file-menu-resaved-project').tree,/Stopped\. Applied groups: 2;/);
const projects=[['project.dmpj',['empty.sgp','source.sgp']],['resaved-project.dmpj',['empty.sgp','resaved.sgp']]].map(([name,refs])=>{
    const b=read(path.join(dir,name)), roots=chunks(b,0,b.length); assert.equal(roots.length,1);
    assert.equal(roots[0].type,'DMPJ'); assert.equal(b.readUInt32LE(4)+8,b.length);
    const cs=roots[0].children, actual=cs.filter(c=>c.id==='file').map(c=>b.subarray(c.data,c.stop).toString('utf16le').replace(/\0+$/,''));
    assert.deepEqual(actual,refs); const version=find(roots[0],'vers'); assert.equal(b.readUInt32LE(version.data),1);
    return {name,sha256:hash(b),bytes:b.length,references:refs.map(reference=>({reference,sha256:fileHash(path.join(dir,reference))})),metadata:cs.filter(c=>c.id!=='file').map(c=>({id:c.id,hex:b.subarray(c.data,c.stop).toString('hex')}))};
});
assert.deepEqual(projects[0].metadata,projects[1].metadata);
const modules=json(path.join(dir,'gui-module-provenance.json'));
assert.equal(modules.passed,true); assert.equal(modules.exeSha256,host.sha256);
assert.equal(modules.captureSha256,fileHash(path.join(dir,'gui-modules.json')));
const close=json(path.join(dir,'normal-close.json'));
assert.equal(close.normalCloseObserved,true); assert.equal(close.exitCode,null);
assert(!close.windows.some(w=>w.id===close.window.id));
assert(state.records.every(r=>r.window.id===close.window.id));
assert.equal(close.window.app.replaceAll('\\','/').toLowerCase(),('process:'+state.executable).replaceAll('\\','/').toLowerCase());
const proof={schema:1,passed:true,createdUtc:new Date().toISOString(),build:state.build,buildSha256:fileHash(state.build),run:state.run,runSha256:fileHash(state.run),exeSha256:host.sha256,sourceCount:build.sources.length,records:state.records.length,statesSha256:fileHash(path.join(dir,'states.json')),inputSha256:input.sha256,nativeProofSha256:fileHash(path.join(nativeDir,'meter-batch-proof.json')),files,projects,moduleCount:modules.moduleCount,moduleProofSha256:fileHash(path.join(dir,'gui-module-provenance.json')),normalCloseEvidenceSha256:fileHash(path.join(dir,'normal-close.json')),normalCloseObserved:true,exitCode:null,scope:'Current GUI group2/Tempo2/TimeSig2 selection, indexed meter edit, two Tempo clocks reanchored, exact save, project reload in same process, default selection reset/reapply, whole-byte resave, project reference retarget with metadata preserved, Play/Stop, one post-Stop module inventory, normal close. Screenshots inspected; accessibility may lag.',pending:['Separate-process reload for this build','Actual audio for this build: this input has no notes','Original dynamic comparison and Band ordering','All40/all8 acceptance'],fullHostAcceptance:false,auditorSha256:fileHash(fileURLToPath(import.meta.url))};
fs.writeFileSync(path.join(dir,'parameter-gui-proof.json'),JSON.stringify(proof,null,2)+'\n');
fs.copyFileSync(fileURLToPath(import.meta.url),path.join(dir,'Inspect-ParameterGui.mjs'));
console.log(JSON.stringify({passed:true,files:files.length,records:proof.records,moduleCount:modules.moduleCount,fullHostAcceptance:false}));

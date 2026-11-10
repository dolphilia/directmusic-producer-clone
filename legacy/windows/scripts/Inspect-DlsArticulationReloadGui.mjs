import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const dir=path.resolve(process.argv[2]);
const read=p=>fs.readFileSync(p), json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const launch=json(dir+'/launch.json'), build=json(launch.buildSummary);
assert(build.passed&&build.sourceSnapshotUnchanged);
assert.equal(hash(launch.buildSummary),launch.buildSummarySha256);
assert.equal(hash(launch.executable),launch.exeSha256);
for(const s of build.sources){assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);assert.equal(hash(s.path),s.sha256);}
const exited=json(launch.priorExit);
assert.equal(exited.state,'exited');assert.equal(exited.exitCode,0);assert.equal(exited.forcedTermination,false);
assert.notEqual(exited.processId,launch.processId);assert.equal(exited.exeSha256,launch.exeSha256);
assert(Date.parse(exited.exitUtc)<Date.parse(launch.createdUtc));
assert.equal(hash(launch.inputProject),launch.inputProjectSha256);
const project=read(launch.inputProject);
assert.equal(project.toString('ascii',0,4),'RIFF');assert.equal(project.readUInt32LE(4)+8,project.length);
assert.equal(project.toString('ascii',8,12),'DMPJ');
const files=[];let at=12;
while(at<project.length){const id=project.toString('ascii',at,at+4),n=project.readUInt32LE(at+4);assert(at+8+n+(n&1)<=project.length);if(id==='file')files.push(project.subarray(at+8,at+8+n).toString('utf16le').replace(/\0+$/,''));at+=8+n+(n&1);}
assert.deepEqual(files,['Initial.sgp','Collection.dls']);
const inputDir=path.dirname(launch.inputProject), inputDls=path.join(inputDir,'TwoLevels.dls');
assert(read(inputDls).equals(read(dir+'/Resaved.dls')));
const states={};
for(const n of ['03-project-loaded','05-dls-restored','06-instrument-restored','07-region-restored','08-level2-restored','09-resaved']){
 const s=json(dir+'/'+n+'.json');assert(Date.parse(s.time)>=Date.parse(launch.createdUtc));assert(s.window.app.toLowerCase().replaceAll('\\','/').endsWith(launch.executable.toLowerCase().replaceAll('\\','/')));states[n]=s;
}
assert.equal(states['03-project-loaded'].window.id,launch.windowId);
const tree=n=>states[n].accessibility.tree;
assert.match(tree('06-instrument-restored'),/Owner Value: Instrument 1/);
assert.match(tree('06-instrument-restored'),/Block 1 — lar2 \/ art1/);
assert.match(tree('06-instrument-restored'),/Value: 1280 ID: 22/);
assert.match(tree('06-instrument-restored'),/Value: -65536 ID: 24/);
assert.match(tree('07-region-restored'),/Owner Value: Region 1 override/);
assert.match(tree('07-region-restored'),/Block 1 — lart \/ art1/);
assert.match(tree('07-region-restored'),/Value: 1 ID: 22/);
assert.match(tree('07-region-restored'),/Value: -65536 ID: 24/);
assert.match(tree('08-level2-restored'),/Block 2 — lar2 \/ art2/);
assert.match(tree('08-level2-restored'),/コンボ ボックス \(disabled, settable, string\) Connection ID: 12/);
assert.match(tree('09-resaved'),/Saved\. Closing/);
const capture=json(dir+'/gui-modules.json'), provenance=json(dir+'/gui-module-provenance.json');
assert.equal(capture.processId,launch.processId);assert(provenance.passed);assert.equal(provenance.captureSha256,hash(dir+'/gui-modules.json'));assert.equal(provenance.exeSha256,launch.exeSha256);
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,scope:'Same saved 65-source product: normal prior exit0, separate GUI DMPJ project reload, Instrument/Region Level1 and empty Level2 UI restore, exact DLS resave, one address-backed module snapshot. Native JAZP, audio and full acceptance unexecuted.',processId:launch.processId,priorProcessId:exited.processId,exeSha256:launch.exeSha256,buildSummarySha256:launch.buildSummarySha256,projectForm:'DMPJ',projectSha256:hash(launch.inputProject),segmentSha256:hash(path.join(inputDir,'Initial.sgp')),dlsSha256:hash(dir+'/Resaved.dls'),exitEvidenceSha256:hash(launch.priorExit),guiModuleProofSha256:hash(dir+'/gui-module-provenance.json'),auditorSha256:hash(new URL(import.meta.url)),fullAcceptancePassed:false};
fs.copyFileSync(new URL(import.meta.url),dir+'/reload-gui-auditor.mjs');fs.writeFileSync(dir+'/reload-gui-proof.json',JSON.stringify(proof,null,2)+'\n');console.log(JSON.stringify(proof,null,2));

import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const unit=path.resolve(process.argv[2]), coreRunPath=path.resolve(process.argv[3]);
const read=p=>fs.readFileSync(p);
const json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const launch=json(path.join(unit,'launch.json'));
const preparation=json(launch.preparation), build=json(launch.buildSummary);
const coreRun=json(coreRunPath), core=path.join(path.dirname(coreRunPath),'core');
assert.equal(hash(launch.buildSummary),launch.buildSummarySha256);
assert.equal(hash(launch.executable),launch.exeSha256);
assert.equal(preparation.exeSha256,launch.exeSha256);
assert.equal(preparation.buildSummarySha256,launch.buildSummarySha256);
assert.equal(coreRun.buildSummarySha256,launch.buildSummarySha256);
assert(coreRun.passed&&coreRun.checks===17&&coreRun.exitCode===0);
assert(build.passed&&build.sourceSnapshotUnchanged);
assert(build.outputs.some(x=>x.sha256===launch.exeSha256&&path.resolve(path.dirname(launch.buildSummary),x.path).toLowerCase()===path.resolve(launch.executable).toLowerCase()));
for(const source of build.sources){
  assert.equal(hash(path.join(build.sourceRoot,source.path)),source.sha256);
  assert.equal(hash(path.resolve(source.path)),source.sha256);
}
const rawPath=path.join(path.dirname(coreRunPath),'normal-style-dls-proof.json'), raw=json(rawPath);
assert(raw.passed&&raw.runSha256===hash(coreRunPath));
const segment=preparation.inputs.find(x=>path.basename(x.path)==='Normal.sgp');
assert(segment);
assert.equal(segment.sha256,hash(path.join(core,'BeforeBand.sgp')));
const unchanged=preparation.inputs.filter(x=>x!==segment);
for(const input of unchanged)assert.equal(hash(input.path),input.sha256);
for(const [actual,expected] of [['Assigned.sgp','Normal.sgp'],['Undone.sgp','BeforeBand.sgp'],['Redone.sgp','Normal.sgp']]){
  assert(read(path.join(unit,actual)).equals(read(path.join(core,expected))),actual+' exact bytes');
}
assert(read(segment.path).equals(read(path.join(unit,'Redone.sgp'))));
const tree=name=>{
  const s=json(path.join(unit,name+'.json'));
  assert.equal(s.window.id,launch.windowId);
  assert.equal(s.window.app.toLowerCase(),('process:'+launch.executable).toLowerCase());
  assert(s.accessibility?.tree);
  return s.accessibility.tree;
};
const opened=tree('05-normal-project-open');
assert.match(opened,/Value: Normal\.sgp ID: 207/);
assert.match(opened,/3072 clocks 180 BPM/);
assert.match(opened,/Value: Heartlnd\.stp \/ Band 1 ID: 803/);
assert.match(opened,/51 .*disabled.*ID: 805/);
for(const name of ['07-style-band-copied-confirmed','11-redo-confirmed']){
  const s=tree(name);
  assert.match(s,/Value: Normal\.sgp \* ID: 207/);
  assert.match(s,/Value: 0 \/ 0 ID: 805/);
}
assert.match(tree('09-undo-confirmed'),/51 .*disabled.*ID: 805/);
for(const name of ['08-assigned-saved','10-undo-saved','12-redo-saved']){
  assert.match(tree(name),/Value: Normal\.sgp ID: 207/);
}
assert.match(tree('04-initial-project-save'),/Value: Initial\.sgp ID: 207/);
assert(fs.statSync(path.join(unit,'Initial.sgp')).size>0);
assert(fs.statSync(path.join(unit,'Initial.dmpj')).size>0);
const observations=fs.readdirSync(unit).filter(x=>/^\d.*\.json$/.test(x)).map(name=>({name,sha256:hash(path.join(unit,name))}));
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,
  scope:'Same-build GUI normal Style Band selection/copy, save, single Undo/Redo and exact bytes',
  processId:launch.processId,windowId:launch.windowId,buildSummarySha256:launch.buildSummarySha256,
  exeSha256:launch.exeSha256,sources:build.sources.length,preparationSha256:hash(launch.preparation),
  coreRunSha256:hash(coreRunPath),rawProofSha256:hash(rawPath),
  segmentBeforeSha256:segment.sha256,segmentAssignedSha256:hash(path.join(unit,'Assigned.sgp')),
  segmentUndoneSha256:hash(path.join(unit,'Undone.sgp')),segmentRedoneSha256:hash(path.join(unit,'Redone.sgp')),
  unchangedInputs:unchanged,observations,
  initialDocuments:['Initial.sgp','Initial.dmpj'].map(name=>({name,sha256:hash(path.join(unit,name))})),
  guiRestartVerified:false,normalExitVerified:false,audioVerified:false,
  originalDynamicComparison:false,fullAcceptancePassed:false,auditorSha256:hash(new URL(import.meta.url))};
fs.copyFileSync(new URL(import.meta.url),path.join(unit,'gui-auditor.mjs'));
fs.writeFileSync(path.join(unit,'normal-style-gui-proof.json'),JSON.stringify(proof,null,2)+'\n');
console.log(JSON.stringify(proof,null,2));

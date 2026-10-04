import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const unit=path.resolve(process.argv[2]), coreRunPath=path.resolve(process.argv[3]);
const read=p=>fs.readFileSync(p);
const json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const launch=json(path.join(unit,'launch.json'));
const preparation=json(launch.preparation);
const coreRun=json(coreRunPath), core=path.join(path.dirname(coreRunPath),'core');
assert.equal(hash(launch.buildSummary),launch.buildSummarySha256);
assert.equal(hash(launch.executable),launch.exeSha256);
assert.equal(preparation.exeSha256,launch.exeSha256);
assert.equal(preparation.buildSummarySha256,launch.buildSummarySha256);
assert.equal(coreRun.buildSummarySha256,launch.buildSummarySha256);
assert(coreRun.passed&&coreRun.checks===29&&coreRun.exitCode===0);
const build=json(launch.buildSummary);
assert(build.passed&&build.sourceSnapshotUnchanged);
for(const source of build.sources){
  assert.equal(hash(path.join(build.sourceRoot,source.path)),source.sha256);
  assert.equal(hash(path.resolve(source.path)),source.sha256);
}
const rawProof=json(path.join(path.dirname(coreRunPath),'style-root-dls-proof.json'));
assert(rawProof.passed&&rawProof.runSha256===hash(coreRunPath));
const styleInput=preparation.inputs.find(x=>path.basename(x.path)==='Heartlnd.stp');
assert(styleInput);
assert.equal(styleInput.sha256,hash(path.join(core,'before-root-dls.stp')));
assert(read(styleInput.path).equals(read(path.join(core,'Heartlnd.stp'))));
assert.equal(hash(styleInput.path),rawProof.styleSha256);
assert.equal(hash(path.join(core,'owned.dls')),rawProof.collectionSha256);
for(const input of preparation.inputs)if(input!==styleInput)assert.equal(hash(input.path),input.sha256);
const state=name=>json(path.join(unit,name+'.json'));
const tree=name=>{
  const s=state(name);
  assert.equal(s.window.id,launch.windowId);
  assert.equal(s.window.app.toLowerCase(),('process:'+launch.executable).toLowerCase());
  assert(s.accessibility?.tree);
  return s.accessibility.tree;
};
assert.match(tree('05-prior-style-5-8'),/Beats Value: 5/);
assert.match(tree('05-prior-style-5-8'),/Denominator Value: 8/);
const opened=tree('09-project-open-meter-4-4');
assert.match(opened,/Value: selection\.sgp ID: 207/);
assert.match(opened,/Beats Value: 4/);
assert.match(opened,/Denominator Value: 4/);
assert.match(opened,/Grids Value: 4/);
assert.match(opened,/Initial meter: 4\/4/);
const before=tree('10-root-style-before');
assert.match(before,/987 編集 .*Value: 48/);
assert.match(before,/Assign DLS Collection ID: 808/);
const assigned=tree('15-after-picker'),saved=tree('17-root-dls-saved');
assert.match(assigned,/Value: Style: Heartlnd\.stp \* ID: 207/);
assert.match(assigned,/987 編集 .*Value: 519/);
assert.match(saved,/Value: Style: Heartlnd\.stp ID: 207/);
assert.match(saved,/987 編集 .*Value: 519/);
assert.match(saved,/988 編集 .*Value: 5/);
assert.match(saved,/989 編集 .*Value: 64/);
assert.match(saved,/990 編集 .*Value: 100/);
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,
  scope:'Same-build GUI root Style DLS assignment/save exact bytes and Project Open meter refresh',
  buildSummarySha256:launch.buildSummarySha256,exeSha256:launch.exeSha256,
  processId:launch.processId,windowId:launch.windowId,sources:build.sources.length,
  preparationSha256:hash(launch.preparation),coreRunSha256:hash(coreRunPath),
  styleBeforeSha256:styleInput.sha256,styleSavedSha256:hash(styleInput.path),
  patch:519,pchannel:5,pan:64,volume:100,explicitMotifBandPreserved:true,
  unchangedInputs:preparation.inputs.filter(x=>x!==styleInput),
  guiRestartVerified:false,normalExitVerified:false,audioVerified:false,originalDynamicComparison:false,
  fullAcceptancePassed:false,auditorSha256:hash(new URL(import.meta.url))};
fs.copyFileSync(new URL(import.meta.url),path.join(unit,'gui-auditor.mjs'));
fs.writeFileSync(path.join(unit,'style-root-dls-gui-proof.json'),JSON.stringify(proof,null,2)+'\n');
console.log(JSON.stringify(proof,null,2));

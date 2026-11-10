import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';import assert from 'node:assert/strict';
const unit=path.resolve(process.argv[2]),read=p=>fs.readFileSync(p),json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,'')),hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const launch=json(unit+'/launch.json'),previous=json(launch.previousGuiLaunch),exit=json(launch.previousGuiExit),build=json(launch.buildSummary);
assert.equal(launch.exeSha256,previous.exeSha256);assert.equal(hash(launch.executable),launch.exeSha256);
assert.equal(launch.buildSummarySha256,previous.buildSummarySha256);assert.equal(hash(launch.buildSummary),launch.buildSummarySha256);
assert(build.passed&&build.sourceSnapshotUnchanged);for(const s of build.sources){assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);assert.equal(hash(path.resolve(s.path)),s.sha256);}
assert.equal(exit.state,'exited');assert.equal(exit.exitCode,0);assert.equal(exit.forcedTermination,false);assert.equal(exit.processId,previous.processId);assert.notEqual(launch.processId,previous.processId);
const previousUnit=path.dirname(launch.previousGuiLaunch),previousProof=json(previousUnit+'/normal-style-gui-proof.json');assert(previousProof.passed);assert.equal(previousProof.exeSha256,launch.exeSha256);
const inputs=json(unit+'/inputs-before.json');for(const x of inputs.inputs)assert.equal(hash(x.path),x.sha256);
assert(read(unit+'/Resaved.sgp').equals(read(previousUnit+'/Redone.sgp')));
assert.equal(hash(unit+'/Resaved.sgp'),previousProof.segmentRedoneSha256);
assert.equal(hash(unit+'/Resaved.dls'),previousProof.unchangedInputs.find(x=>path.basename(x.path)==='owned.dls').sha256);
const state=name=>{const s=json(unit+'/'+name+'.json');assert.equal(s.window.app.toLowerCase(),('process:'+launch.executable).toLowerCase());assert(s.accessibility?.tree);return s;};
for(const name of ['04-normal-project-reloaded','05-normal-segment-resaved']){
 const s=state(name);assert.equal(s.window.id,launch.windowId);assert.match(s.accessibility.tree,/Value: Normal\.sgp ID: 207/);assert.match(s.accessibility.tree,/3072 clocks 180 BPM/);assert.match(s.accessibility.tree,/Value: Heartlnd\.stp \/ Band 1 ID: 803/);assert.match(s.accessibility.tree,/Value: 0 \/ 0 ID: 805/);assert.match(s.accessibility.tree,/Initial meter: 4\/4/);
}
const dls=state('08-owned-dls-dialog');assert.match(dls.accessibility.tree,/MIDI bank .*Value: 2 ID: 11/);assert.match(dls.accessibility.tree,/Program .*Value: 7 ID: 12/);assert.match(dls.accessibility.tree,/8000 Hz, 16 bits, 800 frames/);assert.match(dls.accessibility.tree,/Value: Loop 1 ID: 80/);assert.match(dls.accessibility.tree,/Value: 800 ID: 84/);
assert.match(state('09-owned-dls-resaved').accessibility.tree,/Saved\. Closing keeps this document in the project/);
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,scope:'Same-build separate GUI process reload and Segment/DLS exact byte resave with restored tempo, Band and DLS instrument/Region loop',processId:launch.processId,previousProcessId:previous.processId,windowId:launch.windowId,exeSha256:launch.exeSha256,buildSummarySha256:launch.buildSummarySha256,sourceCount:build.sources.length,previousGuiProofSha256:hash(previousUnit+'/normal-style-gui-proof.json'),previousExitSha256:hash(launch.previousGuiExit),segmentResavedSha256:hash(unit+'/Resaved.sgp'),dlsResavedSha256:hash(unit+'/Resaved.dls'),unchangedInputs:inputs.inputs,guiAudioVerified:false,currentNormalExitVerified:false,fullAcceptancePassed:false,auditorSha256:hash(new URL(import.meta.url))};
fs.copyFileSync(new URL(import.meta.url),unit+'/reload-auditor.mjs');fs.writeFileSync(unit+'/normal-style-gui-reload-proof.json',JSON.stringify(proof,null,2)+'\n');console.log(JSON.stringify(proof,null,2));

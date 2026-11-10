import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [firstDir,slowDir,reloadDir,failureRun]=process.argv.slice(2).map(x=>path.resolve(x));
const read=p=>fs.readFileSync(p);
const json=p=>JSON.parse(read(p).toString('utf8').replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const samePath=(a,b)=>path.resolve(a).toLowerCase()===path.resolve(b).toLowerCase();
const launches=[firstDir,slowDir,reloadDir].map(d=>json(path.join(d,'launch.json')));
const build=json(launches[0].buildSummary);
assert(build.passed&&build.sourceSnapshotUnchanged);
for(const s of build.sources)assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);
const exeOutput=build.outputs.find(o=>o.path==='install/bin/Producer.exe');
assert.equal(hash(launches[0].executable),exeOutput.sha256);
const evidence=[];
const remember=p=>evidence.push({path:p,sha256:hash(p)});
let screenshotCount=0;
const states=[firstDir,slowDir,reloadDir].map((d,i)=>{
  const l=launches[i];assert(l.passed&&l.exitCode===0&&!l.timedOut&&!l.launchError);
  assert.equal(l.exeSha256,exeOutput.sha256);
  assert(samePath(l.buildSummary,launches[0].buildSummary));
  assert.equal(hash(l.buildSummary),l.buildSummarySha256);
  assert.equal(hash(path.join(d,'launcher.ps1')),l.driverSha256);
  for(const input of l.inputs)assert.equal(hash(input.path),input.sha256);
  const s=json(path.join(d,'states.json'));
  assert(samePath(s.executable,l.executable)&&samePath(s.build,l.buildSummary));
  for(const r of s.records){
    assert.equal(r.window.app.toLowerCase(),('process:'+l.executable).toLowerCase());
    for(const shot of r.screenshots){assert.equal(hash(path.join(d,shot.file)),shot.sha256);screenshotCount++;remember(path.join(d,shot.file));}
  }
  remember(path.join(d,'launch.json'));remember(path.join(d,'states.json'));remember(path.join(d,'launcher.ps1'));
  return s;
});
assert.equal(new Set(launches.map(l=>l.processId)).size,3);
const tree=(i,label)=>{const r=states[i].records.find(r=>r.label===label);assert(r,`Missing ${label}`);return r.accessibility.tree;};
assert(tree(0,'restored-moved-selected').includes('saved-moved.sgp'));
assert(tree(0,'play-observed').includes('Stopped (segment ended)'));
assert(tree(1,'slow-project-startup').includes('slow.sgp'));
assert(tree(1,'slow-stop-click').includes('Playing — Groove changed'));
assert(tree(1,'slow-stop-observed').includes('Stopped.'));
assert(tree(1,'segment-resaved').includes('resaved-slow.sgp'));
assert(tree(2,'resaved-project-restarted').includes('resaved-slow.sgp'));
const fixture=path.dirname(launches[1].inputs[0].path);
assert(read(path.join(fixture,'slow.sgp')).equals(read(path.join(fixture,'resaved-slow.sgp'))));
function projectChunks(p){const b=read(p);assert.equal(b.toString('ascii',0,4),'RIFF');assert.equal(b.readUInt32LE(4)+8,b.length);assert.equal(b.toString('ascii',8,12),'DMPJ');let result=[];for(let offset=12;offset<b.length;){const size=b.readUInt32LE(offset+4),end=offset+8+size;assert(end<=b.length);result.push({id:b.toString('ascii',offset,offset+4),data:b.subarray(offset+8,end)});offset=end+(size&1);}return result;}
const before=projectChunks(path.join(fixture,'slow.dmpj')),after=projectChunks(path.join(fixture,'resaved-slow.dmpj'));
assert.equal(before.length,after.length);
for(let i=0;i<before.length;i++){assert.equal(before[i].id,after[i].id);if(before[i].id==='file'){assert.equal(before[i].data.toString('utf16le').replace(/\0+$/,''),'slow.sgp');assert.equal(after[i].data.toString('utf16le').replace(/\0+$/,''),'resaved-slow.sgp');}else assert(before[i].data.equals(after[i].data));}
const audio=json(path.join(firstDir,'audio-confirmation.json'));
assert(samePath(audio.executable,launches[0].executable));assert.equal(hash(audio.input),audio.inputSha256);assert.equal(audio.answer,'聞こえた。途中から速くなった');
const moduleCounts=[];
for(const [i,d] of [firstDir,slowDir].entries()){
  const capture=json(path.join(d,'gui-modules.json')),proof=json(path.join(d,'gui-module-provenance.json'));
  assert.equal(capture.processId,launches[i].processId);assert(proof.passed);assert.equal(proof.exeSha256,exeOutput.sha256);assert.equal(proof.captureSha256,hash(path.join(d,'gui-modules.json')));
  moduleCounts.push(proof.moduleCount);remember(path.join(d,'gui-modules.json'));remember(path.join(d,'gui-module-provenance.json'));
}
const failure=json(failureRun);assert(failure.passed&&failure.cases.length===3);assert.equal(failure.exeSha256,exeOutput.sha256);assert(failure.cases.every(c=>c.passed&&c.exited&&c.exitCode===1));assert.equal(hash(failure.invalidInput.path),failure.invalidInput.sha256);
for(const p of [launches[0].buildSummary,path.join(firstDir,'audio-confirmation.json'),failureRun,...['slow.sgp','resaved-slow.sgp','slow.dmpj','resaved-slow.dmpj'].map(f=>path.join(fixture,f))])remember(p);
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,build:launches[0].buildSummary,exeSha256:exeOutput.sha256,sourceCount:build.sources.length,processIds:launches.map(l=>l.processId),normalExitCodes:launches.map(l=>l.exitCode),screenshotCount,moduleCounts,segmentBytes:read(path.join(fixture,'slow.sgp')).length,segmentSha256:hash(path.join(fixture,'slow.sgp')),projectReferenceOnlyChanged:true,humanAudio:audio,guiNotificationScope:'Groove text mechanically checked; embellishment screenshot was visually observed, accessibility lagged. Stop click at about15.6s of nominal16s; no subsecond runtime stop timing assertion.',remaining:['GUI restart during playback','actual Style Pattern/Groove selection and audio','original JAZP write','all40 responsibilities and all8 acceptance'],fullAcceptancePassed:false,auditorSha256:hash(process.argv[1]),evidence};
fs.writeFileSync(path.join(slowDir,'project-startup-proof.json'),JSON.stringify(proof,null,2)+'\n');
console.log(JSON.stringify({passed:proof.passed,sourceCount:proof.sourceCount,screenshotCount,moduleCounts,normalExitCodes:proof.normalExitCodes,segmentBytes:proof.segmentBytes}));

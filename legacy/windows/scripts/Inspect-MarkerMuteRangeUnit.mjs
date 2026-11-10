import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [unit, buildPath, relatedPath, nativePath, driverPath] = process.argv.slice(2);
assert(driverPath, 'Usage: Inspect-MarkerMuteRangeUnit.mjs UNIT BUILD RELATED NATIVE DRIVER');
const read = p => JSON.parse(fs.readFileSync(p, 'utf8').replace(/^\uFEFF/, ''));
const hash = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const build = read(buildPath), related = read(relatedPath), native = read(nativePath), drivers = read(driverPath);
const candidate = path.basename(path.dirname(buildPath));
assert(build.passed && build.sourceSnapshotUnchanged);
for (const source of build.sources) {
  assert.equal(hash(source.path), source.sha256, source.path);
  assert.equal(hash(path.join(build.sourceRoot, source.path)), source.sha256, source.path);
}
for (const output of build.outputs) assert.equal(hash(path.join(path.dirname(buildPath), output.path)), output.sha256);
for (const run of [related, native, drivers]) assert.equal(run.candidate, candidate);
for (const id of ['timeline-range','marker-document','mute-document','lyric-document','sequence-crud']) {
  assert.equal(related.results.find(r => r.id === id)?.status, '合格', id);
  assert.equal(native.results.find(r => r.id === id)?.status, '合格', id);
}
assert.equal(native.results.find(r => r.id === 'timeline-range').checks, 93);
const save = read(unit + '/save-proof.json');
assert(save.passed && save.allOtherBytesIdentical && save.changes.length === 7);
assert.equal(hash(save.before.path), save.before.sha256);
assert.equal(hash(save.after.path), save.after.sha256);
const author = read(unit + '/author-exit.json'), reload = read(unit + '/reload-exit.json');
assert.notEqual(author.processId, reload.processId);
for (const run of [author,reload]) {
  assert.equal(run.exitCode, 0); assert.equal(run.state, 'exited'); assert(run.passed && !run.forced && !run.timedOut);
  assert.equal(run.exeSha256, build.outputs.find(o => o.path === 'install/bin/Producer.exe').sha256);
  assert.equal(run.buildSummarySha256, hash(buildPath));
}
const observations = read(unit + '/observations/states.json');
const tree = label => {const o = observations.find(o => o.label === label); assert(o, label); return (o.accessibility ?? o.state?.accessibility).tree;};
for (const [label, texts] of [
  ['range-selected-settled',['Range [2304, 3072)','; saved']],
  ['range-move-settled',['Range [3072, 3840)','; modified']],
  ['range-undo',['; saved']], ['range-redo',['; modified']],
  ['reload-main',['3204 clocks 90 BPM','Note 1: 3204 clocks','PChannel (1-4294967292) Value: 17']],
  ['reload-marker',['Marker 1, clocks 1536','Saved. 3 events']],
  ['reload-marker-moved',['Marker 2, clocks 3104']],
  ['reload-enter-moved',['Enter SwitchPoint 3, clocks 3104']],
  ['reload-mute',['30720 clocks; PChannel 2 restored']],
  ['reload-mute-moved-settled',['3204 clocks; PChannel 17 muted','Saved. 3 events']],
  ['reload-mute-boundary-settled',['1536 clocks; PChannel 17 restored']],
  ['reload-lyric',['range; start 3254, belongs to 3404','Before Time Stamp','Saved. 1 lyrics']]
]) for (const value of texts) assert(tree(label).includes(value), label + ': ' + value);
const eventFiles = ['saved-events.sgp','reloaded-resaved-events.sgp'];
assert(fs.readFileSync(unit+'/'+eventFiles[0]).equals(fs.readFileSync(unit+'/'+eventFiles[1])));
assert.equal(hash(unit+'/'+eventFiles[0]), hash(save.after.path));
function timestampOffset(b) {
  let result;
  const walk = (a,z) => {for(let p=a;p<z;){assert(p+8<=z); const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),end=p+8+n; assert(end+(n&1)<=z);
    if(id==='RIFF'||id==='LIST'){assert(n>=4);walk(p+12,end);} else if(id==='filh'){assert.equal(result,undefined);assert(n>=44);result=p+8+16;}
    p=end+(n&1);
  }}; walk(0,b.length); assert(result!==undefined); return result;
}
const projectFiles = ['before-project.pro','saved-project.pro','reloaded-resaved-project.pro'];
const projects = projectFiles.map(n=>fs.readFileSync(unit+'/'+n));
const timestamp = timestampOffset(projects[0]);
const normalized = projects.map(b=>{assert.equal(timestampOffset(b),timestamp);const c=Buffer.from(b);c.fill(0,timestamp,timestamp+8);return c;});
assert(normalized.every(b=>b.equals(normalized[0])), 'Project changes exceed the documented filh last-write FILETIME');
assert.equal(reload.inputs.find(i=>i.path.endsWith('.pro')).sha256,hash(unit+'/saved-project.pro'));
assert.equal(reload.inputs.find(i=>i.path.endsWith('.sgp')).sha256,hash(unit+'/saved-events.sgp'));
const counts = rows => rows.reduce((a,r)=>(a[r.status]=(a[r.status]??0)+1,a),{});
const proof = {schema:1,createdUtc:new Date().toISOString(),candidate,passed:true,scope:'Source provisional five-strip shared range, one Undo/Redo, main save, distinct-process restore and resave; original bulk parity/audio/all40/all8 separate',
  savedSources:build.sources.length,sourceAndOutputIdentityVerified:true,
  related:{path:relatedPath,counts:counts(related.results)},native:{path:nativePath,counts:counts(native.results)},drivers:{path:driverPath,counts:counts(drivers.results)},
  gui:{authorPid:author.processId,reloadPid:reload.processId,authorWindow:author.window,reloadWindow:reload.window,normalExitCodes:[author.exitCode,reload.exitCode],forced:false,oneUndoRedo:true,range:{begin:2304,end:3072,target:3072},changedClocks:7,allOtherEventBytesIdentical:true,resavedEventBytesIdentical:true},
  project:{form:'JAZP',bytes:projects[0].length,identityAndAllNonTimestampBytesIdentical:true,onlyChangedField:'LIST:file/filh last-write FILETIME bytes16..23',timestampOffset:timestamp,versions:projectFiles.map((n,i)=>({path:unit+'/'+n,sha256:hash(unit+'/'+n),filetime:projects[i].subarray(timestamp,timestamp+8).toString('hex')}))},
  evidence:['save-proof.json','author-exit.json','reload-exit.json','input-provenance.json','observations/states.json','actions.json','reload-project-dirty-after-document-save.json'].map(n=>({path:unit+'/'+n,sha256:hash(unit+'/'+n)})),
  auditorSha256:hash(process.argv[1]),originalDynamicCompatibility:'unobserved; existing designer-open obstacle retained',audio:'unexecuted on new candidate',fullAcceptance:false};
fs.writeFileSync(unit+'/unit-proof.json',JSON.stringify(proof,null,2)+'\n');
fs.copyFileSync(process.argv[1],unit+'/unit-auditor.mjs');
fs.copyFileSync('scripts/Inspect-MarkerMuteRangeSave.mjs',unit+'/save-auditor.mjs');
console.log(JSON.stringify({candidate,passed:true,gui:proof.gui,native:proof.native.counts,drivers:proof.drivers.counts,fullAcceptance:false}));

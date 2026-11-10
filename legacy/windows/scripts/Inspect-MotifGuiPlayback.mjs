import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [dir,fixturePath,host] = process.argv.slice(2).map(p=>path.resolve(p));
const json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const launch=json(dir+'/launch.json'), fixture=json(fixturePath), records=json(dir+'/motif-gui-states.json'), actions=json(dir+'/actions.json'), build=json(launch.buildSummary);
assert(launch.passed && launch.exitCode===0 && !launch.timedOut && !launch.launchError);
assert(build.passed && build.sourceSnapshotUnchanged);
assert.equal(hash(launch.buildSummary),launch.buildSummarySha256);
for(const s of build.sources) assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);
assert.equal(hash(launch.executable),launch.exeSha256);
assert.equal(launch.exeSha256,json(host).cases.find(c=>c.name==='host-smoke').sha256);
for(const i of launch.inputs) {assert.equal(hash(i.path),i.sha256);assert(fixture.files.some(f=>path.resolve(f.path)===path.resolve(i.path)&&f.sha256===i.sha256));}
for(const f of fixture.files) assert.equal(hash(f.path),f.sha256);
for(const k of ['Style','Segment','Project']) assert.equal(hash(fixture['source'+k]),fixture['source'+k+'Sha256']);
const src=fs.readFileSync(fixture.sourceStyle), dst=fs.readFileSync(fixture.files.find(f=>f.path.endsWith('Heartlnd.stp')).path), off=fixture.repeatOffset;
assert.equal(src.length,dst.length);assert.equal(src.toString('ascii',off-8,off-4),'mtfs');assert(src.readUInt32LE(off-4)>=20);
assert.equal(src.readUInt32LE(off),1);assert.equal(dst.readUInt32LE(off),63);assert.deepEqual(dst.subarray(0,off),src.subarray(0,off));assert.deepEqual(dst.subarray(off+4),src.subarray(off+4));
for(const r of records) for(const c of r.captures) assert.equal(hash(dir+'/'+c.file),c.sha256);
const state=label=>{const r=records.find(r=>r.label===label);assert(r&&r.captures.length);assert.equal(path.resolve(r.window.app.slice('process:'.length)),path.resolve(launch.executable));return r;};
for(const [label,action] of [['playing-ready','Play Selected Motif'],['restarted-ready','Restart Selected Motif']]) {
 const r=state(label),a=actions.find(a=>a.action===action);assert(a);assert(Date.parse(r.createdUtc)-Date.parse(a.createdUtc)>5000);assert(r.accessibility.tree.includes('テキスト Playing Motif: Owned Motif.'));
}
const stopped=state('stopped-ready'), stop=actions.find(a=>a.action==='Stop');assert(stop);assert(Date.parse(stop.createdUtc)>Date.parse(state('playing-ready').createdUtc));assert(Date.parse(stopped.createdUtc)>Date.parse(stop.createdUtc));assert(stopped.accessibility.tree.includes('テキスト Stopped.'));
const end=state('restart-natural-end');assert(Date.parse(end.createdUtc)>Date.parse(state('restarted-ready').createdUtc));assert(end.accessibility.tree.includes('テキスト Stopped (segment ended).'));
for(const label of ['play-menu','restart-menu']) assert(state(label).accessibility.tree.includes('Play Selected Motif'));
fs.writeFileSync(dir+'/states.json',JSON.stringify({schema:1,build:launch.buildSummary,run:host,executable:launch.executable,records:records.map(r=>({...r,screenshots:r.captures}))},null,2)+'\n');
const proof={schema:1,passed:true,createdUtc:new Date().toISOString(),processId:launch.processId,productExeSha256:launch.exeSha256,normalExitCode:0,fixtureSha256:hash(fixturePath),statesSha256:hash(dir+'/states.json'),actionsSha256:hash(dir+'/actions.json'),auditorSha256:hash(process.argv[1]),scope:'Current GUI selected named Motif; Playing observed after five-second preparation deadline, explicit Stop, restart, subsequent natural-end status and normal exit. Inputs unchanged. GUI digital audio, exact runtime tempo, timing and generated notes unverified; finite CLI recording not transferred.',fullAcceptance:false};
fs.writeFileSync(dir+'/motif-gui-playback-proof.json',JSON.stringify(proof,null,2)+'\n');fs.copyFileSync(process.argv[1],dir+'/motif-gui-playback-auditor.mjs');console.log(JSON.stringify(proof));

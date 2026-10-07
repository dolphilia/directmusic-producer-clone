import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [unitArg, buildArg, styleArg, transportArg] = process.argv.slice(2);
assert(transportArg, 'Usage: Inspect-Q1CurrentScenario.mjs UNIT BUILD STYLE_CAPTURE TRANSPORT_CAPTURE');
const unit = path.resolve(unitArg), buildPath = path.resolve(buildArg);
const json = p => JSON.parse(fs.readFileSync(p, 'utf8').replace(/^\uFEFF/, ''));
const hash = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const relative = p => path.relative(process.cwd(), p).replace(/\\/g, '/');
const build = json(buildPath), candidate = path.basename(path.dirname(buildPath));
assert(build.passed && build.sourceSnapshotUnchanged);
assert.deepEqual([build.configureExitCode, build.buildExitCode, build.installExitCode], [0, 0, 0]);
for (const s of build.sources) {
  assert.equal(hash(path.join(build.sourceRoot, s.path)), s.sha256);
  assert.equal(hash(path.resolve(s.path)), s.sha256);
}
for (const o of build.outputs) assert.equal(hash(path.join(path.dirname(buildPath), o.path)), o.sha256);
const start = json(unit + '/unit-start.json'), inputs = json(unit + '/final-inputs.json');
assert.equal(start.candidate, candidate); assert.equal(inputs.candidate, candidate);
assert.equal(inputs.inputs.length, 12);
for (const i of inputs.inputs) {
  assert.equal(hash(i.path), i.sha256);
  assert.equal(hash(unit + '/author-saved/Fresh/' + path.basename(i.path)), i.sha256);
}
for (const i of start.inputsOnly) assert.equal(hash(i.source), i.sha256);

// Independent persistence audit; original reciprocal/dynamic parity is separate.
function chunks(b, begin = 0, end = b.length) {
  const out = [];
  for (let p = begin; p < end;) {
    assert(p + 8 <= end); const id = b.toString('ascii', p, p + 4), n = b.readUInt32LE(p + 4);
    const next = p + 8 + n + (n & 1); assert(next <= end);
    const c = {id, data: b.subarray(p + 8, p + 8 + n)};
    if (id === 'RIFF' || id === 'LIST') { assert(n >= 4); c.type = b.toString('ascii', p + 8, p + 12); c.children = chunks(b, p + 12, p + 8 + n); }
    out.push(c); p = next;
  }
  return out;
}
const one = (cs, id, type) => { const a = cs.filter(c => c.id === id && (!type || c.type === type)); assert.equal(a.length, 1); return a[0]; };
const projectPath = unit + '/Fresh/Fresh.pro', root = one(chunks(fs.readFileSync(projectPath)), 'RIFF', 'JAZP');
const catalog = root.children.filter(c => c.id === 'LIST' && c.type === 'file'), entries = [];
assert.equal(catalog.length, 10);
for (const c of catalog) {
  const name = one(c.children, 'name').data.toString('utf16le').replace(/\0+$/, '');
  assert(!entries.some(e => e.name === name));
  const h = one(c.children, 'filh').data, p = unit + '/Fresh/' + name, b = fs.readFileSync(p), r = one(chunks(b), 'RIFF');
  assert(h.length >= 44); assert.equal(h.readUInt32LE(24), b.length);
  const identity = one(r.children, r.type === 'DLS ' ? 'dlid' : 'guid').data;
  assert.equal(identity.length, 16); assert(identity.equals(h.subarray(28, 44)));
  entries.push({name, form: r.type, bytes: b.length, sha256: hash(p), documentGuid: identity.toString('hex')});
}
assert.deepEqual(entries.reduce((a, e) => {a[e.form] = (a[e.form] ?? 0) + 1; return a;}, {}), {DMST: 1, DMSG: 4, DMBD: 1, DMAP: 2, 'DLS ': 1, DMSC: 1});
const initialBefore = start.inputsOnly.find(i => path.basename(i.path) === 'Initial.sgp');
assert(!one(one(chunks(fs.readFileSync(initialBefore.source)), 'RIFF').children, 'guid').data.equals(Buffer.from(entries.find(e => e.name === 'Initial.sgp').documentGuid, 'hex')), 'Initial document must be newly created in main');
const duplicateGuids = [...new Set(entries.map(e => e.documentGuid))].map(guid => ({guid, names: entries.filter(e => e.documentGuid === guid).map(e => e.name)})).filter(e => e.names.length > 1);
assert.deepEqual(duplicateGuids, [], 'Owned documents require distinct identities');
const correction = start.inputIdentityCorrection;
assert.equal(correction.name, 'Control.sgp');
const historicalControl = start.historicalInputSources.find(i => i.name === correction.name);
const correctedControl = start.inputsOnly.find(i => path.basename(i.path) === correction.name);
assert(historicalControl && correctedControl);
assert.equal(hash(historicalControl.source), historicalControl.sha256);
const beforeControl = fs.readFileSync(historicalControl.source), afterControl = fs.readFileSync(correctedControl.source);
assert.equal(beforeControl.length, afterControl.length);
const rootGuid = b => one(one(chunks(b), 'RIFF', 'DMSG').children, 'guid').data;
assert.equal(rootGuid(beforeControl).toString('hex'), correction.before);
assert.equal(rootGuid(afterControl).toString('hex'), correction.after);
assert.equal(entries.find(e => e.name === correction.name).documentGuid, correction.after);
assert.notEqual(correction.before, correction.after);
assert(beforeControl.subarray(0, correction.rootGuidOffset).equals(afterControl.subarray(0, correction.rootGuidOffset)));
assert(beforeControl.subarray(correction.rootGuidOffset + 16).equals(afterControl.subarray(correction.rootGuidOffset + 16)));
assert.equal(afterControl.subarray(correction.rootGuidOffset, correction.rootGuidOffset + 16).toString('hex'), correction.after);

const author = json(unit + '/author-normal-exit.json'), reload = json(unit + '/reload-exit.json');
assert.notEqual(author.processId, reload.processId);
const installed = build.outputs.find(o => o.path === 'install/bin/Producer.exe'); assert(installed);
for (const r of [author, reload]) { assert.equal(r.exitCode, 0); assert.equal(r.state, 'exited'); assert.equal(r.forcedTermination, false); assert.equal(r.exeSha256, installed.sha256); assert.equal(hash(r.executable), r.exeSha256); }
for (const r of [author, reload]) {
  assert.equal(r.buildSummarySha256, hash(buildPath));
  assert.equal(hash(r.normalExitBasis.rawLaunch), r.normalExitBasis.rawLaunchSha256);
  assert.equal(hash(r.normalExitBasis.driverPath), r.driverSha256);
  const raw = json(r.normalExitBasis.rawLaunch);
  assert.equal(raw.processId, r.processId); assert.equal(raw.exitCode, 0);
  assert.equal(raw.state, 'exited'); assert.equal(raw.forcedTermination, false);
  assert.equal(raw.exeSha256, installed.sha256);
  assert.equal(raw.driverSha256, r.driverSha256);
  assert.equal(path.basename(r.normalExitBasis.driverPath), 'Watch-ProductGuiExit.ps1');
  assert(Date.parse(raw.exitUtc) >= Date.parse(raw.readyUtc));
}
const observations = json(unit + '/observations/states.json'), evidence = [];
function observe(label, regex) {
  const o = observations.filter(o => o.label === label && regex.test(o.accessibility?.tree ?? '')).at(-1);
  assert(o, 'Missing settled observation: ' + label); evidence.push({label, utc: o.utc, window: o.window.id});
}
for (const [label, regex] of [
  ['New Project settled', /Untitled segment/], ['Initial Undo settled', /Notes: 0\./], ['Initial Redo settled', /Notes: 1\./],
  ['Sequence Change6 settled', /0 clocks 6 BPM/], ['Sequence Undo5 settled', /0 clocks 5 BPM/], ['Sequence Redo6 settled', /0 clocks 6 BPM/],
  ['Style Change109 settled', /Style tempo: 109\.000000 BPM/], ['Style Undo108 settled', /Style tempo: 108\.000000 BPM/], ['Style Redo109 settled; Save menu', /Style tempo: 109\.000000 BPM/],
  ['Band Change101 settled', /volume 101/], ['Band Undo100 settled', /volume 100/], ['Band Redo101 settled; Save menu', /volume 101/],
  ['DLS Change1 settled', /Key low Value: 1 ID:/], ['DLS Undo0 settled', /Key low Value: 0 ID:/], ['DLS Redo1 settled', /Key low Value: 1 ID:/],
  ['AudioPath name change settled', /Name Value: Q1 Current Conflict ID:/], ['AudioPath Undo Conflict settled', /Name Value: Conflict ID:/], ['AudioPath Redo Current settled', /Name Value: Q1 Current Conflict ID:/],
  ['Separate process restored Fresh native Project', /Notes: 1\./], ['source-sequence-final-restored', /0 clocks 5 BPM/],
  ['final-style-reloaded', /Style tempo: 109\.000000 BPM/], ['final-band-reloaded', /volume 101/], ['final-dls-reloaded', /Key low Value: 1 ID:/],
  ['final-audiopath-reloaded', /Name Value: Q1 Current Conflict ID:/], ['final-route-reloaded', /PChannel 0 count 16/],
  ['final-script-reloaded', /Language Value: VBScript[\s\S]*本体 日本 Ω 🎵/], ['Reload final stopped before normal exit', /Stopped\./]
]) observe(label, regex);
const audioProofs = [];
for (const [i, dir] of [path.resolve(styleArg), path.resolve(transportArg)].entries()) {
  const run = json(dir + '/run.json'), launch = json(dir + '/gui-launch-at-capture.json');
  const proofPath = dir + (i ? '/gui-transport-dls-priority-proof.json' : '/gui-project-style-dls-audio-proof.json'), p = json(proofPath);
  assert(p.passed); assert.equal(run.processId, reload.processId); assert.equal(run.exeSha256, installed.sha256);
  assert.equal(run.buildSummarySha256, hash(buildPath)); assert.equal(run.captureExitCode, 0); assert.equal(run.silentKeepAlive, true);
  assert.equal(launch.windowId, reload.windowId); assert.equal(launch.processId, reload.processId);
  assert.equal(launch.inputs.length, inputs.inputs.length);
  for (const x of launch.inputs) assert.equal(hash(x.path), x.sha256);
  for (const x of inputs.inputs) assert(launch.inputs.some(i => path.resolve(i.path) === path.resolve(x.path) && i.sha256 === x.sha256));
  assert(launch.inputs.some(x => path.resolve(x.path) === path.resolve(projectPath) && x.sha256 === hash(projectPath)));
  assert.equal(hash(run.recorder), run.recorderSha256); assert.equal(hash(run.recorderBuildSummary), run.recorderBuildSummarySha256);
  const controlsPath = unit + (i ? '/transport-negative-controls-v2/negative-tests.json' : '/style-negative-controls-v2/negative-tests.json'), controls = json(controlsPath);
  assert(controls.passed); assert.equal(path.resolve(controls.source), dir); assert.equal(controls.results.length, i ? 11 : 6);
  assert(controls.results.every(c => c.kind === 'unchanged' ? c.accepted === true : c.accepted === false || c.rejected === true));
  assert.equal(hash(i ? 'scripts/Inspect-ProductGuiTransportDlsPriorityAudio.mjs' : 'scripts/Inspect-ProductGuiProjectStyleDlsAudio.mjs'), p.analyzerSha256);
  if (i) assert(p.stopResumePassed && p.controlQuiet && p.sustainPassed && p.packetIntegrity);
  audioProofs.push({capture: relative(dir), proof: relative(proofPath), proofSha256: hash(proofPath), controls: relative(controlsPath), controlsSha256: hash(controlsPath), recorderSha256: run.recorderSha256});
}
const proof = {schema: 4, createdUtc: new Date().toISOString(), candidate, passed: true,
  scope: 'Current candidate: fresh main-created native Project with10 owned documents/six forms; five-form edit/UndoRedo/save, author normal exit0, distinct reload normal exit0, serial Style and Transport sounding Stop/quiet/full replay/tempo PCM. All40/all8/original comparison separate',
  project: {path: relative(projectPath), sha256: hash(projectPath), entries},
  inputIdentityLimitations: {duplicateGuids, globalGuidUniquenessPassed: true, correction, allNonIdentityBytesPreserved: true, meaning: 'Control clone has a distinct root identity; all event, route and reference bytes are preserved. All owned Project identities are unique.'},
  exitObservers: [author, reload].map(r => ({path: relative(r.normalExitBasis.rawLaunch), sha256: r.normalExitBasis.rawLaunchSha256, driverSha256: r.driverSha256, exitCode: r.exitCode, forcedTermination: r.forcedTermination})),
  processes: [author, reload].map(r => ({processId: r.processId, exeSha256: r.exeSha256, exitCode: r.exitCode, forcedTermination: r.forcedTermination})),
  observationSource: {path: relative(unit + '/observations/states.json'), sha256: hash(unit + '/observations/states.json')}, observations: evidence,
  audioProofs, auditorSha256: hash(process.argv[1]), originalParity: false, fullAcceptance: false};
fs.writeFileSync(unit + '/scenario-proof.json', JSON.stringify(proof, null, 2) + '\n');
fs.copyFileSync(process.argv[1], unit + '/scenario-auditor.mjs');
console.log(JSON.stringify({candidate, passed: true, entries: entries.length, processes: proof.processes, duplicateGuids, fullAcceptance: false}));

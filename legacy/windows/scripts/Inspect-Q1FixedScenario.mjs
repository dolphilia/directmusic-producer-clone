import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [unitArg, buildArg, styleArg, transportArg] = process.argv.slice(2);
assert(transportArg, 'Usage: Inspect-Q1FixedScenario.mjs UNIT BUILD STYLE_CAPTURE TRANSPORT_CAPTURE');
const unit = path.resolve(unitArg), buildPath = path.resolve(buildArg);
const json = p => JSON.parse(fs.readFileSync(p, 'utf8').replace(/^\uFEFF/, ''));
const hash = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const relative = p => path.relative(process.cwd(), p).replace(/\\/g, '/');
const build = json(buildPath), candidate = path.basename(path.dirname(buildPath));
assert(build.passed && build.sourceSnapshotUnchanged);
assert.deepEqual([build.configureExitCode, build.buildExitCode, build.installExitCode], [0, 0, 0]);
for (const s of build.sources) assert.equal(hash(path.join(build.sourceRoot, s.path)), s.sha256);
for (const o of build.outputs) assert.equal(hash(path.join(path.dirname(buildPath), o.path)), o.sha256);
const start = json(unit + '/unit-start.json'), revision1 = json(unit + '/final-inputs.json');
const revision2 = json(unit + '/final-inputs-v2.json');
assert.equal(start.candidate, candidate); assert.equal(revision2.candidate, candidate);
const v1 = revision1.inputs ?? revision1.files;
assert(Array.isArray(v1));
for (const i of v1) assert.equal(hash(unit + '/inputs-before-script/Fresh/' + path.basename(i.path)), i.sha256);
for (const i of revision2.inputs) assert.equal(hash(i.path), i.sha256);
for (const i of start.inputsOnly) assert.equal(hash(i.source), i.sha256);
const changedForScript = revision2.inputs.filter(i => i.sha256 !== v1.find(p => path.basename(p.path) === path.basename(i.path))?.sha256);
assert.deepEqual(changedForScript.map(i => path.basename(i.path)), ['Fresh.pro']);

// This is a persistence audit, not an original Producer compatibility oracle.
function chunks(b, begin = 0, end = b.length) {
  const out = [];
  for (let p = begin; p < end;) {
    assert(p + 8 <= end); const id = b.toString('ascii', p, p + 4), n = b.readUInt32LE(p + 4);
    const next = p + 8 + n + (n & 1); assert(next <= end);
    const c = {id, raw: b.subarray(p, next), data: b.subarray(p + 8, p + 8 + n)};
    if (id === 'RIFF' || id === 'LIST') { assert(n >= 4); c.type = b.toString('ascii', p + 8, p + 12); c.children = chunks(b, p + 12, p + 8 + n); }
    out.push(c); p = next;
  }
  return out;
}
const one = (cs, id, type) => { const matches = cs.filter(c => c.id === id && (!type || c.type === type)); assert.equal(matches.length, 1); return matches[0]; };
const project = p => {
  const root = one(chunks(fs.readFileSync(p)), 'RIFF', 'JAZP'), entries = new Map();
  for (const c of root.children.filter(c => c.id === 'LIST' && c.type === 'file')) {
    const name = one(c.children, 'name').data.toString('utf16le').replace(/\0+$/, '');
    assert(!entries.has(name)); entries.set(name, one(c.children, 'filh').data);
  }
  return {root, entries};
};
const prior = project(unit + '/inputs-before-script/Fresh/Fresh.pro'), final = project(unit + '/Fresh/Fresh.pro');
assert.equal(prior.entries.size, 9); assert.equal(final.entries.size, 10);
assert(final.entries.has('SourceHost.spp'));
for (const [name, header] of prior.entries) assert(header.equals(final.entries.get(name)), 'Existing catalog entry changed: ' + name);
assert(one(one(prior.root.children, 'LIST', 'proj').children, 'pjct').data.equals(one(one(final.root.children, 'LIST', 'proj').children, 'pjct').data), 'Fresh Project identity changed');
const entries = [];
for (const [name, h] of final.entries) {
  assert(h.length >= 44); const p = unit + '/Fresh/' + name, b = fs.readFileSync(p), root = one(chunks(b), 'RIFF');
  assert.equal(h.readUInt32LE(24), b.length);
  const identity = one(root.children, root.type === 'DLS ' ? 'dlid' : 'guid');
  assert(identity.data.equals(h.subarray(28, 44)), 'Catalog/document GUID mismatch: ' + name);
  entries.push({name, form: root.type, bytes: b.length, sha256: hash(p), documentGuid: identity.data.toString('hex')});
}
assert.deepEqual(entries.reduce((a, e) => {a[e.form] = (a[e.form] ?? 0) + 1; return a;}, {}), {DMSG: 4, DMST: 1, DMBD: 1, 'DLS ': 1, DMAP: 2, DMSC: 1});

const author = json(unit + '/author-exit.json'), corrected = json(unit + '/corrected-author-exit.json'), reload = json(unit + '/reload-v2-exit.json');
assert(new Set([author.processId, corrected.processId, reload.processId]).size === 3);
for (const record of [author, corrected, reload]) { assert.equal(record.exitCode, 0); assert.equal(record.state, 'exited'); assert.equal(record.forcedTermination, false); assert.equal(hash(record.executable), record.exeSha256); }
assert.equal(corrected.buildSummarySha256, hash(buildPath)); assert.equal(reload.buildSummarySha256, hash(buildPath));
assert.equal(reload.exeSha256, author.exeSha256);
const observations = json(unit + '/observations/states.json'), evidence = [];
function observe(label, text, window) {
  const matches = observations.filter(o => o.label === label && (!window || o.window.id === window) && text.test(o.accessibility?.tree ?? ''));
  assert(matches.length > 0, 'Missing observed state: ' + label);
  const o = matches.at(-1); evidence.push({label, utc: o.utc, window: o.window.id});
}
observe('New Project settled', /Untitled segment/);
for (const [label, regex] of [
  ['Initial Undo settled', /Notes: 0\./], ['Initial Redo settled', /Notes: 1\./],
  ['Sequence Change6 settled', /0 clocks 6 BPM/], ['Sequence Undo5 settled', /0 clocks 5 BPM/], ['Sequence Redo6 settled', /0 clocks 6 BPM/],
  ['Style changed109 settled', /Style tempo: 109\.000000 BPM/], ['Style Undo108 settled', /Style tempo: 108\.000000 BPM/], ['Style Redo109 settled', /Style tempo: 109\.000000 BPM/],
  ['Band101 settled', /volume 101/], ['Band Undo100 settled', /volume 100/], ['Band Redo101 settled', /volume 101/],
  ['DLS key1 applied settled', /Key low Value: 1 ID:/], ['DLS Undo0 settled', /Key low Value: 0 ID:/], ['DLS Redo1 settled', /Key low Value: 1 ID:/],
  ['AudioPath name applied settled', /Name Value: Q1 Fixed Conflict ID:/], ['AudioPath Undo settled', /Name Value: Conflict ID:/], ['AudioPath Redo settled', /Name Value: Q1 Fixed Conflict ID:/]
]) observe(label, regex);
for (const [label, regex] of [
  ['Revision2 separate process restored native Project', /Notes: 1\./],
  ['source-sequence-final-restored', /0 clocks 5 BPM/], ['final-style-reloaded', /Style tempo: 109\.000000 BPM/],
  ['final-band-restored-settled', /volume 101/], ['final-dls-restored-window', /Key low Value: 1 ID:/],
  ['final-audiopath-restored', /Name Value: Q1 Fixed Conflict ID:/], ['final-route-restored-settled', /PChannel 0 count 16/]
]) observe(label, regex);

const captures = [path.resolve(styleArg), path.resolve(transportArg)];
const audioProofs = [];
for (const [i, dir] of captures.entries()) {
  const run = json(dir + '/run.json'), capturedLaunch = json(dir + '/gui-launch-at-capture.json');
  const proofPath = dir + (i ? '/gui-transport-dls-priority-proof.json' : '/gui-project-style-dls-audio-proof.json');
  const proof = json(proofPath); assert(proof.passed);
  assert.equal(run.processId, reload.processId); assert.equal(run.exeSha256, reload.exeSha256);
  assert.equal(run.buildSummarySha256, hash(buildPath)); assert.equal(run.captureExitCode, 0);
  assert.equal(hash(run.recorder), run.recorderSha256); assert.equal(hash(run.recorderBuildSummary), run.recorderBuildSummarySha256);
  assert.equal(run.silentKeepAlive, true);
  for (const input of capturedLaunch.inputs) assert.equal(hash(input.path), input.sha256);
  assert(capturedLaunch.inputs.some(x => path.basename(x.path) === 'Fresh.pro' && x.sha256 === hash(unit + '/Fresh/Fresh.pro')));
  const controlsPath = unit + (i ? '/transport-negative-controls/negative-tests.json' : '/style-negative-controls/negative-tests.json');
  const controls = json(controlsPath); assert.equal(controls.passed, true);
  assert.equal(path.resolve(controls.source), dir); assert.equal(controls.results.length, i ? 11 : 6);
  assert(controls.results.every(c => c.kind === 'unchanged' ? c.accepted === true : c.accepted === false || c.rejected === true));
  audioProofs.push({capture: relative(dir), proof: relative(proofPath), proofSha256: hash(proofPath), controls: relative(controlsPath), controlsSha256: hash(controlsPath), recorderSha256: run.recorderSha256});
}
const proof = {schema: 2, createdUtc: new Date().toISOString(), candidate, passed: true,
  scope: 'Q1 representative same-candidate fresh native Project five-form history/save, corrected Script ownership, normal exits, final distinct reload and serial Style/Transport acoustic lifecycle; full40/all8 and original comparison separate',
  project: {path: relative(unit + '/Fresh/Fresh.pro'), sha256: hash(unit + '/Fresh/Fresh.pro'), entries},
  inputCorrection: {changedFiles: ['Fresh.pro'], priorCatalogEntries: 9, finalCatalogEntries: 10, allOtherFilesIdentical: true},
  processes: [author, corrected, reload].map(r => ({processId: r.processId, exeSha256: r.exeSha256, exitCode: r.exitCode, forcedTermination: r.forcedTermination})),
  observationSource: {path: relative(unit + '/observations/states.json'), sha256: hash(unit + '/observations/states.json')}, observations: evidence,
  audioProofs, auditorSha256: hash(process.argv[1]), fullAcceptance: false};
fs.writeFileSync(unit + '/scenario-proof.json', JSON.stringify(proof, null, 2) + '\n');
fs.copyFileSync(process.argv[1], unit + '/scenario-auditor.mjs');
console.log(JSON.stringify({candidate, passed: proof.passed, catalogEntries: entries.length, processes: proof.processes, fullAcceptance: false}));

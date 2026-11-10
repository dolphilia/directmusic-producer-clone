import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [first, second, fixturePath, nativeRunPath, hostPath] = process.argv.slice(2).map(p => path.resolve(p));
const read = p => fs.readFileSync(p);
const json = p => JSON.parse(read(p).toString().replace(/^\uFEFF/, ''));
const hash = p => crypto.createHash('sha256').update(read(p)).digest('hex');
const fixture = json(fixturePath), native = json(nativeRunPath), nativeDir = path.dirname(nativeRunPath);
const launches = [first, second].map(d => json(d + '/launch.json'));
assert(native.passed && json(nativeDir + '/authored-style-band-proof.json').passed);
assert.notEqual(launches[0].processId, launches[1].processId);
assert(Date.parse(launches[1].createdUtc) > Date.parse(launches[0].createdUtc));
assert.equal(hash(fixture.sourceStyle), fixture.sourceStyleSha256);
const expected = read(nativeDir + '/core/Heartlnd.stp');
assert(read(first + '/saved.stp').equals(expected));
assert(read(second + '/resaved.stp').equals(expected));
assert.equal(hash(native.executable), native.executableSha256);
const original = fixture.files.find(f => f.case === 'with-id' && f.path.endsWith('.stp'));
assert.equal(original.sha256, fixture.sourceStyleSha256);

// Parse the saved RIFF independently of the product document implementation.
function chunks(b, start = 0, end = b.length) {
  const result = [];
  for (let p = start; p < end;) {
    assert(p + 8 <= end);
    const id = b.toString('ascii', p, p + 4), size = b.readUInt32LE(p + 4);
    const e = p + 8 + size, next = e + (size & 1), box = id === 'RIFF' || id === 'LIST';
    assert(next <= end);
    result.push({id, type: box ? b.toString('ascii', p + 8, p + 12) : '', raw: b.subarray(p, next), children: box ? chunks(b, p + 12, e) : []});
    p = next;
  }
  return result;
}
const one = (nodes, id, type) => { const matches = nodes.filter(n => n.id === id && n.type === type); assert.equal(matches.length, 1); return matches[0]; };
const before = one(chunks(read(fixture.sourceStyle)), 'RIFF', 'DMST');
const after = one(chunks(expected), 'RIFF', 'DMST');
const oldMotif = one(before.children, 'LIST', 'pttn'), motif = one(after.children, 'LIST', 'pttn');
const band = one(after.children, 'RIFF', 'DMBD');
assert(!oldMotif.children.some(c => c.type === 'DMBD'));
assert.equal(motif.children.length, oldMotif.children.length + 1);
oldMotif.children.forEach((c, i) => assert(c.raw.equals(motif.children[i].raw)));
assert(one(motif.children, 'RIFF', 'DMBD').raw.equals(band.raw));
assert.equal(before.children.length, after.children.length);
before.children.forEach((c, i) => { if (c !== oldMotif) assert(c.raw.equals(after.children[i].raw)); });

for (const [i, dir] of [first, second].entries()) {
  const l = launches[i];
  assert(l.passed && l.exitCode === 0 && !l.timedOut && !l.launchError);
  assert.equal(l.buildSummary, native.buildSummary);
  assert.equal(hash(l.buildSummary), l.buildSummarySha256);
  assert.equal(hash(l.executable), l.exeSha256);
  assert.equal(json(hostPath).cases.find(c => c.name === 'host-smoke').sha256, l.exeSha256);
  assert.equal(hash(dir + '/launcher.ps1'), l.driverSha256);
  const build = json(l.buildSummary);
  assert(build.passed && build.sourceSnapshotUnchanged);
  for (const source of build.sources) {
    assert.equal(hash(path.join(build.sourceRoot, source.path)), source.sha256);
    assert.equal(hash(path.resolve(source.path)), source.sha256);
  }
  const style = l.inputs.find(f => f.path.endsWith('.stp')), project = l.inputs.find(f => f.path.endsWith('.dmpj'));
  assert.equal(style.sha256, i === 0 ? original.sha256 : hash(first + '/saved.stp'));
  assert(read(style.path).equals(expected));
  assert.equal(hash(project.path), project.sha256);
  assert.equal(project.sha256, fixture.files.find(f => f.case === 'with-id' && f.path.endsWith('.dmpj')).sha256);
  assert(!fs.readdirSync(path.dirname(style.path)).some(f => /\.sgp$/i.test(f)));
  const states = json(dir + '/motif-band-states.json');
  const tree = label => { const matches = states.filter(s => s.label === label); assert.equal(matches.length, 1); return matches[0].accessibility.tree; };
  let previous = 0;
  for (const state of states) {
    assert(Date.parse(state.createdUtc) >= previous); previous = Date.parse(state.createdUtc);
    assert.equal(state.window.id, states[0].window.id);
    for (const capture of state.captures) assert.equal(hash(dir + '/' + capture.file), capture.sha256);
  }
  if (i === 0) {
    assert(/メニュー項目 Assign Selected Style Band to Motif ID: 619/.test(tree('assignment-menu')));
    for (const label of ['assigned-ready', 'redo-ready']) assert(tree(label).includes('Style: Heartlnd.stp *'));
    for (const label of ['undo-ready', 'saved']) assert(!tree(label).includes('Style: Heartlnd.stp *'));
  } else {
    const t = tree('reload-save-menu');
    for (const text of ['Value: Authored Motif', 'Value: Motif', 'Value: Part 1: Authored Motif Part', 'Value: Note 1, grid 0', 'Value: Band 1, PChannel 5', 'Value: 48', 'Value: 72']) assert(t.includes(text));
    assert(!tree('reload-saved').includes('Style: Heartlnd.stp *'));
  }
  const modules = json(dir + '/gui-modules.json');
  assert.equal(modules.processId, l.processId);
  assert.equal(modules.executable, l.executable);
  fs.writeFileSync(dir + '/states.json', JSON.stringify({run: hostPath, build: l.buildSummary, executable: l.executable}, null, 2) + '\n');
}
const proof = {schema: 1, passed: true, createdUtc: new Date().toISOString(), build: launches[0].buildSummary, exeSha256: launches[0].exeSha256, coreExeSha256: native.executableSha256, guiDirectories: [first, second], processIds: launches.map(l => l.processId), savedStyleSha256: hash(first + '/saved.stp'), stateSha256: [first, second].map(d => hash(d + '/motif-band-states.json')), auditorSha256: hash(process.argv[1]), scope: 'Current GUI explicit Motif Band assignment, single Undo/Redo, exact save, separate-process reload/resave and normal exits. Root Band/Part fields observed; nested Motif Band verified in raw saved bytes. GUI audio and original equivalence unverified', fullAcceptance: false};
fs.copyFileSync(process.argv[1], first + '/motif-band-gui-auditor.mjs');
fs.writeFileSync(first + '/motif-band-gui-proof.json', JSON.stringify(proof, null, 2) + '\n');
console.log(JSON.stringify(proof));

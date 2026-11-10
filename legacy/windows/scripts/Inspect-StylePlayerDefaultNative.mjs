import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';

const read = p => fs.readFileSync(p);
const json = p => JSON.parse(read(p).toString().replace(/^\uFEFF/, ''));
const hash = p => crypto.createHash('sha256').update(read(p)).digest('hex');
const text = b => { assert.equal(b.length % 2, 0); return b.toString('utf16le').replace(/\0+$/, ''); };
export function chunks(b, a = 0, z = b.length) {
  const out = [];
  for (let p = a; p < z;) {
    assert(p + 8 <= z, 'Truncated RIFF chunk header');
    const id = b.toString('ascii', p, p + 4), n = b.readUInt32LE(p + 4), end = p + 8 + n;
    assert(end + (n & 1) <= z, 'RIFF chunk exceeds parent');
    const container = id === 'RIFF' || id === 'LIST';
    if (container) assert(n >= 4);
    out.push({id, type: container ? b.toString('ascii', p + 8, p + 12) : '',
      data: container ? null : b.subarray(p + 8, end), children: container ? chunks(b, p + 12, end) : []});
    p = end + (n & 1);
  }
  return out;
}
const one = (cs, id, type = '') => {
  const matches = cs.filter(c => c.id === id && c.type === type);
  assert.equal(matches.length, 1, 'Expected one ' + id + '/' + type);
  return matches[0];
};
const descendants = cs => cs.flatMap(c => [c, ...descendants(c.children)]);
const identity = cs => {
  const c = cs[0], b = one(c.children, c.type === 'DLS ' ? 'dlid' : 'guid').data;
  assert.equal(b.length, 16); assert(b.some(x => x !== 0)); return b.toString('hex');
};
const reference = c => {
  const h = one(c.children, 'refh').data; assert.equal(h.length, 20);
  const optional = id => c.children.filter(x => x.id === id);
  for (const id of ['guid', 'file', 'name']) assert(optional(id).length <= 1);
  return {classId: h.subarray(0, 16).toString('hex'), flags: h.readUInt32LE(16),
    guid: optional('guid')[0]?.data.toString('hex') ?? null,
    file: optional('file')[0] ? text(optional('file')[0].data) : null,
    name: optional('name')[0] ? text(optional('name')[0].data) : null};
};

export function inspectNative(root) {
  root = path.resolve(root);
  const author = json(path.join(root, 'author-launch.json'));
  const exit = json(path.join(root, 'author-exit.json'));
  assert.equal(exit.state, 'exited'); assert.equal(exit.exitCode, 0); assert(!exit.forcedTermination);
  assert.equal(exit.processId, author.processId);
  assert.equal(hash(author.executable), author.exeSha256);
  assert.equal(hash(author.buildSummary), author.buildSummarySha256);
  const build = json(author.buildSummary);
  assert(build.passed && build.sourceSnapshotUnchanged);
  for (const s of build.sources) {
    assert.equal(hash(path.join(build.sourceRoot, s.path)), s.sha256);
    assert.equal(hash(path.resolve(s.path)), s.sha256);
  }
  for (const o of build.outputs) assert.equal(hash(path.join(path.dirname(author.buildSummary), o.path)), o.sha256);
  const saved = json(path.join(root, 'saved-inputs.json'));
  const scope = json(path.join(root, 'scope.json'));
  const dir = path.join(root, 'Default');
  const expected = ['Composed.sgp', 'Default.cdm', 'Default.pro', 'Default.stp', 'Initial.sgp', 'owned.dls'];
  assert.equal(saved.files.length, expected.length);
  for (const x of saved.files) {
    assert.equal(path.dirname(path.resolve(x.path)), dir);
    assert(expected.includes(path.basename(x.path)));
    assert.equal(hash(x.path), x.sha256); assert.equal(read(x.path).length, x.bytes);
  }
  assert.equal(new Set(saved.files.map(x => x.path)).size, 6);
  for (const x of scope.sourceInputs) {
    assert.equal(hash(path.resolve(x.path)), x.sha256);
    assert.equal(hash(path.join(dir, path.basename(x.path))), x.sha256, 'Native source bytes must remain unchanged');
  }
  const native = n => chunks(read(path.join(dir, n)));
  const project = one(native('Default.pro'), 'RIFF', 'JAZP');
  const entries = project.children.filter(c => c.id === 'LIST' && c.type === 'file');
  const files = entries.map(c => text(one(c.children, 'name').data));
  assert.equal(files.length, 5); assert.equal(new Set(files).size, 5);
  assert.deepEqual([...files].sort(), expected.filter(n => n !== 'Default.pro').sort());
  const guids = Object.fromEntries(files.map(n => [n, identity(native(n))]));
  assert.equal(new Set(Object.values(guids)).size, 5);
  for (const entry of entries) {
    const name = text(one(entry.children, 'name').data), h = one(entry.children, 'filh').data;
    assert.equal(h.length, 44); assert.equal(h.readUInt32LE(24), read(path.join(dir, name)).length);
    assert.equal(h.subarray(28, 44).toString('hex'), guids[name], 'Project catalog identity');
  }
  const style = one(native('Default.stp'), 'RIFF', 'DMST');
  const styleRefs = one(style.children, 'LIST', 'prrf').children.map(reference);
  assert.equal(styleRefs.length, 1);
  const sourceMap = styleRefs[0];
  assert.equal(sourceMap.classId, '8f28acd29bb3d111870400600893b1bd');
  assert.equal(sourceMap.flags, 0x13); assert.equal(sourceMap.name, null);
  assert.equal(sourceMap.file, 'Default.cdm'); assert.equal(sourceMap.guid, guids['Default.cdm']);
  const map = one(native('Default.cdm'), 'RIFF', 'DMPR');
  // DMUS_IO_CHORDMAP uses a fixed WCHAR[20]; bytes after its first NUL are padding.
  const loadNameField = one(map.children, 'perh').data.subarray(0, 40);
  assert.equal(loadNameField.length, 40);
  const loadName = loadNameField.toString('utf16le').split('\0', 1)[0]; assert.equal(loadName, 'Boogie');
  const segment = one(native('Composed.sgp'), 'RIFF', 'DMSG');
  assert.equal(one(segment.children, 'segh').data.readInt32LE(4), 32 * 4 * 768, '32 measures');
  const tracks = one(segment.children, 'LIST', 'trkl').children;
  assert(tracks.every(c => c.id === 'RIFF' && c.type === 'DMTK'));
  const ids = tracks.map(t => one(t.children, 'trkh').data.subarray(0, 16).toString('hex'));
  const required = {Band: '9428acd29bb3d111870400600893b1bd', Tempo: '8528acd29bb3d111870400600893b1bd',
    Style: '8d28acd29bb3d111870400600893b1bd', ChordMap: '9628acd29bb3d111870400600893b1bd',
    Command: '8c28acd29bb3d111870400600893b1bd', Chord: '8b28acd29bb3d111870400600893b1bd'};
  assert.equal(new Set(ids).size, ids.length);
  for (const [role, id] of Object.entries(required)) assert(ids.includes(id), 'Missing generated track: ' + role);
  const track = role => tracks[ids.indexOf(required[role])];
  const tempo = one(track('Tempo').children, 'tetr').data;
  assert.equal(tempo.readUInt32LE(0), 16); assert.equal(tempo.readInt32LE(4), 0);
  assert.equal(tempo.readDoubleLE(12), 120, 'Public Tempo item value');
  const commands = one(track('Command').children, 'cmnd').data;
  assert.equal(commands.readUInt32LE(0), 12); assert.equal((commands.length - 4) % 12, 0);
  const grooves = []; for (let p = 4; p < commands.length; p += 12) if (commands[p + 7] === 0) grooves.push(commands[p + 8]);
  assert(grooves.length && grooves.every(v => v > 0 && v < 50), 'Quiet groove range');
  const refs = descendants(segment.children).filter(c => c.id === 'LIST' && c.type === 'DMRF').map(reference);
  for (const [role, cls, name] of [['Style', '8a28acd29bb3d111870400600893b1bd', 'Default.stp'],
    ['ChordMap', '8f28acd29bb3d111870400600893b1bd', 'Default.cdm'],
    ['DLS', 'b0f40f48b228d111bef700c04fbf8fef', 'owned.dls']]) {
    const matches = refs.filter(r => r.classId === cls); assert.equal(matches.length, 1, role + ' reference');
    assert.equal(matches[0].guid, guids[name]); assert(matches[0].flags & 1);
    if (matches[0].file !== null) assert.equal(matches[0].file, name);
  }
  const bands = descendants(segment.children).filter(c => c.id === 'RIFF' && c.type === 'DMBD');
  assert.equal(bands.length, 1);
  const instruments = descendants(bands[0].children).filter(c => c.id === 'bins');
  assert.equal(instruments.length, 1); assert.equal(instruments[0].data.readUInt32LE(0), 519);
  assert.equal(instruments[0].data.readUInt32LE(24), 5);
  assert(descendants(track('Chord').children).some(c => c.id === 'crdb'), 'Chord bodies retained');
  const header = path.resolve('work/analysis/sources/dmusicf.h');
  assert.equal(hash(header), '39bf0460f3373f58e6709fcb7eb1c179289f0c98dd828cab29151f6ebefbff38');
  return {schema: 1, createdUtc: new Date().toISOString(), passed: true, kind: 'saved-native-only',
    candidate: path.basename(path.dirname(author.buildSummary)), authorPid: author.processId,
    exeSha256: author.exeSha256, buildSummarySha256: author.buildSummarySha256,
    sourceCount: build.sources.length, savedInputsSha256: hash(path.join(root, 'saved-inputs.json')),
    scopeSha256: hash(path.join(root, 'scope.json')), authorExitSha256: hash(path.join(root, 'author-exit.json')),
    ownedFiles: files, ownedGuids: guids, sourceDefaultReference: sourceMap, loadName, requiredTracks: required,
    observedTrackIds: ids, measures: 32, bpm: 120, grooves, patch: 519, pchannel: 5,
    publicFileContract: {path: header, sha256: hash(header)}, auditorSha256: hash(fileURLToPath(import.meta.url)),
    limitations: ['Separate-process GUI reload and PCM are separately required', 'Original dynamic parity and Q2 remain unverified'], fullAcceptance: false};
}
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const root = path.resolve(process.argv[2]), output = path.resolve(process.argv[3]);
  assert(!fs.existsSync(output), 'Use a fresh output directory');
  fs.mkdirSync(output, {recursive: true});
  const proof = inspectNative(root);
  fs.copyFileSync(fileURLToPath(import.meta.url), path.join(output, 'native-auditor.mjs'));
  fs.writeFileSync(path.join(output, 'native-proof.json'), JSON.stringify(proof, null, 2) + '\n', {flag: 'wx'});
  console.log(JSON.stringify({passed: true, kind: proof.kind, files: proof.ownedFiles.length, tracks: proof.observedTrackIds.length, authorPid: proof.authorPid}));
}

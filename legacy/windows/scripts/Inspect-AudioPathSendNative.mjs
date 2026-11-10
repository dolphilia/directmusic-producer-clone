import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';

const read = p => fs.readFileSync(p);
const json = p => JSON.parse(read(p).toString().replace(/^\uFEFF/, ''));
const hashBytes = b => crypto.createHash('sha256').update(b).digest('hex');
const hash = p => hashBytes(read(p));
const ref = p => ({path: path.resolve(p), sha256: hash(p), bytes: read(p).length});
const one = (cs, id, type = '') => {
  const found = cs.filter(c => c.id === id && c.type === type);
  assert.equal(found.length, 1, 'Expected one ' + id + '/' + type);
  return found[0];
};
const descendants = cs => cs.flatMap(c => [c, ...descendants(c.children)]);
const text = b => {
  assert.equal(b.length % 2, 0);
  return b.toString('utf16le').replace(/\0+$/, '');
};

// Independent public RIFF reader; offsets refer to the original file bytes.
export function chunks(b, start = 0, end = b.length) {
  const out = [];
  for (let p = start; p < end;) {
    assert(p + 8 <= end, 'Truncated chunk');
    const id = b.toString('ascii', p, p + 4), n = b.readUInt32LE(p + 4);
    const next = p + 8 + n + (n & 1), container = id === 'RIFF' || id === 'LIST';
    assert(next <= end, 'Chunk exceeds parent');
    if (container) assert(n >= 4);
    out.push({id, type: container ? b.toString('ascii', p + 8, p + 12) : '',
      offset: p, raw: b.subarray(p, next), data: container ? null : b.subarray(p + 8, p + 8 + n),
      children: container ? chunks(b, p + 12, p + 8 + n) : []});
    p = next;
  }
  return out;
}

// These identities come from preserved original static/OS observations, not the product headers.
const Send = '762160efbbbce0498ccae09a5a152b33';
const FileOutput = '11146d2dd7dce745addeacac85a2425d';
function audio(b, attenuation) {
  const root = one(chunks(b), 'RIFF', 'DMAP');
  const buffers = root.children.filter(c => c.id === 'LIST' && c.type === 'dbfl').map(c => {
    const h = one(c.children, 'ddah').data;
    assert.equal(h.length, 20);
    const dsbc = one(c.children, 'RIFF', 'DSBC');
    const guid = one(dsbc.children, 'guid').data;
    assert.equal(guid.length, 16); assert.deepEqual(guid, h.subarray(0, 16));
    const desc = one(dsbc.children, 'dsbd').data;
    assert.equal(desc.length, 20);
    const effects = one(dsbc.children, 'LIST', 'fxls').children.map(f => {
      assert.equal(f.id, 'RIFF'); assert.equal(f.type, 'DSFX');
      const header = one(f.children, 'fxhr').data;
      assert.equal(header.length, 56);
      return {chunk: f, classId: header.subarray(4, 20).toString('hex'),
        destination: header.subarray(36, 52).toString('hex'),
        parameter: f.children.find(x => x.id === 'data')};
    });
    return {chunk: c, dsbc, id: guid.toString('hex'), flags: h.readUInt32LE(16),
      channels: desc.readUInt16LE(4), effects,
      buses: dsbc.children.filter(x => x.id === 'bsid')};
  });
  assert.equal(buffers.length, 3);
  assert.equal(new Set(buffers.map(x => x.id)).size, 3);
  assert.deepEqual(buffers.map(x => x.flags), [0, 0, 8]);
  assert(buffers.every(x => x.channels === 2));
  assert.deepEqual(buffers.map(x => x.effects.map(f => f.classId)), [[FileOutput], [FileOutput, Send], [FileOutput]]);
  assert.equal(buffers[2].buses.length, 0, 'Mix-in has no synth bus mapping');
  const routes = descendants(root.children).filter(c => c.id === 'pchh').map(c => {
    assert(c.data.length >= 16);
    const count = c.data.readUInt32LE(8);
    assert.equal(c.data.length, 16 + 16 * count);
    assert.equal(c.data.readUInt32LE(12), 0);
    return {base: c.data.readUInt32LE(0), count: c.data.readUInt32LE(4),
      targets: Array.from({length: count}, (_, i) => c.data.subarray(16 + 16 * i, 32 + 16 * i).toString('hex'))};
  });
  assert.deepEqual(routes, [{base: 0, count: 8, targets: [buffers[1].id]},
    {base: 8, count: 8, targets: [buffers[0].id]}]);
  const send = buffers[1].effects[1];
  assert.equal(send.destination, buffers[2].id);
  assert.equal(send.parameter.data.length, 4, 'Native Send signed LONG has four bytes');
  assert.equal(send.parameter.data.readInt32LE(0), attenuation);
  for (const x of buffers.flatMap(x => x.effects).filter(x => x.classId !== Send))
    assert.equal(x.destination, '0'.repeat(32));
  return {root, buffers, routes, send};
}

export function inspectDocuments(root) {
  root = path.resolve(root);
  const fixture = json(path.join(root, 'fixture.json'));
  const launch = json(path.join(root, 'author-launch.json'));
  const build = json(launch.buildSummary);
  assert.equal(path.basename(path.dirname(launch.buildSummary)), fixture.candidate);
  assert.equal(hash(launch.buildSummary), launch.buildSummarySha256);
  assert(build.passed && build.sourceSnapshotUnchanged);
  assert.equal(hash(launch.executable), launch.exeSha256);
  for (const x of build.sources) {
    assert.equal(hash(path.join(build.sourceRoot, x.path)), x.sha256);
    assert.equal(hash(path.resolve(x.path)), x.sha256, 'Working product source must still match saved build');
  }
  for (const x of build.outputs) assert.equal(hash(path.join(path.dirname(launch.buildSummary), x.path)), x.sha256);
  for (const x of fixture.copies) {
    assert.equal(hash(path.resolve(x.source.path)), x.source.sha256, 'Original source fixture retained');
    if (x.variant === 'before') assert.equal(hash(path.resolve(x.path)), x.sha256, 'Before copy retained');
    else if (path.extname(x.path) !== '.pro') assert.equal(hash(path.resolve(x.path)), x.sha256, 'Source document retained');
  }
  const dir = path.join(root, 'author', 'MultiCapture');
  const zeroPath = path.join(dir, 'SendZero.aup'), minusPath = path.join(dir, 'SendMinus600.aup');
  const zeroBytes = read(zeroPath), minusBytes = read(minusPath);
  const zero = audio(zeroBytes, 0), minus = audio(minusBytes, -600);
  assert.equal(zeroBytes.length, minusBytes.length);
  const neutral = Buffer.from(minusBytes);
  neutral.writeInt32LE(0, minus.send.parameter.offset + 8);
  assert.deepEqual(neutral, zeroBytes, 'Only the four-byte Send attenuation may change');
  const before = one(chunks(read(path.join(root, 'before', 'MultiCapture', 'RoutePath.aup'))), 'RIFF', 'DMAP');
  const oldBuffers = before.children.filter(c => c.id === 'LIST' && c.type === 'dbfl');
  assert.equal(oldBuffers.length, 2);
  assert.deepEqual(minus.buffers[0].chunk.raw, oldBuffers[0].raw, 'Control route buffer unchanged');
  for (const old of oldBuffers[1].children) {
    if (old.id === 'RIFF' && old.type === 'DSBC') {
      for (const d of old.children) {
        const next = one(minus.buffers[1].dsbc.children, d.id, d.type);
        if (d.id === 'LIST' && d.type === 'fxls') {
          assert.equal(d.children.length, 1);
          assert.deepEqual(next.children[0].raw, d.children[0].raw, 'Existing FileOutput stays first');
        } else assert.deepEqual(next.raw, d.raw, 'Existing source buffer metadata unchanged');
      }
    } else assert.deepEqual(one(minus.buffers[1].chunk.children, old.id, old.type).raw, old.raw);
  }
  for (const old of before.children.filter(c => !(c.id === 'LIST' && c.type === 'dbfl')))
    assert.deepEqual(one(minus.root.children, old.id, old.type).raw, old.raw, 'AudioPath unknown/port/name data retained');
  const segmentPath = path.join(dir, 'SendMinus600.sgp');
  const segment = one(chunks(read(segmentPath)), 'RIFF', 'DMSG');
  const oldSegment = one(chunks(read(path.join(root, 'before', 'MultiCapture', 'RouteSong.sgp'))), 'RIFF', 'DMSG');
  assert.deepEqual(one(segment.children, 'RIFF', 'DMAP').raw, minusBytes, 'Embedded native AP is byte-identical');
  const nonAudio = cs => cs.filter(c => !(c.id === 'RIFF' && c.type === 'DMAP')).map(c => c.raw);
  assert.deepEqual(nonAudio(segment.children), nonAudio(oldSegment.children), 'All non-AudioPath Segment chunks retained');
  const header = one(segment.children, 'segh').data;
  assert.equal(header.readInt32LE(4), 184320);
  // Public Sequence is a seqt chunk containing evtl/curl chunks, not a LIST.
  const notesChunk = one(chunks(one(descendants(segment.children), 'seqt').data), 'evtl').data;
  assert.equal(notesChunk.readUInt32LE(0), 20); assert.equal(notesChunk.length, 44);
  const notes = [4, 24].map(p => ({clocks: notesChunk.readInt32LE(p),
    durationClocks: notesChunk.readInt32LE(p + 4), pchannel: notesChunk.readUInt32LE(p + 8),
    pitch: notesChunk[p + 15], velocity: notesChunk[p + 16]}));
  assert.deepEqual(notes, fixture.notes);
  const tempo = one(descendants(segment.children), 'tetr').data;
  assert.equal(tempo.readUInt32LE(0), 16); assert.equal(tempo.length, 20);
  assert.equal(tempo.readInt32LE(4), 0); assert.equal(tempo.readDoubleLE(12), fixture.tempoBpm);
  const contractPath = path.resolve('work/analysis/sources/dmusicf.h');
  assert.equal(hash(contractPath), '39bf0460f3373f58e6709fcb7eb1c179289f0c98dd828cab29151f6ebefbff38');
  return {schema: 1, observedUtc: new Date().toISOString(), passed: true,
    kind: 'saved-documents-only', candidate: fixture.candidate, authorPid: launch.processId,
    executable: ref(launch.executable), build: ref(launch.buildSummary),
    fixture: ref(path.join(root, 'fixture.json')), files: [zeroPath, minusPath, segmentPath].map(ref),
    sourceCount: build.sources.length, notes, bpm: tempo.readDoubleLE(12), noteSeconds: 160,
    segmentLengthClocks: header.readInt32LE(4), sendClassBytes: Send, attenuation: -600,
    parametersOffset: minus.send.parameter.offset + 8,
    storedBufferOrder: minus.buffers.map(b => b.id), routes: minus.routes,
    effects: minus.buffers.map(b => b.effects.map(f => f.classId)),
    publicFileContract: ref(contractPath), auditor: ref(fileURLToPath(import.meta.url)),
    limitations: ['Project save, two normal exits, separate-process GUI restore and PCM are separate',
      'Original dynamic parity, external Send lifetime and Q2 remain unverified'], fullAcceptance: false};
}

export function inspectNative(root) {
  root = path.resolve(root);
  const result = inspectDocuments(root), launch = json(path.join(root, 'author-launch.json'));
  const exit = json(path.join(root, 'author-exit.json'));
  assert.equal(exit.state, 'exited'); assert.equal(exit.processId, launch.processId);
  assert.equal(exit.exitCode, 0); assert.equal(exit.forcedTermination, false);
  assert.equal(exit.exeSha256, launch.exeSha256);
  const dir = path.join(root, 'author', 'MultiCapture'), projectPath = path.join(dir, 'MultiCapture.pro');
  const project = one(chunks(read(projectPath)), 'RIFF', 'JAZP');
  const old = one(chunks(read(path.join(root, 'before', 'MultiCapture', 'MultiCapture.pro'))), 'RIFF', 'JAZP');
  const entries = project.children.filter(c => c.id === 'LIST' && c.type === 'file');
  const names = entries.map(c => text(one(c.children, 'name').data));
  assert.deepEqual(names, ['SendMinus600.sgp', 'RouteBand.bnp', 'SendMinus600.aup', 'RouteSource.dls']);
  for (const entry of entries) {
    const name = text(one(entry.children, 'name').data), file = one(chunks(read(path.join(dir, name))), 'RIFF',
      ({'.sgp': 'DMSG', '.bnp': 'DMBD', '.aup': 'DMAP', '.dls': 'DLS '})[path.extname(name)]);
    const guid = one(file.children, name.endsWith('.dls') ? 'dlid' : 'guid').data;
    const h = one(entry.children, 'filh').data;
    assert.equal(h.length, 44); assert.equal(h.readUInt32LE(24), read(path.join(dir, name)).length);
    assert.deepEqual(h.subarray(28, 44), guid, 'Native Project reference identity');
  }
  for (const c of old.children.filter(c => !(c.id === 'LIST' && c.type === 'file')))
    assert.deepEqual(one(project.children, c.id, c.type).raw, c.raw, 'Native Project unknown/root/runtime metadata retained');
  return {...result, kind: 'saved-native-and-author-exit', project: ref(projectPath),
    projectEntries: names, authorExit: ref(path.join(root, 'author-exit.json'))};
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const root = path.resolve(process.argv[2]), output = path.resolve(process.argv[3]);
  const result = process.argv.includes('--documents-only') ? inspectDocuments(root) : inspectNative(root);
  fs.mkdirSync(output, {recursive: true});
  fs.writeFileSync(path.join(output, 'native-proof.json'), JSON.stringify(result, null, 2) + '\n', {flag: 'wx'});
  console.log(JSON.stringify({passed: true, kind: result.kind, candidate: result.candidate, files: result.files.length}));
}

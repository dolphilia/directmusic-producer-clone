import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
const [runPath, outputPath] = process.argv.slice(2);
if (!runPath || !outputPath) throw new Error('Usage: run.json output.json');
const hash = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const requireThat = (value, message) => { if (!value) throw new Error(message); };
const run = JSON.parse(fs.readFileSync(runPath, 'utf8'));
requireThat(run.passed && run.exitCode === 0 && !run.timedOut, 'Targeted test must have passed');
requireThat(hash(run.buildSummary) === run.buildSummarySha256 && hash(run.executable) === run.exeSha256, 'Build/test identity');
const build = JSON.parse(fs.readFileSync(run.buildSummary, 'utf8'));
requireThat(build.passed && build.sourceSnapshotUnchanged, 'Build not successful');
for (const source of build.sources) requireThat(hash(path.join(build.sourceRoot, source.path)) === source.sha256, 'Saved source changed');
for (const input of run.inputs) requireThat(hash(input.path) === input.sha256, 'Observed input changed');
// Separate raw RIFF walker; does not use the product Chunk parser/serializer.
function chunks(bytes, begin, end) {
  const result = [];
  for (let p = begin; p < end;) {
    requireThat(p + 8 <= end, 'Truncated chunk header');
    const id = bytes.toString('ascii', p, p + 4), size = bytes.readUInt32LE(p + 4), next = p + 8 + size + (size & 1);
    requireThat(next <= end, 'Chunk extent');
    const node = { id, start: p, end: next, data: bytes.subarray(p + 8, p + 8 + size) };
    if (id === 'RIFF' || id === 'LIST') { requireThat(size >= 4, 'Container size'); node.type = bytes.toString('ascii', p + 8, p + 12); node.children = chunks(bytes, p + 12, p + 8 + size); }
    result.push(node); p = next;
  }
  return result;
}
const metadataDirectory = path.join(path.dirname(runPath), 'core', 'Metadata');
const names = ['Saved.sgp', 'Saved.stp', 'Saved.bnp', 'Saved.dls'];
const artifacts = [];
function readProject(filename) {
  const full = path.join(metadataDirectory, filename), bytes = fs.readFileSync(full);
  const root = chunks(bytes, 0, bytes.length);
  requireThat(root.length === 1 && root[0].id === 'RIFF' && root[0].type === 'JAZP', 'Native JAZP form');
  const files = new Map();
  for (const node of root[0].children.filter(c => c.id === 'LIST' && c.type === 'file')) {
    const nameChunk = node.children.find(c => c.id === 'name'), header = node.children.find(c => c.id === 'filh');
    requireThat(nameChunk && header && header.data.length === 47, 'Extended file header');
    const name = nameChunk.data.toString('utf16le').replace(/\0+$/, '');
    requireThat(!files.has(name), 'Duplicate file name');
    files.set(name, header);
    requireThat(header.data.subarray(44).equals(Buffer.from([0xaa, 0xbb, 0xcc])), 'Unknown header tail preserved');
    const opaque = node.children.find(c => c.id === 'zzzz');
    requireThat(opaque && opaque.data.equals(Buffer.from([1, 2, 3])) && bytes[opaque.end - 1] === 0xa9, 'Unknown chunk and pad preserved');
  }
  requireThat(files.size === 4 && names.every(n => files.has(n)), 'Four owned entries');
  artifacts.push({ path: full, sha256: hash(full) }); return { bytes, files };
}
const steps = names.map((_, i) => readProject(`after-${i}.pro`));
const transitions = [];
for (let i = 1; i < steps.length; ++i) {
  const before = steps[i - 1], after = steps[i];
  requireThat(before.bytes.length === after.bytes.length, 'Metadata-only project size');
  const header = after.files.get(names[i]);
  const allowed = new Set(Array.from({ length: 12 }, (_, n) => header.start + 8 + 16 + n));
  const differences = [];
  for (let p = 0; p < before.bytes.length; ++p) if (before.bytes[p] !== after.bytes[p]) { requireThat(allowed.has(p), 'Byte change outside saved header time/size'); differences.push(p); }
  requireThat(differences.length > 0, 'Expected actual date/size change');
  transitions.push({ savedDocument: names[i], changedOffsets: differences, onlyTimeAndSize: true });
}
const final = readProject('Metadata.pro'), rows = [];
requireThat(final.bytes.equals(steps.at(-1).bytes), 'Final resave unchanged');
for (const name of names) {
  const full = path.join(metadataDirectory, name), bytes = fs.readFileSync(full), attributes = fs.statSync(full, { bigint: true }), header = final.files.get(name).data;
  // FILETIME is 100 ns since 1601; stat provides the same NTFS file time as ns.
  const actualTime = attributes.mtimeNs / 100n + 116444736000000000n;
  requireThat(header.readBigUInt64LE(16) === actualTime && BigInt(header.readUInt32LE(24)) === attributes.size, 'Current file date/size mismatch');
  const root = chunks(bytes, 0, bytes.length)[0], identity = root.children.find(c => c.id === (root.type === 'DLS ' ? 'dlid' : 'guid'));
  requireThat(identity?.data.length === 16 && identity.data.equals(header.subarray(28, 44)), 'Document GUID mismatch');
  artifacts.push({ path: full, sha256: hash(full) }); rows.push({ name, storedTime: header.readBigUInt64LE(16).toString(), actualTime: actualTime.toString(), bytes: Number(attributes.size), documentGuid: identity.data.toString('hex') });
}
const proof = { schema: 1, createdUtc: new Date().toISOString(), passed: true, buildSummary: run.buildSummary, buildSummarySha256: run.buildSummarySha256, executable: run.executable, exeSha256: run.exeSha256, runPath, runSha256: hash(runPath), auditorSha256: hash(process.argv[1]), transitions, rows, artifacts, scope: 'Independent raw native project metadata and GUID/tail/padding checks; GUI/audio/original save behavior unexecuted', fullAcceptance: false };
fs.writeFileSync(outputPath, JSON.stringify(proof, null, 2));
console.log(JSON.stringify({ passed: true, entries: rows.length, transitions: transitions.length, evidence: outputPath }));

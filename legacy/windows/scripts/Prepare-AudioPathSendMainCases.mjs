import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {chunks, inspectNative} from './Inspect-AudioPathSendNative.mjs';
import {serializeChunk, inspectSendCase} from './Inspect-AudioPathSendMainBindings.mjs';

const [authorRootArg, protocolPathArg, outputArg] = process.argv.slice(2);
assert(outputArg, 'Usage: author-root frozen-protocol fresh-output');
const root = path.resolve(authorRootArg), protocolPath = path.resolve(protocolPathArg), output = path.resolve(outputArg);
assert(!fs.existsSync(output), 'Fresh case directory required');
const read = p => fs.readFileSync(p), hash = p => crypto.createHash('sha256').update(read(p)).digest('hex');
const ref = p => ({path: path.resolve(p), sha256: hash(p)});
const write = (p, value) => fs.writeFileSync(p, JSON.stringify(value, null, 2) + '\n', {flag: 'wx'});
const protocol = JSON.parse(read(protocolPath).toString().replace(/^\uFEFF/, ''));
const nativeProof = inspectNative(root);
assert.equal(nativeProof.candidate, protocol.candidate);
const source = path.join(root, 'author', 'MultiCapture');
const sourceFiles = ['MultiCapture.pro', 'SendMinus600.sgp', 'RouteBand.bnp', 'SendMinus600.aup', 'RouteSource.dls'].map(n => ref(path.join(source, n)));
const one = (xs, id, type = '') => { const found = xs.filter(c => c.id === id && c.type === type); assert.equal(found.length, 1); return found[0]; };
const native = (p, type) => one(chunks(read(p)), 'RIFF', type);
function leaf(id, data) {
  const h = Buffer.alloc(8); h.write(id); h.writeUInt32LE(data.length, 4);
  return one(chunks(Buffer.concat([h, data, ...(data.length & 1 ? [Buffer.alloc(1)] : [])])), id);
}
fs.mkdirSync(output, {recursive: true});
const records = [];
for (const name of protocol.cases) {
  const caseDir = path.join(output, name), dir = path.join(caseDir, 'MultiCapture'); fs.mkdirSync(dir, {recursive: true});
  const ap = native(path.join(source, 'SendMinus600.aup'), 'DMAP');
  const buffers = ap.children.filter(c => c.type === 'dbfl');
  const fxls = one(one(buffers[1].children, 'RIFF', 'DSBC').children, 'LIST', 'fxls');
  const send = fxls.children[1]; one(send.children, 'data').data.writeInt32LE(name === 'dry-zero' ? 0 : -600);
  if (name.includes('waves')) {
    const h = Buffer.alloc(56); Buffer.from('6802fc87559a604395aa004a1d9de26c', 'hex').copy(h, 4);
    fxls.children.splice(name.startsWith('send-before') ? 2 : 1, 0,
      {id: 'RIFF', type: 'DSFX', children: [leaf('fxhr', h)], raw: Buffer.alloc(12)});
  }
  const names = ['Input.sgp', 'RouteBand.bnp', 'Input.aup', 'RouteSource.dls'];
  fs.writeFileSync(path.join(dir, names[2]), serializeChunk(ap), {flag: 'wx'});
  const segment = native(path.join(source, 'SendMinus600.sgp'), 'DMSG');
  segment.children[segment.children.indexOf(one(segment.children, 'RIFF', 'DMAP'))] = native(path.join(dir, names[2]), 'DMAP');
  fs.writeFileSync(path.join(dir, names[0]), serializeChunk(segment), {flag: 'wx'});
  for (const n of [names[1], names[3]]) fs.copyFileSync(path.join(source, n), path.join(dir, n), fs.constants.COPYFILE_EXCL);
  const project = native(path.join(source, 'MultiCapture.pro'), 'JAZP');
  const entries = project.children.filter(c => c.type === 'file'); assert.equal(entries.length, 4);
  for (let i = 0; i < entries.length; i++) {
    const entry = entries[i]; entry.children[entry.children.indexOf(one(entry.children, 'name'))] = leaf('name', Buffer.from(names[i] + '\0', 'utf16le'));
    const p = path.join(dir, names[i]), document = native(p, ['DMSG', 'DMBD', 'DMAP', 'DLS '][i]);
    const h = one(entry.children, 'filh').data; h.writeUInt32LE(read(p).length, 24);
    one(document.children, i === 3 ? 'dlid' : 'guid').data.copy(h, 28);
  }
  fs.writeFileSync(path.join(dir, 'MultiCapture.pro'), serializeChunk(project), {flag: 'wx'});
  const files = ['MultiCapture.pro', ...names].map((n, i) => ({role: ['project', 'segment', 'band', 'audiopath', 'dls'][i], ...ref(path.join(dir, n))}));
  const checked = inspectSendCase(files, name, root);
  const inputPath = path.join(caseDir, 'inputs.json');
  write(inputPath, {schema: 1, name, candidate: protocol.candidate, createdUtc: new Date().toISOString(), protocolSha256: hash(protocolPath),
    files, sourceFiles, checked, preparation: 'Private native derivatives of actual GUI-saved author files; only frozen gain/default-Waves order and catalog file names/sizes change. Not a GUI author-save claim.', fullAcceptance: false});
  records.push({name, inputs: ref(inputPath), files, tapDirectory: path.join(caseDir, 'taps')});
  fs.mkdirSync(path.join(caseDir, 'taps'));
}
for (const s of sourceFiles) assert.equal(hash(s.path), s.sha256, 'Preserved actual author files');
write(path.join(output, 'prepared-cases.json'), {schema: 1, createdUtc: new Date().toISOString(), candidate: protocol.candidate,
  protocol: ref(protocolPath), authorRoot: root, authorNative: nativeProof, sourceFiles, records, driver: ref(fileURLToPath(import.meta.url)), fullAcceptance: false});
console.log(JSON.stringify({passed: true, candidate: protocol.candidate, cases: records.length, actualCaptureExecuted: false}));

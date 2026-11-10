import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {chunks, inspectNative} from './Inspect-StylePlayerDefaultNative.mjs';

const source = path.resolve(process.argv[2]), output = path.resolve(process.argv[3]);
assert(!fs.existsSync(output), 'Use a fresh controls directory');
assert(!output.startsWith(source + path.sep), 'Controls must be outside source evidence');
const read = p => fs.readFileSync(p), json = p => JSON.parse(read(p).toString().replace(/^\uFEFF/, ''));
const hash = p => crypto.createHash('sha256').update(read(p)).digest('hex');
const saved = json(path.join(source, 'saved-inputs.json'));
const retained = saved.files.map(x => ({path: x.path, sha256: hash(x.path)}));
function encode(cs) {
  return Buffer.concat(cs.map(c => {
    const payload = c.id === 'RIFF' || c.id === 'LIST' ? Buffer.concat([Buffer.from(c.type, 'ascii'), encode(c.children)]) : c.data;
    const h = Buffer.alloc(8); h.write(c.id, 0, 'ascii'); h.writeUInt32LE(payload.length, 4);
    return Buffer.concat([h, payload, Buffer.alloc(payload.length & 1)]);
  }));
}
const all = cs => cs.flatMap(c => [c, ...all(c.children)]);
const results = [];
fs.mkdirSync(output, {recursive: true});
for (const name of ['unchanged', 'missing-map-track', 'wrong-tempo', 'wrong-quiet-groove', 'unowned-generated-map', 'escaped-project-entry', 'catalog-identity', 'changed-source-style', 'chunk-overrun']) {
  const dir = path.join(output, name); fs.mkdirSync(path.join(dir, 'Default'), {recursive: true});
  for (const n of ['author-launch.json', 'author-exit.json', 'scope.json']) fs.copyFileSync(path.join(source, n), path.join(dir, n));
  const localSaved = structuredClone(saved);
  for (const x of localSaved.files) {
    const destination = path.join(dir, 'Default', path.basename(x.path)); fs.copyFileSync(x.path, destination); x.path = destination;
  }
  const mutate = (file, fn) => {
    const p = path.join(dir, 'Default', file), cs = chunks(read(p)); fn(cs); fs.writeFileSync(p, encode(cs));
  };
  if (name === 'missing-map-track') mutate('Composed.sgp', cs => {
    const h = all(cs).find(c => c.id === 'trkh' && c.data.subarray(0, 16).toString('hex') === '9628acd29bb3d111870400600893b1bd'); h.data.fill(0, 0, 16);
  });
  if (name === 'wrong-tempo') mutate('Composed.sgp', cs => all(cs).find(c => c.id === 'tetr').data.writeDoubleLE(150, 12));
  if (name === 'wrong-quiet-groove') mutate('Composed.sgp', cs => { all(cs).find(c => c.id === 'cmnd').data[12] = 90; });
  if (name === 'unowned-generated-map') mutate('Composed.sgp', cs => {
    const ref = all(cs).find(c => c.type === 'DMRF' && c.children.find(h => h.id === 'refh')?.data.subarray(0, 16).toString('hex') === '8f28acd29bb3d111870400600893b1bd');
    ref.children.find(c => c.id === 'guid').data.fill(1);
  });
  if (name === 'escaped-project-entry') mutate('Default.pro', cs => {
    const ref = cs[0].children.find(c => c.type === 'file'); ref.children.find(c => c.id === 'name').data = Buffer.from('..\\Default.stp\0', 'utf16le');
  });
  if (name === 'catalog-identity') mutate('Default.pro', cs => cs[0].children.find(c => c.type === 'file').children.find(c => c.id === 'filh').data.fill(1, 28, 44));
  if (name === 'changed-source-style') mutate('Default.stp', cs => cs[0].children.find(c => c.id === 'styh').data.writeDoubleLE(150, 4));
  if (name === 'chunk-overrun') {
    const p = path.join(dir, 'Default', 'Composed.sgp'), b = read(p); b.writeUInt32LE(0xffffffff, 16); fs.writeFileSync(p, b);
  }
  // Refresh derivative hashes so the negative checks must reject the semantic corruption.
  for (const x of localSaved.files) { x.sha256 = hash(x.path); x.bytes = read(x.path).length; }
  fs.writeFileSync(path.join(dir, 'saved-inputs.json'), JSON.stringify(localSaved, null, 2) + '\n');
  let accepted = false, error = null;
  try { const proof = inspectNative(dir); accepted = proof.passed; fs.writeFileSync(path.join(dir, 'proof.json'), JSON.stringify(proof, null, 2) + '\n'); }
  catch (e) { error = String(e.message); }
  assert.equal(accepted, name === 'unchanged', name);
  const causes = {'missing-map-track': /Missing generated track: ChordMap/, 'wrong-tempo': /Public Tempo item value/,
    'wrong-quiet-groove': /Quiet groove range/, 'unowned-generated-map': /strictly equal/,
    'escaped-project-entry': /deep-equal/, 'catalog-identity': /Project catalog identity/,
    'changed-source-style': /Native source bytes must remain unchanged/, 'chunk-overrun': /RIFF chunk exceeds parent/};
  if (causes[name]) assert.match(error, causes[name], name + ' must fail its intended check');
  const result = {name, accepted, expectedAccepted: name === 'unchanged', error, derivedEvidence: true};
  fs.writeFileSync(path.join(dir, 'result.json'), JSON.stringify(result, null, 2) + '\n'); results.push(result);
}
for (const x of retained) assert.equal(hash(x.path), x.sha256);
const auditor = fileURLToPath(new URL('./Inspect-StylePlayerDefaultNative.mjs', import.meta.url));
fs.copyFileSync(auditor, path.join(output, 'Inspect-StylePlayerDefaultNative.mjs'));
fs.copyFileSync(fileURLToPath(import.meta.url), path.join(output, 'test-auditor.mjs'));
const report = {schema: 1, createdUtc: new Date().toISOString(), passed: true, source, retained, results,
  auditorSha256: hash(auditor), testSha256: hash(fileURLToPath(import.meta.url)),
  scope: 'Independent native auditor controls; derivative copies only, no additional product execution', fullAcceptance: false};
fs.writeFileSync(path.join(output, 'controls.json'), JSON.stringify(report, null, 2) + '\n');
console.log(JSON.stringify({passed: true, positive: 1, negative: 8}));

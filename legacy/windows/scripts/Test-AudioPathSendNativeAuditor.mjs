import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {chunks, inspectDocuments, inspectNative} from './Inspect-AudioPathSendNative.mjs';

const source = path.resolve(process.argv[2]), output = path.resolve(process.argv[3]);
const documentsOnly = process.argv.includes('--documents-only');
assert(!fs.existsSync(output), 'Fresh controls directory required');
assert(!output.startsWith(source + path.sep), 'Controls must be outside source evidence');
const read = p => fs.readFileSync(p);
const hash = p => crypto.createHash('sha256').update(read(p)).digest('hex');
const all = cs => cs.flatMap(c => [c, ...all(c.children)]);
const inputNames = ['SendZero.aup', 'SendMinus600.aup', 'SendMinus600.sgp', 'MultiCapture.pro'];
const retained = inputNames.map(n => {
  const p = path.join(source, 'author', 'MultiCapture', n);
  return {path: p, sha256: hash(p)};
});
const cases = ['unchanged', 'wrong-attenuation', 'wrong-send-class', 'wrong-send-destination',
  'source-buffer-changed', 'sequence-note-changed', 'missing-embedded-path', 'chunk-overrun'];
if (!documentsOnly) cases.push('wrong-project-reference', 'wrong-exit');
const results = [];
fs.mkdirSync(output, {recursive: true});
for (const name of cases) {
  const dir = path.join(output, name);
  fs.mkdirSync(dir);
  for (const n of ['fixture.json', 'author-launch.json']) fs.copyFileSync(path.join(source, n), path.join(dir, n));
  if (!documentsOnly) fs.copyFileSync(path.join(source, 'author-exit.json'), path.join(dir, 'author-exit.json'));
  fs.cpSync(path.join(source, 'before'), path.join(dir, 'before'), {recursive: true});
  fs.cpSync(path.join(source, 'author'), path.join(dir, 'author'), {recursive: true});
  const modify = (file, fn) => {
    const p = path.join(dir, 'author', 'MultiCapture', file), b = read(p);
    fn(b, chunks(b)); fs.writeFileSync(p, b);
  };
  const send = cs => all(cs).find(c => c.type === 'DSFX' &&
    c.children.find(x => x.id === 'fxhr')?.data.subarray(4, 20).toString('hex') === '762160efbbbce0498ccae09a5a152b33');
  if (name === 'wrong-attenuation') modify('SendMinus600.aup', (b, cs) => send(cs).children.find(c => c.id === 'data').data.writeInt32LE(-601));
  if (name === 'wrong-send-class') modify('SendMinus600.aup', (b, cs) => send(cs).children.find(c => c.id === 'fxhr').data[4] ^= 1);
  if (name === 'wrong-send-destination') modify('SendMinus600.aup', (b, cs) => send(cs).children.find(c => c.id === 'fxhr').data[36] ^= 1);
  if (name === 'source-buffer-changed') for (const file of ['SendMinus600.aup', 'SendZero.aup']) modify(file, (b, cs) => {
    const sourceBuffer = cs[0].children.filter(c => c.type === 'dbfl')[1];
    all(sourceBuffer.children).find(c => c.id === 'dsbd').data.writeInt32LE(-100, 6);
  });
  if (name === 'sequence-note-changed') modify('SendMinus600.sgp', (b, cs) => {
    const sequence = all(cs).find(c => c.id === 'seqt');
    chunks(sequence.data).find(c => c.id === 'evtl').data[19] = 70;
  });
  if (name === 'missing-embedded-path') modify('SendMinus600.sgp', (b, cs) => b.write('XXXX', cs[0].children.find(c => c.type === 'DMAP').offset + 8, 'ascii'));
  if (name === 'chunk-overrun') modify('SendMinus600.aup', b => b.writeUInt32LE(0xffffffff, 16));
  if (name === 'wrong-project-reference') modify('MultiCapture.pro', (b, cs) => cs[0].children.find(c => c.type === 'file').children.find(c => c.id === 'filh').data[28] ^= 1);
  if (name === 'wrong-exit') {
    const p = path.join(dir, 'author-exit.json'), e = JSON.parse(read(p).toString().replace(/^\uFEFF/, ''));
    e.exitCode = 1; fs.writeFileSync(p, JSON.stringify(e));
  }
  let accepted = false, error = null;
  try { accepted = (documentsOnly ? inspectDocuments(dir) : inspectNative(dir)).passed; }
  catch (e) { error = String(e.message); }
  assert.equal(accepted, name === 'unchanged', name);
  const intended = {'source-buffer-changed': /Existing source buffer metadata unchanged/,
    'sequence-note-changed': /All non-AudioPath Segment chunks retained/,
    'missing-embedded-path': /Expected one RIFF\/DMAP/,
    'chunk-overrun': /Chunk exceeds parent/, 'wrong-project-reference': /Native Project reference identity/};
  if (intended[name]) assert.match(error, intended[name], name + ' must reject at intended semantic check');
  results.push({name, accepted, expectedAccepted: name === 'unchanged', error, derivedEvidence: true});
}
for (const x of retained) assert.equal(hash(x.path), x.sha256, 'Original evidence must remain unchanged');
const auditor = fileURLToPath(new URL('./Inspect-AudioPathSendNative.mjs', import.meta.url));
fs.copyFileSync(auditor, path.join(output, 'auditor.mjs'));
fs.copyFileSync(fileURLToPath(import.meta.url), path.join(output, 'controls.mjs'));
fs.writeFileSync(path.join(output, 'controls.json'), JSON.stringify({schema: 1, observedUtc: new Date().toISOString(),
  passed: true, source, documentsOnly, retained, results, auditorSha256: hash(auditor),
  testSha256: hash(fileURLToPath(import.meta.url)), scope: 'Independent audit controls on derivative copies; no new product execution',
  fullAcceptance: false}, null, 2) + '\n', {flag: 'wx'});
console.log(JSON.stringify({passed: true, positive: 1, negative: cases.length - 1, documentsOnly}));

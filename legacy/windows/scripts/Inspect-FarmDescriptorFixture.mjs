import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';

const sha = b => crypto.createHash('sha256').update(b).digest('hex');
function chunks(bytes, from = 0, end = bytes.length) {
  const items = [];
  for (let p = from; p < end;) {
    assert(p + 8 <= end); const id = bytes.toString('ascii', p, p + 4), size = bytes.readUInt32LE(p + 4), stop = p + 8 + size; assert(stop + (size & 1) <= end);
    const c = {id, data: bytes.subarray(p + 8, stop), raw: bytes.subarray(p, stop + (size & 1)), padding: size & 1 ? bytes[stop] : 0};
    if (id === 'RIFF' || id === 'LIST') {assert(size >= 4); c.type = bytes.toString('ascii', p + 8, p + 12); c.children = chunks(bytes, p + 12, stop);}
    items.push(c); p = stop + (size & 1);
  }
  return items;
}
const one = (c, id, type) => {const all = c.children.filter(x => x.id === id && (!type || x.type === type)); assert.equal(all.length, 1, 'Unique ' + id + '/' + (type ?? '')); return all[0];};
const unicode = c => {assert.equal(c.data.length % 2, 0); assert(c.data.length >= 2); assert.equal(c.data.readUInt16LE(c.data.length - 2), 0); const t = c.data.subarray(0, -2).toString('utf16le'); assert(!t.includes('\0')); assert.deepEqual(Buffer.from(t + '\0', 'utf16le'), c.data); return t;};
const objectEntries = root => one(one(root, 'RIFF', 'DMCN'), 'LIST', 'cosl').children.filter(c => c.id === 'LIST' && c.type === 'cobl');
export function inspectFarmDescriptorFixture(directory, {scriptOverride} = {}) {
  const proofPath = path.join(directory, 'descriptor-fixture.json'), proof = JSON.parse(fs.readFileSync(proofPath, 'utf8'));
  assert.equal(proof.productExecuted, false); assert.equal(proof.fullAcceptance, false); assert.equal(proof.selections.length, 10);
  for (const f of proof.files) {assert.equal(sha(fs.readFileSync(f.source)), f.sha256); assert.equal(sha(fs.readFileSync(f.path)), f.sha256);}
  const original = fs.readFileSync(proof.originalScript.path); assert.equal(sha(original), proof.originalScript.sha256);
  const script = scriptOverride ?? fs.readFileSync(proof.script.path); if (!scriptOverride) assert.equal(sha(script), proof.script.sha256);
  const roots = chunks(script), originals = chunks(original); assert.equal(roots.length, 1); assert.equal(originals.length, 1); const root = roots[0], oldRoot = originals[0]; assert.equal(root.type, 'DMSC');
  const outside = r => r.children.filter(c => !(c.id === 'RIFF' && c.type === 'DMCN')).map(c => c.raw);
  assert.deepEqual(outside(root), outside(oldRoot), 'Script header/source/opaque data unchanged');
  const entries = objectEntries(root), oldEntries = objectEntries(oldRoot); assert.equal(entries.length, 20); assert.equal(oldEntries.length, 10);
  assert.equal(new Set(entries.map(c => unicode(one(c, 'coba')))).size, 20);
  assert.deepEqual(one(one(root, 'RIFF', 'DMCN'), 'conh').raw, one(one(oldRoot, 'RIFF', 'DMCN'), 'conh').raw);
  for (let i = 0; i < 10; i++) {
    const actual = entries[i], before = oldEntries[i], expected = proof.selections[i], ref = one(actual, 'LIST', 'DMRF'), oldRef = one(before, 'LIST', 'DMRF'), h = one(ref, 'refh'), oh = one(oldRef, 'refh');
    assert.equal(unicode(one(actual, 'coba')), expected.alias); assert.deepEqual(one(actual, 'cobh').raw, one(before, 'cobh').raw);
    assert.equal(h.data.readUInt32LE(16), i % 2 === 0 ? 14 : 6); assert.equal(h.data.readUInt32LE(16) & 17, 0, 'No active GUID or filename');
    assert.deepEqual(h.data.subarray(0, 16), oh.data.subarray(0, 16)); assert.deepEqual(h.data.subarray(20), oh.data.subarray(20));
    assert.equal(unicode(one(ref, 'name')), 'Owned ' + expected.alias); assert.equal(unicode(one(ref, 'catg')), 'Farm Descriptor');
    const preserved = r => r.children.filter(c => !['refh', 'name', 'catg'].includes(c.id)).map(c => c.raw); assert.deepEqual(preserved(ref), preserved(oldRef), 'Inactive GUID/file/opaque fields retained');
    const owner = entries[10 + i], payload = one(owner, 'RIFF', 'DMSG'), source = chunks(fs.readFileSync(expected.source))[0], ch = one(owner, 'cobh');
    assert.equal(unicode(one(owner, 'coba')), 'DescriptorOwner' + i); assert.deepEqual(ch.data.subarray(0, 20), one(before, 'cobh').data.subarray(0, 20)); assert.equal(ch.data.toString('ascii', 20, 28), 'RIFFDMSG');
    assert.deepEqual(one(payload, 'guid').data, one(source, 'guid').data); assert.equal(one(payload, 'guid').data.toString('hex'), expected.objectId); assert.equal(sha(payload.raw), expected.ownedPayloadSha256);
    assert.equal(unicode(one(one(payload, 'LIST', 'UNFO'), 'UNAM')), 'Owned ' + expected.alias); assert.equal(unicode(one(payload, 'catg')), 'Farm Descriptor');
    const events = r => r.children.filter(c => c.id !== 'catg' && !(c.id === 'LIST' && c.type === 'UNFO')).map(c => c.raw); assert.deepEqual(events(payload), events(source), 'Segment tracks/AudioPath/GUID/opaque chunks unchanged');
    const priorInfo = source.children.filter(c => c.id === 'LIST' && c.type === 'UNFO'); assert(priorInfo.length <= 1);
    const infoFields = c => c.children.filter(x => x.id !== 'UNAM').map(x => x.raw); assert.deepEqual(infoFields(one(payload, 'LIST', 'UNFO')), priorInfo.length ? infoFields(priorInfo[0]) : []);
  }
  return {schema: 1, passed: true, scriptSha256: sha(script), aliases: 20, activeNameReferences: 10, activeCategoryReferences: 5, activeGuidOrFileReferences: 0, eventBytesPreserved: true, nativeInputsUnchanged: true, productExecuted: false, fullAcceptance: false};
}
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const [directory, destination] = process.argv.slice(2); assert(directory && destination); assert(!fs.existsSync(destination), 'Preserve earlier proof');
  const result = inspectFarmDescriptorFixture(directory); result.createdUtc = new Date().toISOString(); result.inspector = {path: fileURLToPath(import.meta.url), sha256: sha(fs.readFileSync(fileURLToPath(import.meta.url)))};
  fs.writeFileSync(destination, JSON.stringify(result, null, 2) + '\n', {flag: 'wx'}); console.log(JSON.stringify(result));
}

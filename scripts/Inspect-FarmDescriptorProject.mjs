import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {inspectFarmDescriptorNative} from './Inspect-FarmDescriptorNative.mjs';

const sha = b => crypto.createHash('sha256').update(b).digest('hex');
function chunks(b, a = 0, z = b.length) {
  const out = [];
  for (let p = a; p < z;) {
    assert(p + 8 <= z); const id = b.toString('ascii', p, p + 4), n = b.readUInt32LE(p + 4), end = p + 8 + n;
    assert(end + (n & 1) <= z); const c = {id, data: b.subarray(p + 8, end)};
    if (id === 'RIFF' || id === 'LIST') {assert(n >= 4); c.type = b.toString('ascii', p + 8, p + 12); c.children = chunks(b, p + 12, end);}
    out.push(c); p = end + (n & 1);
  }
  return out;
}
const one = (cs, id, type) => {const found = cs.filter(c => c.id === id && (!type || c.type === type)); assert.equal(found.length, 1); return found[0];};
function text(b) {assert(b.length >= 2 && b.length % 2 === 0); assert.equal(b.readUInt16LE(b.length - 2), 0); const s = b.subarray(0, -2).toString('utf16le'); assert(!s.includes('\0')); return s;}
export function inspectFarmDescriptorProject(directory, {projectOverride} = {}) {
  const native = inspectFarmDescriptorNative(directory, path.join(directory, 'FarmDescriptorSaved.spp'), 'FarmDescriptorSaved');
  const projectPath = path.join(directory, path.basename(directory) + '.pro'), bytes = projectOverride ?? fs.readFileSync(projectPath), roots = chunks(bytes);
  assert.equal(roots.length, 1); const root = roots[0]; assert.equal(root.id, 'RIFF'); assert.equal(root.type, 'JAZP');
  const files = root.children.filter(c => c.id === 'LIST' && c.type === 'file'); assert.equal(files.length, 2);
  const expected = new Map([['DescriptorHost.sgp', 'DMSG'], ['FarmDescriptorSaved.spp', 'DMSC']]); const entries = [];
  for (const c of files) {
    const name = text(one(c.children, 'name').data); assert(expected.has(name), 'Unexpected or duplicate native document');
    const documentPath = path.join(directory, name), doc = fs.readFileSync(documentPath), docRoots = chunks(doc); assert.equal(docRoots.length, 1);
    const document = docRoots[0]; assert.equal(document.type, expected.get(name)); const header = one(c.children, 'filh').data;
    assert(header.length >= 44); assert.equal(header.readUInt32LE(24), doc.length, 'Native file size');
    const guid = one(document.children, 'guid').data; assert.equal(guid.length, 16); assert(guid.some(x => x !== 0)); assert.deepEqual(header.subarray(28, 44), guid, 'Native file identity');
    const info = one(c.children, 'LIST', 'UNFO'); assert.equal(text(one(info.children, 'nnam').data), path.basename(name, path.extname(name)));
    const runtimeName = path.basename(name, path.extname(name)) + (document.type === 'DMSG' ? '.sgt' : '.spt'); assert.equal(text(one(info.children, 'rnam').data), runtimeName);
    entries.push({name, form: document.type, path: documentPath, sha256: sha(doc), bytes: doc.length, guid: guid.toString('hex')}); expected.delete(name);
  }
  assert.equal(expected.size, 0); assert.notEqual(entries[0].guid, entries[1].guid);
  const project = one(root.children, 'LIST', 'proj'); assert(one(project.children, 'pjct').data.length >= 18);
  return {schema: 1, passed: true, native, project: {path: projectPath, sha256: sha(bytes), entries}, scope: 'Saved native JAZP catalog has exactly two intended documents with matching byte sizes and document identities; Script retains all descriptor, source and playback bytes except its requested name.', originalComparisonVerified: false, fullAcceptance: false};
}
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const [directory, destination] = process.argv.slice(2); assert(directory && destination); assert(!fs.existsSync(destination));
  const proof = inspectFarmDescriptorProject(path.resolve(directory)); proof.createdUtc = new Date().toISOString(); proof.inspector = {path: fileURLToPath(import.meta.url), sha256: sha(fs.readFileSync(fileURLToPath(import.meta.url)))};
  fs.writeFileSync(destination, JSON.stringify(proof, null, 2) + '\n', {flag: 'wx'}); console.log(JSON.stringify({passed: true, entries: proof.project.entries, fullAcceptance: false}));
}

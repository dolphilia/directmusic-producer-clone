import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {inspectFarmDescriptorFixture} from './Inspect-FarmDescriptorFixture.mjs';

const sha = b => crypto.createHash('sha256').update(b).digest('hex');
function chunks(bytes, from = 0, end = bytes.length) {
  const result = [];
  for (let p = from; p < end;) {
    assert(p + 8 <= end); const id = bytes.toString('ascii', p, p + 4), size = bytes.readUInt32LE(p + 4), stop = p + 8 + size;
    assert(stop + (size & 1) <= end);
    const item = {id, data: bytes.subarray(p + 8, stop), raw: bytes.subarray(p, stop + (size & 1))};
    if (id === 'RIFF' || id === 'LIST') {assert(size >= 4); item.type = bytes.toString('ascii', p + 8, p + 12); item.children = chunks(bytes, p + 12, stop);}
    result.push(item); p = stop + (size & 1);
  }
  return result;
}
const one = (items, id, type) => {const found = items.filter(c => c.id === id && (!type || c.type === type)); assert.equal(found.length, 1, 'Unique ' + id + '/' + (type ?? '')); return found[0];};
export function inspectFarmDescriptorNative(directory, nativePath, expectedName, {nativeOverride} = {}) {
  const fixture = inspectFarmDescriptorFixture(directory), input = fs.readFileSync(path.join(directory, 'FarmOwned.spp')), saved = nativeOverride ?? fs.readFileSync(nativePath);
  const originals = chunks(input), actual = chunks(saved); assert.equal(originals.length, 1); assert.equal(actual.length, 1); assert.equal(actual[0].type, 'DMSC');
  const before = originals[0], after = actual[0];
  const outsideInfo = r => r.children.filter(c => !(c.id === 'LIST' && c.type === 'UNFO')).map(c => c.raw);
  assert.deepEqual(outsideInfo(after), outsideInfo(before), 'Container/source/header/GUID/opaque bytes retained');
  const oldInfo = one(before.children, 'LIST', 'UNFO'), info = one(after.children, 'LIST', 'UNFO');
  const name = one(info.children, 'UNAM'); assert.deepEqual(name.data, Buffer.from(expectedName + '\0', 'utf16le'), 'GUI saved exact Unicode name');
  const otherInfo = c => c.children.filter(x => x.id !== 'UNAM').map(x => x.raw); assert.deepEqual(otherInfo(info), otherInfo(oldInfo), 'Other native metadata retained');
  return {schema: 1, passed: true, fixture, nativeScript: {path: path.resolve(nativePath), sha256: sha(saved), name: expectedName}, inputScript: {path: path.join(directory, 'FarmOwned.spp'), sha256: sha(input)}, aliases: 20, activeNameReferences: 10, activeCategoryReferences: 5, activeGuidOrFileReferences: 0, sourceAndReplayDataPreserved: true, originalComparisonVerified: false, fullAcceptance: false};
}
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const [directory, nativePath, expectedName, destination] = process.argv.slice(2); assert(directory && nativePath && expectedName && destination); assert(!fs.existsSync(destination), 'Preserve prior evidence');
  const result = inspectFarmDescriptorNative(path.resolve(directory), path.resolve(nativePath), expectedName); result.createdUtc = new Date().toISOString(); result.inspector = {path: fileURLToPath(import.meta.url), sha256: sha(fs.readFileSync(fileURLToPath(import.meta.url)))};
  fs.writeFileSync(destination, JSON.stringify(result, null, 2) + '\n', {flag: 'wx'}); console.log(JSON.stringify(result));
}

import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [source, output] = process.argv.slice(2);
assert(source && output, 'Usage: node Create-FarmDescriptorFixture.mjs <owned Farm data directory> <fresh output>');
assert(!fs.existsSync(output), 'Preserve previous inputs');
const sha = b => crypto.createHash('sha256').update(b).digest('hex');
const text = s => Buffer.from(s + '\0', 'utf16le');
function parse(b, from = 0, end = b.length) {
  const chunks = [];
  for (let at = from; at < end;) {
    assert(at + 8 <= end);
    const id = b.toString('ascii', at, at + 4), size = b.readUInt32LE(at + 4), next = at + 8 + size;
    assert(next + (size & 1) <= end);
    const c = {id, data: Buffer.from(b.subarray(at + 8, next)), padding: size & 1 ? b[next] : 0};
    if (id === 'RIFF' || id === 'LIST') {
      assert(size >= 4); c.type = b.toString('ascii', at + 8, at + 12); c.children = parse(b, at + 12, next);
    }
    chunks.push(c); at = next + (size & 1);
  }
  return chunks;
}
function encode(c) {
  const data = c.children ? Buffer.concat([Buffer.from(c.type, 'ascii'), ...c.children.map(encode)]) : c.data;
  const head = Buffer.alloc(8); head.write(c.id, 0, 'ascii'); head.writeUInt32LE(data.length, 4);
  return Buffer.concat([head, data, ...(data.length & 1 ? [Buffer.from([c.padding ?? 0])] : [])]);
}
function one(c, id, type) {const matches = c.children.filter(x => x.id === id && (!type || x.type === type)); assert.equal(matches.length, 1, id + '/' + (type ?? '')); return matches[0];}
function set(c, id, data) {const matches = c.children.filter(x => x.id === id); assert(matches.length <= 1); if (matches.length) matches[0].data = data; else c.children.push({id, data});}
function clone(c) {return parse(encode(c))[0];}
fs.mkdirSync(output, {recursive: true});
const inputFiles = fs.readdirSync(source).filter(n => /\.(spt|spp|sgt|sty|dls|wav)$/i.test(n));
assert(inputFiles.includes('FarmMusic.spt'));
const files = inputFiles.map(name => {
  const from = path.resolve(source, name), to = path.resolve(output, name), bytes = fs.readFileSync(from);
  fs.writeFileSync(to, bytes, {flag: 'wx'}); assert.equal(sha(fs.readFileSync(to)), sha(bytes));
  return {name, source: from, path: to, bytes: bytes.length, sha256: sha(bytes)};
});
const original = fs.readFileSync(path.join(source, 'FarmMusic.spt')), roots = parse(original);
assert.equal(roots.length, 1); const root = roots[0]; assert.equal(root.type, 'DMSC'); assert.deepEqual(encode(root), original);
const container = one(root, 'RIFF', 'DMCN'), list = one(container, 'LIST', 'cosl'), entries = list.children.filter(x => x.id === 'LIST' && x.type === 'cobl');
assert.equal(entries.length, 10); const owners = [], selections = [];
for (let index = 0; index < entries.length; index++) {
  const entry = entries[index], ref = one(entry, 'LIST', 'DMRF'), header = one(ref, 'refh'), file = one(ref, 'file').data.toString('utf16le').replace(/\0$/, ''), alias = one(entry, 'coba').data.toString('utf16le').replace(/\0$/, '');
  const payloadPath = path.resolve(output, file); assert.equal(path.dirname(payloadPath), path.resolve(output));
  const payload = parse(fs.readFileSync(payloadPath))[0]; assert.equal(payload.type, 'DMSG');
  const guid = one(payload, 'guid').data; assert.equal(guid.length, 16); assert(guid.some(x => x));
  const name = 'Owned ' + alias, category = 'Farm Descriptor'; assert(name.length < 64 && category.length < 64);
  let info = payload.children.filter(x => x.id === 'LIST' && x.type === 'UNFO'); assert(info.length <= 1);
  if (!info.length) {payload.children.push({id: 'LIST', type: 'UNFO', children: []}); info = [payload.children.at(-1)];}
  set(info[0], 'UNAM', text(name)); set(payload, 'catg', text(category));
  set(ref, 'name', text(name)); set(ref, 'catg', text(category));
  const flags = index % 2 === 0 ? 14 : 6; header.data.writeUInt32LE(flags, 16);
  // Keep the original inactive GUID/file and all opaque native data.
  const owner = clone(entry), cobh = one(owner, 'cobh'); cobh.data.write('RIFF', 20, 'ascii'); cobh.data.write('DMSG', 24, 'ascii');
  one(owner, 'coba').data = text('DescriptorOwner' + index);
  owner.children[owner.children.indexOf(one(owner, 'LIST', 'DMRF'))] = payload; owners.push(owner);
  selections.push({alias, ownerAlias: 'DescriptorOwner' + index, name, category, flags, classId: header.data.subarray(0, 16).toString('hex'), objectId: guid.toString('hex'), source: payloadPath, sourceSha256: sha(fs.readFileSync(payloadPath)), ownedPayloadSha256: sha(encode(payload))});
}
list.children.push(...owners);
const scriptPath = path.resolve(output, 'FarmOwned.spp'), script = encode(root); fs.writeFileSync(scriptPath, script, {flag: 'wx'});
assert.deepEqual(fs.readFileSync(path.join(source, 'FarmMusic.spt')), original);
for (const f of files) assert.equal(sha(fs.readFileSync(f.path)), f.sha256);
const proof = {schema: 1, createdUtc: new Date().toISOString(), generator: {path: path.resolve(process.argv[1]), sha256: sha(fs.readFileSync(process.argv[1]))}, kind: 'Owned NAME/CATEGORY native references and forward embedded Segment descriptors; original playback event/AudioPath/dependency bytes retained', originalScript: {path: path.resolve(source, 'FarmMusic.spt'), sha256: sha(original)}, script: {path: scriptPath, sha256: sha(script)}, files, selections, aliases: 20, activeTopFileReferences: 0, productExecuted: false, fullAcceptance: false};
fs.writeFileSync(path.join(output, 'descriptor-fixture.json'), JSON.stringify(proof, null, 2) + '\n', {flag: 'wx'});
console.log(JSON.stringify({scriptPath, aliases: proof.aliases, selections: selections.length, fullAcceptance: false}));

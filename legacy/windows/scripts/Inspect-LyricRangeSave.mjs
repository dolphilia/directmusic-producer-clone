// Independent bounded byte audit; this is not an original Producer oracle.
import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [beforePath, afterPath, output] = process.argv.slice(2);
if (!output) throw Error('Usage: Inspect-LyricRangeSave.mjs BEFORE AFTER OUTPUT');
const before = fs.readFileSync(beforePath), after = fs.readFileSync(afterPath);
const expected = Buffer.from(before), patches = [], lyrics = [];
const delta = 768, begin = 2304, end = 3072;
function patch(at, old, value, kind) {
  assert.equal(before.readInt32LE(at), old);
  expected.writeInt32LE(value, at); patches.push({at, old, value, kind});
}
function walk(at, endAt) {
  while (at < endAt) {
    assert(at + 8 <= endAt);
    const id = before.toString('ascii', at, at + 4), size = before.readUInt32LE(at + 4), data = at + 8, stop = data + size;
    assert(stop + (size & 1) <= endAt);
    if (id === 'RIFF' || id === 'LIST') { assert(size >= 4); walk(data + 4, stop); }
    if (id === 'seqt') walk(data, stop);
    if (id === 'tetr') {
      const stride = before.readUInt32LE(data); assert.equal(stride, 16);
      for (let p = data + 4; p < stop; p += stride) { const time = before.readInt32LE(p); if (time >= begin && time < end) patch(p,time,time+delta,'Tempo'); }
    }
    if (id === 'evtl') {
      const stride = before.readUInt32LE(data); assert(stride >= 20);
      for (let p = data + 4; p < stop; p += stride) { const time = before.readInt32LE(p), effective = time + before.readInt16LE(p + 12); if (effective >= begin && effective < end) patch(p,time,time+delta,'Sequence'); }
    }
    if (id === 'lyrh') {
      assert(size >= 16); const logical = before.readInt32LE(data+8), physical = before.readInt32LE(data+12), selected = physical >= begin && physical < end;
      lyrics.push({logical,physical,timing:before.readUInt32LE(data+4),selected,afterLogical:logical+(selected?delta:0),afterPhysical:physical+(selected?delta:0)});
      if(selected) {patch(data+8,logical,logical+delta,'Lyric logical'); patch(data+12,physical,physical+delta,'Lyric physical');}
    }
    at = stop + (size & 1);
  }
  assert.equal(at,endAt);
}
walk(0,before.length);
assert.equal(patches.length,6); assert.equal(lyrics.length,3);
assert(after.equals(expected),'Saved bytes differ outside the six selected clock fields');
const hash = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
const proof = {schema:1,passed:true,scope:'Source GUI range move [2304,3072) to3072: exactly six clock fields; all other bytes, Unicode, delivery, headers and unselected entry retained. Not original dynamic comparison.',beforePath,afterPath,beforeSha256:hash(before),afterSha256:hash(after),patches,lyrics,analyzerSha256:hash(fs.readFileSync(process.argv[1])),fullAcceptance:false};
fs.writeFileSync(output,JSON.stringify(proof,null,2)+'\n'); console.log(JSON.stringify(proof));

import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [beforePath, afterPath, outputPath] = process.argv.slice(2);
assert(outputPath, 'Usage: Inspect-MarkerMuteRangeSave.mjs BEFORE AFTER OUTPUT');
const hash = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const before = fs.readFileSync(beforePath), after = fs.readFileSync(afterPath), expected = Buffer.from(before), changes = [];
const begin = 2304, end = 3072, delta = 768;
function patch(offset, kind) {const value = before.readInt32LE(offset); expected.writeInt32LE(value + delta, offset); changes.push({kind,offset,before:value,after:value+delta});}
function walk(a, z) {
  for(let p=a;p<z;){assert(p+8<=z);const id=before.toString('ascii',p,p+4),n=before.readUInt32LE(p+4),e=p+8+n;assert(e+(n&1)<=z);
    if(id==='RIFF'||id==='LIST'){assert(n>=4);walk(p+12,e);}
    else if(id==='seqt')walk(p+8,e);
    else if(id==='tetr'||id==='evtl'||id==='play'||id==='vals'||id==='mute'){
      const stride=before.readUInt32LE(p+8);assert(stride>0&&(n-4)%stride===0);
      for(let q=p+12;q<e;q+=stride){const t=before.readInt32LE(q);let selected=t>=begin&&t<end;
        if(id==='evtl'){assert(stride>=20);const effective=t+before.readInt16LE(q+12);selected=effective>=begin&&effective<end;}
        if(selected)patch(q,id);
      }
    }else if(id==='lyrh'){assert(n>=16);const physical=before.readInt32LE(p+20);if(physical>=begin&&physical<end){patch(p+16,'lyric logical');patch(p+20,'lyric physical');}}
    p=e+(n&1);
  }
}
walk(0,before.length);
assert.deepEqual(changes.reduce((a,c)=>{a[c.kind]=(a[c.kind]??0)+1;return a;},{}),{tetr:1,evtl:1,'lyric logical':1,'lyric physical':1,play:1,vals:1,mute:1});
assert(expected.equals(after),'Bytes differ beyond the seven selected clocks');
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,scope:'Independent exact seven-clock save audit for this five-strip GUI relocation; original bulk compatibility and playback effects separate',before:{path:beforePath,sha256:hash(beforePath)},after:{path:afterPath,sha256:hash(afterPath)},range:{begin,end,at:3072,delta},changes,allOtherBytesIdentical:true,auditorSha256:hash(process.argv[1]),fullAcceptance:false};
fs.writeFileSync(outputPath,JSON.stringify(proof,null,2)+'\n');console.log(JSON.stringify({passed:true,changedClocks:changes.length,allOtherBytesIdentical:true}));

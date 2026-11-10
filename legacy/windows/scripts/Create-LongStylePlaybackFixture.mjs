import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [sourceDirectory,outputDirectory]=process.argv.slice(2).map(x=>path.resolve(x));
const read=p=>fs.readFileSync(p),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
function children(b,start,end){const a=[];for(let o=start;o<end;){assert(o+8<=end);const size=b.readUInt32LE(o+4),e=o+8+size;assert(e<=end);a.push({id:b.toString('ascii',o,o+4),o,data:o+8,e});o=e+(size&1);assert(o<=end);}return a;}
assert(!fs.existsSync(outputDirectory));fs.mkdirSync(outputDirectory,{recursive:true});
const sb=read(path.join(sourceDirectory,'fill.sgp')),out=Buffer.from(sb);assert.equal(sb.toString('ascii',0,4),'RIFF');assert.equal(sb.toString('ascii',8,12),'DMSG');assert.equal(sb.readUInt32LE(4)+8,sb.length);
const headers=children(sb,12,sb.length).filter(c=>c.id==='segh');assert.equal(headers.length,1);const h=headers[0];assert(h.e-h.data>=24);assert.equal(sb.readUInt32LE(h.data),0);
const originalLength=sb.readInt32LE(h.data+4),length=49152;assert.equal(originalLength,9216);out.writeInt32LE(length,h.data+4);
const normalized=Buffer.from(out);normalized.writeInt32LE(originalLength,h.data+4);assert(normalized.equals(sb));
const files=[];for(const name of ['project.dmpj','Heartlnd.stp','fill.sgp']){const original=read(path.join(sourceDirectory,name)),data=name==='fill.sgp'?out:original;fs.writeFileSync(path.join(outputDirectory,name),data);files.push({name,sourcePath:path.join(sourceDirectory,name),sourceSha256:hash(original),path:path.join(outputDirectory,name),sha256:hash(data)});}
const manifest={schema:1,createdUtc:new Date().toISOString(),files,originalLength,length,changedByteRange:{offset:h.data+4,bytes:4},tempo:120,expectedNominalSeconds:length/768*60/120,scope:'Only Segment header length extended; all tracks, Command events, Style bytes and project references unchanged; GUI/runtime not yet executed'};
fs.writeFileSync(path.join(outputDirectory,'inputs.json'),JSON.stringify(manifest,null,2)+'\n');console.log(JSON.stringify({outputDirectory,length,seconds:manifest.expectedNominalSeconds}));

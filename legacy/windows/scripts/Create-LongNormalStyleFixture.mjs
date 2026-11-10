import fs from 'node:fs';import path from 'node:path';import assert from 'node:assert/strict';import crypto from 'node:crypto';
const source=path.resolve(process.argv[2]),out=path.resolve(process.argv[3]);
assert(!fs.existsSync(out));
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const names=['Normal.dmpj','Normal.sgp','Heartlnd.stp','owned.dls'];
const inputs=names.map(name=>{const p=path.join(source,name);return {name,path:p,sha256:hash(fs.readFileSync(p))};});
const segment=fs.readFileSync(inputs[1].path),long=Buffer.from(segment);
assert.equal(segment.toString('ascii',0,4),'RIFF');assert.equal(segment.toString('ascii',8,12),'DMSG');assert.equal(segment.readUInt32LE(4)+8,segment.length);
let header;
for(let p=12;p<segment.length;){const n=segment.readUInt32LE(p+4),end=p+8+n;assert(end+(n&1)<=segment.length);if(segment.toString('ascii',p,p+4)==='segh'){assert(!header);assert(n>=8);header=p+8;}p=end+(n&1);}
assert(header);assert.equal(segment.readUInt32LE(header+4),49152);
// Preserve every track, Style, Band, DLS and Project byte. Only extend the
// authored fixture's Segment length, with the existing 120→180 tempo map.
const lengthClocks=294912;long.writeUInt32LE(lengthClocks,header+4);
const normalized=Buffer.from(long);normalized.writeUInt32LE(49152,header+4);assert(normalized.equals(segment));
fs.mkdirSync(out,{recursive:true});
for(const x of inputs)fs.writeFileSync(path.join(out,x.name),x.name==='Normal.sgp'?long:fs.readFileSync(x.path));
const outputs=names.map(name=>({name,path:path.join(out,name),sha256:hash(fs.readFileSync(path.join(out,name)))}));
const manifest={schema:1,createdUtc:new Date().toISOString(),profile:'long-normal-style',sourceSegmentSha256:inputs[1].sha256,inputs,outputs,lengthClocks,originalLengthClocks:49152,changedByteRange:{offset:header+4,bytes:4},nominalSeconds:2+(lengthClocks-3072)/768/3,tempos:[{clocks:0,bpm:120},{clocks:3072,bpm:180}],driverSha256:hash(fs.readFileSync(new URL(import.meta.url))),scope:'Only Segment length extended for GUI timing; no product length-editor acceptance',fullAcceptance:false};
fs.writeFileSync(out+'/inputs.json',JSON.stringify(manifest,null,2)+'\n');console.log(JSON.stringify({out,nominalSeconds:manifest.nominalSeconds}));

import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const source=path.resolve(process.argv[2]),dir=path.resolve(process.argv[3]);assert(!fs.existsSync(dir));
const raw=fs.readFileSync(source),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
function parse(b){const id=b.toString('ascii',0,4),n=b.readUInt32LE(4);assert.equal(n+8+(n&1),b.length);const c={id,data:Buffer.from(b.subarray(8,8+n)),pad:n&1?b[b.length-1]:null};if(id==='RIFF'||id==='LIST'){c.type=b.toString('ascii',8,12);c.children=[];for(let p=12;p<8+n;){const size=b.readUInt32LE(p+4);c.children.push(parse(b.subarray(p,p+8+size+(size&1))));p+=8+size+(size&1);}}return c;}
function encode(c){const data=c.children?Buffer.concat([Buffer.from(c.type),...c.children.map(encode)]):c.data;const h=Buffer.alloc(8);h.write(c.id);h.writeUInt32LE(data.length,4);return Buffer.concat([h,data,...(data.length&1?[Buffer.from([c.pad??0])]:[])]);}
function all(c,id,type){return [...(c.id===id&&(!type||c.type===type)?[c]:[]),...(c.children??[]).flatMap(x=>all(x,id,type))];}
const root=parse(raw);assert.equal(root.type,'DMSG');assert(encode(root).equals(raw));
const lists=all(root,'LIST','lbdl');assert.equal(lists.length,1);const items=lists[0].children;assert.equal(items.length,2);
const header=c=>all(c,'bd2h')[0].data;assert.equal(header(items[0]).readInt32LE(0),0);assert.equal(header(items[1]).readInt32LE(0),3072);
const first=all(items[0],'bins')[0];assert(first);assert.equal(first.data.readUInt32LE(0),0); // GM piano
const second=encode(items[1]);lists[0].children=[items[0]];
const output=encode(root);lists[0].children=items;assert(encode(root).equals(raw)); // Only second Band event omitted, plus ancestor sizes.
fs.mkdirSync(dir,{recursive:true});fs.writeFileSync(path.join(dir,'tempo.sgp'),output);
fs.writeFileSync(path.join(dir,'inputs.json'),JSON.stringify({schema:1,createdUtc:new Date().toISOString(),source,sourceSha256:hash(raw),path:path.join(dir,'tempo.sgp'),sha256:hash(output),removedBandEventSha256:hash(second),scope:'Only time3072 Band switch omitted; original Sequence and120->180 Tempo tracks unchanged; output remains GM piano'},null,2)+'\n');
console.log(dir);

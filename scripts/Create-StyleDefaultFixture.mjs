import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [sourceStyle,sourceMap,sourceDls,destination]=process.argv.slice(2).map(p=>path.resolve(p));
assert(!fs.existsSync(destination),'Use a fresh fixture directory');
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
function chunks(b,a=0,z=b.length){const result=[];for(let p=a;p<z;){assert(p+8<=z);const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),e=p+8+n,c=id==='LIST'||id==='RIFF';assert(e+(n&1)<=z);result.push({id,type:c?b.toString('ascii',p+8,p+12):'',data:c?null:Buffer.from(b.subarray(p+8,e)),children:c?chunks(b,p+12,e):[],padding:n&1?b[e]:0});p=e+(n&1);}return result;}
function encode(c){const payload=c.data??Buffer.concat([Buffer.from(c.type),...c.children.map(encode)]),h=Buffer.alloc(8);h.write(c.id);h.writeUInt32LE(payload.length,4);return Buffer.concat([h,payload,...(payload.length&1?[Buffer.from([c.padding])]:[])]);}
const one=(xs,id,type='')=>{const found=xs.filter(c=>c.id===id&&c.type===type);assert.equal(found.length,1);return found[0];};
const leaf=(id,data)=>({id,type:'',data,children:[],padding:0});
const styleBytes=fs.readFileSync(sourceStyle),mapBytes=fs.readFileSync(sourceMap),dlsBytes=fs.readFileSync(sourceDls);
const style=one(chunks(styleBytes),'RIFF','DMST'),map=one(chunks(mapBytes),'RIFF','DMPR');
const identity=one(map.children,'guid').data;assert.equal(identity.length,16);
const header=Buffer.from('8f28acd29bb3d111870400600893b1bd00000000','hex');header.writeUInt32LE(1|2|16,16);
const descriptor={id:'LIST',type:'DMRF',data:null,padding:0,children:[leaf('refh',header),leaf('guid',identity),leaf('file',Buffer.from('Default.cdm\0','utf16le'))]};
const removed=style.children.filter(c=>c.id==='LIST'&&c.type==='prrf').map(c=>({bytes:encode(c).length,sha256:hash(encode(c))}));
style.children=style.children.filter(c=>!(c.id==='LIST'&&c.type==='prrf'));style.children.push({id:'LIST',type:'prrf',data:null,padding:0,children:[descriptor]});
const files={'Default.stp':encode(style),'Default.cdm':mapBytes,'owned.dls':dlsBytes};
fs.mkdirSync(destination,{recursive:true});for(const [name,b] of Object.entries(files))fs.writeFileSync(path.join(destination,name),b);
fs.copyFileSync(new URL(import.meta.url),path.join(destination,'builder.mjs'));
const record={schema:1,createdUtc:new Date().toISOString(),inputs:[sourceStyle,sourceMap,sourceDls].map(p=>({path:p,sha256:hash(fs.readFileSync(p))})),files:Object.entries(files).map(([name,b])=>({path:path.join(destination,name),bytes:b.length,sha256:hash(b)})),removedReferenceLists:removed,defaultMapGuidBytes:identity.toString('hex'),scope:'Public dmusicf.h Style prrf/DMRF reference added to immutable source Style; all other chunks/padding and complete map/DLS bytes retained. Selection must be observed through OS GetDefaultChordMap; no original Producer creation/parity claim.',fullAcceptance:false};
fs.writeFileSync(path.join(destination,'fixture.json'),JSON.stringify(record,null,2)+'\n');console.log(destination);

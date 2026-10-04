import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const dir=path.resolve(process.argv[2]);
const read=p=>fs.readFileSync(path.join(dir,p));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
function parse(b,a=0,z=b.length){
 const out=[];
 for(let p=a;p<z;){
  assert(p+8<=z);const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),e=p+8+n;
  assert(e+(n&1)<=z);const c={id,data:Buffer.from(b.subarray(p+8,e)),pad:n&1?b[e]:0};
  if(['RIFF','LIST'].includes(id)){assert(n>=4);c.type=b.toString('ascii',p+8,p+12);c.children=parse(b,p+12,e);}
  out.push(c);p=e+(n&1);
 }
 return out;
}
function encode(c){
 const d=c.children?Buffer.concat([Buffer.from(c.type),...c.children.map(encode)]):c.data;
 const b=Buffer.alloc(8+d.length+(d.length&1));b.write(c.id);b.writeUInt32LE(d.length,4);d.copy(b,8);if(d.length&1)b[b.length-1]=c.pad;return b;
}
const root=p=>{const r=parse(read(p));assert.equal(r.length,1);assert(encode(r[0]).equals(read(p)));return r[0];};
const child=(c,id,type)=>{const a=c.children.filter(x=>x.id===id&&(!type||x.type===type));assert.equal(a.length,1);return a[0];};
const str=c=>c.data.toString('utf16le').replace(/\0+$/,'');
const original=root('before/Owned.aup'),actual=root('Source/Owned.aup');
assert.equal(actual.type,'DMAP');
const nm=child(child(actual,'LIST','UNFO'),'UNAM');assert.equal(str(nm),'無人Stereo保存');
nm.data=child(child(original,'LIST','UNFO'),'UNAM').data;
const port=child(child(actual,'LIST','pcsl'),'LIST','pcfl');
const ph=child(port,'pcfh').data;assert.equal(ph.readUInt32LE(16),0);assert.equal(ph.readUInt32LE(20),16);
const route=child(child(port,'LIST','pchl'),'pchh');
assert.equal(route.data.readUInt32LE(0),0);assert.equal(route.data.readUInt32LE(4),8);assert.equal(route.data.readUInt32LE(8),1);
route.data.writeUInt32LE(16,4);
assert(encode(actual).equals(encode(original)),'Only Unicode name and route count changed; all port/buffer/opaque bytes exact');
const segment=root('Source/Normal.sgp'),oldSegment=root('before/Normal.sgp');
const embedded=child(segment,'RIFF','DMAP');assert(encode(embedded).equals(read('Source/Owned.aup')),'Segment owns exact edited AudioPath');
segment.children[segment.children.indexOf(embedded)]=child(oldSegment,'RIFF','DMAP');
assert(encode(segment).equals(encode(oldSegment)),'All Segment tracks and metadata outside AudioPath unchanged');
const project=root('Source/Source.pro'),oldProject=root('before/Source.pro');assert.equal(project.type,'JAZP');
let metadata=0,names=0;
for(const c of project.children.filter(x=>x.id==='LIST'&&x.type==='file')){
 const name=str(child(c,'name'));if(!['Owned.aup','Normal.sgp'].includes(name))continue;
 const old=oldProject.children.find(x=>x.id==='LIST'&&x.type==='file'&&str(child(x,'name'))===name);assert(old);
 if(name==='Owned.aup'){const n=child(child(c,'LIST','UNFO'),'nnam');assert.equal(str(n),'無人Stereo保存');n.data=child(child(old,'LIST','UNFO'),'nnam').data;names++;}
 const f=child(c,'filh').data,of=child(old,'filh').data,stat=fs.statSync(path.join(dir,'Source',name),{bigint:true});
 assert.equal(f.length,44);assert.equal(f.readUInt32LE(24),Number(stat.size));assert.equal(f.readBigUInt64LE(16),stat.mtimeNs/100n+116444736000000000n);
 assert(f.subarray(0,16).equals(of.subarray(0,16))&&f.subarray(28).equals(of.subarray(28)));of.copy(f,16,16,28);metadata++;
}
assert.equal(names,1);assert.equal(metadata,2);assert(encode(project).equals(encode(oldProject)),'Only AP nnam and two size/write FILETIMEs changed');
const unchanged=[];for(const n of fs.readdirSync(path.join(dir,'before')))if(!['Owned.aup','Normal.sgp','Source.pro'].includes(n)){assert(read('Source/'+n).equals(read('before/'+n)));unchanged.push(n);}
assert(read('undo-tree.txt').toString().includes('PChannel 0 count 16, buffers 1'));
assert(read('redo-tree.txt').toString().includes('PChannel 0 count 8, buffers 1'));
for(const n of ['ap-saved-tree.txt','ap-reopened-tree.txt']){
 const t=read(n).toString();assert(t.includes('Name Value: 無人Stereo保存'));assert(t.includes('PChannel 0 count 8, buffers 1'));assert(t.includes('Value: 無人Stereo保存 — Owned.aup'));assert(t.includes('Target Segment Value: Segment 1 — Normal.sgp'));
}
for(const n of ['exit-first.json','exit-second.json']){const e=JSON.parse(read(n).toString().replace(/^\uFEFF/,''));assert.equal(e.state,'exited');assert.equal(e.exitCode,0);assert.equal(e.forcedTermination,false);}
const files=['Source/Owned.aup','Source/Normal.sgp','Source/Source.pro','undo-tree.txt','redo-tree.txt','ap-saved-tree.txt','ap-reopened-tree.txt','exit-first.json','exit-second.json',...unchanged.map(n=>'Source/'+n)].map(p=>({path:p,sha256:hash(path.join(dir,p))}));
const proof={passed:true,createdUtc:new Date().toISOString(),scope:'Same-build actual GUI AP name/route edit, Undo/Redo, assignment, AP/Segment/native Project saves, normal exit and separate GUI reload. Independent whole-file audit. Edited audio and full acceptance unverified.',port:{base:0,count:16},route:{base:0,count:8},nativeNames:1,nativeFileMetadata:2,unchangedFiles:unchanged,files,auditorSha256:hash(import.meta.filename),fullAcceptance:false};
fs.writeFileSync(path.join(dir,'gui-file-proof.json'),JSON.stringify(proof,null,2)+'\n');console.log(JSON.stringify(proof));

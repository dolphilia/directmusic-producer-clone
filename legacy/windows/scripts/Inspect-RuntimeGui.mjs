import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
// Audit actual GUI-written files independently of the product's serializers.
const dir=path.resolve(process.argv[2]), baseline=path.resolve(process.argv[3]);
const read=p=>fs.readFileSync(p), hash=b=>crypto.createHash('sha256').update(b).digest('hex');
function chunks(b,a=0,z=b.length){const out=[];for(let p=a;p<z;){assert(p+8<=z);const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),end=p+8+n;assert(end+(n&1)<=z);const c={id,data:b.subarray(p+8,end),pad:n&1?b[end]:0};if(['RIFF','LIST'].includes(id)){c.type=b.toString('ascii',p+8,p+12);c.children=chunks(b,p+12,end);}out.push(c);p=end+(n&1);}return out;}
function encode(c){const d=c.children?Buffer.concat([Buffer.from(c.type),...c.children.map(encode)]):c.data,b=Buffer.alloc(8+d.length+(d.length&1));b.write(c.id);b.writeUInt32LE(d.length,4);d.copy(b,8);if(d.length&1)b[b.length-1]=c.pad;return b;}
const before=read(baseline+'/SourceHeld/Source.pro'),after=read(dir+'/Source/Source.pro');
const root=chunks(after)[0];assert.equal(root.type,'JAZP');let changed=0;
function normalize(c){if(c.id==='rnam'&&c.data.toString('utf16le').replace(/\0+$/,'')==='GuiChanged.sgt'){c.data=Buffer.from('Changed.sgt\0','utf16le');changed++;}for(const x of c.children??[])normalize(x);}
normalize(root);assert.equal(changed,1);assert(encode(root).equals(before),'Only the selected runtime name may change in native Project');
const files=[];
for(const name of fs.readdirSync(baseline+'/SourceHeld')){assert(read(dir+'/Source/'+name).equals(name==='Source.pro'?after:read(baseline+'/SourceHeld/'+name)));files.push({path:'Source/'+name,sha256:hash(read(dir+'/Source/'+name))});}
const expected=['Bands/Changed.bnd','Collections/Changed.dls','Paths/Changed.aud','Segments/GuiChanged.sgt'];
const actual=fs.readdirSync(dir+'/Runtime',{recursive:true,withFileTypes:true}).filter(x=>x.isFile()).map(x=>path.relative(dir+'/Runtime',path.join(x.parentPath,x.name)).replaceAll('\\','/')).sort();assert.deepEqual(actual,expected);
for(const name of expected){const b=read(dir+'/Runtime/'+name),old=name.replace('GuiChanged.sgt','Changed.sgt');assert(b.equals(read(baseline+'/Runtime/'+old)));files.push({path:'Runtime/'+name,sha256:hash(b)});}
const proof={passed:true,createdUtc:new Date().toISOString(),baseline,baselineProjectSha256:hash(before),nativeProjectSha256:hash(after),changedRuntimeNames:1,unchangedSourceFiles:files.filter(x=>x.path!=='Source/Source.pro'&&x.path.startsWith('Source/')).length,runtimeOutputs:4,files,scope:'GUI settings and save exact bytes; process identity, normal exit, separate GUI reload and audio require separate evidence',fullAcceptance:false};
fs.writeFileSync(dir+'/gui-file-proof.json',JSON.stringify(proof,null,2)+'\n');console.log(JSON.stringify(proof));

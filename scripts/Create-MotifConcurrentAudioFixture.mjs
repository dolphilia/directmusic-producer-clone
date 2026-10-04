import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';import assert from 'node:assert/strict';
const source=path.resolve(process.argv[2]),dir=path.resolve(process.argv[3]),repeats=Number(process.argv[4]??15);assert(Number.isInteger(repeats)&&repeats>=1&&repeats<=127);assert(!fs.existsSync(dir));fs.mkdirSync(dir,{recursive:true});
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
function parse(b,a=0,z=b.length){const out=[];for(let p=a;p<z;){const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),end=p+8+n,c=id==='LIST'||id==='RIFF';assert(end+(n&1)<=z);out.push({id,type:c?b.toString('ascii',p+8,p+12):'',data:c?null:Buffer.from(b.subarray(p+8,end)),children:c?parse(b,p+12,end):[],padding:n&1?b[end]:0});p=end+(n&1);}return out;}
function encode(c){const data=c.data??Buffer.concat([Buffer.from(c.type),...c.children.map(encode)]),h=Buffer.alloc(8);h.write(c.id);h.writeUInt32LE(data.length,4);return Buffer.concat([h,data,...(data.length&1?[Buffer.from([c.padding])]:[])]);}
const one=(c,id,type='')=>{const xs=c.children.filter(x=>x.id===id&&x.type===type);assert.equal(xs.length,1);return xs[0];};
const initial=fs.readFileSync(source+'/Heartlnd.stp'),sourceDls=fs.readFileSync(source+'/owned.dls'),dlsRoot=parse(sourceDls)[0],instruments=one(dlsRoot,'LIST','lins').children;
const selected=instruments.filter(i=>{const h=one(i,'insh').data;return h.readUInt32LE(4)===3&&h.readUInt32LE(8)===9;});assert.equal(selected.length,1);
const regions=one(selected[0],'LIST','lrgn').children;assert.equal(regions.length,1);const range=one(regions[0],'rgnh').data;range.writeUInt16LE(60,0);range.writeUInt16LE(72,2);
const files={'source.stp':initial,'source.dls':sourceDls,'owned.dls':encode(dlsRoot)};
for(const [name,pitch,channel,identity]of [['primary.stp',60,4,0x33],['secondary.stp',67,5,0x77]]){
 const root=parse(initial)[0],pattern=one(root,'LIST','pttn');one(root,'guid').data[0]^=identity;one(pattern,'mtfs').data.writeUInt32LE(repeats,0);
 const part=one(root,'LIST','part'),notes=one(part,'note').data;assert.equal(notes.readUInt32LE(0),24);for(let p=4;p<notes.length;p+=24)notes.writeUInt16LE(pitch,p+14);
 const reference=one(one(pattern,'LIST','pref'),'prfc').data;reference.writeUInt32LE(channel,24);
 const instrument=one(one(pattern,'RIFF','DMBD'),'LIST','lbil').children[0],bins=one(instrument,'bins').data;assert.equal(bins.readUInt32LE(0),777);bins.writeUInt32LE(channel,24);files[name]=encode(root);
}
for(const [name,bytes]of Object.entries(files))fs.writeFileSync(dir+'/'+name,bytes);fs.copyFileSync(new URL(import.meta.url),dir+'/builder.mjs');fs.writeFileSync(dir+'/fixture.json',JSON.stringify({schema:1,source,scope:'Distinct non-harmonic DLS pitches/PChannels; fixed long finite Motifs for concurrent digital acceptance, no GUI creation claim',primary:{file:'primary.stp',pitch:60,pchannel:4},secondary:{file:'secondary.stp',pitch:67,pchannel:5},repeats,patch:777,files:Object.entries(files).map(([file,b])=>({file,sha256:hash(b)})),fullAcceptance:false},null,2)+'\n');console.log(dir);

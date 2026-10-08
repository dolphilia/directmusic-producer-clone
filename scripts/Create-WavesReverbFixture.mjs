import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';import assert from 'node:assert/strict';
const [source,destination]=process.argv.slice(2).map(p=>path.resolve(p));
assert(source&&destination&&!fs.existsSync(destination),'Source and a fresh destination required');
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const waves=Buffer.from('6802fc87559a604395aa004a1d9de26c','hex');
function parse(b,start=0,end=b.length){const out=[];for(let p=start;p<end;){assert(p+8<=end);const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),next=p+8+n+(n&1),container=id==='RIFF'||id==='LIST';assert(next<=end);assert(!container||n>=4);out.push({id,type:container?b.toString('ascii',p+8,p+12):'',data:container?null:Buffer.from(b.subarray(p+8,p+8+n)),children:container?parse(b,p+12,p+8+n):[],padding:n&1?b[p+8+n]:0});p=next;}return out;}
function encode(c){const data=c.data??Buffer.concat([Buffer.from(c.type),...c.children.map(encode)]),h=Buffer.alloc(8);h.write(c.id);h.writeUInt32LE(data.length,4);return Buffer.concat([h,data,...(data.length&1?[Buffer.from([c.padding])]:[])]);}
const one=(xs,id,type='')=>{const selected=xs.filter(c=>c.id===id&&c.type===type);assert.equal(selected.length,1,`${id}/${type} unique`);return selected[0];};
const files=['MultiCapture.pro','RouteBand.bnp','RoutePath.aup','RouteSong.sgp','RouteSource.dls'];
const input=Object.fromEntries(files.map(name=>[name,fs.readFileSync(path.join(source,name))]));
const audio=one(parse(input['RoutePath.aup']),'RIFF','DMAP');const groups=audio.children.filter(c=>c.id==='LIST'&&c.type==='dbfl');
const ports=one(audio.children,'LIST','pcsl'),port=one(ports.children,'LIST','pcfl'),route=one(port.children,'LIST','pchl').children.find(c=>c.id==='pchh');
assert(route&&route.data.readUInt32LE(8)>0);const identity=route.data.subarray(16,32);const selected=groups.findIndex(g=>one(g.children,'ddah').data.subarray(0,16).equals(identity));assert(selected>=0);
const buffer=groups[selected];assert.equal(one(buffer.children,'ddah').data.readUInt32LE(16)&2,0,'Fixture requires an existing custom buffer; materialization is tested by native mode');
const descriptor=one(buffer.children,'RIFF','DSBC'),description=one(descriptor.children,'dsbd');assert(description.data.length>=20);
let fx=descriptor.children.find(c=>c.id==='LIST'&&c.type==='fxls');if(!fx){fx={id:'LIST',type:'fxls',data:null,children:[],padding:0};descriptor.children.push(fx);}
assert(!fx.children.some(c=>c.id==='RIFF'&&c.type==='DSFX'&&one(c.children,'fxhr').data.subarray(4,20).equals(waves)),'Do not duplicate a source Waves effect');
description.data.writeUInt32LE(description.data.readUInt32LE(0)|0x200,0);
const header=Buffer.alloc(56);waves.copy(header,4);fx.children.push({id:'RIFF',type:'DSFX',data:null,padding:0,children:[{id:'fxhr',type:'',data:header,children:[],padding:0}]});
const wetAudio=encode(audio),song=one(parse(input['RouteSong.sgp']),'RIFF','DMSG');const embedded=one(song.children,'RIFF','DMAP');assert(encode(embedded).equals(input['RoutePath.aup']),'Source Segment must embed the same AudioPath');
song.children[song.children.indexOf(embedded)]=audio;
const wet={...input,'RoutePath.aup':wetAudio,'RouteSong.sgp':encode(song)};
const variants={dry:input,wet};const outputs=[];for(const [variant,entries] of Object.entries(variants)){const dir=path.join(destination,variant,'MultiCapture');fs.mkdirSync(dir,{recursive:true});for(const [name,bytes] of Object.entries(entries)){const out=path.join(dir,name);fs.writeFileSync(out,bytes);outputs.push({variant,path:path.relative(destination,out).replaceAll('\\','/'),bytes:bytes.length,sha256:hash(bytes)});}}
const record={schema:1,createdUtc:new Date().toISOString(),builder:{path:'scripts/Create-WavesReverbFixture.mjs',sha256:hash(fs.readFileSync(new URL(import.meta.url)))},inputs:files.map(name=>({path:path.join(source,name),sha256:hash(input[name]),bytes:input[name].length})),outputs,buffer:selected,pchannel:route.data.readUInt32LE(0),bufferIndex:0,classGuid:'87FC0268-9A55-4360-95AA-004A1D9DE26C',parameterPolicy:'No data chunk; public OS factory defaults',scope:'Independent controlled native dry/wet inputs; only DSBCAPS_CTRLFX and appended Waves effect/embedded path change. Product build/native/GUI/audio/original are unexecuted.',productExecuted:false,fullAcceptance:false};
fs.writeFileSync(path.join(destination,'fixture.json'),JSON.stringify(record,null,2)+'\n');console.log(JSON.stringify({destination,files:outputs.length,buffer:selected,productExecuted:false}));

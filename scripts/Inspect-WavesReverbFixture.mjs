import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
export function inspectWavesFixture(directory){
    const read=p=>fs.readFileSync(p),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
    const record=JSON.parse(read(path.join(directory,'fixture.json')));assert.equal(record.classGuid,'87FC0268-9A55-4360-95AA-004A1D9DE26C');
    // Independent walker stores original container bytes as well as children.
    function walk(bytes){const children=[];let offset=0;while(offset<bytes.length){assert(offset+8<=bytes.length,'RIFF header');const size=bytes.readUInt32LE(offset+4),end=offset+8+size;assert(end+(size&1)<=bytes.length,'RIFF bounds');const id=bytes.toString('ascii',offset,offset+4),container=['LIST','RIFF'].includes(id);assert(!container||size>=4);const raw=Buffer.from(bytes.subarray(offset,end+(size&1)));children.push({id,type:container?bytes.toString('ascii',offset+8,offset+12):'',payload:Buffer.from(bytes.subarray(offset+8,end)),raw,children:container?walk(bytes.subarray(offset+12,end)):[]});offset=end+(size&1);}assert.equal(offset,bytes.length);return children;}
    function serialize(c){if(!['RIFF','LIST'].includes(c.id))return c.raw;const payload=Buffer.concat([Buffer.from(c.type),...c.children.map(serialize)]),header=Buffer.from(c.raw.subarray(0,8));header.writeUInt32LE(payload.length,4);return Buffer.concat([header,payload,...(payload.length&1?[c.raw.subarray(c.raw.length-1)]:[])]);}
    const one=(xs,id,type='')=>{const matches=xs.filter(c=>c.id===id&&c.type===type);assert.equal(matches.length,1,`unique ${id}/${type}`);return matches[0];};
    assert.equal(record.outputs.length,10);assert.equal(record.inputs.length,5);assert.equal(new Set(record.outputs.map(o=>o.path)).size,10);
    for(const item of record.inputs){const b=read(item.path);assert.equal(hash(b),item.sha256,'Original input hash');assert.equal(b.length,item.bytes);}
    for(const item of record.outputs){assert(!path.isAbsolute(item.path)&&!item.path.split(/[\\/]/).includes('..'),'Fixture path contained');const b=read(path.join(directory,item.path));assert.equal(hash(b),item.sha256,'Fixture output hash');assert.equal(b.length,item.bytes);}
    const get=(variant,name)=>{const item=record.outputs.find(o=>o.variant===variant&&path.basename(o.path)===name);assert(item,'Required output');return read(path.join(directory,item.path));};
    for(const item of record.inputs)assert(get('dry',path.basename(item.path)).equals(read(item.path)),'Dry copies original exactly');
    for(const name of ['MultiCapture.pro','RouteBand.bnp','RouteSource.dls'])assert(get('dry',name).equals(get('wet',name)),'Dependencies unchanged');
    const dry=get('dry','RoutePath.aup'),wet=get('wet','RoutePath.aup'),d=one(walk(dry),'RIFF','DMAP'),w=one(walk(wet),'RIFF','DMAP');
    const buffers=w.children.filter(c=>c.id==='LIST'&&c.type==='dbfl');assert(record.buffer>=0&&record.buffer<buffers.length);const ds=one(buffers[record.buffer].children,'RIFF','DSBC'),fx=one(ds.children,'LIST','fxls');
    const added=fx.children.pop();assert(added&&added.id==='RIFF'&&added.type==='DSFX');assert.equal(added.children.length,1);const h=one(added.children,'fxhr').payload;
    const expected=Buffer.alloc(56);Buffer.from('6802fc87559a604395aa004a1d9de26c','hex').copy(expected,4);assert(h.equals(expected),'Public Waves class, zero flags/reserved/Send, factory defaults');
    const dryBuffers=d.children.filter(c=>c.id==='LIST'&&c.type==='dbfl'),dryDs=one(dryBuffers[record.buffer].children,'RIFF','DSBC');
    if(!dryDs.children.some(c=>c.id==='LIST'&&c.type==='fxls')){assert.equal(fx.children.length,0);ds.children.splice(ds.children.indexOf(fx),1);}
    const desc=one(ds.children,'dsbd'),oldDesc=one(dryDs.children,'dsbd');assert.equal(desc.payload.readUInt32LE(0),oldDesc.payload.readUInt32LE(0)|0x200,'Required buffer effect flag');desc.raw.writeUInt32LE(oldDesc.payload.readUInt32LE(0),8);
    assert(serialize(w).equals(dry),'Only appended Waves and CTRLFX changed in AudioPath');
    const low=one(one(d.children,'LIST','pcsl').children,'LIST','pcfl'),route=one(low.children,'LIST','pchl').children.find(c=>c.id==='pchh');assert(route);assert.equal(record.pchannel,route.payload.readUInt32LE(0));assert.equal(record.bufferIndex,0);assert(one(dryBuffers[record.buffer].children,'ddah').payload.subarray(0,16).equals(route.payload.subarray(16,32)),'Selected buffer follows first route rather than storage order');
    const drySong=get('dry','RouteSong.sgp'),wetSong=get('wet','RouteSong.sgp'),originalSong=one(walk(drySong),'RIFF','DMSG'),changedSong=one(walk(wetSong),'RIFF','DMSG');
    const originalPath=one(originalSong.children,'RIFF','DMAP'),changedPath=one(changedSong.children,'RIFF','DMAP');assert(originalPath.raw.equals(dry));assert(changedPath.raw.equals(wet),'Embedded path equals owned wet document');changedSong.children[changedSong.children.indexOf(changedPath)]=originalPath;assert(serialize(changedSong).equals(drySong),'Only embedded AudioPath changed in Segment');
    return {schema:1,passed:true,scope:'Independent fixture byte contract only; C++ DSP/native GUI/PCM/original not executed',inputCount:5,outputCount:10,buffer:record.buffer,pchannel:record.pchannel,productExecuted:false,fullAcceptance:false};
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){const [dir,out]=process.argv.slice(2);const result=inspectWavesFixture(path.resolve(dir));if(out)fs.writeFileSync(out,JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result));}

import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [input,output]=process.argv.slice(2);
assert(input&&output,'Usage: node Inspect-ChordFile.mjs INPUT OUTPUT');
const bytes=fs.readFileSync(input);
function parse(begin,end){const chunks=[];for(let at=begin;at<end;){assert(at+8<=end);const id=bytes.toString('ascii',at,at+4),size=bytes.readUInt32LE(at+4),stop=at+8+size,next=stop+(size&1);assert(next<=end);const container=id==='RIFF'||id==='LIST';assert(!container||size>=4);chunks.push({id,type:container?bytes.toString('ascii',at+8,at+12):'',data:bytes.subarray(at+8,stop),children:container?parse(at+12,stop):[]});at=next;}return chunks;}
const roots=parse(0,bytes.length);assert.equal(roots.length,1);assert.equal(roots[0].type,'DMSG');
const chordLists=[];
function walk(c){if(c.id==='LIST'&&c.type==='cord')chordLists.push(c);for(const child of c.children)walk(child);}
walk(roots[0]);
const tracks=chordLists.map(c=>{
    const headers=c.children.filter(v=>v.id==='crdh');assert.equal(headers.length,1);assert(headers[0].data.length>=4);
    return {scale:headers[0].data.readUInt32LE(0),events:c.children.filter(v=>v.id==='crdb').map(v=>{
        const b=v.data;assert(b.length>=4);const size=b.readUInt32LE(0);assert(size>=40&&size<=b.length-12);const count=b.readUInt32LE(4+size),stride=b.readUInt32LE(8+size);assert(stride>=20);assert.equal(12+size+count*stride,b.length);
        const name=b.subarray(4,36).toString('utf16le').split('\0')[0];
        return {name,time:b.readInt32LE(36),measure:b.readUInt16LE(40),beat:b[42],flags:b[43],chordStride:size,subchordStride:stride,subchords:Array.from({length:count},(_,i)=>{const p=12+size+i*stride;return {chordPattern:b.readUInt32LE(p),scalePattern:b.readUInt32LE(p+4),inversionPoints:b.readUInt32LE(p+8),levels:b.readUInt32LE(p+12),chordRoot:b[p+16],scaleRoot:b[p+17]};})};
    })};
});
const result={schema:1,input,sha256:crypto.createHash('sha256').update(bytes).digest('hex'),bytes:bytes.length,tracks,scope:'Independent SDK-layout inspection only; not original dynamic comparison, GUI, or audio acceptance'};
fs.writeFileSync(output,JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify({tracks:tracks.length,events:tracks.map(t=>t.events.length),output}));

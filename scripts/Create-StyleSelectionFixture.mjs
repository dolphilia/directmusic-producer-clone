import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [sourceSegment,sourceStyle,output]=process.argv.slice(2).map(x=>path.resolve(x));
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
function parse(b,start=0,end=b.length){const result=[];for(let o=start;o<end;){assert(o+8<=end);const id=b.toString('ascii',o,o+4),size=b.readUInt32LE(o+4),e=o+8+size;assert(e<=end);const container=id==='RIFF'||id==='LIST';assert(!container||size>=4);result.push({id,type:container?b.toString('ascii',o+8,o+12):null,data:container?null:Buffer.from(b.subarray(o+8,e)),children:container?parse(b,o+12,e):[]});o=e+(size&1);assert(o<=end);}return result;}
function encode(c){const payload=c.type?Buffer.concat([Buffer.from(c.type),...c.children.map(encode)]):c.data;const h=Buffer.alloc(8);h.write(c.id);h.writeUInt32LE(payload.length,4);return Buffer.concat([h,payload,...(payload.length&1?[Buffer.alloc(1)]:[])]);}
const clone=c=>parse(encode(c))[0];
const sb=fs.readFileSync(sourceStyle),songBytes=fs.readFileSync(sourceSegment);
const style=parse(sb)[0],song=parse(songBytes)[0];assert.equal(style.type,'DMST');assert.equal(song.type,'DMSG');
const pattern=style.children.find(c=>c.type==='pttn'&&c.children.find(x=>x.id==='ptnh').data.readUInt16LE(6)===0);
const pref=pattern.children.find(c=>c.type==='pref');
const id=pref.children.find(c=>c.id==='prfc').data.subarray(0,16);
const part=style.children.find(c=>c.type==='part'&&c.children.find(x=>x.id==='prth').data.subarray(132,148).equals(id));assert(part);
const variants=[{name:'Low60',bottom:1,top:49,pitch:60,id:'00112233445566778899aabbccddee01'},{name:'High84',bottom:50,top:100,pitch:84,id:'00112233445566778899aabbccddee02'}];
const generated=[];
for(const v of variants){
  const p=clone(part),h=p.children.find(c=>c.id==='prth').data;Buffer.from(v.id,'hex').copy(h,132);h.writeUInt16LE(1,148);h[150]=0;for(let i=0;i<32;i++)h.writeUInt32LE(0xffffffff,4+i*4);
  const notes=Buffer.alloc(4+4*24);notes.writeUInt32LE(24,0);for(let i=0;i<4;i++){const o=4+i*24;notes.writeInt32LE(i*4,o);notes.writeUInt32LE(0xffffffff,o+4);notes.writeInt32LE(384,o+8);notes.writeUInt16LE(v.pitch,o+14);notes[o+16]=96;notes[o+21]=0;notes[o+22]=1;}
  p.children=p.children.filter(c=>!['note','crve','mrkr','rsln'].includes(c.id));p.children.push({id:'note',type:null,data:notes,children:[]});
  const pt=clone(pattern),ph=pt.children.find(c=>c.id==='ptnh').data;ph[4]=v.bottom;ph[5]=v.top;ph.writeUInt16LE(1,8);ph[10]=1;ph[11]=100;
  const ref=clone(pref);Buffer.from(v.id,'hex').copy(ref.children.find(c=>c.id==='prfc').data,0);ref.children.find(c=>c.id==='prfc').data[21]=0;
  pt.children=pt.children.filter(c=>c.type!=='pref');pt.children.push(ref);const unfo=pt.children.find(c=>c.type==='UNFO');if(unfo)unfo.children.find(c=>c.id==='UNAM').data=Buffer.from(v.name+'\0','utf16le');
  generated.push(p,pt);
}
style.children=style.children.filter(c=>!['part','pttn'].includes(c.type));style.children.push(...generated);
const segh=song.children.find(c=>c.id==='segh').data;segh.writeUInt32LE(0,0);segh.writeInt32LE(9216,4);
const tracks=song.children.find(c=>c.type==='trkl').children;
const tempo=tracks.flatMap(c=>c.children).find(c=>c.id==='tetr');tempo.data.writeDoubleLE(120,12);
const command=tracks.flatMap(c=>c.children).find(c=>c.id==='cmnd');const cm=Buffer.alloc(4+3*12);cm.writeUInt32LE(12,0);[25,75,25].forEach((g,i)=>{const o=4+i*12;cm.writeInt32LE(i*3072,o);cm.writeUInt16LE(i,o+4);cm[o+8]=g;});command.data=cm;
fs.mkdirSync(output,{recursive:true});const outStyle=encode(style),outSong=encode(song);
fs.writeFileSync(path.join(output,'Heartlnd.stp'),outStyle);fs.writeFileSync(path.join(output,'selection.sgp'),outSong);
const leaf=(id,data)=>({id,type:null,data,children:[]});const version=Buffer.alloc(4);version.writeUInt32LE(1);
const project=encode({id:'RIFF',type:'DMPJ',children:[leaf('vers',version),leaf('name',Buffer.from('Style selection\0','utf16le')),leaf('file',Buffer.from('selection.sgp\0','utf16le')),leaf('file',Buffer.from('Heartlnd.stp\0','utf16le'))]});
fs.writeFileSync(path.join(output,'selection.dmpj'),project);
const manifest={schema:1,sourceSegment:{path:sourceSegment,sha256:hash(songBytes)},sourceStyle:{path:sourceStyle,sha256:hash(sb)},styleSha256:hash(outStyle),segmentSha256:hash(outSong),variants,commands:[{time:0,groove:25},{time:3072,groove:75},{time:6144,groove:25}],length:9216,tempo:120,scope:'Generated owned fixture: two normal Patterns with disjoint Groove ranges and fixed MIDI notes, no randomness in notes; original source inputs retained'};
manifest.projectSha256=hash(project);fs.writeFileSync(path.join(output,'inputs.json'),JSON.stringify(manifest,null,2)+'\n');console.log(JSON.stringify(manifest));

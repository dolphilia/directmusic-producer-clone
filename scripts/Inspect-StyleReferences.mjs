// Static SDK-format observation. Does not load original or runtime COM servers.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
const [outputDirectory,...inputFiles]=process.argv.slice(2);
if(!outputDirectory||!inputFiles.length)throw Error('Supply output directory and input RIFF files');
fs.mkdirSync(outputDirectory,{recursive:true});
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
function inspect(file,index){
  const bytes=fs.readFileSync(file),records=[];
  function walk(begin,end,parents,depth=0){
    if(depth>64)throw Error('RIFF depth');
    const counts=new Map();
    for(let at=begin;at<end;){
      if(end-at<8)throw Error('Truncated chunk');
      const id=bytes.toString('ascii',at,at+4),size=bytes.readUInt32LE(at+4),data=at+8,stop=data+size;
      if(stop>end)throw Error('Chunk boundary');
      const container=id==='RIFF'||id==='LIST';
      if(container&&size<4)throw Error('Container type missing');
      const type=container?bytes.toString('ascii',data,data+4):null;
      const label=id+(type?':'+type:''),number=counts.get(label)??0;counts.set(label,number+1);
      const location=[...parents,`${label}[${number}]`];
      if(container)walk(data+4,stop,location,depth+1);
      else if(['styh','ptnh','prth','prfc','rhtm','note','crve','mrkr','rsln','anpn','mtfs','stmp','refh','guid','file','name','UNAM'].includes(id)){
        const payload=bytes.subarray(data,stop),r={location:location.join('/'),id,size,sha256:hash(payload)};
        if(id==='styh'||id==='ptnh'){
          if(size<4)throw Error('Meter header truncated');
          r.beats=payload[0];r.denominator=payload[1];r.grids=payload.readUInt16LE(2);
          if(id==='styh'&&(size===12||size>=16)){r.tempoOffset=size===12?4:8;r.tempo=payload.readDoubleLE(r.tempoOffset);}
          if(id==='ptnh'&&size>=10){r.grooveBottom=payload[4];r.grooveTop=payload[5];r.embellishment=payload.readUInt16LE(6);r.measures=payload.readUInt16LE(8);if(size>=16){r.destinationGrooveBottom=payload[10];r.destinationGrooveTop=payload[11];r.flags=payload.readUInt32LE(12);}}
          r.hex=payload.toString('hex');
        }else if(id==='prth'){if(size<150)throw Error('Part header truncated');r.beats=payload[0];r.denominator=payload[1];r.grids=payload.readUInt16LE(2);r.partGuid=payload.subarray(132,148).toString('hex');r.measures=payload.readUInt16LE(148);}
        else if(id==='prfc'){if(size<20)throw Error('Part reference truncated');r.partGuid=payload.subarray(0,16).toString('hex');if(size>=28)r.pchannel=payload.readUInt32LE(24);}
        else if(id==='rhtm'){if(size%4)throw Error('Rhythm DWORD array alignment');r.measureWords=Array.from({length:size/4},(_,i)=>payload.readUInt32LE(i*4));}
        else if(['note','crve','mrkr','rsln','anpn'].includes(id)){if(size<4)throw Error('Record size missing');r.recordBytes=payload.readUInt32LE(0);if(!r.recordBytes||(size-4)%r.recordBytes)throw Error('Record array layout');r.count=(size-4)/r.recordBytes;if(id==='note'){if(r.recordBytes<22)throw Error('Note prefix too small');r.firstNotes=Array.from({length:Math.min(r.count,4)},(_,i)=>{const at=4+i*r.recordBytes;return {gridStart:payload.readInt32LE(at),variation:payload.readUInt32LE(at+4),durationClocks:payload.readInt32LE(at+8),timeOffset:payload.readInt16LE(at+12),musicValue:payload.readUInt16LE(at+14),velocity:payload[at+16]};});}}
        else if(id==='mtfs'){if(size<20)throw Error('Motif settings truncated');r.playStart=payload.readInt32LE(4);r.loopStart=payload.readInt32LE(8);r.loopEnd=payload.readInt32LE(12);}
        else if(id==='stmp'){if(size!==4)throw Error('Style time size');r.clocks=payload.readInt32LE(0);}
        else if(id==='refh'||id==='guid')r.hex=payload.toString('hex');
        else {if(size%2)throw Error('Odd UTF-16');r.text=payload.toString('utf16le').replace(/\0+$/,'');}
        records.push(r);
      }
      at=stop+(size&1);if(at>end)throw Error('Padding missing');
    }
  }
  walk(0,bytes.length,[]);
  const saved=`${index}-${path.basename(file)}`;fs.copyFileSync(file,path.join(outputDirectory,saved));
  return {source:path.resolve(file),saved,sha256:hash(bytes),bytes:bytes.length,records};
}
const inputs=inputFiles.map(inspect);
const evidence={schema:1,createdUtc:new Date().toISOString(),scope:'static Style/Pattern/Part headers, rhythm and record arrays, Style references; no original runtime or behavior comparison',observerSha256:hash(fs.readFileSync(new URL(import.meta.url))),inputs};
fs.copyFileSync(new URL(import.meta.url),path.join(outputDirectory,'Inspect-StyleReferences.mjs'));
fs.writeFileSync(path.join(outputDirectory,'observation.json'),JSON.stringify(evidence,null,2)+'\n');
console.log(JSON.stringify({outputDirectory,inputs:inputs.map(i=>({saved:i.saved,bytes:i.bytes,records:i.records.length}))}));

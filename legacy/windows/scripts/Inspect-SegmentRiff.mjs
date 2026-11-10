// Read-only inspection of saved RIFF segments. Does not load Producer modules.
import fs from 'node:fs';
import crypto from 'node:crypto';
import path from 'node:path';

const files=process.argv.slice(2);
if(!files.length)throw Error('Supply one or more segment files');
const reports=files.map(file=>{
  const bytes=fs.readFileSync(file),chunks=[],tempo=[];
  if(bytes.toString('ascii',0,4)!=='RIFF')throw Error('Expected RIFF segment: '+file);
  function walk(start,end,parents,depth=0){
    if(depth>64)throw Error('RIFF nesting limit');
    let at=start;
    while(at<end){
      if(end-at<8)throw Error('Truncated chunk header');
      const id=bytes.toString('ascii',at,at+4),size=bytes.readUInt32LE(at+4),data=at+8,stop=data+size;
      if(stop>end)throw Error('Chunk exceeds container');
      const container=id==='RIFF'||id==='LIST';
      if(container&&size<4)throw Error('Missing container type');
      const kind=container?bytes.toString('ascii',data,data+4):null;
      const location=[...parents,kind?`${id}:${kind}`:id];
      chunks.push({id,kind,offset:at,size,parents});
      if(container)walk(data+4,stop,location,depth+1);
      if(id==='tetr'){
        if(size<4)throw Error('Missing tempo record size');
        const recordSize=bytes.readUInt32LE(data);
        if(![16,24].includes(recordSize)||(size-4)%recordSize)throw Error('Unsupported tempo record layout');
        const events=[];
        for(let pos=data+4;pos<stop;pos+=recordSize){
          const bpm=bytes.readDoubleLE(pos+8);
          if(!Number.isFinite(bpm))throw Error('Nonfinite BPM');
          events.push({time:bytes.readInt32LE(pos),bpm,...(recordSize===24?{tick:bytes.readInt32LE(pos+16)}:{})});
        }
        tempo.push({offset:at,recordSize,parents,events});
      }
      at=stop+(size&1);
      if(at>end)throw Error('Missing chunk padding');
    }
    if(at!==end)throw Error('Container end mismatch');
  }
  walk(0,bytes.length,[]);
  return {file:path.resolve(file),bytes:bytes.length,sha256:crypto.createHash('sha256').update(bytes).digest('hex'),chunks,tempo};
});
process.stdout.write(JSON.stringify({schema:1,reports},null,2)+'\n');

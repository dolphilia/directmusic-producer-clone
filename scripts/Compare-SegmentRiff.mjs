// Compare leaf chunk payloads without normalizing or removing any bytes.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
const [leftFile,rightFile]=process.argv.slice(2);
if(!leftFile||!rightFile)throw Error('Supply two RIFF segment files');
const sha=bytes=>crypto.createHash('sha256').update(bytes).digest('hex');
function inspect(file){
  const bytes=fs.readFileSync(file),leaves=[];
  if(bytes.toString('ascii',0,4)!=='RIFF')throw Error('Expected RIFF');
  function walk(start,end,parents,depth=0){
    if(depth>64)throw Error('Nesting limit');
    const counts=new Map();
    for(let at=start;at<end;){
      if(end-at<8)throw Error('Truncated chunk');
      const id=bytes.toString('ascii',at,at+4),size=bytes.readUInt32LE(at+4),data=at+8,stop=data+size;
      if(stop>end)throw Error('Chunk out of bounds');
      const container=id==='LIST'||id==='RIFF';
      if(container&&size<4)throw Error('Missing type');
      const kind=container?bytes.toString('ascii',data,data+4):null;
      const label=kind?`${id}:${kind}`:id,index=counts.get(label)??0;counts.set(label,index+1);
      const location=[...parents,`${label}[${index}]`];
      if(container)walk(data+4,stop,location,depth+1);
      else leaves.push({location:location.join('/'),id,offset:at,size,bytes:bytes.subarray(data,stop)});
      at=stop+(size&1);
      if(at>end)throw Error('Missing padding');
    }
  }
  walk(0,bytes.length,[]);
  return {file:path.resolve(file),sha256:sha(bytes),bytes:bytes.length,leaves};
}
const left=inspect(leftFile),right=inspect(rightFile);
const lmap=new Map(left.leaves.map(x=>[x.location,x])),rmap=new Map(right.leaves.map(x=>[x.location,x]));
const changes=[],same=[];
for(const location of new Set([...lmap.keys(),...rmap.keys()])){
  const l=lmap.get(location),r=rmap.get(location);
  if(l&&r&&l.bytes.equals(r.bytes)){same.push({location,size:l.size,sha256:sha(l.bytes)});continue;}
  const describe=x=>x?{offset:x.offset,size:x.size,sha256:sha(x.bytes),hex:x.bytes.toString('hex'),utf16Text:x.bytes.toString('utf16le').replace(/\0+$/,'')}:null;
  const differingByteRanges=[];
  if(l&&r){
    let beginning=null;
    for(let i=0;i<=Math.max(l.size,r.size);i++){
      const differs=i<Math.max(l.size,r.size)&&l.bytes[i]!==r.bytes[i];
      if(differs&&beginning===null)beginning=i;
      if(!differs&&beginning!==null){differingByteRanges.push({start:beginning,endExclusive:i});beginning=null;}
    }
  }
  changes.push({location,differingByteRanges,left:describe(l),right:describe(r)});
}
const metadata=x=>({file:x.file,sha256:x.sha256,bytes:x.bytes,leafChunks:x.leaves.length});
process.stdout.write(JSON.stringify({schema:1,left:metadata(left),right:metadata(right),sameLeafChunks:same.length,same,changes,normalization:'None; all leaf payload bytes compared'},null,2)+'\n');

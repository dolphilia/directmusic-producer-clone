import fs from 'node:fs';
import crypto from 'node:crypto';
const file=process.argv[2];if(!file)throw Error('Supply DLS file');const b=fs.readFileSync(file);
function tree(a,z){const result=[];for(let at=a;at<z;){const id=b.toString('ascii',at,at+4),n=b.readUInt32LE(at+4),end=at+8+n,container=id==='RIFF'||id==='LIST';if(end+(n&1)>z||container&&n<4)throw Error('Bounds');result.push({id,type:container?b.toString('ascii',at+8,at+12):'',offset:at,size:n,hex:!container&&n<=32?b.subarray(at+8,end).toString('hex'):undefined,children:container?tree(at+12,end):undefined});at=end+(n&1);}return result;}
const observation={schema:1,file,sha256:crypto.createHash('sha256').update(b).digest('hex'),scope:'Read-only original RIFF tree and typed field offsets; original application not executed',tree:tree(0,b.length)};
if(process.argv[3])fs.writeFileSync(process.argv[3],JSON.stringify(observation,null,2)+'\n');console.log(JSON.stringify(observation,null,2));

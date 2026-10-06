import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [input,output]=process.argv.slice(2);assert(input&&output,'Usage: INPUT_JOURNAL OUTPUT_JSON');
const b=fs.readFileSync(input),hash=v=>crypto.createHash('sha256').update(v).digest('hex');
function parse(start,end){const out=[];for(let p=start;p<end;){assert(p+8<=end);const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),stop=p+8+n,next=stop+(n&1);assert(next<=end);const container=id==='LIST'||id==='RIFF';assert(!container||n>=4);out.push({id,type:container?b.toString('ascii',p+8,p+12):'',data:b.subarray(p+8,stop),children:container?parse(p+12,stop):[]});p=next;}return out;}
const roots=parse(0,b.length);assert.equal(roots.length,1);assert.equal(roots[0].type,'RTUP');
const field=(c,id)=>{const items=c.children.filter(x=>x.id===id);assert.equal(items.length,1);return items[0].data;};
const owned=path.resolve('work').toLowerCase()+path.sep;
const files=roots[0].children.filter(c=>c.id==='LIST'&&c.type==='file').map(c=>{
    const target=field(c,'path').toString('utf16le').replace(/\0+$/,''),existed=field(c,'info').readUInt32LE(0)!==0,old=field(c,'oldb'),next=field(c,'newb');assert(path.resolve(target).toLowerCase().startsWith(owned));
    const exists=fs.existsSync(target),current=exists?fs.readFileSync(target):null;
    const before=existed?(exists&&current.equals(old)):!exists,after=exists&&current.equals(next);
    return {target,existed,beforeBytes:old.length,beforeSha256:hash(old),afterBytes:next.length,afterSha256:hash(next),currentExists:exists,currentBytes:current?.length??null,currentSha256:current?hash(current):null,state:before?'Before':after?'After':'Conflict'};
});
fs.writeFileSync(output,JSON.stringify({schema:1,observedUtc:new Date().toISOString(),input,journalSha256:hash(b),files,scope:'Read-only CURRENT bytes compared with retained journal; does not recover or replay, identify a historical lock owner, or prove original compatibility'},null,2)+'\n');console.log(JSON.stringify({states:files.map(f=>f.state),output}));

import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';import assert from 'node:assert/strict';
import {inspectWavesFixture} from './Inspect-WavesReverbFixture.mjs';
const [fixture,controls]=process.argv.slice(2).map(p=>path.resolve(p));assert(fixture&&controls&&!fs.existsSync(controls),'Existing fixture and fresh retained controls directory required');
fs.mkdirSync(controls,{recursive:true});const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const base=inspectWavesFixture(fixture);assert(base.passed);const results=[{control:'unchanged',expected:'pass',observed:'pass'}];
const waves=Buffer.from('6802fc87559a604395aa004a1d9de26c','hex');
const cases=[
    ['wrong-class','RoutePath.aup',b=>{const at=b.indexOf(waves);assert(at>0);b[at]^=0x40;return b;}],
    ['reserved-header','RoutePath.aup',b=>{const at=b.indexOf(waves);assert(at>0);b[at+48]=1;return b;}],
    ['effect-options','RoutePath.aup',b=>{const at=b.indexOf(waves);assert(at>0);b[at-4]=1;return b;}],
    ['embedded-diff','RouteSong.sgp',b=>{const at=b.indexOf(waves);assert(at>0);b[at+1]^=0x10;return b;}],
    ['dependency-diff','RouteSource.dls',b=>{b[b.length-1]^=1;return b;}],
    ['riff-truncated','RoutePath.aup',b=>b.subarray(0,b.length-1)],
    ['route-index','',()=>null],
    ['unverified-buffer','',()=>null]
];
for(const [name,file,mutate] of cases){const dest=path.join(controls,name);fs.cpSync(fixture,dest,{recursive:true});const recordPath=path.join(dest,'fixture.json'),record=JSON.parse(fs.readFileSync(recordPath,'utf8'));
    if(file){const item=record.outputs.find(o=>o.variant==='wet'&&path.basename(o.path)===file);const target=path.join(dest,item.path),bytes=mutate(Buffer.from(fs.readFileSync(target)));fs.writeFileSync(target,bytes);item.sha256=hash(bytes);item.bytes=bytes.length;}
    else if(name==='route-index')record.bufferIndex=1;else record.buffer=record.buffer===0?1:0;
    fs.writeFileSync(recordPath,JSON.stringify(record,null,2)+'\n');let message=null;try{inspectWavesFixture(dest);}catch(error){message=error.message;}assert(message,`${name} must reject with proof hashes recomputed`);results.push({control:name,expected:'reject',observed:'reject',message,directory:dest});
}
const result={schema:1,passed:true,createdUtc:new Date().toISOString(),fixture,path:controls,controls:results,scope:'Independent fixture auditor negative controls; no product execution',productExecuted:false,fullAcceptance:false};fs.writeFileSync(path.join(controls,'controls.json'),JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify({passed:true,controls:results.length,productExecuted:false}));

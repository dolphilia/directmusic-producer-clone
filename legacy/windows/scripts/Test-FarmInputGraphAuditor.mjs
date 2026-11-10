import fs from 'node:fs';import path from 'node:path';import assert from 'node:assert/strict';import crypto from 'node:crypto';
import {inspectFarm} from './Inspect-FarmInputGraph.mjs';
const [directory,output]=process.argv.slice(2);assert(directory&&output,'Usage: node Test-FarmInputGraphAuditor.mjs <input directory> <fresh output>');assert(!fs.existsSync(output),'Preserve earlier controls');
const files=new Map(fs.readdirSync(directory).filter(n=>/\.(spt|sgt|sty|dls|wav)$/i.test(n)).map(n=>[n,fs.readFileSync(path.join(directory,n))]));const results=[];
inspectFarm(files);results.push({id:'unchanged-input',accepted:true});
for(const name of files.keys()){
  const missing=new Map(files);missing.delete(name);assert.throws(()=>inspectFarm(missing));results.push({id:'missing-'+name,rejected:true});
  const truncated=new Map(files);truncated.set(name,files.get(name).subarray(0,files.get(name).length-1));assert.throws(()=>inspectFarm(truncated));results.push({id:'truncated-'+name,rejected:true});
}
const cow=files.get('SfxCow.wav'),guidHeader=Buffer.from('guid','ascii');const offset=cow.indexOf(guidHeader);assert(offset>=12,'Wave input native GUID present');const wrong=Buffer.from(cow);wrong[offset+8]^=1;const changed=new Map(files);changed.set('SfxCow.wav',wrong);assert.throws(()=>inspectFarm(changed));results.push({id:'referenced-wave-guid-mismatch',rejected:true});
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const result={schema:1,createdUtc:new Date().toISOString(),processId:process.pid,auditor:{path:'scripts/Inspect-FarmInputGraph.mjs',sha256:sha('scripts/Inspect-FarmInputGraph.mjs')},controls:{path:process.argv[1],sha256:sha(process.argv[1])},inputDirectory:path.resolve(directory),passed:true,results,scope:'Independent raw-input auditor controls; no native C++/runtime/GUI/audio/original comparison',fullAcceptance:false};fs.writeFileSync(output,JSON.stringify(result,null,2)+'\n',{flag:'wx'});console.log(JSON.stringify({passed:true,controls:results.length,fullAcceptance:false}));

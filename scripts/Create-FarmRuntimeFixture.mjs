import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [source,output]=process.argv.slice(2);
assert(source&&output,'Usage: node Create-FarmRuntimeFixture.mjs <Farm sample directory> <fresh data-only directory>');
assert(!fs.existsSync(output),'Output must be fresh; preserve prior fixtures');
const names=['FarmMusic.spt','BGDawn.sgt','BGNight.sgt','BGPredawn.sgt','SSBird.sgt',
  ...['Alarm','Cougar','Cow','Rooster','Sheep','Wolf'].flatMap(s=>['Sfx'+s+'.sgt','Sfx'+s+'.wav']),
  'FarmGame.sty','FarmGame.dls'];
for(const name of names)assert(fs.statSync(path.join(source,name)).isFile(),name+' missing');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
fs.mkdirSync(output,{recursive:true});
const files=names.map(name=>{const from=path.resolve(source,name),to=path.resolve(output,name),b=fs.readFileSync(from);fs.writeFileSync(to,b,{flag:'wx'});assert.equal(sha(fs.readFileSync(to)),sha(b));return {name,source:from,path:to,bytes:b.length,sha256:sha(b)};});
const result={schema:1,createdUtc:new Date().toISOString(),generator:{path:path.resolve(process.argv[1]),sha256:sha(fs.readFileSync(process.argv[1]))},
  kind:'Exact native input data copies, no product/runtime/original executable invocation',files,
  excluded:'Farm.exe and UI/source/build files are not input dependencies; APFarm.aud is not referenced by this score graph and remains a separate AudioPath obligation',fullAcceptance:false};
fs.writeFileSync(path.join(output,'fixture.json'),JSON.stringify(result,null,2)+'\n',{flag:'wx'});
console.log(JSON.stringify({inputFiles:files.length,fixture:path.resolve(output,'fixture.json'),fullAcceptance:false}));

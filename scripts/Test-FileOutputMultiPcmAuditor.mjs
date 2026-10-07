import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';import assert from 'node:assert/strict';import {spawnSync} from 'node:child_process';
const [sourceArg,buildArg,rootArg]=process.argv.slice(2);assert(rootArg,'Usage: Test-FileOutputMultiPcmAuditor.mjs RECORD_DIRECTORY BUILD_SUMMARY NEW_CONTROLS_DIRECTORY');
const source=path.resolve(sourceArg),build=path.resolve(buildArg),root=path.resolve(rootArg),read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,'')),hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
assert(!fs.existsSync(root));assert(!root.toLowerCase().startsWith(source.toLowerCase()+path.sep));const original=read(source+'/file-output-multi-pcm-proof.json');assert(original.passed);fs.mkdirSync(root,{recursive:true});const results=[];
for(const kind of ['unchanged','reversed-order','silent-second','mixed-groups','missing-restart','bad-riff-length','missing-second']){
 const dir=path.join(root,kind);fs.mkdirSync(dir);for(const name of ['Record.wav','Record1.wav'])if(kind!=='missing-second'||name!=='Record1.wav')fs.copyFileSync(source+'/'+name,dir+'/'+name);
 if(kind==='reversed-order'){fs.copyFileSync(source+'/Record1.wav',dir+'/Record.wav');fs.copyFileSync(source+'/Record.wav',dir+'/Record1.wav');}
 if(['silent-second','mixed-groups','missing-restart','bad-riff-length'].includes(kind)){
  const file=dir+(kind==='silent-second'?'/Record1.wav':'/Record.wav'),b=fs.readFileSync(file);let data=0,count=0,fmt;
  for(let p=12;p+8<=b.length;){const n=b.readUInt32LE(p+4),id=b.toString('ascii',p,p+4);if(id==='fmt ')fmt=b.subarray(p+8,p+8+n);if(id==='data'){data=p+8;count=n;}p+=8+n+(n&1);}assert(fmt&&data&&count);
  const tag=fmt.readUInt16LE(0),bits=fmt.readUInt16LE(14),align=fmt.readUInt16LE(12),channels=fmt.readUInt16LE(2),rate=fmt.readUInt32LE(4),silence=tag===1&&bits===8?128:0;
  if(kind==='silent-second')b.fill(silence,data,data+count);
  if(kind==='missing-restart'){const attack=original.recordings[0].onsets[1];b.fill(silence,data+Math.round((attack-.02)*rate)*align,data+count);}
  if(kind==='bad-riff-length')b.writeUInt32LE(b.readUInt32LE(4)+2,4);
  if(kind==='mixed-groups')for(let f=0;f<count/align;f++)if(original.recordings[0].onsets.some(t=>f/rate>=t+.05&&f/rate<t+.65))for(let c=0;c<channels;c++){const p=data+f*align+c*bits/8,x=tag===3?b.readFloatLE(p):bits===8?(b[p]-128)/128:bits===16?b.readInt16LE(p)/32768:bits===24?b.readIntLE(p,3)/8388608:b.readInt32LE(p)/2147483648,v=Math.max(-.99,Math.min(.99,x+.04*Math.sin(2*Math.PI*261.6255653005986*f/rate)));if(tag===3)b.writeFloatLE(v,p);else if(bits===8)b[p]=Math.round(v*127+128);else if(bits===16)b.writeInt16LE(Math.round(v*32767),p);else if(bits===24)b.writeIntLE(Math.round(v*8388607),p,3);else b.writeInt32LE(Math.round(v*2147483647),p);}
  fs.writeFileSync(file,b);
 }
 const execution=spawnSync(process.execPath,[path.resolve('scripts/Inspect-FileOutputMultiPcm.mjs'),dir,build,original.sourceFolder],{encoding:'utf8'});fs.writeFileSync(dir+'/stdout.txt',execution.stdout);fs.writeFileSync(dir+'/stderr.txt',execution.stderr);assert.equal(execution.status,kind==='unchanged'?0:1,kind);results.push({kind,accepted:execution.status===0,rejected:execution.status===1,exitCode:execution.status,stderrSha256:hash(dir+'/stderr.txt')});
}
const proof={schema:1,passed:true,candidate:original.candidate,sourceCapture:source,sourceProofSha256:hash(source+'/file-output-multi-pcm-proof.json'),auditorSha256:hash('scripts/Inspect-FileOutputMultiPcm.mjs'),driverSha256:hash(process.argv[1]),results,fullAcceptance:false};fs.writeFileSync(root+'/negative-tests.json',JSON.stringify(proof,null,2)+'\n');fs.copyFileSync(process.argv[1],root+'/driver.mjs');console.log(JSON.stringify(proof));

import fs from 'node:fs';import path from 'node:path';import {spawnSync} from 'node:child_process';import assert from 'node:assert/strict';
const original=path.resolve(process.argv[2]),root=original+'/negative-controls';fs.mkdirSync(root,{recursive:true});const cases=[];
for(const kind of ['silence','baseline-sound','slow-with-fast-attack']){
 const dir=root+'/'+kind;for(const v of ['Fast','Slow'])fs.cpSync(original+'/'+v,dir+'/'+v,{recursive:true});
 const variant=kind==='slow-with-fast-attack'?'Slow':'Fast',wav=dir+'/'+variant+'/output.wav',b=fs.readFileSync(wav);let at,rate,align,channels;for(let p=12;p<b.length;){const n=b.readUInt32LE(p+4),id=b.toString('ascii',p,p+4);if(id==='fmt '){rate=b.readUInt32LE(p+12);channels=b.readUInt16LE(p+10);align=b.readUInt16LE(p+20);}if(id==='data')at=p+8;p+=8+n+(n&1);}assert(at&&rate&&align&&channels);
 if(kind==='silence')b.fill(0,at);
 else if(kind==='baseline-sound'){for(let i=Math.round(.4*rate);i<Math.round(.8*rate);i++)for(let c=0;c<channels;c++)b.writeFloatLE(.01*Math.sin(2*Math.PI*440*i/rate),at+i*align+c*4);}
 else {const proof=JSON.parse(fs.readFileSync(original+'/audio-proof.json','utf8')),p=proof.results.find(x=>x.variant==='Slow').phases;for(const start of [p['play-ready'],p['restart-ready']])for(let n=0;n<3;n++){const begin=Math.round((start+n*2+.1)*rate),end=Math.round((start+n*2+.2)*rate);for(let i=begin;i<end;i++)for(let c=0;c<channels;c++)b.writeFloatLE(.0915*Math.sin(2*Math.PI*440*i/rate),at+i*align+c*4);}}
 fs.writeFileSync(wav,b);const result=spawnSync(process.execPath,['scripts/Inspect-EnvelopeAudio.mjs',dir],{encoding:'utf8'});fs.writeFileSync(dir+'/audit.stdout.txt',result.stdout);fs.writeFileSync(dir+'/audit.stderr.txt',result.stderr);assert.notEqual(result.status,0,kind+' must fail');const proof=JSON.parse(fs.readFileSync(dir+'/audio-proof.json','utf8'));assert.equal(proof.passed,false);cases.push({kind,exitCode:result.status,rejected:true,scope:'Counterfactual derived PCM only; not a fresh recording'});
}
fs.writeFileSync(root+'/results.json',JSON.stringify({schema:1,passed:true,cases},null,2)+'\n');console.log(JSON.stringify({passed:true,rejected:cases.length}));

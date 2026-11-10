import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
const source=path.resolve(process.argv[2]),root=path.resolve(process.argv[3]);
assert(!root.toLowerCase().startsWith(source.toLowerCase()+path.sep));assert(!fs.existsSync(root));fs.mkdirSync(root,{recursive:true});
const proof=JSON.parse(fs.readFileSync(path.join(source,'lifecycle-audio-proof.json'),'utf8'));assert(proof.passed);
const results=[];
for(const kind of ['unchanged','sound-during-stop','silent-restart','wrong-restart-pitch']){
 const dir=path.join(root,kind);fs.cpSync(source,dir,{recursive:true});const file=path.join(dir,'output.wav'),wav=fs.readFileSync(file);let dataOffset,rate,align;
 for(let p=12;p+8<wav.length;){const n=wav.readUInt32LE(p+4),id=wav.toString('ascii',p,p+4);if(id==='fmt '){rate=wav.readUInt32LE(p+12);align=wav.readUInt16LE(p+20);}if(id==='data')dataOffset=p+8;p+=8+n+(n&1);}assert(dataOffset&&rate&&align);
 const target=proof.intervals.find(p=>p.name===(kind==='sound-during-stop'?'stop-hold':'restarted-playing'));
 const a=dataOffset+Math.round(target.from*rate)*align,b=dataOffset+Math.round(target.to*rate)*align;
 if(kind==='unchanged'){}
 else if(kind==='silent-restart')wav.fill(0,a,b);
 else if(kind==='wrong-restart-pitch'){const pitch=proof.dlsMode?60:target.expectedMidi===72?60:72;for(let p=a;p+align<=b;p+=align){const t=(p-dataOffset)/align/rate;for(let c=0;c<align/4;c++)wav.writeFloatLE(.04*Math.sin(2*Math.PI*440*2**((pitch-69)/12)*t),p+c*4);}}
 else{const original=Buffer.from(wav);const active=proof.intervals.find(p=>p.name==='first-playing'),start=dataOffset+Math.round(active.from*rate)*align,length=Math.round((active.to-active.from)*rate)*align;for(let p=a;p<b;p++)wav[p]=original[start+(p-a)%length];}
 fs.writeFileSync(file,wav);const result=spawnSync(process.execPath,[path.resolve('scripts/Inspect-AudioLifecycle.mjs'),dir,...(proof.dlsMode?['--dls']:[])],{encoding:'utf8'});
 fs.writeFileSync(path.join(dir,'control.stdout.txt'),result.stdout);fs.writeFileSync(path.join(dir,'control.stderr.txt'),result.stderr);
 const modified=JSON.parse(fs.readFileSync(path.join(dir,'lifecycle-audio-proof.json'),'utf8'));assert.equal(result.status,kind==='unchanged'?0:1);assert.equal(modified.passed,kind==='unchanged');
 results.push({kind,rejected:kind!=='unchanged',exitCode:result.status,interval:modified.intervals.find(p=>p.name===target.name)});
}
fs.writeFileSync(path.join(root,'negative-tests.json'),JSON.stringify({source,scope:'Derived modified WAV copies only; native API success retained',passed:true,results},null,2)+'\n');console.log(JSON.stringify(results));

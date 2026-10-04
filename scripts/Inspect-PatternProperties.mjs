import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [runPath,...noteRuns]=process.argv.slice(2);
const read=p=>fs.readFileSync(p),hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const json=p=>JSON.parse(read(p).toString('utf8').replace(/^\uFEFF/,''));
const run=json(runPath),dir=path.dirname(path.resolve(runPath)),build=json(run.buildSummary);
assert(run.passed&&run.exitCode===0&&!run.launchError&&!run.timedOut&&run.checks===20);
assert.equal(hash(run.buildSummary),run.buildSummarySha256);assert.equal(hash(path.join(dir,'driver.ps1')),run.driverSha256);
assert(build.passed&&build.sourceSnapshotUnchanged);for(const s of build.sources)assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);
assert.equal(hash(run.executable),run.executableSha256);assert.equal(run.executableSha256,build.outputs.find(o=>o.path==='build/Release/producer_core_tests.exe').sha256);
for(const input of run.inputs)assert.equal(hash(input.path),input.sha256);
assert.equal(hash(path.join(dir,'stdout.txt')),run.stdoutSha256);assert(json(path.join(dir,'stdout.txt')).passed);
for(const f of run.files)assert.equal(hash(path.join(dir,'core/pattern-properties',f.name)),f.sha256);
function parse(b,start=0,end=b.length){const a=[];for(let o=start;o<end;){assert(o+8<=end);const id=b.toString('ascii',o,o+4),size=b.readUInt32LE(o+4),e=o+8+size;assert(e<=end);const container=id==='RIFF'||id==='LIST';assert(!container||size>=4);a.push({id,type:container?b.toString('ascii',o+8,o+12):null,data:container?null:Buffer.from(b.subarray(o+8,e)),children:container?parse(b,o+12,e):[],pad:size&1?b[e]:0});o=e+(size&1);assert(o<=end);}return a;}
function encode(c){const b=c.type?Buffer.concat([Buffer.from(c.type),...c.children.map(encode)]):c.data;const h=Buffer.alloc(8);h.write(c.id);h.writeUInt32LE(b.length,4);return Buffer.concat([h,b,...(b.length&1?[Buffer.from([c.pad])]:[])]);}
const clone=c=>parse(encode(c))[0],leaf=(a,id)=>{const v=a.children.filter(c=>c.id===id);assert.equal(v.length,1);return v[0];};
const list=(a,type)=>{const v=a.children.filter(c=>c.type===type);assert.equal(v.length,1);return v[0];};
const patterns=t=>t.children.filter(c=>c.type==='pttn'),parts=t=>t.children.filter(c=>c.type==='part');
const guid=p=>leaf(p,'prth').data.subarray(132,148),binding=p=>leaf(list(p,'pref'),'prfc').data;

const source=read(run.inputs[1].path),root=parse(source)[0],base=path.join(dir,'core/pattern-properties');
assert(read(path.join(base,'original.stp')).equals(source));assert(read(path.join(base,'fill.sgp')).equals(read(run.inputs[0].path)));
function edit(tree,index,name,kind){const pt=patterns(tree)[index];leaf(list(pt,'UNFO'),'UNAM').data=Buffer.from(name+'\0','utf16le');leaf(pt,'ptnh').data.writeUInt16LE(kind,6);}
const expected=clone(root);edit(expected,1,'Converted Intro 64',4);assert(read(path.join(base,'first.stp')).equals(encode(expected)));
edit(expected,3,'Converted Fill 72',1);const finalPath=path.join(base,'Heartlnd.stp');for(const name of ['edited.stp','Heartlnd.stp','resaved.stp'])assert(read(path.join(base,name)).equals(encode(expected)));
assert.equal(patterns(expected).length,5);assert.equal(parts(expected).length,5);
const proofs=[];for(const noteRun of noteRuns){const nr=json(noteRun),nd=path.dirname(path.resolve(noteRun)),proofPath=path.join(nd,'notes-proof.json'),np=json(proofPath);
assert(np.passed&&np.noteCount===12);assert.equal(np.runSha256,hash(noteRun));assert.equal(np.exeSha256,build.outputs.find(o=>o.path==='install/bin/Producer.exe').sha256);
assert.equal(np.dependencies[0].sha256,hash(finalPath));assert.equal(np.dependencies[0].path,path.resolve(finalPath));
const name=path.basename(nr.groupPlaybackInput.path),pitch=name==='fill.sgp'?72:name==='intro.sgp'?64:null;assert(pitch);
assert(np.actual.map(n=>n.midiValue).every((v,i)=>v===(i>=4&&i<8?pitch:60)));
proofs.push({run:noteRun,runSha256:hash(noteRun),proof:proofPath,proofSha256:hash(proofPath),pitch,noteCount:np.noteCount,moduleCount:np.moduleCount});
}assert.equal(proofs.length,2);assert.deepEqual(proofs.map(p=>p.pitch).sort(),[64,72]);
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,build:run.buildSummary,buildSha256:run.buildSummarySha256,coreExeSha256:run.executableSha256,producerExeSha256:build.outputs.find(o=>o.path==='install/bin/Producer.exe').sha256,checks:run.checks,runSha256:hash(runPath),sourceStyle:run.inputs[1],editedStyle:{path:finalPath,sha256:hash(finalPath)},playback:proofs,auditorSha256:hash(process.argv[1]),scope:'Only selected names and embellishment WORDs changed; all other bytes retained. Same-build Framework restore/resave and two generated-note paths; GUI/audio/original dynamic equivalence unverified',fullAcceptancePassed:false};
fs.copyFileSync(process.argv[1],path.join(dir,'properties-auditor.mjs'));fs.writeFileSync(path.join(dir,'properties-proof.json'),JSON.stringify(proof,null,2)+'\n');console.log(JSON.stringify({passed:true,checks:run.checks,playback:proofs.map(x=>({pitch:x.pitch,noteCount:x.noteCount}))}));

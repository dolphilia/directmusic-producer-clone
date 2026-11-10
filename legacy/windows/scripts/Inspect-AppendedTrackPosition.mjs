import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const read=p=>fs.readFileSync(p),hash=b=>crypto.createHash('sha256').update(b).digest('hex'),json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,''));
const proofPath=path.resolve(process.argv[2]),proof=json(proofPath);assert(proof.passed&&proof.wholeSnapshotExact&&proof.runtimeOrderMatchesRiff);
assert.equal(proof.inputProducer.buildSummarySha256,proof.buildSha256);const core=json(proof.inputProducer.run);assert.equal(hash(read(proof.inputProducer.run)),proof.inputProducer.sha256);assert(core.cases.find(c=>c.name==='core').passed);
const build=json(proof.build),repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');assert.equal(hash(read(proof.build)),proof.buildSha256);
for(const s of build.sources){assert.equal(hash(read(path.join(build.sourceRoot,s.path))),s.sha256);assert.equal(hash(read(path.join(repo,s.path))),s.sha256);}
const bytes=read(proof.input.path);assert.equal(hash(bytes),proof.input.sha256);
const positions=[];function walk(start,end){for(let p=start;p<end;){const size=bytes.readUInt32LE(p+4),stop=p+8+size,next=stop+(size&1);assert(next<=end);const id=bytes.toString('ascii',p,p+4);if(id==='RIFF'||id==='LIST')walk(p+12,stop);else if(id==='trkh'){assert(size>=32);positions.push({type:bytes.subarray(p+8,p+24).toString('hex'),position:bytes.readUInt32LE(p+24),group:bytes.readUInt32LE(p+28)});}p=next;}}
walk(0,bytes.length);assert.deepEqual(positions.map(h=>h.position),[0,1,2,3,4,5,6]);assert.deepEqual(positions.map(h=>h.group),[1,1,1,2,2,4,4]);
for(const b of proof.batches)assert.deepEqual(b.actual,b.riff);
const modulesPath=path.resolve(path.dirname(proofPath),'..','group-playback-module-provenance.json'),modules=json(modulesPath);assert(modules.passed);assert.equal(modules.runSha256,proof.runSha256);assert(modules.modules.every(m=>m.originalHashMatches.length===0));
const report={schema:1,passed:true,createdUtc:new Date().toISOString(),runtimeProof:proofPath,runtimeProofSha256:hash(read(proofPath)),build:proof.build,buildSha256:proof.buildSha256,executableSha256:proof.executableSha256,input:proof.input,positions,queries:proof.queries,parameterCount:proof.parameterCount,modules:modulesPath,modulesSha256:hash(read(modulesPath)),moduleCount:modules.moduleCount,coreRun:proof.inputProducer,coreStdoutSha256:hash(read(path.join(path.dirname(proof.inputProducer.run),'core.stdout.txt'))),auditorSha256:hash(read(fileURLToPath(import.meta.url))),scope:'Same-build native new-track positions and runtime group order; UI, Band event identity, audio and original Producer unverified',fullAcceptance:false};
const out=path.join(path.dirname(proofPath),'appended-position-proof.json');fs.writeFileSync(out,JSON.stringify(report,null,2)+'\n');fs.copyFileSync(fileURLToPath(import.meta.url),path.join(path.dirname(proofPath),'appended-position-auditor.mjs'));console.log(JSON.stringify({passed:true,proof:out,positions,moduleCount:modules.moduleCount}));

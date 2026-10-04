import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [firstDir,reloadDir,sourcePath]=process.argv.slice(2).map(x=>path.resolve(x));
const read=p=>fs.readFileSync(p),hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const json=p=>JSON.parse(read(p).toString('utf8').replace(/^\uFEFF/,''));
function parse(b,start=0,end=b.length){const a=[];for(let o=start;o<end;){assert(o+8<=end);const id=b.toString('ascii',o,o+4),size=b.readUInt32LE(o+4),e=o+8+size;assert(e<=end);const container=id==='RIFF'||id==='LIST';assert(!container||size>=4);a.push({id,type:container?b.toString('ascii',o+8,o+12):null,data:container?null:Buffer.from(b.subarray(o+8,e)),children:container?parse(b,o+12,e):[],pad:size&1?b[e]:0});o=e+(size&1);assert(o<=end);}return a;}
function encode(c){const b=c.type?Buffer.concat([Buffer.from(c.type),...c.children.map(encode)]):c.data;const h=Buffer.alloc(8);h.write(c.id);h.writeUInt32LE(b.length,4);return Buffer.concat([h,b,...(b.length&1?[Buffer.from([c.pad])]:[])]);}
const clone=c=>parse(encode(c))[0],leaf=(a,id)=>{const v=a.children.filter(c=>c.id===id);assert.equal(v.length,1);return v[0];};
const list=(a,type)=>{const v=a.children.filter(c=>c.type===type);assert.equal(v.length,1);return v[0];};
const patterns=t=>t.children.filter(c=>c.type==='pttn'),parts=t=>t.children.filter(c=>c.type==='part');
const guid=p=>leaf(p,'prth').data.subarray(132,148),binding=p=>leaf(list(p,'pref'),'prfc').data;


const first=json(path.join(firstDir,'launch.json')),reload=json(path.join(reloadDir,'launch.json')),build=json(first.buildSummary);
assert(first.passed&&reload.passed&&first.exitCode===0&&reload.exitCode===0);assert.notEqual(first.processId,reload.processId);
assert.equal(hash(first.buildSummary),first.buildSummarySha256);assert.equal(first.buildSummarySha256,reload.buildSummarySha256);assert.equal(first.exeSha256,reload.exeSha256);
assert(build.passed&&build.sourceSnapshotUnchanged);for(const s of build.sources)assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);
assert.equal(hash(first.executable),first.exeSha256);assert.equal(first.exeSha256,build.outputs.find(o=>o.path==='install/bin/Producer.exe').sha256);
const firstStyle=first.inputs.find(x=>path.basename(x.path)==='Heartlnd.stp'),reloadedStyle=reload.inputs.find(x=>path.basename(x.path)==='Heartlnd.stp');
assert.equal(firstStyle.path,reloadedStyle.path);assert.equal(firstStyle.sha256,hash(sourcePath));assert.equal(reloadedStyle.sha256,hash(firstStyle.path));
const root=parse(read(sourcePath))[0],expected=clone(root);for(const [index,name,kind]of [[1,'GUI Intro 64',4],[3,'GUI Fill 72',1]]){const pt=patterns(expected)[index];leaf(list(pt,'UNFO'),'UNAM').data=Buffer.from(name+'\0','utf16le');leaf(pt,'ptnh').data.writeUInt16LE(kind,6);}
assert(read(firstStyle.path).equals(encode(expected)));for(const input of first.inputs.filter(x=>x!==firstStyle)){assert.equal(hash(input.path),input.sha256);assert.equal(reload.inputs.find(x=>x.path===input.path).sha256,input.sha256);}
const captures=[];for(const dir of [firstDir,reloadDir]){const states=json(path.join(dir,'states.json')),modules=json(path.join(dir,'gui-module-provenance.json')),inventory=json(path.join(dir,'gui-modules.json')),launch=json(path.join(dir,'launch.json'));
assert.equal(inventory.processId,launch.processId);assert(modules.passed&&modules.exeSha256===first.exeSha256&&modules.modules.every(m=>m.originalHashMatches.length===0));
assert.equal(path.resolve(states.executable),path.resolve(first.executable));assert.equal(hash(states.build),first.buildSummarySha256);
for(const record of states.records)for(const sc of record.screenshots){assert.equal(hash(path.join(dir,sc.file)),sc.sha256);captures.push({path:path.join(dir,sc.file),sha256:sc.sha256});}}
const a=json(path.join(firstDir,'states.json')),b=json(path.join(reloadDir,'states.json'));
const tree=(states,label)=>{const r=states.records.find(x=>x.label===label);assert(r);return r.accessibility.tree;};
assert(tree(a,'properties-undo').includes('Pattern name / type Value: Fill64'));assert(tree(a,'properties-undo').includes('Value: Fill'));
assert(tree(a,'properties-redo').includes('Pattern name / type Value: GUI Intro 64'));assert(tree(a,'properties-redo').includes('Value: Intro'));
assert(tree(b,'restored-intro').includes('Pattern name / type Value: GUI Intro 64'));assert(tree(b,'restored-intro').includes('Value: Intro'));
assert(tree(b,'restored-fill').includes('Pattern name / type Value: GUI Fill 72'));assert(tree(b,'restored-fill').includes('Value: Fill'));
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,build:first.buildSummary,buildSha256:first.buildSummarySha256,exeSha256:first.exeSha256,sourceStyle:{path:sourcePath,sha256:hash(sourcePath)},savedStyle:{path:firstStyle.path,sha256:hash(firstStyle.path)},resaveExact:true,normalExitCodes:[first.exitCode,reload.exitCode],processIds:[first.processId,reload.processId],launches:[firstDir,reloadDir].map(d=>({path:path.join(d,'launch.json'),sha256:hash(path.join(d,'launch.json'))})),states:[firstDir,reloadDir].map(d=>({path:path.join(d,'states.json'),sha256:hash(path.join(d,'states.json'))})),moduleProofs:[firstDir,reloadDir].map(d=>({path:path.join(d,'gui-module-provenance.json'),sha256:hash(path.join(d,'gui-module-provenance.json')),moduleCount:json(path.join(d,'gui-module-provenance.json')).moduleCount})),captures,auditorSha256:hash(process.argv[1]),scope:'Same-build GUI combined metadata edit/UndoRedo/save/separate PID restore/exact resave/normal exit0; GUI playback/audio/full acceptance unverified',fullAcceptancePassed:false};
fs.copyFileSync(process.argv[1],path.join(firstDir,'properties-gui-auditor.mjs'));fs.writeFileSync(path.join(firstDir,'properties-gui-proof.json'),JSON.stringify(proof,null,2)+'\n');console.log(JSON.stringify({passed:true,images:captures.length,pids:proof.processIds,styleSha256:proof.savedStyle.sha256}));

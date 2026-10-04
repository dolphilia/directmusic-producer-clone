import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const self=fileURLToPath(import.meta.url),repo=path.resolve(path.dirname(self),'..'),dir=path.resolve(process.argv[2]);
const read=p=>fs.readFileSync(p),hash=b=>crypto.createHash('sha256').update(b).digest('hex'),json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,''));
const statePath=path.join(dir,'states.json'),state=json(statePath),build=json(state.buildSummary);
assert(build.passed&&build.sourceSnapshotUnchanged);assert.equal(hash(read(state.exe)),state.exeSha256);
for(const s of build.sources){assert.equal(hash(read(path.join(build.sourceRoot,s.path))),s.sha256);assert.equal(hash(read(path.join(repo,s.path))),s.sha256);}
for(const record of state.records){assert.equal(record.window.app.toLowerCase(),('process:'+state.exe.replaceAll('/','\\')).toLowerCase());for(const shot of record.screenshots)assert.equal(hash(read(path.join(dir,shot.path))),shot.sha256);}
const record=label=>{const r=state.records.find(r=>r.label===label);assert(r,label);return r;};
assert(record('command-editor-opened').accessibility.tree.includes('Saved. 2 commands.'));
assert(record('groove-applied-settled').accessibility.tree.includes('Modified. 2 commands.'));
assert(record('command-saved').accessibility.tree.includes('Saved. 2 commands.'));
// Undo/Redo screen captures are retained for visual review; a lagging tree is
// not converted into byte-level evidence for the in-memory history checkpoint.
record('command-undo');record('command-redo');
function chunks(b,start=0,end=b.length){const out=[];for(let p=start;p<end;){assert(p+8<=end);const n=b.readUInt32LE(p+4),stop=p+8+n,next=stop+(n&1);assert(next<=end);const id=b.toString('ascii',p,p+4),container=id==='RIFF'||id==='LIST';if(container)assert(n>=4);out.push({id,type:container?b.toString('ascii',p+8,p+12):'',offset:p,children:container?chunks(b,p+12,stop):[],data:b.subarray(p+8,stop)});p=next;}return out;}
const owned=read(path.join(dir,'owned.sgp')),edited=read(path.join(dir,'edited.sgp'));
const root=chunks(owned)[0];assert.equal(root.type,'DMSG');const tracks=root.children.filter(c=>c.id==='LIST'&&c.type==='trkl');assert.equal(tracks.length,1);
const commands=tracks[0].children.flatMap(c=>c.children.filter(d=>d.id==='cmnd'));assert.equal(commands.length,1);const c=commands[0];assert.equal(c.data.readUInt32LE(0),12);assert.equal(c.data.length,28);assert.equal(c.data[12],50);
const expected=Buffer.from(owned);expected[c.offset+8+12]=65;assert(edited.equals(expected),'Only the selected first Command Groove byte may change');
const proof={schema:1,passed:true,fullAcceptance:false,createdUtc:new Date().toISOString(),buildSummary:state.buildSummary,buildSummarySha256:hash(read(state.buildSummary)),exeSha256:state.exeSha256,sourceCount:build.sources.length,statesSha256:hash(read(statePath)),captureCount:state.records.length,ownedSha256:hash(owned),editedSha256:hash(edited),changedByteOffset:c.offset+20,grooveBefore:50,grooveAfter:65,auditorSha256:hash(read(self)),scope:'Current main Command entry, standard selected event Groove edit/dirty and SaveAs, exact single-byte saved patch; Undo/Redo screenshots retained. GUI group/index/move/add/delete/reload/normal exit/audio/Style Pattern/original comparison/all40/all8 unfinished.'};
fs.copyFileSync(self,path.join(dir,'Inspect-CommandEditorGui.mjs'));fs.writeFileSync(path.join(dir,'command-editor-gui-progress-proof.json'),JSON.stringify(proof,null,2)+'\n');console.log(JSON.stringify(proof));

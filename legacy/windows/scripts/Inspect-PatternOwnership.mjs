import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [runPath,notesPath,guiDirectory,reloadDirectory]=process.argv.slice(2);
const read=p=>fs.readFileSync(p),hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const json=p=>JSON.parse(read(p).toString('utf8').replace(/^\uFEFF/,''));
const run=json(runPath),dir=path.dirname(path.resolve(runPath)),build=json(run.buildSummary);
assert(run.passed&&run.exitCode===0&&!run.launchError&&!run.timedOut&&run.checks===36);
assert.equal(hash(run.buildSummary),run.buildSummarySha256);assert.equal(hash(path.join(dir,'driver.ps1')),run.driverSha256);
assert(build.passed&&build.sourceSnapshotUnchanged);for(const s of build.sources)assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);
assert.equal(hash(run.executable),run.executableSha256);assert.equal(run.executableSha256,build.outputs.find(o=>o.path==='build/Release/producer_core_tests.exe').sha256);
for(const input of run.inputs)assert.equal(hash(input.path),input.sha256);
assert.equal(hash(path.join(dir,'stdout.txt')),run.stdoutSha256);assert(json(path.join(dir,'stdout.txt')).passed);
for(const f of run.files)assert.equal(hash(path.join(dir,'core/pattern-ownership',f.name)),f.sha256);
function parse(b,start=0,end=b.length){const a=[];for(let o=start;o<end;){assert(o+8<=end);const id=b.toString('ascii',o,o+4),size=b.readUInt32LE(o+4),e=o+8+size;assert(e<=end);const container=id==='RIFF'||id==='LIST';assert(!container||size>=4);a.push({id,type:container?b.toString('ascii',o+8,o+12):null,data:container?null:Buffer.from(b.subarray(o+8,e)),children:container?parse(b,o+12,e):[],pad:size&1?b[e]:0});o=e+(size&1);assert(o<=end);}return a;}
function encode(c){const b=c.type?Buffer.concat([Buffer.from(c.type),...c.children.map(encode)]):c.data;const h=Buffer.alloc(8);h.write(c.id);h.writeUInt32LE(b.length,4);return Buffer.concat([h,b,...(b.length&1?[Buffer.from([c.pad])]:[])]);}
const clone=c=>parse(encode(c))[0],leaf=(a,id)=>{const v=a.children.filter(c=>c.id===id);assert.equal(v.length,1);return v[0];};
const list=(a,type)=>{const v=a.children.filter(c=>c.type===type);assert.equal(v.length,1);return v[0];};
const patterns=t=>t.children.filter(c=>c.type==='pttn'),parts=t=>t.children.filter(c=>c.type==='part');
const guid=p=>leaf(p,'prth').data.subarray(132,148),binding=p=>leaf(list(p,'pref'),'prfc').data;
const source=read(run.inputs[1].path),root=parse(source)[0],base=path.join(dir,'core/pattern-ownership');
assert(read(path.join(base,'original.stp')).equals(source));assert(read(path.join(base,'selection.sgp')).equals(read(run.inputs[0].path)));
const sourcePattern=patterns(root)[0],sourcePart=parts(root).find(p=>guid(p).equals(binding(sourcePattern).subarray(0,16)));assert(sourcePart);
function duplicate(name){const t=clone(root),pt=clone(sourcePattern);leaf(list(pt,'UNFO'),'UNAM').data=Buffer.from(name+'\0','utf16le');t.children.push(pt);return t;}
assert(read(path.join(base,'duplicate.stp')).equals(encode(duplicate('Low Copy'))));
function expectedIndependent(bytes,name,noteCount,grooves){const actual=parse(bytes)[0];assert.equal(patterns(actual).length,3);assert.equal(parts(actual).length,3);const pt=patterns(actual)[2],id=binding(pt).subarray(0,16);assert(!id.every(x=>x===0)&&parts(root).every(p=>!guid(p).equals(id)));const part=parts(actual).find(p=>guid(p).equals(id));assert(part);assert(actual.children.indexOf(part)<actual.children.indexOf(pt));
  const expected=duplicate(name),copy=patterns(expected)[2],newPart=clone(sourcePart);Buffer.from(id).copy(leaf(newPart,'prth').data,132);Buffer.from(id).copy(binding(copy),0);const notes=leaf(newPart,'note').data;assert.equal(notes.readUInt32LE(0),24);for(let i=0;i<noteCount;i++)notes.writeUInt16LE(72,4+i*24+14);
  if(grooves){leaf(patterns(expected)[0],'ptnh').data[5]=24;leaf(copy,'ptnh').data[4]=25;}
  expected.children.splice(expected.children.indexOf(copy),0,newPart);assert(bytes.equals(encode(expected)));return{id:id.toString('hex'),partPosition:actual.children.indexOf(part),patternPosition:actual.children.indexOf(pt)};
}
const unshared=expectedIndependent(read(path.join(base,'unshared.stp')),'Low Copy',0,false);
const finalPath=path.join(base,'Heartlnd.stp'),final=read(finalPath),edited=expectedIndependent(final,'Low Copy',4,true);
assert(read(path.join(base,'resaved.stp')).equals(final));
const noteProofPath=path.join(path.dirname(notesPath),'notes-proof.json'),np=json(noteProofPath),nr=json(notesPath);
assert(np.passed&&np.noteCount===12&&np.originalHashMatches===0&&nr.cases[0].passed);assert.equal(np.runSha256,hash(notesPath));assert.equal(np.exeSha256,build.outputs.find(o=>o.path==='install/bin/Producer.exe').sha256);assert.equal(np.dependencies[0].sha256,hash(finalPath));assert.equal(np.dependencies[0].path,path.resolve(finalPath));assert(np.actual.map(n=>n.midiValue).every((v,i)=>v===(i<4||i>=8?72:84)));
let gui=null;if(guiDirectory){const launch=json(path.join(guiDirectory,'launch.json'));assert(launch.passed&&launch.exitCode===0);assert.equal(launch.exeSha256,np.exeSha256);const styleInput=launch.inputs.find(i=>i.path.endsWith('Heartlnd.stp'));assert.equal(styleInput.sha256,hash(run.inputs[1].path));const bytes=read(styleInput.path);gui={...expectedIndependent(bytes,'Low60 Copy',1,false),path:styleInput.path,sha256:hash(styleInput.path),launchSha256:hash(path.join(guiDirectory,'launch.json')),scope:'GUI saved copy/unshare and first-note edit only; no GUI playback claim'};
  const images=[];for(const d of [guiDirectory,...(reloadDirectory?[reloadDirectory]:[])]){const states=json(path.join(d,'states.json')),modules=json(path.join(d,'gui-module-provenance.json'));assert(modules.passed&&modules.exeSha256===np.exeSha256&&modules.modules.every(m=>m.originalHashMatches.length===0));for(const capture of states.records)for(const sc of capture.screenshots){assert.equal(hash(path.join(d,sc.file)),sc.sha256);images.push({path:path.join(d,sc.file),sha256:sc.sha256});}}gui.images=images;
  if(reloadDirectory){const reload=json(path.join(reloadDirectory,'launch.json'));assert(reload.passed&&reload.exitCode===0&&reload.processId!==launch.processId);assert.equal(reload.exeSha256,np.exeSha256);assert.equal(reload.inputs.find(i=>i.path===styleInput.path).sha256,gui.sha256);const states=json(path.join(reloadDirectory,'states.json'));const restored=states.records.find(c=>c.label==='reloaded-independent-part');assert(restored.accessibility.tree.includes('Pattern 3: Low60 Copy')&&restored.accessibility.tree.includes('Value: Part 3:')&&restored.accessibility.tree.includes('Music value 72'));gui.reload={launchSha256:hash(path.join(reloadDirectory,'launch.json')),statesSha256:hash(path.join(reloadDirectory,'states.json')),firstPid:launch.processId,secondPid:reload.processId,resaveExact:true};gui.scope='GUI copy/unshare/UndoRedo/first-note edit/save/separate launch restore and exact re-save; both normal exit0; no GUI playback/audio claim';}
}
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,build:run.buildSummary,buildSha256:run.buildSummarySha256,coreExeSha256:run.executableSha256,producerExeSha256:np.exeSha256,checks:run.checks,runSha256:hash(runPath),noteProofSha256:hash(noteProofPath),sourceStyle:run.inputs[1],unshared,edited,gui,auditorSha256:hash(process.argv[1]),scope:'Exact source Pattern copy and selected reference Part clone, all opaque bytes retained; same-build Framework save/reload and generated notes; full acceptance not proven',fullAcceptancePassed:false};
fs.copyFileSync(process.argv[1],path.join(dir,'ownership-auditor.mjs'));fs.writeFileSync(path.join(dir,'ownership-proof.json'),JSON.stringify(proof,null,2)+'\n');console.log(JSON.stringify({passed:true,checks:run.checks,notes:np.noteCount,guiSaved:!!gui}));

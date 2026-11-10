import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [unit,buildPath,relatedPath,nativePath,driverPath,authorDir,reloadDir,audioDir]=process.argv.slice(2);
assert(audioDir,'Usage: Inspect-SourceToolUnit UNIT BUILD RELATED NATIVE DRIVER AUTHOR RELOAD AUDIO');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const bytes=p=>fs.readFileSync(p), hash=p=>crypto.createHash('sha256').update(bytes(p)).digest('hex');
const same=(a,b)=>assert(bytes(a).equals(bytes(b)),a+' differs from '+b);
const relative=p=>path.relative(process.cwd(),p).replace(/\\/g,'/');
const candidate=path.basename(path.dirname(buildPath)),build=read(buildPath),summaryHash=hash(buildPath);
assert(build.passed&&build.sourceSnapshotUnchanged);
for(const s of build.sources){assert.equal(hash(s.path),s.sha256);assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);}
for(const o of build.outputs)assert.equal(hash(path.join(path.dirname(buildPath),o.path)),o.sha256);
const related=read(relatedPath),native=read(nativePath),drivers=read(driverPath);
for(const run of [related,native,drivers]){assert.equal(run.candidate,candidate);assert.equal(run.buildSummarySha256,summaryHash);}
const expected={'timeline-range':93,'tool-graph-runtime':27,'param-control-runtime':15,'param-control':53};
for(const [id,count]of Object.entries(expected)){const r=related.results.find(r=>r.id===id);assert(r&&r.status==='合格'&&r.exitCode===0&&r.checks===count);}
const counts=run=>run.results.reduce((a,r)=>(a[r.status]=(a[r.status]||0)+1,a),{});
assert.equal(native.results.length,77);assert.equal(drivers.results.length,107);
assert.deepEqual(counts(native),{'障害あり':30,'合格':47});assert.deepEqual(counts(drivers),{'未実行':66,'障害あり':21,'合格':20});
assert.equal(native.results.find(r=>r.id==='core').processId,null);
const author=read(authorDir+'/launch.json'),reload=read(reloadDir+'/launch.json'),audioRun=read(audioDir+'/run.json');
for(const launch of [author,reload]){assert.equal(launch.buildSummarySha256,summaryHash);assert(launch.passed&&launch.exitCode===0&&!launch.timedOut);assert.equal(hash(launch.executable),launch.exeSha256);}
assert.notEqual(author.processId,reload.processId);assert.equal(audioRun.processId,reload.processId);assert.equal(audioRun.buildSummarySha256,summaryHash);assert.equal(audioRun.exeSha256,reload.exeSha256);
const input=read(unit+'/gui-inputs-before.json');assert.equal(input.candidate,candidate);
for(const f of input.sourceFiles)assert.equal(hash(path.join(input.source,f.name)),f.sha256);
for(const f of author.inputs)assert.equal(f.sha256,input.files.find(i=>path.basename(i.path)===path.basename(f.path)).sha256);
const authorSaved=unit+'/author-saved/SourceToolProject',finalSaved=unit+'/reload-final/SourceToolProject';
for(const f of reload.inputs)assert.equal(hash(path.join(authorSaved,path.basename(f.path))),f.sha256);
function riff(b,a=0,z=b.length){const out=[];for(let p=a;p<z;){assert(p+8<=z);const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),e=p+8+n;assert(e+(n&1)<=z);const container=['RIFF','LIST','seqt'].includes(id);out.push({id,type:container&&id!=='seqt'?b.toString('ascii',p+8,p+12):'',start:p,data:b.subarray(p+8,e),raw:b.subarray(p,e+(n&1)),children:container?riff(b,p+8+(id==='seqt'?0:4),e):[]});p=e+(n&1);}return out;}
const flat=cs=>cs.flatMap(c=>[c,...flat(c.children)]), root=p=>{const r=riff(bytes(p));assert.equal(r.length,1);return r[0];};
const graph=root(authorSaved+'/Tools.tgp'),oldGraph=root(input.source+'/Tools.tgp');assert.equal(graph.type,'DMTG');
const tools=g=>g.children.find(c=>c.type==='toll').children;
assert.equal(tools(oldGraph).length,2);assert.equal(tools(graph).length,3);
for(let i=0;i<2;i++)assert(tools(graph)[i].raw.equals(tools(oldGraph)[i].raw));
const gainData=tools(graph)[2].children.find(c=>c.id==='data').data;assert.equal(gainData.length,8);assert.equal(gainData.readUInt32LE(0),1);assert(Math.abs(gainData.readFloatLE(4)-.8)<1e-6);
const aup=root(authorSaved+'/Tools.aup');assert.equal(aup.type,'DMAP');assert(aup.children.find(c=>c.type==='DMTG').raw.equals(graph.raw));
const segment=root(authorSaved+'/Authoring.sgp'),oldSegment=root(input.source+'/Authoring.sgp');assert.equal(segment.type,'DMSG');assert(segment.children.find(c=>c.type==='DMAP').raw.equals(aup.raw));
for(const c of oldSegment.children.filter(c=>c.type!=='DMAP'&&c.type!=='trkl'))assert(segment.children.find(x=>x.id===c.id&&x.type===c.type).raw.equals(c.raw));
const oldTracks=oldSegment.children.find(c=>c.type==='trkl').children,newTracks=segment.children.find(c=>c.type==='trkl').children;
assert.equal(newTracks.length,oldTracks.length+1);for(let i=0;i<oldTracks.length;i++)assert(newTracks[i].raw.equals(oldTracks[i].raw));
const curves=flat([newTracks.at(-1)]).find(c=>c.id==='prcc');assert(curves&&curves.data.length===52);
same(authorSaved+'/Automation.sgp',input.source+'/Automation.sgp');assert.equal(hash(authorSaved+'/Control.sgp'),input.files.find(f=>path.basename(f.path)==='Control.sgp').sha256);
const saved=[];for(const n of fs.readdirSync(authorSaved)){if(!n.endsWith('.pro'))same(authorSaved+'/'+n,finalSaved+'/'+n);same(finalSaved+'/'+n,unit+'/SourceToolProject/'+n);saved.push({name:n,authorSha256:hash(authorSaved+'/'+n),reloadSha256:hash(finalSaved+'/'+n),documentBytesEqual:!n.endsWith('.pro')});}
function catalog(p){const b=bytes(p),r=riff(b)[0];assert.equal(r.type,'JAZP');const normalized=Buffer.from(b),files=[];for(const f of r.children.filter(c=>c.type==='file')){const n=f.children.find(c=>c.id==='name').data.toString('utf16le').replace(/\0+$/,''),h=f.children.find(c=>c.id==='filh');assert(h&&h.data.length>=44);normalized.fill(0,h.start+8+16,h.start+8+24);const doc=root(path.join(path.dirname(p),n)),guid=doc.children.find(c=>c.id==='guid');assert.equal(h.data.readUInt32LE(24),bytes(path.join(path.dirname(p),n)).length);assert(guid&&guid.data.equals(h.data.subarray(28,44)));files.push({name:n,bytes:h.data.readUInt32LE(24),guid:h.data.subarray(28,44).toString('hex'),writeTime:h.data.subarray(16,24).toString('hex'),format:doc.type});}return {files,normalized};}
const ca=catalog(authorSaved+'/SourceToolProject.pro'),cr=catalog(finalSaved+'/SourceToolProject.pro');assert.equal(ca.files.length,5);assert(ca.normalized.equals(cr.normalized),'Project differs outside filh document write times');
const actions=read(unit+'/gui-author-actions.json');assert.equal(actions.processId,author.processId);assert.equal(actions.actions.length,12);
const observationNames=['gui-008-graph-gain-redo.json','gui-040-parameter-redo-settled.json','gui-057-reload-parameter-curves-first.json','gui-059-reload-second-curve-settled.json','gui-060-reload-three-discovered-tools.json','gui-086-reload-project-final-save-settled.json'];
assert(read(unit+'/'+observationNames[0]).accessibility.tree.includes('0.800000'));assert(read(unit+'/'+observationNames[1]).accessibility.tree.includes('3072..6144: 0.750000'));assert(read(unit+'/'+observationNames[2]).accessibility.tree.includes('0.250000'));assert(read(unit+'/'+observationNames[3]).accessibility.tree.includes('3072..6144: 0.750000'));
const audioProof=read(audioDir+'/source-tool-gui-audio-proof.json'),controls=read(unit+'/audio-audit-controls/proof.json');assert(audioProof.passed&&controls.passed);assert.equal(audioProof.candidate,candidate);assert.equal(controls.positiveProofSha256,hash(audioDir+'/source-tool-gui-audio-proof.json'));assert.equal(controls.auditorSha256,hash('scripts/Inspect-SourceToolGuiAudio.mjs'));assert.equal(controls.results.length,6);
for(const c of controls.results){assert(c.rejected&&c.exitCode===1);const d=unit+'/audio-audit-controls/'+c.name;if(c.name==='packet-gap'){assert(fs.readFileSync(d+'/stderr.txt','utf8').includes('Packet frame/QPC origin mismatch'));continue;}const p=read(d+'/source-tool-gui-audio-proof.json');assert(!p.passed);if(c.name==='swapped-pitches')assert(!p.authored.pitchPassed&&p.authored.tempoPassed);if(c.name==='wrong-tempo-preserved-pitch')assert(p.authored.pitchPassed&&!p.authored.tempoPassed);if(c.name==='unchanged-gain')assert(p.control.pitchPassed&&p.authored.pitchPassed&&p.authored.tempoPassed&&!p.effectPassed);}
const start=read(unit+'/unit-start.json'),changed=start.sourceBefore.filter(s=>hash(s.path)!==s.sha256).map(s=>({...s,currentSha256:hash(s.path)}));for(const p of ['src/producer/source_tools.h','src/producer/source_tools.cpp','tests/producer/source_tool_tests.h'])changed.push({path:p,before:null,currentSha256:hash(p)});
fs.writeFileSync(unit+'/changed-sources.json',JSON.stringify({candidate,changes:changed},null,2)+'\n');
const proof={schema:1,createdUtc:new Date().toISOString(),candidate,passed:true,scope:'Source-owned velocity factory/capability authoring, main native save/distinct reload, music-time parameter effect; no all40/all8 or original dynamic parity',build:{path:relative(buildPath),sha256:summaryHash,savedSources:build.sources.length},related:{run:relative(relatedPath),checks:expected},native:{run:relative(nativePath),counts:counts(native)},drivers:{run:relative(driverPath),counts:counts(drivers)},gui:{authorRun:relative(authorDir),reloadRun:relative(reloadDir),authorProcessId:author.processId,reloadProcessId:reload.processId,normalExitCodes:[author.exitCode,reload.exitCode],actions:relative(unit+'/gui-author-actions.json'),observations:observationNames.map(n=>relative(unit+'/'+n))},project:{files:saved,catalog:cr.files,projectResaveDifference:'Only filh bytes16..23 document FILETIME refreshed after Segment resave; all other catalog bytes and all five owned documents equal'},audio:{directory:relative(audioDir),proofSha256:hash(audioDir+'/source-tool-gui-audio-proof.json'),auditorSha256:controls.auditorSha256,controls:relative(unit+'/audio-audit-controls/proof.json'),controlOnsets:audioProof.control.onsets.length,authoredOnsets:audioProof.authored.onsets.length,lowerMean:audioProof.lowerMean,higherMean:audioProof.higherMean,earlyStopRestart:false},evidence:[authorDir+'/launch.json',reloadDir+'/launch.json',audioDir+'/run.json',audioDir+'/source-tool-gui-audio-proof.json',unit+'/audio-audit-controls/proof.json',unit+'/gui-author-actions.json',unit+'/original-new-types.json'].map(p=>({path:relative(p),sha256:hash(p)})),auditorSha256:hash(process.argv[1]),fullAcceptance:false};
fs.writeFileSync(unit+'/unit-proof.json',JSON.stringify(proof,null,2)+'\n');fs.copyFileSync(process.argv[1],unit+'/unit-auditor.mjs');console.log(JSON.stringify({candidate,passed:true,native:proof.native.counts,drivers:proof.drivers.counts,ownedFiles:cr.files.length,changedSources:changed.length,audio:true,fullAcceptance:false}));

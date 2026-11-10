import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const dir=path.resolve(process.argv[2]),self=fileURLToPath(import.meta.url),repo=path.resolve(path.dirname(self),'..');
const read=p=>fs.readFileSync(p),hash=b=>crypto.createHash('sha256').update(b).digest('hex'),json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,''));
const state=json(path.join(dir,'states.json')),build=json(state.build),run=json(state.run);
assert(build.passed);assert.equal(run.buildSummarySha256,hash(read(state.build)));
const host=run.cases.find(c=>c.name==='host-smoke'),core=run.cases.find(c=>c.name==='core');
assert(host.passed&&core.passed);assert.equal(host.exitCode,0);assert.equal(core.exitCode,0);
assert.equal(hash(read(state.executable)),host.sha256);assert.equal(state.executableSha256,host.sha256);
assert(build.outputs.some(o=>path.resolve(path.dirname(state.build),o.path)===path.resolve(state.executable)&&o.sha256===host.sha256));
for(const s of build.sources){assert.equal(hash(read(path.join(build.sourceRoot,s.path))),s.sha256);assert.equal(hash(read(path.join(repo,s.path))),s.sha256);}
function chunks(b,start=0,end=b.length){const out=[];for(let p=start;p<end;){assert(p+8<=end);const n=b.readUInt32LE(p+4),stop=p+8+n,next=stop+(n&1);assert(next<=end);const id=b.toString('ascii',p,p+4),container=id==='RIFF'||id==='LIST';if(container)assert(n>=4);out.push({id,type:container?b.toString('ascii',p+8,p+12):'',data:b.subarray(p+8,stop),raw:b.subarray(p,next),children:container?chunks(b,p+12,stop):[]});p=next;}return out;}
const child=(c,id,type)=>{const hits=c.children.filter(x=>x.id===id&&(type===undefined||x.type===type));assert.equal(hits.length,1,`${id}/${type}`);return hits[0];};
const root=(b,type)=>{const r=chunks(b);assert.equal(r.length,1);assert.equal(r[0].id,'RIFF');assert.equal(r[0].type,type);return r[0];};
const source=read(path.join(dir,'source.sgp')),resaved=read(path.join(dir,'resaved.sgp')),band=read(path.join(dir,'piano.bnp'));
assert(source.equals(resaved),'whole Segment resave');
const bins=child(child(child(root(band,'DMBD'),'LIST','lbil'),'LIST','lbin'),'bins').data;
assert.equal(bins.length,44);assert.equal(bins.readUInt32LE(0),0);assert.equal(bins.readUInt32LE(24),0);assert.equal(bins[32],64);assert.equal(bins[33],100);
const ts=child(root(source,'DMSG'),'LIST','trkl').children;assert.equal(ts.length,5);
const tempo='8528acd29bb3d111870400600893b1bd',sequence='8628acd29bb3d111870400600893b1bd',bandType='9428acd29bb3d111870400600893b1bd';
const tracks=ts.map((t,i)=>{assert.equal(t.type,'DMTK');const h=child(t,'trkh').data;assert.equal(h.length,32);const type=h.subarray(0,16).toString('hex'),group=h.readUInt32LE(20),position=h.readUInt32LE(16);assert.equal(position,i);
 if(type===tempo){const d=child(t,'tetr').data;assert.equal(d.length,20);assert.equal(d.readUInt32LE(0),16);assert.equal(d.readInt32LE(4),0);assert.equal(d.readDoubleLE(12),120);}
 if(type===sequence){const seq={children:chunks(child(t,'seqt').data)},d=child(seq,'evtl').data;assert.equal(d.length,24);assert.equal(d.readUInt32LE(0),20);assert.equal(d.readInt32LE(4),0);assert.equal(d.readInt32LE(8),384);assert.equal(d.readUInt32LE(12),0);assert.equal(d.readInt16LE(16),0);assert.deepEqual([...d.subarray(18,21)],[0x90,60,96]);}
 if(type===bandType){const event=child(child(child(t,'RIFF','DMBT'),'LIST','lbdl'),'LIST','lbnd');assert(child(event,'RIFF','DMBD').raw.equals(band));const stamp=child(event,'bd2h').data;assert.equal(stamp.length,8);assert.equal(stamp.readInt32LE(0),0);assert.equal(stamp.readInt32LE(4),0);}
 return {type,position,group};});
assert.deepEqual(tracks.map(t=>t.type),[tempo,sequence,bandType,sequence,bandType]);assert.deepEqual(tracks.map(t=>t.group),[1,1,1,2,2]);
for(const r of state.records)assert.equal(hash(read(path.join(dir,r.screenshot))),r.screenshotSha256);
const record=label=>{const r=state.records.find(r=>r.label===label);assert(r,label);return r;};
for(const [label,mask] of [['project-reloaded-clean',1],['reloaded-group2-confirmed',2]]){const t=record(label).accessibility.tree;assert.match(t,new RegExp(`Applied groups: ${mask};`));assert.match(t,/Notes: 1/);assert.match(t,/Value: piano.bnp ID: 803/);assert.match(t,/Value: 0 \/ 0 ID: 805/);}
assert.match(record('reloaded-play-confirmed').accessibility.tree,/Playing document snapshot\. Applied groups: 2;/);
assert.match(record('resave-menu').accessibility.tree,/Stopped\. Applied groups: 2;/);
assert.match(record('project-resaved').accessibility.tree,/Value: resaved.sgp ID: 207/);
const closes=['normal-close-first.json','normal-close-second.json'].map(name=>{const c=json(path.join(dir,name));assert(c.normalCloseObserved);assert(!c.windows.some(w=>w.id===c.window.id));assert.equal(c.exitCode,null);assert.equal(c.window.app.toLowerCase(),('process:'+state.executable).toLowerCase());return {name,sha256:hash(read(path.join(dir,name))),window:c.window.id,exitCode:null};});
assert.notEqual(closes[0].window,closes[1].window);assert.equal(record('separate-launch').window.id,closes[1].window);assert.equal(record('launch').window.id,closes[0].window);
function project(name,expected){const b=read(path.join(dir,name)),r=root(b,'DMPJ'),refs=r.children.filter(c=>c.id==='file').map(c=>{assert.equal(c.data.length%2,0);return c.data.toString('utf16le').replace(/\0+$/,'');});assert.deepEqual(refs,expected);assert.equal(child(r,'vers').data.readUInt32LE(),1);return {name,sha256:hash(b),bytes:b.length,references:refs.map(ref=>{const resolved=path.resolve(dir,ref),data=read(resolved);assert.equal(path.dirname(resolved),dir);return {ref,sha256:hash(data),bytes:data.length};}),metadata:r.children.filter(c=>c.id!=='file').map(c=>c.raw.toString('hex'))};}
const projects=[project('project.dmpj',['source.sgp','piano.bnp']),project('resaved-project.dmpj',['resaved.sgp','piano.bnp']),project('starter-project.dmpj',['starter.sgp'])];assert.deepEqual(projects[0].metadata,projects[1].metadata);
const modulePath=path.join(dir,'gui-module-provenance.json'),modules=json(modulePath);assert(modules.passed);assert.equal(modules.exeSha256,host.sha256);assert(modules.modules.every(m=>m.originalHashMatches.length===0));
const proof={schema:1,passed:true,createdUtc:new Date().toISOString(),build:state.build,buildSha256:hash(read(state.build)),run:state.run,runSha256:hash(read(state.run)),exeSha256:host.sha256,sourceCount:build.sources.length,statesSha256:hash(read(path.join(dir,'states.json'))),records:state.records.length,files:['source.sgp','resaved.sgp','piano.bnp','starter.sgp'].map(name=>{const b=read(path.join(dir,name));return {name,sha256:hash(b),bytes:b.length,exact:name==='resaved.sgp'||name==='source.sgp'};}),tracks,projects,closes,modulePath,moduleSha256:hash(read(modulePath)),moduleCount:modules.moduleCount,auditorSha256:hash(read(self)),scope:'GUI append two group Sequence/Band pairs, project save, close, separate launch, project reload, selection reapply, Play/Stop UI, exact Segment resave and project reference update. Source/exe/run identity and screenshots checked. Module inventory one point after Play. Unsaved startup document/project both saved before target project open. UIA lag uses subsequent confirmed records. Exit code unmeasured; actual audio and original comparison unverified.',fullAcceptance:false};
fs.writeFileSync(path.join(dir,'appended-position-gui-proof.json'),JSON.stringify(proof,null,2)+'\n');fs.copyFileSync(self,path.join(dir,'appended-position-gui-auditor.mjs'));console.log(JSON.stringify({passed:true,tracks,records:proof.records,moduleCount:modules.moduleCount}));

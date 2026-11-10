import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const read=p=>fs.readFileSync(p),json=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,''));
const runPath=path.resolve(process.argv[2]),run=json(runPath),build=json(run.buildSummary);
const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
assert.equal(hash(read(run.buildSummary)),run.buildSummarySha256);assert.equal(build.passed,true);
assert(run.cases.every(c=>c.passed));for(const c of run.cases)assert.equal(hash(read(c.executable)),c.sha256);
for(const s of build.sources){assert.equal(hash(read(path.join(build.sourceRoot,s.path))),s.sha256);assert.equal(hash(read(path.join(repo,s.path))),s.sha256);}
const dir=path.join(path.dirname(runPath),'core/sequence-band-groups');
function parse(b,start=0,end=b.length){const out=[];for(let p=start;p<end;){assert(p+8<=end);const n=b.readUInt32LE(p+4),stop=p+8+n,next=stop+(n&1);assert(next<=end);const id=b.toString('ascii',p,p+4),container=id==='RIFF'||id==='LIST';if(container)assert(n>=4);out.push({id,type:container?b.toString('ascii',p+8,p+12):'',data:container?null:Buffer.from(b.subarray(p+8,stop)),children:container?parse(b,p+12,stop):[],padding:n&1?b[stop]:0});p=next;}return out;}
function encode(c){const b=c.data??Buffer.concat([Buffer.from(c.type),...c.children.map(encode)]),h=Buffer.alloc(8);h.write(c.id);h.writeUInt32LE(b.length,4);return Buffer.concat([h,b,...(b.length&1?[Buffer.from([c.padding])]:[])]);}
function load(name){const bytes=read(path.join(dir,name)),roots=parse(bytes);assert.equal(roots.length,1);assert(encode(roots[0]).equals(bytes));return {bytes,root:roots[0]};}
const find=(c,id,type)=>c.children.find(x=>x.id===id&&(type===undefined||x.type===type));
const before=load('before.sgp'),notes=load('note-edited.sgp'),moved=load('band-moved.sgp'),created=load('created.sgp'),saved=load('saved.sgp'),project=load('project.dmpj');
const tracks=find(before.root,'LIST','trkl').children;
assert.deepEqual(tracks.map(t=>find(t,'trkh').data.readUInt32LE(20)),[1,1,1,2,2,6,6]);
for(let i=1;i<7;++i){assert.equal(find(tracks[i],'trkh').data.length,36);assert.deepEqual(find(tracks[i],'zzzz').data,Buffer.from([7,8,9]));assert.equal(find(tracks[i],'zzzz').padding,0xa3);}
// Patch the raw selected evtl record, without calling the C++ inserter.
const sequence=find(tracks[5],'seqt'),parts=parse(sequence.data),evtl=parts.find(c=>c.id==='evtl');
assert.equal(evtl.data.length,24);assert.equal(evtl.data.readUInt32LE(0),20);assert.equal(evtl.data[4+15],64);
const note=Buffer.alloc(20);note.writeInt32LE(192,0);note.writeInt32LE(48,4);note.writeUInt32LE(0,8);note[14]=0x90;note[15]=67;note[16]=99;
evtl.data=Buffer.concat([evtl.data,note]);sequence.data=Buffer.concat(parts.map(encode));assert(encode(before.root).equals(notes.bytes),'only selected Sequence insertion');
const band=find(tracks[6],'RIFF','DMBT'),items=find(band,'LIST','lbdl'),item=find(items,'LIST','lbnd'),stamp=find(item,'bd2h');assert.equal(stamp.data.readInt32LE(0),96);assert.equal(stamp.data.readInt32LE(4),96);stamp.data.writeInt32LE(384,0);stamp.data.writeInt32LE(360,4);assert(encode(before.root).equals(moved.bytes),'only selected Band timestamps');
const createdTracks=find(created.root,'LIST','trkl').children;assert.equal(createdTracks.length,9);for(let i=0;i<7;++i)assert(encode(tracks[i]).equals(encode(createdTracks[i])));
for(let i=7;i<9;++i)assert.equal(find(createdTracks[i],'trkh').data.readUInt32LE(20),8);
const newSeq=parse(find(createdTracks[7],'seqt').data).find(c=>c.id==='evtl').data;assert.equal(newSeq.length,24);assert.equal(newSeq.readUInt32LE(0),20);assert.equal(newSeq.readInt32LE(4),0);assert.equal(newSeq.readInt32LE(8),48);assert.equal(newSeq[19],70);assert.equal(newSeq[20],96);
const newBand=find(find(find(createdTracks[8],'RIFF','DMBT'),'LIST','lbdl'),'LIST','lbnd');const newStamp=find(newBand,'bd2h');assert.equal(newStamp.data.readInt32LE(0),0);assert.equal(newStamp.data.readInt32LE(4),0);assert(encode(find(newBand,'RIFF','DMBD')).equals(encode(find(item,'RIFF','DMBD'))));
assert(saved.bytes.equals(created.bytes));assert.equal(project.root.type,'DMPJ');assert.deepEqual(project.root.children.filter(c=>c.id==='file').map(c=>c.data.toString('utf16le').replace(/\0+$/,'')),['saved.sgp']);
const core=json(path.join(path.dirname(runPath),'core.stdout.txt'));assert.equal(core.passed,true);
const names=['before.sgp','note-edited.sgp','band-moved.sgp','created.sgp','saved.sgp','project.dmpj'];
const proof={schema:1,passed:true,createdUtc:new Date().toISOString(),run:runPath,runSha256:hash(read(runPath)),build:run.buildSummary,buildSha256:run.buildSummarySha256,sourceCount:build.sources.length,workspaceSourcesMatched:true,coreChecks:core.checks,files:Object.fromEntries(names.map(n=>[n,{sha256:hash(read(path.join(dir,n))),bytes:read(path.join(dir,n)).length}])),wholeBytesNoteInsertion:true,wholeBytesBandMove:true,preexistingTracksPreservedOnCreation:true,newTrackMask:8,savedWholeBytesMatch:true,projectReferences:['saved.sgp'],sdkTrackHeader:{path:'work/analysis/sources/dmusicf.h',sha256:hash(read(path.join(repo,'work/analysis/sources/dmusicf.h')))},auditorSha256:hash(read(fileURLToPath(import.meta.url))),scope:'Native Sequence/Band group/index, atomic edits/history, complete saved bytes and separate Framework reload. GUI indices, runtime group selection, original dynamic comparison and whole acceptance pending.',fullAcceptance:false};
fs.writeFileSync(path.join(dir,'sequence-band-group-proof.json'),JSON.stringify(proof,null,2)+'\n');fs.copyFileSync(fileURLToPath(import.meta.url),path.join(dir,'group-auditor.mjs'));console.log(JSON.stringify({passed:true,checks:core.checks,proof:path.join(dir,'sequence-band-group-proof.json')}));

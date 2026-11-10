// Independent byte audit of the exact owned/runtime snapshots emitted by Producer.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const runPath=path.resolve(process.argv[2]??'');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const read=p=>fs.readFileSync(p);
const json=p=>JSON.parse(read(p).toString('utf8').replace(/^\uFEFF/,''));
const requireFact=(v,message)=>{if(!v)throw Error(message);};
const run=json(runPath),base=path.join(path.dirname(runPath),'style-playback');
const native=run.cases.find(c=>c.name==='style-playback-api');
requireFact(native?.passed&&sha(read(native.executable))===native.sha256,'Exact successful executable required');
requireFact(sha(read(run.buildSummary))===run.buildSummarySha256,'Build record changed');
const build=json(run.buildSummary);
requireFact(build.sources.every(s=>sha(read(path.join(build.sourceRoot,s.path)))===s.sha256),'Frozen sources changed');
const workspace=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
requireFact(build.sources.every(s=>sha(read(path.join(workspace,s.path)))===s.sha256),'Workspace differs from tested snapshot');
const report=json(path.join(base,'playback.json'));
requireFact(report.passed&&report.editedRelocatedFilenameAndMissingGuidPassed,'Native mapping case failed');
const files={};
function riff(name){
  const file=path.join(base,name),b=read(file),leaves=new Map();
  requireFact(b.toString('ascii',0,4)==='RIFF'&&b.readUInt32LE(4)+8===b.length,'Root size mismatch');
  function walk(start,end,parent,depth=0){
    requireFact(depth<64,'Depth limit');const counts=new Map();
    for(let at=start;at<end;){
      requireFact(end-at>=8,'Truncated chunk');const id=b.toString('ascii',at,at+4),n=b.readUInt32LE(at+4),data=at+8,stop=data+n;
      requireFact(stop+(n&1)<=end,'Chunk bounds');const container=id==='RIFF'||id==='LIST';requireFact(!container||n>=4,'Container type missing');
      const label=container?id+':'+b.toString('ascii',data,data+4):id,index=counts.get(label)??0;counts.set(label,index+1);
      const location=parent+label+'['+index+']';
      if(container)walk(data+4,stop,location+'/',depth+1);else leaves.set(location,b.subarray(data,stop));at=stop+(n&1);
    }
  }
  walk(0,b.length,'');files[name]={file,sha256:sha(b),bytes:b.length,leaves:leaves.size};return {b,leaves};
}
function compare(a,b,allowed){
  let same=0;const changes=[];
  for(const key of new Set([...a.leaves.keys(),...b.leaves.keys()])){
    const l=a.leaves.get(key),r=b.leaves.get(key);
    if(l&&r&&l.equals(r)){same++;continue;}
    requireFact(allowed(key,l,r),'Unexpected mutation '+key);changes.push({location:key,before:l?.toString('hex')??null,after:r?.toString('hex')??null});
  }
  return {sameLeafChunks:same,changes};
}
const refs=r=>[...r.leaves].filter(([k])=>k.endsWith('/LIST:DMRF[0]/refh[0]'));
function mapping(source,runtime,style,filenameOnly){
  const before=refs(source),after=refs(runtime);requireFact(before.length===1&&after.length===1,'One reference required');
  const [key,h]=before[0],[newKey,n]=after[0];requireFact(key===newKey&&h.length===20&&n.length===20,'Reference layout mismatch');
  requireFact(h.subarray(0,16).equals(n.subarray(0,16))&&n.readUInt32LE(16)===((h.readUInt32LE(16)|1)&~48),'Runtime reference flags/class changed incorrectly');
  if(filenameOnly)requireFact(!(h.readUInt32LE(16)&1)&&(h.readUInt32LE(16)&16),'Expected filename-only source');
  const guidKey=key.replace('/refh[0]','/guid[0]'),guid=runtime.leaves.get(guidKey),styleGuid=style.leaves.get('RIFF:DMST[0]/guid[0]');
  requireFact(guid?.length===16&&styleGuid?.length===16&&guid.equals(styleGuid),'Runtime Segment/Style identities differ');
  return compare(source,runtime,(k,l,r)=>k===key&&l.equals(h)&&r.equals(n)||k===guidKey&&r?.equals(guid)&&(!l||l.equals(guid)));
}
const input=riff('input.sgp'),runtime=riff('runtime-input.sgp'),original=riff('style-0.stp'),runtimeStyle=riff('runtime-style-0.stp');
requireFact(sha(input.b)===run.referenceInput.sha256&&original.b.equals(runtimeStyle.b),'Original copy identity mismatch');
const originalSource=run.referenceStyles.files.find(s=>s.sha256===sha(original.b));requireFact(originalSource&&sha(read(originalSource.path))===originalSource.sha256,'Original Style changed');
const filename=riff('filename-source.sgp'),filenameRuntime=riff('filename-runtime.sgp'),owned=riff('filename-owned-style.stp'),ownedRuntime=riff('filename-runtime-style.stp');
requireFact(owned.b.equals(ownedRuntime.b),'Edited owned Style changed on playback');
const missing=riff('missing-guid-source.stp'),generatedStyle=riff('generated-runtime-style.stp'),generatedSegment=riff('generated-runtime.sgp');
requireFact(!missing.leaves.has('RIFF:DMST[0]/guid[0]'),'Missing GUID fixture has GUID');
const generatedDiff=compare(missing,generatedStyle,(k,l,r)=>k==='RIFF:DMST[0]/guid[0]'&&!l&&r?.length===16);
const noteEdit=json(path.join(base,'edited-note.json'));requireFact(Number.isSafeInteger(noteEdit.partIndex)&&noteEdit.partIndex>=0&&noteEdit.noteIndex===0&&noteEdit.duration===576&&noteEdit.velocity===88,'Note edit fixture identity');
const editedDiff=compare(original,owned,(k,l,r)=>{
  if(k==='RIFF:DMST[0]/styh[0]')return l.length===12&&r.length===12&&r[0]===3&&r[1]===4&&r.readUInt16LE(2)===2&&r.readDoubleLE(4)===132;
  if(k==='RIFF:DMST[0]/LIST:pttn[0]/ptnh[0]')return l.length===r.length&&r[0]===3&&r[1]===4&&r.readUInt16LE(2)===2&&r[4]===5&&r[5]===50&&r.readUInt16LE(8)===2&&l.subarray(6,8).equals(r.subarray(6,8))&&l.subarray(10).equals(r.subarray(10));
  if(k==='RIFF:DMST[0]/LIST:pttn[0]/rhtm[0]')return l.length===4&&r.length===8&&r.subarray(0,4).equals(l)&&r.readUInt32LE(4)===0;
  if(k===`RIFF:DMST[0]/LIST:part[${noteEdit.partIndex}]/note[0]`){if(l.length!==r.length||l.length<28)return false;const expected=Buffer.from(l);expected.writeInt32LE(noteEdit.duration,12);expected[20]=noteEdit.velocity;return expected.equals(r);}return false;
});
requireFact(fs.readdirSync(path.join(base,'runtime-empty')).length===0,'Search directory not empty');
const projectFiles=[];
for(const location of ['owned','moved']){
  const directory=path.join(base,'filename-case',location);
  for(const entry of fs.readdirSync(directory,{recursive:true,withFileTypes:true}))if(entry.isFile()){
    const file=path.join(entry.parentPath,entry.name),bytes=read(file);projectFiles.push({file,sha256:sha(bytes),bytes:bytes.length});
    if(entry.name==='filename.sgp')requireFact(bytes.equals(filename.b),'Saved/relocated Segment changed');
    if(/\.stp$/i.test(entry.name))requireFact(bytes.equals(owned.b),'Saved/relocated Style changed');
  }
  const project=read(path.join(directory,'project.dmpj'));requireFact(project.toString('ascii',8,12)==='DMPJ','Saved project type');
}
const proof={schema:1,createdUtc:new Date().toISOString(),run:runPath,runSha256:sha(read(runPath)),buildSummarySha256:run.buildSummarySha256,executableSha256:native.sha256,auditorSha256:sha(read(new URL(import.meta.url))),sourceCount:build.sources.length,files,
  originalMapping:mapping(input,runtime,runtimeStyle,false),filenameMapping:mapping(filename,filenameRuntime,ownedRuntime,true),generatedMapping:mapping(filename,generatedSegment,generatedStyle,true),generatedStyleDiff:generatedDiff,editedStyleDiff:editedDiff,
  noteEdit,noteEditSha256:sha(read(path.join(base,'edited-note.json'))),projectFiles,workspaceSourcesMatched:true,originalStyleUnchanged:true,editedStyleRuntimeBytesIdentical:true,searchDirectoryEmpty:true,nativeApiPassed:true,scope:'Exact current native API and raw RIFF identity proof; no audible acceptance, general file-open trace or GUI acceptance. Module inventory captured during initial GUID case, before filename/missing-GUID cases.'};
fs.writeFileSync(path.join(base,'mapping-proof.json'),JSON.stringify(proof,null,2)+'\n');
fs.copyFileSync(new URL(import.meta.url),path.join(base,'mapping-auditor.mjs'));
process.stdout.write(JSON.stringify({proof:path.join(base,'mapping-proof.json'),passed:true})+'\n');

import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const classes={
  '8228acd29bb3d111870400600893b1bd':'DMSG',
  '8a28acd29bb3d111870400600893b1bd':'DMST',
  'b0f40f48b228d111bef700c04fbf8fef':'DLS ',
  '5471668acbf9d211ad8a0060b0575abc':'WAVE'
};
const expectedAliases=['BGDawn','BGNight','BGPredawn','SfxAlarm','SfxCougar','SfxCow','SfxRooster','SfxSheep','SfxWolf','SSBird'];
function parse(b,a=0,z=b.length){const out=[];for(let p=a;p<z;){assert(p+8<=z,'Truncated chunk header');const id=b.toString('ascii',p,p+4),n=b.readUInt32LE(p+4),e=p+8+n;assert(e+(n&1)<=z,'Truncated chunk payload');const c={id,start:p,data:b.subarray(p+8,e)};if(['RIFF','LIST','seqt'].includes(id)){const skip=id==='seqt'?0:4;assert(n>=skip);if(skip)c.type=b.toString('ascii',p+8,p+12);c.children=parse(b,p+8+skip,e);}out.push(c);p=e+(n&1);}return out;}
const one=(c,id,type)=>{const matches=(c.children??[]).filter(c=>c.id===id&&(!type||c.type===type));assert.equal(matches.length,1,'Unique '+id+(type??''));return matches[0];};
const flat=c=>[c,...(c.children??[]).flatMap(flat)];
const text=c=>{assert(c.data.length%2===0&&c.data.length>=2);const s=c.data.toString('utf16le');assert(s.endsWith('\0')&&!s.slice(0,-1).includes('\0'),'Terminated text without embedded NUL');return s.slice(0,-1);};
export function inspectFarm(files){
  const opened=new Map(),edges=[],tracks=new Set();
  function load(name,expectedClass,expectedGuid){
    assert(files.has(name),'Missing explicit input '+name);const bytes=files.get(name),roots=parse(bytes);assert.equal(roots.length,1);const root=roots[0];assert.equal(root.id,'RIFF');assert.equal(root.type,classes[expectedClass]??'DMSC','Class/form');const guid=one(root,root.type==='DLS '?'dlid':'guid').data.toString('hex');assert.equal(guid.length,32,'GUID bytes');
    if(opened.has(name)){assert.equal(opened.get(name).sha256,sha(bytes));return root;}
    opened.set(name,{name,bytes:bytes.length,sha256:sha(bytes),form:root.type,guid});
    for(const t of flat(root).filter(c=>c.id==='trkh')){assert(t.data.length>=32);tracks.add(t.data.subarray(0,16).toString('hex'));}
    for(const ref of flat(root).filter(c=>c.id==='LIST'&&c.type==='DMRF')){
      const h=one(ref,'refh');assert(h.data.length>=20);const cls=h.data.subarray(0,16).toString('hex'),flags=h.data.readUInt32LE(16);assert(classes[cls],'Declared native class');assert(flags&16,'Explicit file');assert(flags&1,'Farm reference GUID');assert(!(flags&(64|1024|2048)),'No URL/memory/stream source reference');
      const filename=text(one(ref,'file'));assert.equal(path.basename(filename),filename,'Farm dependencies are local filenames');const targetGuid=one(ref,'guid').data.toString('hex');assert.equal(targetGuid.length,32);
      const targetRoot=load(filename,cls,targetGuid),actualGuid=one(targetRoot,targetRoot.type==='DLS '?'dlid':'guid').data.toString('hex');edges.push({owner:name,target:filename,classId:cls,guid:targetGuid,actualGuid,guidMatches:targetGuid===actualGuid,flags});
    }return root;
  }
  const script=load('FarmMusic.spt');assert.equal(script.type,'DMSC');const container=one(script,'RIFF','DMCN');assert.equal(one(container,'conh').data.readUInt32LE(),2,'Farm NOLOADS');
  const aliases=one(container,'LIST','cosl').children.filter(c=>c.type==='cobl').map(c=>{const h=one(c,'cobh');assert(h.data.length>=28);assert.equal(h.data.readUInt32LE(16),0,'Farm KEEP is initially off');assert.equal(h.data.toString('ascii',20,28),'LISTDMRF');return text(one(c,'coba'));});
  assert.deepEqual([...aliases].sort(),[...expectedAliases].sort());
  assert.equal(opened.size,19,'One Script, ten Segments, Style, DLS, six Waves');
  const forms=[...opened.values()].reduce((v,c)=>(v[c.form]=(v[c.form]??0)+1,v),{});assert.deepEqual(forms,{'DMSC':1,'DMSG':10,'DMST':1,'DLS ':1,'WAVE':6});
  const fallback=edges.filter(e=>!e.guidMatches);assert.equal(fallback.length,10,'Original Farm has exactly ten stale Segment reference GUIDs');assert(fallback.every(e=>e.owner==='FarmMusic.spt'&&e.classId==='8228acd29bb3d111870400600893b1bd'),'Unmatched GUIDs confined to observed top-level input');assert(fallback.every(e=>![...opened.values()].some(o=>o.form==='DMSG'&&o.guid===e.guid)),'No owned GUID target for the ten filename selections');
  return {files:[...opened.values()],edges,tracks:[...tracks].sort(),aliases,forms,observedFilenameSelections:fallback,allReferenceGuidsMatch:false,
    basis:'Independent native byte grammar; observed mismatches retained. Official Loader contract permits explicit filename when requested GUID has no owned match; C++/COM/GUI/audio/original execution remains unverified',fullAcceptance:false};
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
  const [directory,output]=process.argv.slice(2);assert(directory&&output,'Usage: node Inspect-FarmInputGraph.mjs <data-only directory> <fresh output>');assert(!fs.existsSync(output),'Preserve previous proof');
  const files=new Map(fs.readdirSync(directory).filter(n=>/\.(spt|sgt|sty|dls|wav)$/i.test(n)).map(n=>[n,fs.readFileSync(path.join(directory,n))]));
  const result={schema:1,createdUtc:new Date().toISOString(),processId:process.pid,auditor:{path:path.resolve(process.argv[1]),sha256:sha(fs.readFileSync(process.argv[1]))},inputDirectory:path.resolve(directory),...inspectFarm(files)};
  fs.writeFileSync(output,JSON.stringify(result,null,2)+'\n',{flag:'wx'});console.log(JSON.stringify({auditPassed:true,inputFiles:result.files.length,edges:result.edges.length,productExecuted:false,fullAcceptance:false}));
}

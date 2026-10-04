import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [runPath,mode]=process.argv.slice(2);assert.equal(mode,'embellishment');
const read=p=>fs.readFileSync(p),hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const json=p=>JSON.parse(read(p).toString('utf8').replace(/^\uFEFF/,''));
const run=json(runPath),dir=path.dirname(path.resolve(runPath)),build=json(run.buildSummary),c=run.cases.find(c=>c.name==='notes-api');
assert(c.passed&&c.exitCode===0&&!c.launchError&&!c.timedOut);
assert.equal(hash(run.buildSummary),run.buildSummarySha256);assert.equal(hash(run.runtimeDriverCopy),run.runtimeDriverSha256);
assert(build.passed&&build.sourceSnapshotUnchanged);for(const s of build.sources)assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);
assert.equal(hash(c.executable),c.sha256);assert.equal(c.sha256,build.outputs.find(o=>o.path==='install/bin/Producer.exe').sha256);
assert.equal(hash(run.groupPlaybackInput.path),run.groupPlaybackInput.sha256);
for(const input of run.dependencyInputs??[])assert.equal(hash(input.path),input.sha256);
const reportPath=path.join(dir,'notes','notes.json'),r=json(reportPath);assert(r.passed&&r.started&&r.ended&&!r.overflow&&!r.error);assert.equal(r.forwardingFailed,false);assert(r.calls.every(c=>c.hresult<0x80000000));
for(const operation of ['CoCreate note graph','Insert note observer','Set note graph','PlaySegmentEx','StopEx','CloseDown'])assert(r.calls.some(c=>c.operation===operation&&c.hresult===0));
const modulePath=path.join(dir,'notes-module-provenance.json'),modules=json(modulePath);assert(modules.passed);assert.equal(modules.runSha256,hash(runPath));assert.equal(modules.smokeSha256,hash(reportPath));assert(modules.modules.every(m=>m.originalHashMatches.length===0));
function chunks(b,start=0,end=b.length){let a=[];for(let o=start;o<end;){assert(o+8<=end);const id=b.toString('ascii',o,o+4),size=b.readUInt32LE(o+4),e=o+8+size;assert(e<=end);const container=id==='RIFF'||id==='LIST';assert(!container||size>=4);a.push({id,type:container?b.toString('ascii',o+8,o+12):null,data:b.subarray(o+8,e),children:container?chunks(b,o+12,e):[]});o=e+(size&1);assert(o<=end);}return a;}
const one=(a,fn)=>{let matches=a.filter(fn);assert.equal(matches.length,1);return matches[0];};
const inputPath=path.join(dir,'notes','input.sgp');assert(read(inputPath).equals(read(run.groupPlaybackInput.path)));
const song=one(chunks(read(inputPath)),c=>c.type==='DMSG'),tracks=one(song.children,c=>c.type==='trkl').children;
let expected=[],patterns=[];
if(mode==='sequence'){
  const payload=one(tracks.flatMap(c=>c.children),c=>c.id==='seqt').data;
  const events=one(chunks(payload),c=>c.id==='evtl').data,stride=events.readUInt32LE(0);assert.equal(stride,20);assert.equal((events.length-4)%stride,0);
  for(let o=4;o<events.length;o+=stride)if((events[o+14]&0xf0)===0x90&&events[o+16])expected.push({clocks:events.readInt32LE(o)+events.readInt16LE(o+12),duration:events.readInt32LE(o+4),channel:events.readUInt32LE(o+8),group:1,musicValue:events[o+15],midiValue:events[o+15],velocity:events[o+16],flags:1,playMode:0});
}else{
  assert.equal(run.dependencyInputs.length,1);
  const sourcePath=path.join(dir,'notes','source-style-0.stp'),runtimePath=path.join(dir,'notes','runtime-style-0.stp');
  assert(read(sourcePath).equals(read(runtimePath)));assert(read(sourcePath).equals(read(run.dependencyInputs[0].path)));
  const style=one(chunks(read(sourcePath)),c=>c.type==='DMST'),parts=style.children.filter(c=>c.type==='part');
  const list=style.children.filter(c=>c.type==='pttn');assert.equal(list.length,5);
  for(const pt of list){const h=one(pt.children,c=>c.id==='ptnh').data;assert([0,1,2,4,8].includes(h.readUInt16LE(6)));assert.equal(h.readUInt16LE(8),1);assert.equal(h[0],4);assert.equal(h[1],4);assert.equal(h.readUInt16LE(2),4);
    const ref=one(pt.children,c=>c.type==='pref'),rf=one(ref.children,c=>c.id==='prfc').data;
    const part=one(parts,p=>one(p.children,c=>c.id==='prth').data.subarray(132,148).equals(rf.subarray(0,16)));
    const nh=one(part.children,c=>c.id==='prth').data;assert.equal(nh.readUInt16LE(148),1);assert.equal(nh[150],0);
    const data=one(part.children,c=>c.id==='note').data,stride=data.readUInt32LE(0);assert.equal(stride,24);const notes=[];
    for(let o=4;o<data.length;o+=stride){assert.equal(data[o+21],0);assert.equal(data.readUInt32LE(o+4),0xffffffff);assert(data.subarray(o+17,o+20).every(x=>x===0));notes.push({clocks:data.readInt32LE(o)*192+data.readInt16LE(o+12),duration:data.readInt32LE(o+8),channel:rf.readUInt32LE(24),group:1,musicValue:data.readUInt16LE(o+14),midiValue:data.readUInt16LE(o+14),velocity:data[o+16],flags:1,playMode:0});}
    patterns.push({embellishment:h.readUInt16LE(6),bottom:h[4],top:h[5],notes});
  }
  const commands=one(tracks.flatMap(c=>c.children),c=>c.id==='cmnd').data;assert.equal(commands.readUInt32LE(0),12);
  const length=one(song.children,c=>c.id==='segh').data.readInt32LE(4);
  for(let o=4;o<commands.length;o+=12){const time=commands.readInt32LE(o),groove=commands[o+8];const commandType=commands[o+7],embellishment=[0,1,4,2,8][commandType];assert.notEqual(embellishment,undefined);assert.equal(commands[o+9],0);const pt=one(patterns,p=>p.embellishment===embellishment&&p.bottom<=groove&&groove<=p.top);const end=o+12<commands.length?commands.readInt32LE(o+12):length;assert.equal(end-time,3072);for(const n of pt.notes)expected.push({...n,clocks:time+n.clocks});}
  const actualCommands=r.noteEvents.filter(e=>e.command&&e.currentSegment).map(e=>e.clocks-r.start);
  assert.deepEqual(actualCommands,[0,3072,6144]);
}
const actual=r.notes.map(n=>({...n,clocks:n.clocks-r.start}));assert.deepEqual(actual,expected);
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,mode,build:run.buildSummary,buildSha256:run.buildSummarySha256,exeSha256:c.sha256,input:run.groupPlaybackInput,dependencies:run.dependencyInputs??[],noteCount:actual.length,expected,actual,patterns,moduleCount:modules.moduleCount,originalHashMatches:0,runSha256:hash(runPath),reportSha256:hash(reportPath),moduleProofSha256:hash(modulePath),auditorSha256:hash(process.argv[1]),scope:'Exact generated note messages from this runtime path; fixed embellishment fixture attribution only, no audio/GUI/original Producer equivalence or full acceptance',fullAcceptancePassed:false};
fs.copyFileSync(process.argv[1],path.join(dir,'notes-auditor.mjs'));fs.writeFileSync(path.join(dir,'notes-proof.json'),JSON.stringify(proof,null,2)+'\n');console.log(JSON.stringify({passed:true,mode,noteCount:actual.length,moduleCount:modules.moduleCount}));

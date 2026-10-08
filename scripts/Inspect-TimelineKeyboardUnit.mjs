import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [unitArg, captureArg] = process.argv.slice(2);
assert(captureArg, 'Usage: UNIT CAPTURE');
const unit = path.resolve(unitArg), capture = path.resolve(captureArg);
const read = p => JSON.parse(fs.readFileSync(p, 'utf8').replace(/^\uFEFF/, ''));
const bytes = p => fs.readFileSync(p);
const hash = p => crypto.createHash('sha256').update(bytes(p)).digest('hex');
const rel = p => path.relative(process.cwd(), p).replaceAll('\\', '/');
const evidence = p => ({path: rel(p), sha256: hash(p)});
const adoption = read(unit + '/candidate-adoption.json'), start = read(unit + '/unit-start.json');
const buildPath = path.resolve(adoption.build), build = read(buildPath);
assert.equal(hash(buildPath), adoption.buildSha256);
assert.equal(path.basename(path.dirname(buildPath)), adoption.candidate);
assert(build.passed && build.sourceSnapshotUnchanged);
assert.deepEqual([build.configureExitCode, build.buildExitCode, build.installExitCode], [0, 0, 0]);
for (const s of build.sources) for (const root of [build.sourceRoot, process.cwd()]) assert.equal(hash(path.join(root, s.path)), s.sha256);
for (const o of build.outputs) assert.equal(hash(path.join(path.dirname(buildPath), o.path)), o.sha256);
for (const c of start.contracts) assert.equal(hash(c.path), c.sha256);
const exe = build.outputs.find(o => o.path === 'install/bin/Producer.exe'); assert(exe);

// Raw RIFF traversal, including seqt's headerless child list. The expected
// result copies the seed records selected by [0,768), with only music time
// translated to3072. No product parser/serializer or output-derived oracle.
function tree(b, a = 0, z = b.length) {
  const cs = [];
  for (let p = a; p < z;) {
    assert(p + 8 <= z); const id = b.toString('ascii', p, p + 4), n = b.readUInt32LE(p + 4), end = p + 8 + n;
    assert(end + (n & 1) <= z);
    const c = {id, start:p, data:b.subarray(p + 8, end), raw:b.subarray(p, end + (n & 1)), pad:n & 1 ? b[end] : 0};
    if (['RIFF', 'LIST', 'seqt'].includes(id)) {
      const prefix = id === 'seqt' ? 0 : 4; assert(n >= prefix);
      if (prefix) c.type = b.toString('ascii', p + 8, p + 12);
      c.children = tree(b, p + 8 + prefix, end);
    }
    cs.push(c); p = end + (n & 1);
  }
  return cs;
}
function encode(c) {
  if (!c.children && !c.changed) return c.raw;
  const data = c.children ? Buffer.concat([...(c.type ? [Buffer.from(c.type)] : []), ...c.children.map(encode)]) : c.data;
  const h = Buffer.alloc(8); h.write(c.id); h.writeUInt32LE(data.length, 4);
  return Buffer.concat([h, data, ...(data.length & 1 ? [Buffer.from([c.pad])] : [])]);
}
const one = (cs,id,type) => {const f = cs.filter(c => c.id === id && (!type || c.type === type)); assert.equal(f.length,1,`${id}/${type??''}`); return f[0];};
const flat = cs => cs.flatMap(c => [c,...flat(c.children??[])]);
const seed = read(unit + '/seed-inputs.json');
for (const i of seed.files) assert.equal(hash(i.source), i.sourceSha256);
const source = name => seed.files.find(i => path.basename(i.source) === name).source;
const authored = unit + '/author-saved', restored = unit + '/restored-saved';
const originalSong = bytes(source('LoopSong.sgp')), expectedSong = one(tree(originalSong), 'RIFF', 'DMSG');
const all = flat([expectedSong]);
for (const [id,stride] of [['evtl',20],['tetr',16]]) {
  const c = one(all,id); assert.equal(c.data.readUInt32LE(0),stride);
  assert.equal(c.data.length,id==='evtl'?44:20);
  assert.equal(c.data.readInt32LE(4),0);
  const copy = Buffer.from(c.data.subarray(4,4+stride)); copy.writeInt32LE(3072,0);
  c.data = Buffer.concat([c.data,copy]); c.changed = true;
}
const expectedBytes = encode(expectedSong); assert.equal(expectedBytes.length,originalSong.length+36);
assert(bytes(authored+'/LoopSong.sgp').equals(expectedBytes),'Only first selected note/Tempo copied at3072; all other chunks/order/padding/identities retained');
for (const n of ['LoopSong.sgp','LoopBand.bnp','LoopSource.dls']) {
  assert.equal(hash(authored+'/'+n),hash(restored+'/'+n));
  assert.equal(hash(restored+'/'+n),hash(unit+'/author/SampleInheritance/'+n));
  if(n!=='LoopSong.sgp') assert.equal(hash(authored+'/'+n),hash(source(n)));
}
function catalog(p, translateSeedSize=false) {
  const b=bytes(p),r=one(tree(b),'RIFF','JAZP'),normal=Buffer.from(b),entries=[];
  for(const f of r.children.filter(c=>c.id==='LIST'&&c.type==='file')) {
    const name=one(f.children,'name').data.toString('utf16le').replace(/\0+$/,''),h=one(f.children,'filh');
    assert.equal(path.basename(name),name); assert.equal(h.data.length,44);
    normal.fill(0,h.start+8+16,h.start+8+24); // SDK project document last-write FILETIME only.
    if(translateSeedSize&&name==='LoopSong.sgp') normal.writeUInt32LE(h.data.readUInt32LE(24)+36,h.start+8+24);
    if(!translateSeedSize) {
      const doc=bytes(path.join(path.dirname(p),name)),dr=one(tree(doc),'RIFF');
      assert.equal(h.data.readUInt32LE(24),doc.length);
      assert(one(dr.children,dr.type==='DLS '?'dlid':'guid').data.equals(h.data.subarray(28,44)));
    }
    entries.push({name,bytes:h.data.readUInt32LE(24),guid:h.data.subarray(28,44).toString('hex'),writeTime:h.data.subarray(16,24).toString('hex')});
  }
  assert.equal(entries.length,3); return {normal,entries};
}
const oldProject=catalog(source('SampleInheritance.pro'),true),authorProject=catalog(authored+'/SampleInheritance.pro'),reloadProject=catalog(restored+'/SampleInheritance.pro');
assert(oldProject.normal.equals(authorProject.normal),'Seed Project changes only Segment size and declared file write times');
assert(authorProject.normal.equals(reloadProject.normal),'Project resave changes only document last-write FILETIME');
assert.equal(hash(restored+'/SampleInheritance.pro'),hash(unit+'/author/SampleInheritance/SampleInheritance.pro'));

const normal = p => p.replaceAll('\\','/').toLowerCase();
const exits=['author-session','runtime-session'].map(d => {const p=unit+'/'+d+'/exit.json',e=read(p); assert.equal(e.exitCode,0); assert.equal(e.forcedTermination,false); assert.equal(e.exeSha256,exe.sha256); assert.equal(hash(e.executable),exe.sha256); assert.equal(e.driverSha256,hash('scripts/Watch-ProductGuiExit.ps1')); return {...evidence(p),...e};});
assert.notEqual(exits[0].processId,exits[1].processId); assert(Date.parse(exits[0].exitUtc)<Date.parse(exits[1].startUtc));
const actions=read(unit+'/author-session/actions.json'),states=read(unit+'/author-session/states.json');
for(const [name,key] of [['ctrl-c-after-target-input','Control_L+c'],['ctrl-v-after-copy','Control_L+v'],['ctrl-z-document','Control_L+z'],['ctrl-y-document','Control_L+y'],['ctrl-a-canvas','Control_L+a']]) {
 const a=actions.find(a=>a.name===name); assert(a,name); assert.equal(a.value,key); assert.equal(a.processId,exits[0].processId); assert.equal(normal(a.window.app),'process:'+normal(exits[0].executable));
}
for(const [label,match] of [
 ['ctrl-c-after-target-input',/Range \[0, 768\) clocks; Tempo 1; notes 2;.*saved/],
 ['merge-settled',/Range \[3072, 3840\) clocks; Tempo 2; notes 3;.*modified/],
 ['undo-settled',/Tempo 1; notes 2;.*saved/],
 ['edit-after-merge-focus',/Tempo 2; notes 3;.*modified/],
 ['select-all-settled',/Range \[0, 30720\) clocks; Tempo 2; notes 3;.*modified/],
 ['edit-ctrl-a-isolated',/Range \[0, 768\) clocks; Tempo 1; notes 2;.*saved/],
 ...['edit-ctrl-y-isolated','edit-ctrl-c-isolated','edit-ctrl-v-isolated'].map(n=>[n,/Tempo 2; notes 3;.*modified/])
]) assert(states.some(s=>s.label===label&&match.test(s.accessibility?.tree)),label);
const textUndo=states.find(s=>s.label==='edit-ctrl-y-isolated'); assert(/Target at Value: 0 ID: 12/.test(textUndo.accessibility.focused_element),'Edit CtrlZ changes text without document Undo');
const restoration=read(unit+'/runtime-session/states.json');
assert(restoration.some(s=>/3072 clocks 12 BPM/.test(s.accessibility?.tree)&&/Notes: 3/.test(s.accessibility?.tree)));
const roundPath=path.resolve(adoption.native.path),round=read(roundPath),driverPath=path.resolve(adoption.drivers.path),drivers=read(driverPath);
assert.equal(round.buildSummarySha256,hash(buildPath)); assert.equal(drivers.buildSummarySha256,hash(buildPath));
const related=['timeline-range','sequence-crud','dls-runtime-export'].map(id=>{const p=path.dirname(roundPath)+'/'+id+'/run.json',r=read(p);assert.equal(r.exitCode,0);assert.equal(r.timedOut,false);return {id,checks:r.checks,...evidence(p)};});
assert.deepEqual(related.map(r=>r.checks),[93,26,44]);
const audioPath=capture+'/timeline-keyboard-audio-proof.json',audio=read(audioPath),run=read(capture+'/run.json'),launch=read(capture+'/gui-launch-at-capture.json');
assert(audio.passed&&!audio.fullAcceptance);assert.equal(audio.candidate,adoption.candidate);assert.equal(audio.processId,exits[1].processId);assert.equal(audio.onsetCount,6);
for(const [p,h] of [[capture+'/run.json',audio.runSha256],[capture+'/output.wav',audio.wavSha256],[run.guiRun+'/actions.json',audio.actionsSha256],[launch.observationLog,audio.observationsSha256],[run.guiRun+'/exit.json',audio.exitSha256],['scripts/Inspect-TimelineKeyboardAudio.mjs',audio.analyzerSha256]]) assert.equal(hash(p),h);
const controlsPath=unit+'/audio-controls/negative-tests.json',controls=read(controlsPath);assert(controls.passed&&!controls.fullAcceptance);assert.equal(controls.results.length,19);assert.equal(controls.sourceProofSha256,hash(audioPath));assert.equal(controls.driverSha256,hash('scripts/Test-TimelineKeyboardAudioAuditor.mjs'));assert(controls.results.every(r=>r.kind==='unchanged'?r.exitCode===0:r.exitCode===1));
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,candidate:adoption.candidate,scope:'Bounded Timeline CtrlC/V merge/A/Z/Y, Edit focus isolation, native save/distinct process restoration and automatic three-note PCM. No original dynamic/all strips/OLE/ABI/all40/all8 claim.',build:evidence(buildPath),sourceCount:build.sources.length,exeSha256:exe.sha256,contracts:start.contracts,related,nativeRound:evidence(roundPath),driverRound:evidence(driverPath),processes:exits.map(e=>({processId:e.processId,startUtc:e.startUtc,exitUtc:e.exitUtc,exitCode:e.exitCode,path:e.path,sha256:e.sha256})),song:{...evidence(restored+'/LoopSong.sgp'),expectedBytes:expectedBytes.length,seedBytes:originalSong.length,exactExpectedTransformation:true},project:{entries:reloadProject.entries,onlyChanges:'Seed to author: copied Segment size+36 and last-write FILETIME; author to reload: only filh bytes16..23 last-write FILETIME',author:evidence(authored+'/SampleInheritance.pro'),restored:evidence(restored+'/SampleInheritance.pro')},gui:['author-session/actions.json','author-session/states.json','runtime-session/actions.json','runtime-session/states.json'].map(n=>evidence(unit+'/'+n)),audio:{proof:evidence(audioPath),controls:evidence(controlsPath),onsets:6,maxStoppedRms:audio.maxStoppedRms,intervals:audio.phases.map(p=>p.interval)},auditorSha256:hash(process.argv[1]),fullAcceptance:false};
fs.writeFileSync(unit+'/unit-proof.json',JSON.stringify(proof,null,2)+'\n');fs.copyFileSync(process.argv[1],unit+'/unit-auditor.mjs');console.log(JSON.stringify(proof));

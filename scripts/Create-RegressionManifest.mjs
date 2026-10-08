import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
const read=p=>fs.readFileSync(p,'utf8').replace(/^\uFEFF/,'');
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const source=read('tests/producer/core_tests.cpp');
const existingManifest=fs.existsSync('docs/analysis/regression-manifest.json')?JSON.parse(read('docs/analysis/regression-manifest.json')):null;
const segment='work/producer/samples/QuickStart/heartland.sgp';
const style='work/producer/samples/QuickStart/Heartlnd.stp';
const project='work/producer/samples/QuickStart/QuickStart.pro';
const ap='work/producer/samples/Tutorial/FinishedProject/APFarm.aup';
const dls='work/acceptance/normal-style-dls/20261004T025021534Z/core/owned.dls';
const observed='work/analysis/dls-instrument-original/20261003T231831Z/after-instrument.dlp';
// Reuse immutable INPUTS only, never old pass results or generated executables.
const previous=new Map();
function visit(dir) {for(const e of fs.readdirSync(dir,{withFileTypes:true})) {
  const p=path.join(dir,e.name);if(e.isDirectory()){if(e.name!=='regression')visit(p);continue;}
  if(e.name!=='run.json')continue;
  try {const r=JSON.parse(read(p));if(!r.passed)continue;
    for(const c of [r,...(r.cases??[])]) {const a=c.arguments;if(!Array.isArray(a))continue;
      const i=a.findIndex(v=>typeof v==='string'&&/^--/.test(v));if(i<0)continue;
      const inputs=a.slice(i+1).map(v=>String(v).replace(/^"|"$/g,''));
      if(!inputs.length||!inputs.every(v=>fs.existsSync(v)))continue;
      const stamp=r.createdUtc??p;if(!previous.has(a[i])||previous.get(a[i]).stamp<stamp)previous.set(a[i],{stamp,inputs,evidence:p});
    }
  }catch{ /* unrelated record */ }
}}
visit('work/acceptance');
const tests=[{id:'core',runner:'producer_core_tests',arguments:[],inputs:[],scope:'ordinary core; excludes dedicated dispatch modes',status:'未実行'}];
for(const m of source.matchAll(/if\((?:\(argc==4\|\|argc==5\)|\(argc==3\|\|argc==4\)|argc(==|>=)(\d+))&&std::wstring\(argv\[2\]\)==L"(--[^"]+)"\)/g)) {
  const flag=m[3], n=Number(m[2]??(flag==='--midi-import'?4:5))-3;
  let inputs=[];
  if(n===1) inputs=[flag.includes('audiopath')||flag.startsWith('--runtime-')?ap:flag==='--jazp-save'?project:dls];
  if(n===2) inputs=flag.startsWith('--dls-')?[dls,observed]:[segment,style];
  if(n>0&&previous.has(flag)&&previous.get(flag).inputs.length===n)inputs=previous.get(flag).inputs;
  if(flag==='--dls-add-wave')inputs=['work/analysis/dls-author-original/20261003T224300Z/Tone.wav','work/analysis/dls-author-original/20261003T224300Z/after-wave.dlp'];
  if(flag==='--dls-create-instrument')inputs=['work/analysis/dls-author-original/20261003T224300Z/Tone.wav','work/analysis/dls-instrument-original/20261003T231831Z/before.dlp'];
  if(flag==='--style-root-dls'||flag==='--motif-dls')inputs=['work/acceptance/runtime-motif-source/20261004T132200Z/Unassigned.dls'];
  if(flag==='--chordmap-document')inputs=['work/producer/style-library','work/analysis/q3-chordmap/20261004T191300Z/OriginalEmpty.cdp'];
  if(flag==='--chordmap-palette')inputs=['work/analysis/q3-chordmap/20261004T191300Z/OriginalEmpty.cdp'];
  if(flag==='--midi-import')inputs=['work/producer/samples/QuickStart/DemoMIDI.mid'];
  if(flag==='--mute-document')inputs=['work/analysis/q3-mute/20261004T222600Z'];
  if(flag==='--lyric-document')inputs=['work/analysis/q3-lyric/20261004T212600Z'];
  if(flag==='--marker-document')inputs=['work/analysis/q3-marker/20261004T204000Z'];
  if(flag==='--wave-document')inputs=['work/producer/samples/Tutorial/FinishedProject/SfxCow.wvp','work/producer/samples/Tutorial/FinishedProject/SfxCow.sgp'];
  if(flag==='--wave-track')inputs=['work/producer/samples/Tutorial/FinishedProject/SfxCow.sgp'];
  if(flag==='--style-player')inputs=['work/producer/samples/QuickStart/Heartlnd.stp','work/producer/style-library/BOOGIE.CDM'];
  if(flag==='--style-player-default')inputs=['work/producer/samples/QuickStart/Heartlnd.stp','work/producer/style-library/BOOGIE.CDM'];
  // Keep reviewed input bindings. Inferring a format from a flag's arity can
  // substitute an AudioPath for a Project or a DLS for a Wave.
  const registered=existingManifest?.tests?.find(t=>t.arguments?.[0]===flag);
  if(registered?.inputs?.length===inputs.length)inputs=registered.inputs;
  if(flag==='--script-document')inputs=['work/producer/samples/FarmGame/FarmMusic.spt','work/producer/samples/Tutorial/FinishedProject/FarmMusic.spp'];
  if(flag==='--script-dependencies'||flag==='--farm-script-runtime')inputs=['work/analysis/q3-farm-player/20261007T085530733Z/native-inputs'];
  const dependent=flag.endsWith('-verify');
  tests.push({id:flag.slice(2),runner:'producer_core_tests',arguments:[flag],inputs,originalExecutableRequired:false,
    originalDataRequired:flag==='--script-dependencies'||flag==='--farm-script-runtime'||inputs.some(p=>p.includes('/samples/')||p.includes('/style-library')||p===observed),scope:'dedicated native mode; GUI/audio/dynamic original comparison separate',
    workflow:dependent?'Requires fresh prepare, Producer recovery, then verify in the same directory':null,status:'未実行'});
  // Native Producer Project names must match their containing directory.
  if(flag==='--style-player-default')tests.at(-1).outputDirectoryName='Default';
  if(flag==='--script-dependencies'||flag==='--farm-script-runtime'){
    tests.at(-1).inputSetup={generator:'scripts/Create-FarmRuntimeFixture.mjs',sha256:hash('scripts/Create-FarmRuntimeFixture.mjs'),source:'work/producer/samples/FarmGame',proof:'work/analysis/q3-farm-player/20261007T085530733Z/native-inputs/fixture.json',scope:'19 exact native data copies; no original executable in input directory; all referenced input bytes covered by directory hash'};
    if(flag==='--farm-script-runtime')tests.at(-1).timeoutMs=45000;
  }
}
// Shared dispatch conditions also expose independent modes. Do not silently
// omit the owned/runtime alternatives when regenerating the inventory.
for(const m of source.matchAll(/if\(argc==3&&\(std::wstring\(argv\[2\]\)==L"(--[^"]+)"\|\|std::wstring\(argv\[2\]\)==L"(--[^"]+)"\)/g)) {
  for(const flag of [m[1],m[2]])if(!tests.some(t=>t.id===flag.slice(2)))tests.push({
    id:flag.slice(2),runner:'producer_core_tests',arguments:[flag],inputs:[],
    originalExecutableRequired:false,originalDataRequired:false,
    scope:'shared-dispatch dedicated native mode; GUI/audio/original comparison separate',
    timeoutMs:flag.endsWith('-runtime')?45000:15000,status:'未実行'
  });
}
const drivers=fs.readdirSync('scripts').filter(p=>/^Test-.*\.(ps1|mjs)$/.test(p)).map(p=>{
  const text=read('scripts/'+p);
  const delegated=[...text.matchAll(/-Only\s+['"]([\w-]+)['"]/g)]
    .flatMap(m=>tests.find(t=>t.id===m[1])?.arguments??[]);
  return {id:p.replace(/\.(ps1|mjs)$/,''),runner:'scripts/'+p,sha256:hash('scripts/'+p),
    dedicatedModes:[...new Set([...text.matchAll(/['"](--[\w-]+)['"]/g)].map(m=>m[1]).concat(delegated))],
    inputSetup:'See runner parameters and preparation/auditor prerequisites; missing input is not a pass',status:'未実行'};
});
// Explicitly registered inspectors and evidence-local auditors are obligations
// too. Regenerating the Test-* inventory must not silently drop them.
for(const previous of existingManifest?.drivers??[])if(!drivers.some(d=>d.id===previous.id)){
  const entry={...previous,status:'未実行'};delete entry.lastResult;
  if(fs.existsSync(entry.runner))entry.sha256=hash(entry.runner);
  drivers.push(entry);
}
const manifest={schema:1,createdUtc:new Date().toISOString(),sourceDispatcher:{path:'tests/producer/core_tests.cpp',sha256:hash('tests/producer/core_tests.cpp')},
  environment:'Windows Win32 Release; authorized normal host execution; preserve OS-denied cases',
  passRule:'exit 0 AND parsed passed=true AND unchanged candidate/input hashes; expected rejection requires a separate explicit oracle',
  tests,drivers,fullAcceptance:false};
// Keep a recorded refusal gate, never an old pass result, across inventory
// regeneration. A gate may be removed only with recorded changed conditions.
for(const kind of ['tests','drivers'])for(const entry of manifest[kind]){
  const previous=existingManifest?.[kind]?.find(v=>v.id===entry.id);
  if(previous?.blockedByKnownOsRefusal)entry.blockedByKnownOsRefusal=previous.blockedByKnownOsRefusal;
  if(previous){
    entry.history=previous.history??[];
    entry.resultHistory=[...(previous.resultHistory??[])];
    if(previous.lastResult&&!entry.resultHistory.some(r=>r.run===previous.lastResult.run&&r.id===previous.lastResult.id))entry.resultHistory.push(previous.lastResult);
    if(previous.timeoutMs)entry.timeoutMs=previous.timeoutMs;
    if(previous.independentEvidence)entry.independentEvidence=previous.independentEvidence;
  }
}
fs.writeFileSync('docs/analysis/regression-manifest.json',JSON.stringify(manifest,null,2)+'\n');
console.log(JSON.stringify({native:tests.length,drivers:drivers.length,missingInputs:tests.flatMap(t=>t.inputs).filter(p=>!fs.existsSync(p))}));

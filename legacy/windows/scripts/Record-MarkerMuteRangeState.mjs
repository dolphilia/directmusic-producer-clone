import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [unit,buildPath,nativePath,driverPath] = process.argv.slice(2);
assert(driverPath,'Usage: Record-MarkerMuteRangeState.mjs UNIT BUILD NATIVE DRIVER');
assert(!fs.existsSync(unit+'/unit-record.json'),'Unit already promoted');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const write=(p,v)=>fs.writeFileSync(p,JSON.stringify(v,null,2)+'\n');
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const relative=p=>path.relative(process.cwd(),p).replace(/\\/g,'/');
const build=read(buildPath),proof=read(unit+'/unit-proof.json'),native=read(nativePath),drivers=read(driverPath),candidate=path.basename(path.dirname(buildPath));
assert(proof.passed&&build.passed&&build.sourceSnapshotUnchanged);
for(const run of [proof,native,drivers])assert.equal(run.candidate,candidate);
for(const s of build.sources){assert.equal(hash(s.path),s.sha256);assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);}
for(const o of build.outputs)assert.equal(hash(path.join(path.dirname(buildPath),o.path)),o.sha256);
const files=['product-state.json','acceptance-status.json','regression-manifest.json','acceptance-status.md','implementation-status.md','feature-map.csv'];
for(const n of files){const source='docs/analysis/'+n,backup=unit+'/'+n+'.before-promotion';assert(!fs.existsSync(backup));fs.copyFileSync(source,backup);}
const scope='Five source strips Tempo/Sequence/Lyric/Marker/Mute share selected-group/nth half-open range and one history; main native save, distinct reload and two normal exits0; original bulk parity and all40/all8 incomplete';
const residuals=[
 'Original bulk dynamic selection/duplicate/overlap/meter reanchor comparison remains unavailable; independent RIFF/PCM evidence is separate',
 'Remaining strip types, graphical row mapping/cycling, snap, OLE/COM/MIDI clipboard, external Producer ABI and meter reanchor remain incomplete',
 'New candidate Q1 fresh native five-format lifecycle and serial Style/Transport activeStop/restart/tempo WASAPI unexecuted; completed202431545Z scenario retained only as history',
 'Q2 independent Windows without original Producer binaries, registration and lookup paths remains unavailable',
 'Ordinary core and other unchanged Windows5 publication/recovery conditions frozen; current dedicated47 pass30 blocked and driver20 pass21 blocked66 unexecuted exclude all unavailable tests',
 'Other media/generation/runtime/reference/export/cache/factory/output/deployment all40 responsibilities retained in feature-map and prior units'
];
const nextAction='Q3E: connect existing ToolGraph/Parameter implementation to main source Tool factory provisioning and capability discovery; define a bounded authored native Project→parameter edit/history/save→distinct reload→actual playback/PCM unit. Read current contracts/source first; retain original comparison/Q2 blockers and do not remake existing model/runtime. Re-run Q1 after the next product candidate is fixed.';
const entry={schema:2,createdUtc:new Date().toISOString(),candidate,phase:'Q3C',featureIds:['MarkerStripMgr.dll','MuteStripMgr.dll','Timeline.dll','DMUSProd.exe'],
 targetGap:'Marker/Mute absent from shared range/atomic multi-strip history; extended-stride empty-destination copy compatibility absent',
 endCondition:'related/native/registered round on saved build; main five-strip one move/UndoRedo/native save/normal exit/distinct restore, independent exact byte audit; original unavailable steps remain blocked',
 status:'限定成立・原版bulk比較未完',scope,
 changes:['Marker play/enter and Mute/remap raw range copy/delete/merge/overwrite preserve full stride/extensions/opaque data; empty destination adopts source stride and populated mismatch rejects',
 'Strict half-open bounds, DWORD allowed/reserved channels, duplicate Mute collision validation and private whole-document transaction preserve bytes/dirty/selection/Redo on rejected input',
 'TRNG v1 adds mark/mutc with explicit selected-group/nth tracks and main five-strip checkbox/selection/move controls',
 'Native range regression expanded60→93; unrelated source and prior evidence retained',
 'Main20544 moves768 clocks then one Undo/Redo saves native Project, exits0; independent22036 restores all five strips, resaves and exits0'],
 build:{path:buildPath,sha256:hash(buildPath),savedSources:build.sources.length,configureExitCode:build.configureExitCode,buildExitCode:build.buildExitCode,installExitCode:build.installExitCode,outputs:build.outputs},
 related:proof.related,native:proof.native,drivers:proof.drivers,gui:{candidate,status:'限定合格',passed:true,...proof.gui,evidence:unit+'/unit-proof.json',scope},
 project:proof.project,audio:{candidate,status:'未実行',scope:'No new-candidate audio claim; previous Q1 audio is historical'},
 original:{status:'障害あり',scope:'Original bulk designers unavailable; no new dynamic oracle',evidence:['work/analysis/q3-marker-boundaries/20261006T143436Z/unit-record.json','work/analysis/q3-mute/20261004T222600Z/original-observation.json']},
 priorityReview:{reason:'Three bounded Timeline units now connect2→3→5 strips and reduce concrete missing responsibility, but same-group fine detail will not close original/Q2/all8; next independent Q3E factory gap',nextGroup:'E'},
 evidence:[buildPath,nativePath,driverPath,unit+'/unit-start.json',unit+'/changed-sources.json',unit+'/unit-proof.json',unit+'/save-proof.json'].map(p=>({path:relative(p),sha256:hash(p)})),
 residuals,nextAction,fullAcceptance:false};
const manifest=read('docs/analysis/regression-manifest.json');
for(const [kind,runPath,run] of [['tests',nativePath,native],['drivers',driverPath,drivers]])for(const result of run.results){const t=manifest[kind].find(t=>t.id===result.id);assert(t,result.id);
 if(t.lastResult){t.resultHistory??=[];t.resultHistory.push(t.lastResult);}if(t.latest){t.history??=[];t.history.push(t.latest);}
 t.lastResult={candidate,run:relative(runPath),...result};t.latest=t.lastResult;t.status=result.status;
}
manifest.latestNativeRound={candidate,run:relative(nativePath),counts:proof.native.counts};
manifest.latestDriverRound={candidate,run:relative(driverPath),counts:proof.drivers.counts};
manifest.updatedUtc=entry.createdUtc;
manifest.helperAuditors??=[];manifest.helperAuditors.push({unit,script:'scripts/Inspect-MarkerMuteRangeUnit.mjs',sha256:hash('scripts/Inspect-MarkerMuteRangeUnit.mjs'),proof:unit+'/unit-proof.json',scope:'Saved evidence auditor; separate from77 native and107 driver inventory',fullAcceptance:false});
write('docs/analysis/regression-manifest.json',manifest);
const state=read('docs/analysis/product-state.json'),c=state.current;
c.unitHistory??=[];c.unitHistory.push({candidate:c.candidate,unit:c.latestUnit,archive:unit+'/product-state.json.before-promotion'});
const oldAudio=c.audio,oldGui=c.gui,oldQ1=c.q1CurrentIntegration;
Object.assign(c,{candidate,build:buildPath,latestUnit:unit+'/unit-record.json',latestReport:'docs/analysis/q3-marker-mute-range-2026-10-07.md',updatedUtc:entry.createdUtc,residuals,nextAction,nextIndependentUnit:nextAction,fullAcceptance:false});
for(const [field,exitCode]of[['configuration',build.configureExitCode],['compilation',build.buildExitCode],['install',build.installExitCode]])c[field]={candidate,status:exitCode===0?'合格':'失敗',exitCode,evidence:buildPath,scope:'Saved195 source Win32 Release reference tools OFF; all40/Q2 separate'};
c.core={candidate,status:'障害あり',evidence:relative(nativePath),reason:'Historical20261005T051458521Z atomic Chordmap Project Windows5 after690; unchanged condition frozen',scope:'Current ordinary core unexecuted, not a pass'};
c.nativeDedicated={...manifest.latestNativeRound,timelineRangeChecks:93,sequenceChecks:native.results.find(r=>r.id==='sequence-crud')?.checks,fullAcceptance:false};
c.driverRound=manifest.latestDriverRound;c.registeredDrivers={...manifest.latestDriverRound,fullAcceptance:false};c.drivers=manifest.latestDriverRound;
c.smoke={candidate,status:drivers.results.find(r=>r.id==='Test-ProductHost')?.status??'未実行',evidence:relative(driverPath),scope:'Bounded main host smoke only'};
c.gui={...entry.gui,historical:oldGui};c.audio={...entry.audio,historical:oldAudio,fullAcceptance:false};
c.q1CurrentIntegration={candidate,status:'未実行',historical:oldQ1,historicalUnit:'work/analysis/q1-fixed-integration/20261006T205000Z/unit-record.json',scope:'Previous complete representative chain is on202431545Z, not transferred to new product',fullAcceptance:false};
c.q1FiveDocumentScenario=c.q1CurrentIntegration;c.q1Integration=c.q1CurrentIntegration;c.integrationProgress={candidate,status:'作業中',scope};
for(const id of ['MarkerStripMgr.dll','MuteStripMgr.dll','Timeline.dll','TempoStripMgr.dll','SequenceStripMgr.dll','LyricStripMgr.dll','DMUSProd.exe']){
 const f=c.featureResponsibilities[id];assert(f,id);f.history??=[];f.history.push({candidate:f.candidate,evidence:f.evidence,scope:f.scope,remaining:f.remaining});
 const prior=Array.isArray(f.remaining)?f.remaining:[f.remaining].filter(Boolean);
 const remaining=prior.map(r=>String(r).replace('All other Strip classes','Strip classes other than the verified five').replace('Extended-stride clipboard compatibility when destination is empty','Original extended-stride reciprocal clipboard interoperability'));
 Object.assign(f,{candidate,status:'作業中',scope,evidence:unit+'/unit-record.json',remaining:[...remaining,'Original bulk parity, remaining strips/OLE/snap/meter/external ABI, new candidate Q1/audio, Q2 and all40/all8 remain']});
}
write('docs/analysis/product-state.json',state);
const acceptance=read('docs/analysis/acceptance-status.json');
acceptance.history??=[];acceptance.history.push({candidate:acceptance.candidate,latestUnit:acceptance.latestUnit,archive:unit+'/acceptance-status.json.before-promotion'});
Object.assign(acceptance,{candidate,build:buildPath,latestUnit:unit+'/unit-record.json',updatedUtc:entry.createdUtc,fullAcceptance:false,residuals,regressionRound:manifest.latestNativeRound,driverRound:manifest.latestDriverRound});
for(const field of ['configuration','compilation','install'])acceptance[field]=c[field];
for(const a of acceptance.criteria){a.candidate=candidate;a.evidence=unit+'/unit-record.json';a.status=['repeat-invalid-input','original-independence'].includes(a.id)?'障害あり':'作業中';
 a.scope=a.id==='play-stop-tempo-audio'?'New candidate audio unexecuted; previous Q1 success retained only as history':scope;a.currentScope=a.scope;
 a.remaining=a.id==='original-independence'?residuals[3]:a.id==='play-stop-tempo-audio'?residuals[2]:a.id==='repeat-invalid-input'?residuals[4]:'Bounded five-strip evidence does not complete all40 duties or this all8 criterion; original dynamic and remaining responsibility gaps retained';
}
write('docs/analysis/acceptance-status.json',acceptance);
fs.writeFileSync('docs/analysis/acceptance-status.md',`# 全体8受入の現在状態\n\n候補 \`${candidate}\`。最新単位 [Marker/Mute共通範囲](../../${unit}/unit-record.json)。全体未完了、\`fullAcceptance=false\`。\n\n|受入|状態|現候補の範囲・残差|\n|---|---|---|\n${acceptance.criteria.map(a=>`|${a.name}|${a.status}|${a.scope}; ${a.remaining}|`).join('\n')}\n\n構成/build/install各exit0、195保存ソース。native77=47合格/30障害、driver107=20合格/21障害/66未実行。五strip共有範囲93件、本体一操作/UndoRedo/保存/別復元/exit0は限定成立。障害・未実行を合格に含めない。前候補Q1の音声判定器2補足成功は新候補へ転用しない。\n`);
fs.writeFileSync('docs/analysis/implementation-status.md',`# 本体再構築の現在状態\n\n候補 \`${candidate}\`。正本は [product-state.json](product-state.json) の current。最新単位 [Marker/Mute共通範囲](../../${unit}/unit-record.json)。全体未完了。\n\n195保存ソースの構成/build/install各exit0。通常coreは既知Windows5条件凍結で現候補未実行。専用native77=47合格/30障害、driver107=20合格/21障害/66未実行。関連合格: ${['timeline-range','marker-document','mute-document','lyric-document','sequence-crud'].map(id=>id+' '+native.results.find(r=>r.id===id).checks).join(' / ')}。\n\n五strip範囲の一回移動・UndoRedo・native保存、作者20544と別復元22036の通常exit0、7時刻以外のSegment全bytes一致と再保存一致を確認。原版bulk比較・残strip/OLE/Snap/ABI・新候補Q1/音声・Q2は未完。[全体8状態表](acceptance-status.md) は6作業中/2障害で合格0。\n\n全40責務の入口 [feature-map.csv](feature-map.csv) を保持し、古い未着手/次は実行指示としない。次: ${nextAction}\n`);
write(unit+'/unit-record.json',entry);
fs.copyFileSync(process.argv[1],unit+'/state-recorder.mjs');
console.log(JSON.stringify({candidate,native:proof.native.counts,drivers:proof.drivers.counts,gui:true,fullAcceptance:false}));

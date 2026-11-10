import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [unitArg,scenarioArg,buildArg,nativeArg,driverArg]=process.argv.slice(2);
assert(driverArg,'Usage: Record-DlsSampleState.mjs UNIT SCENARIO BUILD NATIVE DRIVER');
const unit=path.resolve(unitArg),scenario=path.resolve(scenarioArg),buildPath=path.resolve(buildArg),nativePath=path.resolve(nativeArg),driverPath=path.resolve(driverArg);
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,'')),write=(p,v)=>fs.writeFileSync(p,JSON.stringify(v,null,2)+'\n');
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex'),rel=p=>path.relative(process.cwd(),p).replaceAll('\\','/');
const count=rs=>rs.reduce((a,r)=>{a[r.status]=(a[r.status]??0)+1;return a;},{});
const proof=read(scenario+'/unit-proof.json'),build=read(buildPath),native=read(nativePath),drivers=read(driverPath),candidate=proof.candidate,utc=new Date().toISOString();
assert(proof.passed&&!proof.fullAcceptance&&build.passed);assert.equal(native.candidate,candidate);assert.equal(drivers.candidate,candidate);
assert.equal(proof.auditorSha256,hash('scripts/Inspect-DlsSampleUnit.mjs'));
assert(!fs.existsSync(unit+'/unit-record.json'),'Promotion is one-shot');
const controls=read(proof.audio.controls),audio=read(proof.audio.proof);assert(controls.passed&&audio.passed);assert.equal(hash(proof.audio.controls),proof.audio.controlsSha256);assert.equal(hash(proof.audio.proof),proof.audio.sha256);
const files=['product-state.json','acceptance-status.json','regression-manifest.json'];
for(const f of files){const p='docs/analysis/'+f,b=unit+'/'+f+'.before-promotion';if(fs.existsSync(b))assert.equal(hash(p),hash(b));else fs.copyFileSync(p,b);}
const manifest=read('docs/analysis/regression-manifest.json');assert.equal(native.results.length,manifest.tests.length);assert.equal(drivers.results.length,manifest.drivers.length);
assert(native.results.every(r=>['合格','障害あり','未実行','検証待ち'].includes(r.status)),'Unresolved native failure');
assert(!drivers.results.some(r=>r.status==='失敗'),'Unresolved driver failure');
assert.equal(native.results.find(r=>r.id==='dls-sample-policy')?.status,'合格');
assert(native.results.find(r=>r.id==='dls-sample-policy').checks>=28);
for(const t of manifest.tests){const r=native.results.find(r=>r.id===t.id);assert(r);t.resultHistory??=[];if(t.lastResult)t.resultHistory.push(t.lastResult);t.lastResult=t.latest={candidate,...r};t.status=r.status;}
for(const t of manifest.drivers){const r=drivers.results.find(r=>r.id===t.id);assert(r);t.resultHistory??=[];if(t.lastResult)t.resultHistory.push(t.lastResult);t.lastResult=t.latest={candidate,...r};t.status=r.status;}
const registered=manifest.drivers.find(t=>t.id==='Test-DlsSampleInheritanceAudioAuditor');assert(registered);assert.equal(hash(registered.runner),controls.driverSha256);assert.equal(registered.sha256,controls.driverSha256);
const supplement={candidate,id:registered.id,status:'合格',runner:registered.runner,runnerSha256:controls.driverSha256,evidence:proof.audio.controls,evidenceSha256:proof.audio.controlsSha256,sourceCapture:proof.audio.capture,scope:'Positive current PCM and seven derived negative controls; original/full8 separate',fullAcceptance:false};
registered.resultHistory.push(registered.lastResult);registered.lastResult=registered.latest=supplement;registered.status='合格';
const effective=drivers.results.map(r=>r.id===registered.id?supplement:r),driverRound={candidate,run:rel(driverPath),rawCounts:count(drivers.results),counts:count(effective),supplements:[supplement],total:manifest.drivers.length,fullAcceptance:false};
const nativeRound={candidate,run:rel(nativePath),counts:count(native.results),total:manifest.tests.length,fullAcceptance:false};
manifest.latestNativeRound=nativeRound;manifest.latestDriverRound=driverRound;manifest.updatedUtc=utc;manifest.helperAuditors??=[];
for(const p of ['scripts/Inspect-DlsSampleUnit.mjs','scripts/Inspect-DlsSampleInheritanceAudio.mjs'])manifest.helperAuditors.push({candidate,auditor:p,sha256:hash(p),evidence:rel(scenario+'/unit-proof.json'),scope:proof.scope,fullAcceptance:false});
write('docs/analysis/regression-manifest.json',manifest);
const residuals=[
 'Q1: prior222153066Z representative five-form/native Project/audio is historical; current candidate fresh five-form/tempo full chain remains unexecuted. This three-form DLS scenario is bounded.',
 'Q2: independent Windows without original Producer binaries, registration or search paths remains unavailable; no whole original-independence claim.',
 'Original same-input dynamic/reciprocal DLS/Wave/other comparison remains blocked by unavailable original components; RIFF and PCM are independent evidence, not a substitute.',
 'Current ordinary core remains unexecuted/blocked by frozen historical atomic Chordmap Project Windows5 after690; dedicated failures/blocked/unexecuted are never passes.',
 'Wave Track waih loop-enable/end semantics, music-time/stream/variation/multiple-track/shared Timeline remain; DLS ADSR/release/full waveform, inherited DLS runtime export and all-component audition remain.',
 'Full40 responsibility mapping, common Timeline/OLE/COM, cache/reference repair, runtime/export/recovery, source ABI, file output/live/multibuffer/generation/deployment remain.'
];
const nextAction='Q3F FileOutput multi-buffer: current Conductor rejects multiple FileOutput effects despite existing writer/DMO. Preserve those implementations; connect two directly routed mix-group buffers to one recording lifecycle with Record.wav/Record1.wav ordering from original Help. Verify native AudioPath/save/normal exit/distinct restore and both output PCM files plus WASAPI; keep Send/unrouted groups/concurrency/legacy ABI separate. Preserve original/Q2/core blocks; WaveTrack loop bits, inherited DLS runtime export and fixture shared-GUID resolution remain queued.';
const evidence=[buildPath,nativePath,driverPath,scenario+'/unit-proof.json',scenario+'/author-exit.json',scenario+'/reload-exit.json',scenario+'/final-inputs.json',proof.audio.proof,proof.audio.controls,unit+'/audio-234743632-failure.json',unit+'/diagnostic.json'].map(p=>({path:rel(p),sha256:hash(p)}));
const record={schema:2,createdUtc:utc,candidate,phase:'Q3D',featureIds:['DLSDesigner.ocx','Conductor.dll','WaveStripMgr.dll'],targetGap:read(unit+'/unit-start.json').targetGap,endCondition:read(unit+'/unit-start.json').endCondition,status:'DLSサンプル継承限定合格・全体未完',scope:proof.scope,
 changes:['Validate Wave/Region MIDI unity0..127 before adoption/playback; preserve invalid source load/save and rejection history','Private playback collection resolves inherited entire Wave WSMP only for absent Region override; source inheritance and explicit one-shot preserved','Reject same-GUID conflicting owned byte snapshots even when private runtime normalization makes their bytes equal','Dedicated sample-policy regression28 and same-candidate main/save/normal exits/distinct reload/differential PCM with seven negative controls'],
 build:{path:rel(buildPath),sha256:hash(buildPath),sources:build.sources.length,configureExitCode:build.configureExitCode,buildExitCode:build.buildExitCode,installExitCode:build.installExitCode,outputs:build.outputs},core:{status:'障害あり',executed:false,reason:residuals[3]},native:nativeRound,drivers:driverRound,gui:{passed:true,evidence:rel(scenario+'/unit-proof.json'),processes:proof.processes,observations:proof.observed},audio:{passed:true,...proof.audio,plays:audio.plays},original:{status:'障害あり',dynamicParity:false,scope:residuals[2]},q2:{status:'障害あり',scope:residuals[1]},evidence,
 preservedFailures:[unit+'/diagnostic.json',unit+'/guid-conflict-diagnostic/diagnostic.json',unit+'/capture-overrun.json',unit+'/audio-234743632-failure.json',scenario+'/active-stop-timing-failure.json',scenario+'/recorder-argument-failure.json','work/build/product-snapshot/20261007T001004058Z/build-summary.json','work/acceptance/regression/20261007T001504956Z/script-document/stderr.txt'],residuals,nextAction,fullAcceptance:false};
write(unit+'/unit-record.json',record);
const state=read('docs/analysis/product-state.json'),c=state.current;
c.unitHistory??=[];c.unitHistory.push({candidate:c.candidate,unit:c.latestUnit,archive:rel(unit+'/product-state.json.before-promotion')});
for(const key of ['q1CurrentIntegration','q1Integration','q1FiveDocumentScenario','integrationProgress']){c[key+'History']??=[];if(c[key])c[key+'History'].push(c[key]);c[key]={candidate,status:'未実行',scope:residuals[0],historicalEvidence:'work/analysis/q1-current-integration/20261006T225403Z/unit-record.json',fullAcceptance:false};}
Object.assign(c,{candidate,build:rel(buildPath),latestUnit:rel(unit+'/unit-record.json'),latestReport:'docs/analysis/q3-dls-sample-inheritance-2026-10-07.md',updatedUtc:utc,inProgressUnit:null,residuals,nextAction,nextIndependentUnit:nextAction,nativeDedicated:nativeRound,registeredDrivers:driverRound,drivers:driverRound,driverRound,gui:{candidate,...record.gui,scope:proof.scope,fullAcceptance:false},audio:{candidate,status:'限定合格',...record.audio,scope:proof.scope,fullAcceptance:false},core:{candidate,status:'障害あり',executed:false,evidence:rel(nativePath),reason:residuals[3]},smoke:{candidate,status:drivers.results.find(r=>r.id==='Test-ProductHost')?.status??'未実行',evidence:rel(driverPath),scope:'Bounded host smoke only'},fullAcceptance:false});
for(const [key,code] of [['configuration',build.configureExitCode],['compilation',build.buildExitCode],['install',build.installExitCode]])c[key]={candidate,status:code===0?'合格':'失敗',exitCode:code,evidence:rel(buildPath),scope:build.sources.length+' saved source Win32 Release reference tools OFF; all40/Q2 separate'};
c.featureResponsibilities??={};
for(const id of ['DLSDesigner.ocx','Conductor.dll']){const f=c.featureResponsibilities[id]??{status:'作業中',remaining:residuals.slice(1)};f.sampleInheritanceHistory??=[];if(f.sampleInheritance)f.sampleInheritanceHistory.push(f.sampleInheritance);f.sampleInheritance={candidate,status:'限定合格',evidence:rel(unit+'/unit-record.json'),scope:proof.scope,fullAcceptance:false};c.featureResponsibilities[id]=f;}
const wave=c.featureResponsibilities['WaveStripMgr.dll'];if(wave)wave.sampleInheritanceScope={candidate,status:'WaveTrack部分は未実行',evidence:rel(unit+'/unit-record.json'),scope:'DLS Wave defaults resolved; WaveTrack overrides/loop-enable/end semantics remain queued',fullAcceptance:false};
state.updatedUtc=utc;state.inProgressUnit=null;state.pendingSourceChange=null;write('docs/analysis/product-state.json',state);
const acceptance=read('docs/analysis/acceptance-status.json');acceptance.history??=[];acceptance.history.push({candidate:acceptance.candidate,unit:acceptance.latestUnit,archive:rel(unit+'/acceptance-status.json.before-promotion')});
Object.assign(acceptance,{candidate,build:rel(buildPath),updatedUtc:utc,latestUnit:rel(unit+'/unit-record.json'),configuration:c.configuration,compilation:c.compilation,install:c.install,regression:nativeRound,regressionRound:nativeRound,driverRound,residuals,fullAcceptance:false});
const scopes={
 'clean-build':`${build.sources.length}保存source configure/build/install各0。全40責務/Q2未完。`,
 'startup-shutdown':`DLS三文書作者${proof.processes.author}/別再読込${proof.processes.reload}正常exit0。全画面/全責務不足。`,
 'original-data-load':'同候補専用回帰とsource fixture読込。全原版最小文書・意味/動的比較不足。',
 'edit-save':'DLS継承/明示one-shot/UndoRedo/保存の限定合格。全strip/Timeline/clipboard/ABI不足。',
 'separate-process-reload':'native三文書保存byte不変と別PID継承復元の限定合格。全責務不足。',
 'play-stop-tempo-audio':'継承持続/単発/Undo復元、2音10秒間隔、Stop無音の現候補限定PCM合格。Q1五形式/tempo変更・全責務/原版比較不足。',
 'repeat-invalid-input':residuals[3],
 'original-independence':residuals[1]
};
for(const criterion of acceptance.criteria){criterion.candidate=candidate;criterion.productSha256=proof.exeSha256;criterion.scope=criterion.currentScope=scopes[criterion.id];criterion.evidence=rel(unit+'/unit-record.json');criterion.fullAcceptance=false;criterion.remaining=scopes[criterion.id];}
write('docs/analysis/acceptance-status.json',acceptance);
fs.writeFileSync('docs/analysis/acceptance-status.md',`# 全体8受入の現在状態\n\n候補 \`${candidate}\`。[最新単位](../../${rel(unit)}/unit-record.json)。全体未完、\`fullAcceptance=false\`。\n\n|受入|状態|現行範囲と不足|\n|---|---|---|\n${acceptance.criteria.map(c=>`|${c.name}|${c.status}|${c.currentScope}|`).join('\n')}\n\nsource${build.sources.length}、configure/build/install各0。native${nativeRound.total}=${JSON.stringify(nativeRound.counts)}。driver${driverRound.total}原始一巡=${JSON.stringify(driverRound.rawCounts)}、同候補音声判定器補足後=${JSON.stringify(driverRound.counts)}。障害・未実行は合格に含めない。Q1旧候補の五形式統合は履歴、Q2独立環境・全40責務・全8最終受入は未完。\n`);
console.log(JSON.stringify({candidate,native:nativeRound.counts,drivers:driverRound.counts,fullAcceptance:false,nextAction}));

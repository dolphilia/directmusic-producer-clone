import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const write=(p,v)=>fs.writeFileSync(p,JSON.stringify(v,null,2)+'\n');
const unit='work/analysis/q3-chord/20261004T180000Z',candidate='20261004T181307552Z';
const buildPath=`work/build/product-snapshot/${candidate}/build-summary.json`,build=read(buildPath);
assert(build.passed&&build.sourceSnapshotUnchanged&&build.configureExitCode===0&&build.buildExitCode===0&&build.installExitCode===0);
for(const s of build.sources){assert.equal(hash(s.path),s.sha256);assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);}
for(const o of build.outputs)assert.equal(hash(path.join(path.dirname(buildPath),o.path)),o.sha256);
const product=build.outputs.find(o=>o.path==='install/bin/Producer.exe');
const runs={normal:'work/acceptance/product/20261004T181557230Z/run.json',regression:'work/acceptance/regression/20261004T181557090Z/run.json',drivers:'work/acceptance/registered-drivers/20261004T181557451Z/run.json',configured:'work/acceptance/runtime-recovery-configured/20261004T181620850Z/run.json',failedRecovery:'work/acceptance/runtime-recovery-write/20261004T181623764Z/run.json',notifications:'work/acceptance/product-notifications/20261004T182457365Z/run.json',monitor:'work/acceptance/short-playback-monitor/20261004T182457080Z/run.json',notes:'work/acceptance/product-notes/20261004T182457192Z/run.json'};
const results=Object.fromEntries(Object.entries(runs).map(([k,p])=>[k,read(p)]));
for(const r of Object.values(results)){assert.equal(r.buildSummarySha256,hash(buildPath));}
assert(results.normal.cases.every(c=>c.passed&&c.exitCode===0));
const core=JSON.parse(fs.readFileSync(path.join(path.dirname(runs.normal),'core.stdout.txt'),'utf8').replace(/^\uFEFF/,''));assert(core.passed&&core.checks===592);
const chord=results.regression.results.find(r=>r.id==='chord-track');assert(chord.status==='合格'&&chord.exitCode===0&&chord.checks===36);
const chordResult=JSON.parse(fs.readFileSync(path.join(chord.evidence,'stdout.txt'),'utf8').replace(/^\uFEFF/,''));assert(chordResult.passed&&chordResult.checks===36);
assert(results.configured.passed&&results.configured.cases.length===8&&results.configured.cases.every(c=>c.passed));
assert(results.failedRecovery.passed===false);
const manifestPath='docs/analysis/regression-manifest.json',manifest=read(manifestPath);
assert.equal(manifest.sourceDispatcher.sha256,hash(manifest.sourceDispatcher.path));
for(const t of manifest.tests){const r=results.regression.results.find(v=>v.id===t.id);assert(r);t.status=r.status;t.evidence=r.evidence;t.candidate=candidate;t.result={exitCode:r.exitCode,checks:r.checks,error:r.error};}
const configured=manifest.tests.find(t=>t.id==='runtime-recovery-configured-verify');configured.status='合格';configured.evidence=runs.configured;configured.result={exitCode:0,expectedExit:0,scope:'fresh configured prepare / owned separate Producer recovery / verify; all eight cases passed'};
for(const id of ['runtime-recovery-verify','runtime-recovery-gui-verify']){const t=manifest.tests.find(t=>t.id===id);t.status='障害あり';t.evidence=id==='runtime-recovery-verify'?runs.failedRecovery:runs.regression;t.result={exitCode:null,error:'Required fresh preparation failed; verifier not launched; no inherited pass'};}
for(const d of manifest.drivers){const r=results.drivers.results.find(v=>v.id===d.id);assert(r);d.status=r.status;d.evidence=r.evidence;d.candidate=candidate;d.result={reason:r.reason,error:r.error};}
for(const [id,key] of [['Test-PlaybackNotifications','notifications'],['Test-ShortPlaybackMonitor','monitor'],['Test-PlaybackNotes','notes']]){const r=results[key];assert(!r.executionBlocked&&r.cases.every(c=>c.passed&&c.exitCode===0));const d=manifest.drivers.find(d=>d.id===id);assert(d);d.status='合格';d.evidence=runs[key];d.result={scope:r.scope};}
manifest.updatedUtc=new Date().toISOString();manifest.candidate=candidate;manifest.fullAcceptance=false;write(manifestPath,manifest);write(unit+'/regression-manifest.json',manifest);
const counts=items=>items.reduce((a,v)=>(a[v.status]=(a[v.status]??0)+1,a),{});
const steps=read(unit+'/gui-steps.json');
for(const [action,text] of [['place-CM7','Value: 1: CM7'],['change-Dm-root2','Value: 1: Dm'],['undo-CM7-root0','Value: 1: CM7'],['redo-Dm-root2','Value: 1: Dm']])assert(steps.find(s=>s.action===action)?.tree.includes(text));
const process=read(unit+'/gui-process.json');assert(process.Path.toLowerCase().includes(candidate.toLowerCase()));
const next='Q0: Recovery準備の実例外がassert前に記録されない診断不足を修正し、JAZP error5/Recovery障害の条件を切り分ける。同じOS拒否を再試行しない。独立Q3: Chord GUI保存/別起動とStyleに対する発音効果・原版比較を閉じる一単位を残し、SignPost/Chordmap参照の責務へ進む。Q1五文書再生接続は別残単位として維持。';
const residuals=['同候補回帰: JAZP専用入口で保存replace Windows error5。Recovery GUI準備は第3check、独立RecoveryWrite準備は第1check失敗。実例外記録不足。失敗ファイル・journal・ログを保持、無条件再試行なし','Chord GUI配置/変更/UndoRedoは確認。保存ダイアログのcached入力欄が扱えず中断、保存/終了0/別GUI復元未確認。GUI PID3472に未保存のDmコードを保持','Chord global scale/key編集、clipboard/drag/MIDI、meter reanchor、演奏中変更、Style発音と原版相互編集、原版デフォルト未確認','新候補PCM・Q1五文書再生接続/Transport default/embedded優先未実行。旧候補録音・GUI証拠は旧版限定として保持','Q2原版なし独立Windows環境未用意。原版はregistry更新警告/window未取得、動的比較未実施','全40責務・全8受入未完了。native Project移動制約とChannel旧ラベルをqueueに保持'];
const record={schema:2,createdUtc:new Date().toISOString(),candidate,build:{path:buildPath,sha256:hash(buildPath)},sources:build.sources.length,productSha256:product.sha256,configuration:'合格',compilation:'合格',install:'合格',feature:'ChordStripMgr.dll Segment Chord CRUD',status:'native編集/保存/履歴/Project復元とGUI配置/変更/UndoRedoの部分確認; Chord全責務未完了',changes:['cord/crdh/crdb typed codec with preserved extensions and transactional edits','selected-group Timeline beat replacement and complete-document Undo/Redo','eight-subchord editor opened from installed Producer','ordinary core plus dedicated Chord manifest/driver'],coreChecks:core.checks,chordChecks:chord.checks,regression:counts(manifest.tests),drivers:counts(manifest.drivers),runs,gui:{processId:process.Id,mainWindow:15010654,editorWindow:5180584,evidence:unit+'/gui-steps.json',placementChangeUndoRedo:'合格',save:'障害あり; no file saved',separateProcessReload:'未実行',normalExit:'未実行',obstacle:unit+'/gui-save-obstacle.json'},audio:'未実行',originalDynamicComparison:'障害あり; prior original startup registry warning retained',originalFreeEnvironment:'障害あり; not available',fullAcceptance:false,residuals,nextAction:next,previousUnit:read(unit+'/previous-product-state.json').current.latestUnit};
const evidence=new Set([buildPath,...Object.values(runs),unit+'/unit-start.json',unit+'/contract.md',unit+'/gui-steps.json',unit+'/gui-process.json',unit+'/gui-save-obstacle.json',unit+'/authored-chord-audit.json',unit+'/original-sample-chord-audit.json',unit+'/regression-manifest.json','scripts/Record-ChordState.mjs','scripts/Inspect-ChordFile.mjs','scripts/Test-ChordTrack.ps1',...read(unit+'/unit-start.json').failedBuilds]);
for(const p of Object.values(runs)){const dir=path.dirname(p);for(const f of fs.readdirSync(dir,{withFileTypes:true}))if(f.isFile())evidence.add(path.join(dir,f.name));}
for(const r of results.regression.results)for(const name of ['stdout.txt','stderr.txt','run.json']){const p=path.join(r.evidence,name);if(fs.existsSync(p))evidence.add(p);}
for(const name of ['original-startup-obstacle.json','exit-observer-obstacle.json']){const p='work/analysis/q1-five-documents/20261004T173000Z/'+name;if(fs.existsSync(p))evidence.add(p);}
for(const name of fs.readdirSync(unit))if(/^gui-final-/.test(name))evidence.add(unit+'/'+name);
for(const p of ['work/analysis/sources/dmusicf.h','work/analysis/sources/dmusici.h','work/analysis/sources/dmplugin.h','work/analysis/help/htm/insertingandeditingchordsinasegment.htm','work/analysis/help/htm/chordscaletab.htm','work/producer/samples/QuickStart/heartland.sgp'])evidence.add(p);
record.evidence=[...evidence].map(p=>({path:p.replaceAll('\\','/'),sha256:hash(p)}));write(unit+'/unit-record.json',record);
const statePath='docs/analysis/product-state.json',state=read(statePath);state.current={...state.current,candidate,build:buildPath,latestUnit:unit+'/unit-record.json',latestReport:unit+'/report.md',configuration:'合格',compilation:'合格',install:'合格',core:'合格: 592 checks; same candidate',nativeDedicated:JSON.stringify(record.regression),gui:'Chord CM7→Dm/root2 UndoRedoを本体で確認。保存はhelper障害で未確認、別GUI/終了未実行',audio:'現候補PCM未実行。旧163608301ZのSequence録音は旧版限定の履歴',fullAcceptance:false,residuals,nextAction:next,featureResponsibilities:{...(state.current.featureResponsibilities??{}),'ChordStripMgr.dll':{status:'作業中',implementation:'src/producer/chord.cpp / chord_editor.cpp / SegmentDocument',scope:record.status,remaining:residuals[2],evidence:unit+'/unit-record.json'}}};state.fullGoal='active; incomplete';state.updatedJst='2026-10-05';write(statePath,state);
const acceptancePath='docs/analysis/acceptance-status.json',acceptance=read(acceptancePath);acceptance.updatedUtc=record.createdUtc;acceptance.candidate=candidate;acceptance.build=record.build;acceptance.configuration='合格';acceptance.compilation='合格';acceptance.install='合格';acceptance.fullAcceptance=false;
for(const c of acceptance.criteria){c.historicalEvidence=[...new Set([...(c.historicalEvidence??[]),...(c.evidence??[])])];c.candidate=candidate;c.productSha256=product.sha256;c.evidence=[unit+'/unit-record.json'];c.scope='今回の部分証拠; 全対象の合格ではない';c.status=c.id==='repeat-invalid-input'?'失敗':'作業中';c.remaining={
    'clean-build':'80保存ソースの構成/build/install0。全対象実装と原版なし独立環境での再現不足',
    'startup-shutdown':'本体smoke0、Chord GUI起動。現候補の正常GUI終了0未確認',
    'original-data-load':'原版Heartland Chord5件をSDK静的監査。原版動的比較と全形式の本体読込不足',
    'edit-save':'Chord36/native Project復元とGUI配置/変更/UndoRedoを確認。GUI保存はhelper障害。全責務の編集保存不足',
    'separate-process-reload':'Chord Framework/native Projectのcore復元は成功。今回GUI保存・終了・別GUI復元未実行',
    'play-stop-tempo-audio':'現候補通知/短時間監視/note出力APIは成功。WASAPI PCMと全機能の発音・Stop・再開・tempo未実行',
    'repeat-invalid-input':'通常core592/Chord36成功。回帰JAZP error5とRecovery準備失敗を保持。失敗を合格へ含めない',
    'original-independence':'source-only構成を保持。現候補全機能のmodule由来と原版なし独立環境の検証不足'
}[c.id];}write(acceptancePath,acceptance);
const report=['# Chord編集の現候補と残責務','',`候補 ${candidate}、保存${build.sources.length}ソース、構成/build/install exit0。全体未完了。製品SHA256 ${product.sha256}。`,
    '',`本体smoke exit0、通常core ${core.checks} checks、Chord専用${chord.checks} checksが合格。Chordの拍衝突置換、Undo/Redo、入力拒否のbytes/history不変、WCHAR16/8 SubChord、拡張・予約byte保持、Segment保存とnative JAZP復元を検証。`,
    '',`38入口: ${JSON.stringify(record.regression)}。89driver: ${JSON.stringify(record.drivers)}。Configured Recovery後段のみ同候補8casesの新しい証拠で合格。JAZP入口とRecovery準備の失敗は保持し、別入口の成功で上書きしない。`,
    '', 'GUI PID3472の本体Chord画面で CM7/root0→Dm/root2、Undo→CM7/root0、Redo→Dm/root2を確認。保存dialogはcached欄を扱えず中断、保存済みとの主張はしない。未保存文書は起動中の画面に保持。native保存復元とGUI保存は区別する。',
    '', '原版HeartlandのChord5件は独立SDK-layout監査。原版の動的比較・現候補録音・原版なし環境の代用ではない。',
    '', '再現: PowerShell7で Build-ProductSnapshot.ps1 → Test-ProductSnapshot.ps1 / Test-RegressionManifest.ps1 / Test-RegisteredNativeDrivers.ps1 に build-summary.json を指定。Test-ChordTrack.ps1も同引数。MSBuildのSDK探索先がsandbox拒否になる場合は通常権限の承認審査を使う。OS security/ExecutionPolicyを変更しない。失敗候補・入力・journalは保持する。',
    '', '残責務:',...residuals.map(r=>'- '+r),'', '次の一手: '+next,'', '[契約](contract.md)、[詳細とhash](unit-record.json)、[全8状態表](../../../../docs/analysis/acceptance-status.json)。'];fs.writeFileSync(unit+'/report.md',report.join('\n')+'\n');
const planPath='docs/analysis-plan.md';if(!fs.existsSync(unit+'/previous-analysis-plan.md'))fs.copyFileSync(planPath,unit+'/previous-analysis-plan.md');let plan=fs.readFileSync(planPath,'utf8');
plan=plan.replace(/\| 候補 \|[^\n]+/,`| 候補 | \`${candidate}\`、80保存ソース、configure/build/install各exit 0 | 全体完成ではない |`)
 .replace(/\| 通常試験 \|[^\n]+/,'| 通常試験 | 現候補本体smoke exit0、通常core592 checks exit0、Chord36 checks exit0 | 旧失敗を保持。全8合格ではない |')
 .replace(/\| 現行専用試験 \|[^\n]+/,`| 現行専用試験 | 38入口の合格34・失敗2・障害2。89driverの合格37・失敗1・未実行51 | JAZP error5/Recovery準備失敗。全登録群合格は未達 |`)
 .replace(/\| GUI \|[^\n]+/,'| GUI | Chord配置/変更/UndoRedoを現候補本体で確認 | 保存dialogのhelper障害、保存/別GUI復元/終了0未確認。旧五文書・Sequence録音は旧版限定 |')
 .replace(/基準build：[^\n]+/,`基準build：[build-summary](../${buildPath})。`)
 .replace(/今回の通常実行：[^\n]+/,`今回の通常実行：[run.json](../${runs.normal})。`)
 .replace(/最新単位：[^\n]+/,`最新単位：[Chord編集](../${unit}/report.md)。旧五文書/Sequence録音は先行候補の履歴に保持。`)
 .replace(/製品EXE SHA-256：[^\n]+/,`製品EXE SHA-256：\`${product.sha256}\`。`)
 .replace('専用36モード、88driver','専用37モード、89driver')
 .replace(/Q0の契約\/通知不整合[^\n]+/,`Q0のPChannel契約修正を現候補core592で維持。Chord CRUD/履歴/native Project復元と本体GUI変更を実装。今回登録群一巡でJAZP error5/Recovery準備失敗を発見。${next} native Project移動制約と旧Channelラベルをqueueに保持。`);
fs.writeFileSync(planPath,plan);console.log(JSON.stringify({candidate,core:core.checks,chord:chord.checks,native:record.regression,drivers:record.drivers,evidence:record.evidence.length,fullAcceptance:false}));

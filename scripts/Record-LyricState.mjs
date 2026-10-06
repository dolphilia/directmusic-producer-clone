import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const candidate=process.argv[2]; assert(candidate==='20261004T214510749Z');
const unit='work/analysis/q3-lyric/20261004T212600Z';
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const write=(p,v)=>fs.writeFileSync(p,JSON.stringify(v,null,2)+'\n');
const buildPath=`work/build/product-snapshot/${candidate}/build-summary.json`, build=read(buildPath);
assert(build.passed&&build.sourceSnapshotUnchanged);
for(const s of build.sources){assert.equal(hash(s.path),s.sha256);assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);}
for(const o of build.outputs)assert.equal(hash(path.join(path.dirname(buildPath),o.path)),o.sha256);
const runs={normal:'work/acceptance/product/20261004T214806051Z/run.json',lyric:'work/acceptance/lyric-document/20261004T214805170Z/run.json',regression:'work/acceptance/regression/20261004T214831615Z/run.json',drivers:'work/acceptance/registered-drivers/20261004T214835486Z/run.json',runtimeFailure:'work/acceptance/runtime-update/20261004T214855744Z/run.json'};
const results=Object.fromEntries(Object.entries(runs).map(([k,p])=>[k,read(p)]));
for(const r of Object.values(results))assert.equal(r.buildSummarySha256,hash(buildPath));
assert(results.normal.cases.every(c=>c.passed&&c.exitCode===0)); assert(results.lyric.passed); assert.equal(results.runtimeFailure.exitCode,1);
const coreChecks=read(path.join(path.dirname(runs.normal),'core.stdout.txt')).checks;
const mp='docs/analysis/regression-manifest.json', manifest=read(mp);
assert.equal(manifest.sourceDispatcher.sha256,hash(manifest.sourceDispatcher.path));
for(const [kind,run] of [['tests',results.regression],['drivers',results.drivers]])for(const e of manifest[kind]){
  const r=run.results.find(v=>v.id===e.id); assert(r);
  Object.assign(e,{candidate,status:r.status,evidence:r.evidence,result:{exitCode:r.exitCode,checks:r.checks,error:r.error,reason:r.reason}});
}
const gate={candidate,evidence:runs.runtimeFailure,reason:'Controlled Z.sty replacement refusal was followed by Windows error 5 restoring A.sgt; rollback incomplete, recovery preserved. Future identical driver workflow is gated until changed conditions are evidenced. Actual failed run stays failed; native same-candidate run at a different fresh directory passed.'};
manifest.drivers.find(e=>e.id==='Test-RuntimeUpdate').blockedByKnownOsRefusal=gate;
Object.assign(manifest,{candidate,lastNativeRun:runs.regression,lastDriverRun:runs.drivers});write(mp,manifest);
const counts=a=>a.reduce((o,r)=>(o[r.status]=(o[r.status]??0)+1,o),{});
const product=build.outputs.find(o=>o.path==='install/bin/Producer.exe'), gui=read(unit+'/gui-record.json');
assert.equal(gui.productSha256,product.sha256);
for(const f of gui.files)assert.equal(hash(path.join(unit,f.path)),f.sha256);
const next='Q1へ戻り、現候補の新規native ProjectにSegment/Style/Band/DLS/AudioPathを結び、同一保存文書のPlay/Stop/再開/テンポを既存WASAPI録音・解析で確認する。終了コード取得はOS拒否を迂回せず未確認を保持。Q2独立環境は障害として維持し、独立Q3 Mute/SegmentRefの既存責務点検・本体接続を次単位とする。';
const residuals=[
 'Lyric playback Tool delivery/Message Window, timeline multiselection/drag/meter changes, negative pickup clock behavior and equal-time original oracle remain incomplete. Unicode text ownership is implemented, not merely unknown chunk retention.',
 'Lyric current GUI add/change/copy/paste/UndoRedo/native Project save/separate-process reload/original reciprocal edit passed in bounded scope. GUI delete not exercised; original second Properties not separately inspected; normal UI close observed, exitCode null after EnableRaisingEvents access denial.',
 'RuntimeUpdate dedicated driver exit1: Windows error5 rollback incomplete after controlled replacement refusal. Retained recovery and failed run; no identical retry. Different native directory passed22 checks, not proof of failed recovery.',
 'Marker bulk Mark/Unmark, timeline and playback stop/cue/enter; full Chordmap graph original rejection, Ref/Strip/composition/palette/ceed and other responsibilities remain.',
 'Q1 same-scenario five-document integration and Play/Stop/resume/tempo PCM incomplete; previous candidate results remain historical.',
 'Known Windows copy-publication/RuntimeSettings/Defaults/Recovery/JAZP/DLS refusals remain. Q2 independent original-free Windows environment unavailable. All40 responsibilities and all8 acceptance incomplete.'
];
const sp='docs/analysis/product-state.json', state=read(sp);
const record={schema:2,createdUtc:new Date().toISOString(),candidate,feature:'LyricStripMgr.dll',status:'bounded owned Unicode lyric document and main editor complete; full responsibility incomplete',build:{path:buildPath,sha256:hash(buildPath)},sources:build.sources.length,productSha256:product.sha256,configuration:'合格',compilation:'合格',install:'合格',coreChecks,lyricChecks:results.lyric.checks,native:counts(manifest.tests),drivers:counts(manifest.drivers),runs,contract:'docs/analysis/lyric-contract.md',originalObservation:unit+'/original-observation.json',gui,audio:{status:'未実行',remaining:'Lyric tool notification playback; Q1 same-scenario PCM'},changes:['Typed original LIST lyrt/lyrl/lyre, lyrh clocks and timing, UTF16LE lyrn; absent payload on original empty track accepted.','Owned CRUD, independent physical/logical clock validation, clipboard extensions, group/nth-track selection and full-document Undo/Redo; invalid input preserves bytes/selection/redo.','Main File Segment Lyrics editor with Unicode dynamic text length and native Project save/reload.','Original GUI observations and source-to-original-to-source reciprocal editing; native parser audit is supplemental.'],supersededBuilds:[{candidate:'20261004T214216250Z',reason:'Empty original track omits payload; source fixed before any runtime validation. Build retained, runtime unexecuted.'}],runtimeFailure:gate,residuals,nextAction:next,previousUnit:state.current.latestUnit,fullAcceptance:false};
const files=d=>fs.readdirSync(d,{withFileTypes:true}).flatMap(e=>e.isDirectory()?files(path.join(d,e.name)):[path.join(d,e.name)]);
const evidence=new Set([buildPath,mp,record.contract,'scripts/Record-LyricState.mjs','scripts/Test-LyricDocument.ps1','scripts/Watch-GuiProcessExit.ps1']);
for(const p of Object.values(runs))for(const f of files(path.dirname(p)))evidence.add(f);
for(const f of files(unit))if(!['unit-record.json','report.md'].includes(path.basename(f)))evidence.add(f);
record.evidence=[...evidence].map(p=>({path:p.replaceAll('\\','/'),sha256:hash(p)})); write(unit+'/unit-record.json',record);
Object.assign(state.current,{candidate,build:buildPath,latestUnit:unit+'/unit-record.json',latestReport:unit+'/report.md',configuration:'合格',compilation:'合格',install:'合格',core:`合格:${coreChecks} checks`,nativeDedicated:JSON.stringify(record.native),registeredDrivers:record.drivers,gui,audio:record.audio,residuals,nextAction:next,fullAcceptance:false});state.fullGoal='active; incomplete';
state.current.featureResponsibilities['LyricStripMgr.dll']={status:'作業中',implementation:'lyric.cpp / lyric_editor.cpp / SegmentDocument',scope:record.status,remaining:residuals[0],evidence:unit+'/unit-record.json'};write(sp,state);
const ap='docs/analysis/acceptance-status.json', acceptance=read(ap);
Object.assign(acceptance,{updatedUtc:record.createdUtc,candidate,build:record.build,configuration:'合格',compilation:'合格',install:'合格',residuals,fullAcceptance:false});
for(const c of acceptance.criteria){c.historicalEvidence=[...new Set([...(c.historicalEvidence??[]),...(c.evidence??[])])];Object.assign(c,{candidate,productSha256:product.sha256,evidence:[unit+'/unit-record.json'],status:['original-data-load','repeat-invalid-input'].includes(c.id)?'失敗':'作業中',scope:'Current candidate bounded Lyric and regression evidence; all responsibilities incomplete'});}
const set=(id,text)=>acceptance.criteria.find(c=>c.id===id).remaining=text;
set('clean-build',`${build.sources.length} saved sources configure/build/install0; full responsibilities and isolated environment missing`);
set('startup-shutdown','Lyric main new Project and normal UI close/PID gone observed; exit code monitor access denied, null. All required screens incomplete.');
set('edit-save',`core${coreChecks}/Lyric${record.lyricChecks}; GUI Unicode add/change/copy/paste/UndoRedo saved; delete native only, full feature gaps remain`);
set('separate-process-reload','Same candidate PID10888 save/close → PID11064 native Project reload: 2 Lyric entries and independent clocks/timing restored. Other responsibilities incomplete.');
set('original-data-load','Original three lyric samples native read and source/native Project→original Properties→original edit/save→source main reload observed. Full authored Chordmap graph rejection remains historical unresolved compatibility failure.');
set('repeat-invalid-input','Lyric malformed/invalid inputs preserve document/selection/redo. Current RuntimeUpdate driver failed with rollback access denied; unexecuted/blocked cases remain.');
write(ap,acceptance);
fs.writeFileSync(unit+'/report.md',`# Lyric文書所有と本体編集\n\n候補 ${candidate}、${build.sources.length}保存ソース、configure/build/install各0、本体smoke0、core${coreChecks}、Lyric専用${record.lyricChecks}。全体未完了。\n\n42入口 ${JSON.stringify(record.native)}、93driver ${JSON.stringify(record.drivers)}。失敗・障害・未実行を合格に含めない。\n\n原版で空track、Unicode歌詞、開始3085/所属6144/Before設定8を作成保存し、契約を実装。空trackのlyrt省略を受理する修正を行い、未検証の初期候補を保持。Unicode編集、clipboard、group/nth選択、全bytes履歴、不正入力不変を実装。\n\n同じ候補のGUI新規native Projectで2歌詞を作成、変更/UndoRedo/コピー貼付、保存/通常UI終了/別プロセス復元を確認。ソース保存歌詞を原版で読込み、第一PropertiesのUnicode/開始/所属/通知設定一致を確認。原版で原版 revised Ω/Quick設定に変更して別名保存し、ソースGUI復元を確認。3入力とGUI保存3ファイルのhashはunit-recordに結合。独立RIFF監査は補助であり原版動的操作の代用ではない。\n\n終了コードはEnableRaisingEventsアクセス拒否でnull。RuntimeUpdate専用driverは保存拒否後の復旧A.sgt書込みもWindows5で拒否されexit1、復旧データを保持。native別ディレクトリ22checks成功でこの失敗を相殺しない。\n\n${residuals.map(r=>'- '+r).join('\n')}\n\n次の具体的な一手: ${next}\n\n再現: pwsh -NoProfile -File scripts/Build-ProductSnapshot.ps1。生成build-summaryをTest-ProductSnapshot.ps1、Test-LyricDocument.ps1に指定。node scripts/Create-RegressionManifest.mjs後、同じbuild-summaryでTest-RegressionManifest.ps1、Test-RegisteredNativeDrivers.ps1を一巡。既知OS拒否gateを保持。原版比較はgui-record.jsonの操作順と固定入力を使用。GUI/audio/全体受入は別判定。\n\n[一次実行・生成物・入力hash](unit-record.json)\n`);
let plan=fs.readFileSync('docs/analysis-plan.md','utf8');
plan=plan.replace(/\| 候補 \|[^\n]+/,`| 候補 | \`${candidate}\`、${build.sources.length}保存ソース、configure/build/install各exit0 | 全体完成ではない |`).replace(/\| ソースと証拠 \|[^\n]+/,`| ソースと証拠 | 現候補${build.sources.length}保存ソースと4生成物hash照合 | hash確認は試験再実行ではない |`).replace(/\| 通常試験 \|[^\n]+/,`| 通常試験 | 本体smoke0、core${coreChecks}、Lyric${record.lyricChecks} | 全8合格ではない |`).replace(/\| 現行専用試験 \|[^\n]+/,`| 現行専用試験 | 42入口 ${JSON.stringify(record.native)}。93driver ${JSON.stringify(record.drivers)} | 失敗・未実行・障害を合格に含めない |`).replace(/\| GUI \|[^\n]+/,'| GUI | 現候補Lyric新規native Project/追加/変更/コピー貼付/UndoRedo/保存/通常UI終了/別プロセス復元、原版相互編集 | 終了コードOS拒否、GUI delete未実行。Q1全代表経路/再生/停止/再開/テンポPCM不足 |').replace(/基準build：[^\n]+/,`基準build：[build-summary](../${buildPath})。`).replace(/今回の通常実行：[^\n]+/,`今回の通常実行：[run.json](../${runs.normal})。`).replace(/最新単位：[^\n]+/,`最新単位：[Lyric文書所有と本体編集](../${unit}/report.md)。旧候補の成功は履歴。`).replace(/製品EXE SHA-256：[^\n]+/,`製品EXE SHA-256：\`${product.sha256}\`。`).replace('専用41モード、92driver','専用42モード、93driver');
plan=plan.replace('現在のqueue先頭はQ3群C Lyricの文書所有・本体接続。Q1の全代表経路は未完了。','現在のqueue先頭はQ1同一候補・全代表経路の統合とWASAPI録音。Lyric文書所有・本体接続の限定単位を閉じ、残細部をqueueに保持。');
plan=plan.replace(/Q0のPChannel契約をcore\d+[^\n]+/,`Q0のPChannel契約をcore${coreChecks}で維持。Lyric文書所有/Unicode/独立時刻/履歴/clipboard/本体保存復元と原版相互編集の限定単位を閉じた。${next} Lyric Tool通知/負のpickup/Timeline、Marker bulk/実再生、群B互換残差とその他未対応責務はqueueに保持。RuntimeUpdate専用driverの復旧書込みOS拒否exit1を保持し同条件再試行しない。`);
fs.writeFileSync('docs/analysis-plan.md',plan);
console.log(JSON.stringify({candidate,coreChecks,lyricChecks:record.lyricChecks,native:record.native,drivers:record.drivers,evidence:record.evidence.length,fullAcceptance:false}));

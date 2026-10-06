import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const root=process.cwd(),dir=path.resolve(process.argv[2]);
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const write=(p,v)=>fs.writeFileSync(p,JSON.stringify(v,null,2)+'\n');
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const rel=p=>path.relative(root,path.resolve(p)).replaceAll('\\','/');
const author=read(dir+'/author-launch.json'),reopen=read(dir+'/reopen-process.json');
assert.notEqual(author.processId,reopen.processId);
assert.equal(author.exeSha256,reopen.exeSha256);
assert.equal(hash(author.executable),author.exeSha256);
assert.equal(hash(author.buildSummary),author.buildSummarySha256);
assert.equal(read(dir+'/author-close.json').processAbsent,true);
const build=read(author.buildSummary);
for(const s of build.sources)assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);
const steps=read(dir+'/states.json');
const step=(label)=>{const s=steps.find(s=>s.label===label);assert(s,`Missing GUI observation: ${label}`);return s.accessibility?.tree??'';};
assert.match(step('Change settled'),/0 clocks 6 BPM/);
assert.match(step('Undo restores first tempo 5'),/0 clocks 5 BPM/);
assert.match(step('Redo restores first tempo 6'),/0 clocks 6 BPM/);
assert.match(step('Second Undo restores original Song'),/0 clocks 5 BPM/);
assert.match(step('Reopened Song 8 notes tempo5/7.5'),/Notes: 8/);
assert.match(step('Reopened Style name Scenario Style'),/Style name Value: Scenario Style/);
assert.match(step('Reopened Band patch0 channel0 pan64 volume100'),/PChannel 0 patch 0 pan 64 volume 100/);
const dls=step('Reopened DLS instrument region wave');
for(const v of ['Instrument 1','Region 1','Key high Value: 127','Velocity high Value: 127','8000 Hz, 16 bits, 800 frames'])assert(dls.includes(v));
const ap=step('Reopened AudioPath Scenario Route');
for(const v of ['Scenario Route — Route.aup','PChannel 0 count 16, buffers 1','186CC545-DB29-11D3-9BD1-0080C7150A74'])assert(ap.includes(v));
const initial=read(dir+'/unit-start.json');
assert.equal(hash(dir+'/Scenario/Song.sgp'),initial.songSha256);
const names=fs.readdirSync(dir+'/Scenario');
const inputs=names.map(n=>({path:rel(dir+'/Scenario/'+n),sha256:hash(dir+'/Scenario/'+n),bytes:fs.statSync(dir+'/Scenario/'+n).size}));
assert.equal(fs.readFileSync(dir+'/Scenario/Scenario.pro').toString('ascii',8,12),'JAZP');
const statePath='docs/analysis/product-state.json',state=read(statePath);
assert.equal(state.current.candidate,author.candidate);
for(const name of ['product-state','acceptance-status','regression-manifest']){
  const dest=dir+'/previous-'+name+'.json';
  if(!fs.existsSync(dest))fs.copyFileSync('docs/analysis/'+name+'.json',dest);
}
const residuals=[
  'Q1文書間のStyle/Band/DLS再生接続とTransport default/embedded優先のGUI/PCMは未実行。この5文書シナリオの音声を旧2 Segment結果で合格にしない',
  '今回作者GUI Closeと消失を確認したがexitコード観測スクリプトがOS execution policyで拒否。正常終了0は未検証; 再読込側は継続起動中',
  'GUI Recovery verifyと全88登録driver/auditor一巡未完了',
  'Q2原版なし独立Windows環境未用意',
  'Q3和声ほか未接続責務/原版動的比較。既存原版起動でregistry更新エラー、操作可能window未取得。native Project移動制約',
  '全40責務と全8受入未完了'
];
const next='保存済み5文書シナリオの再生接続・Transport default/embeddedを一つの残単位に保持。OS拒否を迂回せず、独立Q3 BのChordイベント契約・CRUD/UndoRedo/保存復元を実装する。原版動的比較は障害扱いを維持。';
const evidence=['unit-start.json','author-launch.json','reopen-process.json','states.json','author-close.json','exit-observer-obstacle.json','original-startup-obstacle.json','reopened-catalog.jpg','reopened-dls.jpg','reopened-audiopath.jpg'].map(n=>({path:rel(dir+'/'+n),sha256:hash(dir+'/'+n)}));
evidence.push({path:'scripts/Record-Q1FiveDocumentState.mjs',sha256:hash('scripts/Record-Q1FiveDocumentState.mjs')});
const record={schema:1,createdUtc:new Date().toISOString(),candidate:author.candidate,featureIds:initial.features,
  closes:'新規native ProjectにSegment/Style/Band/DLS/AudioPathをGUI作成/編集/保存し、別プロセスで各文書の値を復元する不足',
  state:'カタログ保存復元の部分確認完了; Q1統合単位全体は未完了',productChanges:[],
  build:rel(author.buildSummary),buildSummarySha256:author.buildSummarySha256,exeSha256:author.exeSha256,
  configuration:'合格',compilation:'合格',install:'合格',
  gui:{nativeProjectSaved:true,authorProcessId:author.processId,reopenProcessId:reopen.processId,authorCloseObserved:true,authorExitCode:null,separateProcess:true,formatsRestored:['Segment','Style','Band','DLS','AudioPath'],songUndoRedo:true,scope:'カタログと記載値の復元; Styleは名前/既定tempoのみ、BandはGM1音色、DLSは1wave/1instrument/1region、AudioPathは既定route1本'},
  audio:'未実行; 過去の2 Segment録音はその単位に限定',originalComparison:'未実行; registry警告と操作可能window未取得',
  inputs,evidence,environment:'既存Windows host。OS DirectMusic依存。原版なし独立環境ではない',
  regressions:[{path:'work/acceptance/short-playback-monitor/20261004T172415890Z/run.json',scope:'同候補short-monitor',passed:true},{path:'work/acceptance/product-notes/20261004T172418752Z/run.json',scope:'同候補notes-api',passed:true}],
  residuals,nextAction:next,fullAcceptance:false};
for(const r of record.regressions){const run=read(r.path);assert.equal(run.buildSummarySha256,author.buildSummarySha256);assert(run.cases.length>0&&run.cases.every(c=>c.passed&&c.exitCode===0&&!c.timedOut&&c.sha256===author.exeSha256));r.sha256=hash(r.path);}
write(dir+'/unit-record.json',record);
fs.writeFileSync(dir+'/report.md',`# Q1 五文書カタログの保存復元\n\n候補 ${author.candidate}。全体未完了、Q1統合も未完了。生成物変更なし。\n\n作者PID ${author.processId}で新規Project、Initial/Song、Style、Band、DLS、AudioPathを作成/保存しnative JAZP 1146 bytesへ保存。Songの5→6 BPM変更/Undo5/Redo6/Undo5と保存を実操作で記録。GUI Close後作者window/processは消失。別PID ${reopen.processId}でScenario.proを開き、Song8音・5/7.5 BPM、Style名、Band patch0/channel0/pan64/volume100、DLS波形8000Hz/16bit/800frames・音色1・全鍵域Region1、AudioPath名/16PChannel route/既定bufferを確認。保存入力hashとUIA/画像はunit-record.json。\n\nStyle/所有DLS/Band/AudioPathの再生接続と優先はこの入力で未実行。旧Sequence録音を今回の成功には含めない。終了コード監視はPowerShell実行ポリシーで拒否され、今回exit0は未検証。ポリシー変更/迂回は実施していない。原版起動のregistry更新警告も保存。\n\n同候補short-monitor/notes-apiの一次runはexit0/parsed passedで確認。88driver一巡の完了ではない。\n\n${residuals.map(g=>'- '+g).join('\n')}\n\n次：${next}\n`);
state.current.latestUnit=rel(dir)+'/unit-record.json';state.current.latestReport=rel(dir)+'/report.md';
state.current.gui='同候補5文書native Project新規保存/別GUIの各値復元、Song5→6 UndoRedo確認。今回exit0はOS監視拒否で未検証';
state.current.residuals=residuals;state.current.nextAction=next;state.current.fullAcceptance=false;
state.current.q1FiveDocumentScenario=rel(dir)+'/Scenario/Scenario.pro';
delete state.current.ongoingUnit;write(statePath,state);
const acceptance=read('docs/analysis/acceptance-status.json');acceptance.updatedUtc=record.createdUtc;
for(const c of acceptance.criteria){
  if(['startup-shutdown','edit-save','separate-process-reload'].includes(c.id)){c.evidence=[...new Set([...c.evidence,rel(dir)+'/unit-record.json'])];c.scope='同候補五文書カタログ/記載値の部分復元。全責務の受入ではない';}
  if(c.id==='startup-shutdown')c.remaining='既存2 Segment作者/再読込exit0。今回五文書作者Close/消失は確認、exit観測はOS拒否で未検証。全編集画面一巡不足';
  if(c.id==='edit-save')c.remaining='五文書GUI新規保存とSong5→6 UndoRedoを確認。全対象CRUD/選択/コピー貼付け/演奏効果は不足';
  if(c.id==='separate-process-reload')c.remaining='五文書native Projectカタログ/記載値を別PID復元。全対象と文書間接続/演奏効果は不足';
  c.status='作業中';
}acceptance.fullAcceptance=false;write('docs/analysis/acceptance-status.json',acceptance);
const manifest=read('docs/analysis/regression-manifest.json');
for(const [id,evidencePath] of [['Test-ShortPlaybackMonitor',record.regressions[0].path],['Test-PlaybackNotes',record.regressions[1].path]]){
  const d=manifest.drivers.find(d=>d.id===id);assert(d,`Missing driver ${id}`);d.status='合格';d.evidence=evidencePath;d.scope='同候補の専用CLI観測のみ; GUI/PCM/全体受入ではない';
}manifest.updatedUtc=record.createdUtc;write('docs/analysis/regression-manifest.json',manifest);
let plan=fs.readFileSync('docs/analysis-plan.md','utf8');
plan=plan.replace(/\| GUI \|[^\n]+/,`| GUI | 現候補5文書native Project新規保存/別GUIカタログと値の復元、Song5→6 UndoRedoを確認。先行2 Segmentの途中Stop/再開PCM成功は別単位 | 今回exit0はOS監視拒否で未検証。Style/Band/DLS再生接続、Transport default/embedded優先未完了 |`);
plan=plan.replace(/最新単位：[^\n]+/,`最新単位：[Q1五文書カタログ](../${rel(dir)}/report.md)。Sequenceライフサイクル録音は先行単位に限定して保持。`);
plan=plan.replace(/Q0のPChannel修正[^\n]+/,`Q0の契約/通知不整合は現候補で修正・通常core556確認済み。五文書カタログ保存復元とSong自身のUndoRedoを確認。Q1再生接続/Transport優先は残単位に保持し、正常終了観測スクリプトのOS拒否を迂回しない。Q1群の細分化が続いたため、独立Q3 BのChord契約・編集実装を次に進める。全88driver一巡/GUI Recoveryは未完了。native Project移動制約と旧Channelラベルはqueueに保持。`);
plan=plan.replace(/Q2はWindowsSandbox[^\n]+/,'Q2は独立Windows環境未用意で障害。Q3原版Chord動的比較は既存DMUSProd起動でregistry更新警告、操作可能window未取得。一次資料・SDK契約を動的比較の代用にせず、環境変更や同条件再試行を行わず独立実装を進める。');
fs.writeFileSync('docs/analysis-plan.md',plan);
console.log(JSON.stringify({candidate:author.candidate,catalog:'partial passed',q1Complete:false,fullAcceptance:false,nextAction:next}));

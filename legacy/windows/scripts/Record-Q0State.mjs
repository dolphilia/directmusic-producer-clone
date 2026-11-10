import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
const json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const build='work/build/product-snapshot/20261004T154732811Z/build-summary.json';
const b=json(build),candidate=path.basename(path.dirname(build));
const unit='work/analysis/q0-regression/20261004T155400Z';fs.mkdirSync(unit,{recursive:true});
const runs=['work/acceptance/product/20261004T154920458Z/run.json','work/acceptance/regression/20261004T154937460Z/run.json',
 'work/acceptance/regression/20261004T155042657Z/run.json','work/acceptance/runtime-recovery-write/20261004T154922164Z/run.json',
 'work/acceptance/runtime-recovery-configured/20261004T154923436Z/run.json'];
const manifest=json('docs/analysis/regression-manifest.json');
const driverRun='work/acceptance/registered-drivers/20261004T155739669Z/run.json';
for(const r of json(driverRun).results){const d=manifest.drivers.find(d=>d.id===r.id);if(d){d.status=r.status;d.evidence=r.evidence;d.reason=r.reason;d.error=r.error;}}
for(const [id,p] of [['Test-LoopbackAudio','work/acceptance/audio-loopback/20261004T155510607Z/tempo-audio-proof.json'],['Test-TempoAudioAuditor','work/acceptance/q0-tempo-controls/20261004T155900Z/negative-tests.json']]) {
 const d=manifest.drivers.find(d=>d.id===id);if(d&&fs.existsSync(p)){d.status='合格';d.evidence=p;d.scope='限定tempo profileのみ; 他profileの全体合格ではない';}
}
const latest=new Map();for(const p of runs){const r=json(p);for(const t of r.results??[])latest.set(t.id,{...t,run:p});}
for(const t of manifest.tests){const r=latest.get(t.id);t.status=r?.status??'未実行';t.evidence=r?.run??null;}
for(const [id,p] of [['runtime-recovery-verify',runs[3]],['runtime-recovery-configured-verify',runs[4]]]) {
 const r=json(p),t=manifest.tests.find(t=>t.id===id);t.status=r.passed?'合格':'失敗';t.evidence=p;t.workflowEvidence=r.cases;
}
fs.writeFileSync('docs/analysis/regression-manifest.json',JSON.stringify(manifest,null,2)+'\n');
const residuals=['全85 driver/auditorの固定候補一巡は未完了; native35driverと限定tempo音声/反例成功','GUI Recovery verifyは未実行','Q1同一native Projectの全操作・音声は未接続',
 'Q2原版バイナリ・COM登録・探索先のない独立Windows環境は未用意','Q3和声/時間付きイベント/メディア/実行拡張/出力/配布責務が残る','原版動的比較・全対象編集互換・全8受入は未完了'];
const ids=['clean-build','startup-shutdown','original-data-load','edit-save','separate-process-reload','play-stop-tempo-audio','repeat-invalid-input','original-independence'];
const names=['クリーンビルド','起動と終了','原版データの読込','編集と保存','終了後の再読込','再生と停止','繰り返しと異常入力','原版依存の解消'];
const gaps=['独立環境からの宣言依存導入再現と全対象buildが未確認','新規native Project作成から正常終了まで未接続','全形式の原版最小文書と同一操作比較が不足','全責務の編集/Clipboard/選択/UndoRedo保存が不足','Q1同一シナリオの別プロセスGUI復元が未確認','現候補のGUI/CLI WASAPI録音と全対象音声が未完了','coreと専用の部分検証のみ; 全形式異常入力と反復不足','独立環境未用意; hash一致0を全機能解消と扱わない'];
const status={schema:1,updatedUtc:new Date().toISOString(),candidate,build:{path:build,sha256:hash(build)},configuration:'合格',compilation:'合格',install:'合格',fullAcceptance:false,
 criteria:ids.map((id,i)=>({id,name:names[i],status:i===7?'障害あり':'作業中',candidate,productSha256:b.outputs.find(o=>o.path==='install/bin/Producer.exe').sha256,
 evidence:i===0?[build]:i===6?runs:[],scope:'部分証拠; 全対象の合格ではない',remaining:gaps[i]})),residuals};
fs.writeFileSync('docs/analysis/acceptance-status.json',JSON.stringify(status,null,2)+'\n');
const old=json('docs/analysis/product-state.json');if(!fs.existsSync(unit+'/previous-product-state.json'))fs.copyFileSync('docs/analysis/product-state.json',unit+'/previous-product-state.json');
// Preserve legacy history verbatim; a versioned current object is authoritative.
old.schema=2;old.current={candidate,build,latestUnit:unit+'/unit-record.json',latestReport:unit+'/report.md',regressionManifest:'docs/analysis/regression-manifest.json',acceptanceStatus:'docs/analysis/acceptance-status.json',
 configuration:'合格',compilation:'合格',install:'合格',core:'合格',nativeDedicated:'部分合格; 残るworkflow/driverはmanifest参照',gui:'起動/画面観測; 操作継続中',audio:'検証待ち',fullAcceptance:false,
 nextAction:'Q0 driver/判定器一巡を続け、Q1 native Project GUI同一シナリオを接続。Q2隔離障害を保持し独立Q3責務へ進む。',residuals};
old.legacyFieldsScope='schema1由来フィールドは版別履歴。現行判定とqueueはcurrent/acceptance-status/analysis-planを使う。';
old.fullGoal='active; incomplete';
fs.writeFileSync('docs/analysis/product-state.json',JSON.stringify(old,null,2)+'\n');
const evidence=[build,...runs,driverRun,'work/acceptance/audio-loopback/20261004T155510607Z/tempo-audio-proof.json','work/acceptance/audio-loopback/20261004T155154006Z/run.json','work/acceptance/jazp-save/20261004T155746081Z/jazp-save-proof.json','scripts/Inspect-JazpSave.mjs','docs/analysis/regression-manifest.json','docs/analysis/acceptance-status.json','work/analysis/sources/dmusicf.h','work/analysis/sources/dmusici.h',
 'src/producer/sequence.cpp','tests/producer/core_tests.cpp','scripts/Test-RegressionManifest.ps1','scripts/Create-RegressionManifest.mjs'].map(p=>({path:p,sha256:hash(p)}));
fs.writeFileSync(unit+'/unit-record.json',JSON.stringify({schema:2,createdUtc:new Date().toISOString(),candidate,target:'Q0 Sequence PChannel / regression contracts',acceptanceGaps:['repeat-invalid-input','edit-save','全登録群の回帰漏れ'],
 changes:['PChannel 0..0xfffffffb許可/予約broadcast4値拒否と保存復元・不正時bytes/cache/dirty/historyを検証','低層Sequence insert検証をchangeと整合','Band生成GUIDだけを意味差として比較し保存/historyは全bytes比較','Runtime再出力の不変・unrelated file保護を現行更新契約で試験','専用36モードと85driverを列挙・固定候補で実行と未実行を区別'],
 evidence,failedCandidates:['20261004T154012658Z (SDK sandboxアクセス拒否)','20261004T154050983Z (旧試験契約/fixture/判定器の失敗証拠保持)'],residuals,fullAcceptance:false},null,2)+'\n');
fs.writeFileSync(unit+'/report.md',`# Q0 PChannelと回帰契約\n\n候補 ${candidate}、構成/build/install各exit0。通常coreと本体smoke成功。全体未完了。\n\nDMUS_IO_SEQ_ITEMのDWORDとdmusici.hの予約broadcast4定数を根拠に0..0xfffffffbを許可する。16/31/65536/最大許可値を保存復元と履歴で確認し、4予約値と不正ノートでclean/dirty/cache/Undo/Redo/output index不変を確認。低層insertも不正値を拒否する。\n\n専用回帰で新規BandのランダムGUIDを独立生成全bytesと比較する旧期待、既存Runtime更新を常時拒否する旧期待を修正。判定器のstdout前置き診断とfixture選択誤りも修正。失敗記録を保持。入力だけを旧証拠から取得し、成功結果は転用しない。\n\n現行状態はacceptance-status.jsonとregression-manifest.json。schema2 product-state.currentを正本にし、旧フィールドと移行前JSONを保持。\n\n再現: Build-ProductSnapshot.ps1 → Create-RegressionManifest.mjs → Test-ProductSnapshot.ps1 / Test-RegressionManifest.ps1 (同BuildSummaryPath)。RecoveryはTest-RuntimeRecoveryWrite/Configuredの別本体process検証を使う。\n\n残責務: ${residuals.join('。')}。次はcurrent.nextActionに従う。\n`);
console.log(JSON.stringify({candidate,unit,statuses:manifest.tests.reduce((a,t)=>(a[t.status]=(a[t.status]??0)+1,a),{})}));

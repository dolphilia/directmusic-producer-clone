import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const peRoot=path.join(root,'work/analysis/pe');
const registration=JSON.parse(fs.readFileSync(path.join(root,'docs/analysis/registration-capture.json'),'utf8'));
const captureByName=new Map(registration.captures.map(m=>[m.module,m]));
const product=JSON.parse(fs.readFileSync(path.join(root,'docs/analysis/product-state.json'),'utf8'));
if(product.schema>=2&&product.current){
  const c=product.current,acceptance=JSON.parse(fs.readFileSync(path.join(root,c.acceptanceStatus),'utf8'));
  const manifest=JSON.parse(fs.readFileSync(path.join(root,c.regressionManifest),'utf8'));
  const counts=items=>Object.entries(items.reduce((a,t)=>(a[t.status]=(a[t.status]??0)+1,a),{})).map(([s,n])=>`${s} ${n}`).join('、');
  const stateText=v=>typeof v==='string'?v:JSON.stringify(v);
  const currentCounts=items=>counts(items.map(t=>({status:t.lastResult?.candidate===c.candidate?t.lastResult.status:'未実行'})));
  const text=['# 本体再構築の現在状態','',`候補 ${c.candidate}。全体未完了。正本は [product-state.json](product-state.json) の current。旧schema1フィールドと従来CSVは版別履歴として保持する。`,
    '',`構成 ${stateText(c.configuration)}、compile ${stateText(c.compilation)}、install ${stateText(c.install)}、通常core ${stateText(c.core)}。現候補の専用native: ${currentCounts(manifest.tests)}。driver: ${currentCounts(manifest.drivers)}。`,
    '',`最新単位: ${c.latestUnit}。build: ${c.build}。機能責務の入口 [feature-map.csv](feature-map.csv) は全40行を維持し、古い「未着手」を現在の未実装判定へ転用しない。`,
    '', '| 全体受入 | 状態 | 残差 |','| --- | --- | --- |',...acceptance.criteria.map(a=>`| ${a.name} | ${a.status} | ${a.scope} |`),
    '',`次の作業: ${c.nextAction}`,'','旧implementation-status.csvは現行の完成率やqueueへ使用しない。専用試験と全8受入の正本は [regression-manifest.json](regression-manifest.json)、[acceptance-status.json](acceptance-status.json)。',''];
  fs.writeFileSync(path.join(root,'docs/analysis/implementation-status.md'),text.join('\n'));
  console.log(JSON.stringify({candidate:c.candidate,legacyCsvPreserved:true,fullAcceptance:false}));
  process.exit(0);
}
const productModules=new Set(['DMUSProd.exe','Timeline.dll','SegmentDesigner.ocx','Conductor.dll','SequenceStripMgr.dll','BandEditor.ocx','BandStripMgr.dll','StyleDesigner.ocx','StyleRefStripMgr.dll','DLSDesigner.ocx']);
const quote=value=>'"'+String(value??'').replaceAll('"','""')+'"';
const modules=fs.readdirSync(peRoot,{withFileTypes:true}).filter(f=>f.isDirectory()&&fs.existsSync(path.join(peRoot,f.name,'pe.json')))
  .map(f=>JSON.parse(fs.readFileSync(path.join(peRoot,f.name,'pe.json'),'utf8'))).map(pe=>{
    const name=path.basename(pe.input);
    const capture=captureByName.get(name);
    const category=['MFC42.DLL','MSVCRT.DLL','MSFLXGRD.OCX'].includes(name)?'依存ライブラリ':pe.input.includes('/app/')?'Producer 本体・同梱モジュール':'同梱サンプル・補助アプリ';
    const implementation=productModules.has(name)?'製品用の限定本体/サービスソースあり。原版COM ABI全体の互換実装ではない':['TempoStripMgr.dll','TimeSigStripMgr.dll'].includes(name)?'限定経路の互換ソースあり':'互換実装なし';
    const comparison=name==='TempoStripMgr.dll'?'新候補00c3の非システムクリップボード比較で6914レコード・445通常ファイル・123コピー・66画像一致':name==='TimeSigStripMgr.dll'?'候補630bの322観測・27ファイルと試験用Timeline接続160観測が原版と正規化なしで一致。Style管理対象の検索失敗経路のみ':name==='Timeline.dll'?'原版を Tempo 接続試験で使用':'互換実装との比較未実施';
    return {module:name,input:pe.input,sha256:pe.sha256,category,registration:capture?(capture.completed?'専用ハイブで登録成功':'専用ハイブで登録失敗'):'登録観測未実施・対象確認要',implementation,comparison,
      cleanBuild:productModules.has(name)?`限定製品構成/install成功。${product.productBuild}; ${product.coreTests}`:name==='TempoStripMgr.dll'?'共通の保存49ファイルから全12ターゲットビルド成功、今回生成物の実行未実施':name==='TimeSigStripMgr.dll'?'保存17ファイルの旧候補で原版比較成功。現行19ファイル・5ターゲットと共通49ファイル・12ターゲットのビルド成功、今回生成物は未実行。修正Styleプローブの別ハッシュはOS起動拒否':'互換実装のクリーンビルド未実施',
      hostAcceptance:productModules.has(name)?product.hostAcceptance:name==='TempoStripMgr.dll'?'原版本体を使った旧候補690と00c3の限定UI/clipboard比較は既存記録に保持。自作製品の全体受入とは別。':'互換実装の本体受入未実施'};
  }).sort((a,b)=>a.input.localeCompare(b.input));
const columns=Object.keys(modules[0]);
fs.writeFileSync(path.join(root,'docs/analysis/implementation-status.csv'),columns.map(quote).join(',')+'\n'+modules.map(m=>columns.map(k=>quote(m[k])).join(',')).join('\n')+'\n');
const lines=['# 全体再構築の検証状態','',`更新日：${product.updatedJst}。取得済み40 PEモジュールの暫定管理表。対象範囲の削減や工程6の完了を示す一覧ではない。`,
  '', '製品の最新状態は [本体実装と引継ぎ](product-host.md)、[機能台帳](feature-map.csv)、[機械可読状態](product-state.json)。原版不要の本体 producer と producer_core、producer_core_tests の構成・コンパイル・install成功は限定製品構成についてのみ。ユーザーの環境変更後、native実行と限定GUI試験を実施した。音声と全機能の受入は未完了。',
  '', `最新製品ビルド: ${product.productBuild}。現行native記録: ${product.runtimeAttempt}。旧拒否記録: ${product.previousPolicyRefusal}。${product.coreTests}。全体完成は未達。`,
  '', '識別情報と個別状態は [implementation-status.csv](implementation-status.csv)。依存ライブラリ・同梱サンプルも台帳に残し、必要なビルド・配布条件を今後確定する。',
  '', '| 群 | 現在の状態 | 次の判定 |','| --- | --- | --- |',
  '| TempoStripMgr | 候補690のクリップボード・終了再起動と原版相互読込・聴取を確認。新候補00c3は選択更新のネイティブ6914件と本体表示が一致。別の本体試験でコピー・切り取り・再貼付け保存とUndo/Redoバイト一致を確認。別試験の古いUIツリーは証拠から除外。原版TimelineのExportカウンター残存は失敗履歴として保持 | 新しい保存物の候補再起動・原版読込、現在のUI記録、候補音声、実ドラッグ・複数文書、非テンポ保存差分を調べる |',
  '| TimeSigStripMgr | 保存17ソースからビルド・実行し、原版と322観測・27ファイル、試験用Timeline接続160観測が完全一致。接続比較器7テストと既存検証器10テスト成功。Style管理対象の検索失敗経路のみ。修正した実TimelineプローブはOS起動拒否 | 実Timeline・Style拍子データ取込み・描画・編集・UI・通知配送・ランタイム同期と本体置換を確立する |',
  `| DMUSProd 本体・Framework・Timeline・Segment文書 | 型付きfactory/所有権、RIFF/metadata保持、Tempo/明示拍子/最小Sequence編集、保存、Undo/Redoを実装。${product.coreTests}。${product.hostAcceptance} | Style参照/拍子、残るtrack/COM通知と全体受入を接続 |`,
  `| Conductor・最小Sequence/BandTrack | ${product.playbackApi}。${product.audioAcceptance} | 全音符/Band/DLS/AudioPath編集、現在版の音声測定/endpoint、原版相互読込を拡張 |`,
  `| BandEditor・DLSDesigner・その他の Producer モジュール | ${product.coreTests}。${product.hostAcceptance} | Wave参照付き削除/loop編集、Style/Segment自身SaveAs/履歴、articulation/group-aware tracks・他40責務 |`,
  '| MFC42 / MSVCRT / MSFLXGRD | 原版の依存ライブラリとして識別 | 配布と呼出し規約、使用範囲を確認する |',
  '| Farm / StylePlayer | 同梱サンプル・補助アプリとして識別 | 全体受入での役割と依存を確認する |',
  '', '現行の共通CMakeを保存49ファイルから全12ターゲット構成・ビルドした記録は work/build/recovery-snapshot/20261002T120616172Z/build-summary.json。比較プローブ・静的ライブラリを含む12ターゲットであり、Producer12モジュールの再構築ではない。TimeSig単独の現行19ソース・5ターゲットは work/build/time-signature-snapshot/20261002T120513711Z/build-summary.json。今回生成物は全て未実行。TimeSigの成功比較は旧候補630bと保存17ソースのビルド work/build/time-signature-snapshot/20261002T114801406Z/build-summary.json を使用した。Styleプローブの修正前の不完全な保存観測は互換基準から除外し、修正後の別ハッシュの起動拒否を work/reference/time-signature-style/20261002T120234201Z/run.json に保持する。',
  '', '現行の Tempo 比較は原版 Timeline・TimeSigStripMgr と Windows の dmime.dll に依存する。Producer全体を原版固有モジュールなしで動かす構成と、実プロジェクトの作成・編集・保存・再読込・音声出力の受入は未完了。全体完成の判定は [計画書](../analysis-plan.md) の受入試験を維持する。'];
fs.writeFileSync(path.join(root,'docs/analysis/implementation-status.md'),lines.join('\n')+'\n');
console.log(JSON.stringify({modules:modules.length,withCompatibilitySource:modules.filter(m=>m.implementation==='限定経路の互換ソースあり').length}));

import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const peRoot=path.join(root,'work/analysis/pe');
const registration=JSON.parse(fs.readFileSync(path.join(root,'docs/analysis/registration-capture.json'),'utf8'));
const captureByName=new Map(registration.captures.map(m=>[m.module,m]));
const quote=value=>'"'+String(value??'').replaceAll('"','""')+'"';
const modules=fs.readdirSync(peRoot,{withFileTypes:true}).filter(f=>f.isDirectory()&&fs.existsSync(path.join(peRoot,f.name,'pe.json')))
  .map(f=>JSON.parse(fs.readFileSync(path.join(peRoot,f.name,'pe.json'),'utf8'))).map(pe=>{
    const name=path.basename(pe.input);
    const capture=captureByName.get(name);
    const category=['MFC42.DLL','MSVCRT.DLL','MSFLXGRD.OCX'].includes(name)?'依存ライブラリ':pe.input.includes('/app/')?'Producer 本体・同梱モジュール':'同梱サンプル・補助アプリ';
    const implementation=name==='TempoStripMgr.dll'?'限定経路の互換ソースあり':'互換実装なし';
    const comparison=name==='TempoStripMgr.dll'?'6887レコード・443通常ファイル・123コピー・66画像一致':name==='Timeline.dll'||name==='TimeSigStripMgr.dll'?'原版を Tempo 接続試験で使用':'互換実装との比較未実施';
    return {module:name,input:pe.input,sha256:pe.sha256,category,registration:capture?(capture.completed?'専用ハイブで登録成功':'専用ハイブで登録失敗'):'登録観測未実施・対象確認要',implementation,comparison,
      cleanBuild:name==='TempoStripMgr.dll'?'保存35ソースからビルド成功・新規生成物の実行は拒否':'互換実装のクリーンビルド未実施',
      hostAcceptance:name==='DMUSProd.exe'||name==='TempoStripMgr.dll'?'基本編集の候補終了再起動と原版相互読込を確認。新候補のコピー・切取・貼付・保存・Undo/Redo・プロジェクト再読込も確認。再生未検証':'互換実装の本体受入未実施'};
  }).sort((a,b)=>a.input.localeCompare(b.input));
const columns=Object.keys(modules[0]);
fs.writeFileSync(path.join(root,'docs/analysis/implementation-status.csv'),columns.map(quote).join(',')+'\n'+modules.map(m=>columns.map(k=>quote(m[k])).join(',')).join('\n')+'\n');
const lines=['# 全体再構築の検証状態','','更新日：2026-10-02。取得済み40 PEモジュールの暫定管理表。対象範囲の削減や工程6の完了を示す一覧ではない。',
  '', '識別情報と個別状態は [implementation-status.csv](implementation-status.csv)。依存ライブラリ・同梱サンプルも台帳に残し、必要なビルド・配布条件を今後確定する。',
  '', '| 群 | 現在の状態 | 次の判定 |','| --- | --- | --- |',
  '| TempoStripMgr | 基本操作と新候補のクリップボード操作を本体で確認。ネイティブ4ケースも機能一致。原版TimelineはExport4回後にカウンター4が残り、全体試験のアンロード受入は失敗 | 新候補保存物の原版相互読込・終了再起動、実ドラッグ・複数文書・音声出力、非テンポ保存差分を調べる |',
  '| DMUSProd 本体 | 一時登録後の原版起動、QuickStart読込、テンポ編集・保存・再読込を確認。互換実装なし | 候補 DLL の本体置換を比較し、プロジェクト管理の仕様を復元する |',
  '| その他の Producer モジュール | 識別情報と33 DLLの専用ハイブ登録観測。互換実装なし | 初期化・所有権・文書編集・再生の接続を復元し、最初の置換後に順次比較する |',
  '| MFC42 / MSVCRT / MSFLXGRD | 原版の依存ライブラリとして識別 | 配布と呼出し規約、使用範囲を確認する |',
  '| Farm / StylePlayer | 同梱サンプル・補助アプリとして識別 | 全体受入での役割と依存を確認する |',
  '', '現行の Tempo 比較は原版 Timeline・TimeSigStripMgr と Windows の dmime.dll に依存する。原版 Producer 固有モジュールを使わないビルドと、実プロジェクトの作成・編集・保存・再読込・音声出力の受入は未完了。全体完成の判定は [計画書](../analysis-plan.md) の受入試験を維持する。'];
fs.writeFileSync(path.join(root,'docs/analysis/implementation-status.md'),lines.join('\n')+'\n');
console.log(JSON.stringify({modules:modules.length,withCompatibilitySource:modules.filter(m=>m.implementation==='限定経路の互換ソースあり').length}));

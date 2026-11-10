# Choose DLS Instrumentの内部名と空リスト取消し

画像43・SSP07。175125037Zでコレクション名をファイル名からnative `LIST INFO/INAM`優先へ修正した。名前がない場合は従来のファイル名・通し番号へ戻す。変更は`sequence_part_editor.cpp`の表示処理1ファイル。割当・保存・OK条件は変更していない。

[一次記録](../../work/analysis/ui-dls-empty-cancel/20261011T173102Z/unit-record-v2.json)と[独立最終監査](../../work/analysis/ui-dls-empty-cancel/20261011T173102Z/final-audit-v1.json)を参照。単位は部分成立であり、Enter操作が未確認のため`boundedFinished=false`、`completed=false`、`fullAcceptance=false`を維持する。

| 段階 | 今回の結果 |
| --- | --- |
| 保存候補 | 175125037Z。configure/build/install全exit0、291保存ソースと4出力一致、作業ソース差分0 |
| 関連回帰 | common-open 53、dls-sample-policy 28、dls-articulation 43、sequence-crud 213、計4モード337検査合格。全登録一巡ではない |
| 空コレクション | 内部名DLS Collection1、Instrument 0件、Bank 0/0、Patch16、候補OK無効。Cancel後Organ1、Undo/Redo無効、PRO/SGP/DLP全バイト不変 |
| コレクションなし | Collections/Instrument空、候補OK無効。Cancel後Organ1、Undo/Redo無効、PRO/SGP全バイト不変 |
| 保存・復元 | 空コレクション条件でSave File→Save Project。SGP/DLP全バイト不変、PRO差分はSegmentのfilh更新時刻8バイト内のみ。別プロセスでも内部名/空リスト/Organ1を確認 |
| 終了 | 作者21912、復元10468、コレクションなし22220の3プロセスが通常exit0。実PIDは起動記録を正とする |
| 依存 | コレクションなし停止中の73モジュールはx86 67/WOW64 6、原版40hash一致0。全機能の依存解消やQ2成立ではない |
| 原版保護 | インストールEXEと保護対象6ファイルのhash一致 |

空コレクションはSSP07のPartsProofをコピーし、DLSのcolhを0、linsを空にした明示fixture。waves・INFO・GUID等の他root chunkは独立監査で保持を確認した。ProjectのDLSファイルサイズmetadataは元の値を保持しており、実ファイルサイズとは異なる。このfixtureは原版と同じ空セッションの全操作比較ではない。

正式原版ではOther DLSの途中にInstrument Load Patch Conflictが現れた。Fix/Ignoreを受諾せずEscapeした後のChoose DLS InstrumentはDLS Collection1、空一覧、OK有効だった。Cancel後Organ1/8ノートは保持したが選択表示とTransport選択は変化した。OK確定・割当・原版保存は行っていない。候補の無効OKを原版一致と呼ばず、空OKの効果を未確認で保持する。

以前の170010候補によるコレクションなし保存は、[時刻だけが変わるという監査](../../work/analysis/ui-dls-empty-cancel/20261011T173102Z/no-collection-project-save-audit-v1.json)が不合格だった。独立解析では入力PROのfilhが2894バイト、入力SGP実体が2902バイトであり、明示保存時に時刻に加えてサイズが2902へ更新された。Cancel時点の全バイト不変と区別し、以前の不合格を合格へ変更しない。

Computer Useでは座標操作が成立する一方、キー/文字入力の結果を確立できず、復旧後もEnterを有効な入力として評価できなかった。原版の空OK、同条件の全操作比較、厳密外観、名前なしfallbackの本体確認、実DLS音色は未確認で保持する。Cancelは新しい音色を試聴しないため、この単位の音声は非適用。

次はSSP08通常速度のDLS発音中Stopを、新しい候補と改善した時刻制御で検証する。旧通常速度2録音の操作期限失敗と50%条件16/16を分離し、閾値を変更しない。SSP04親ドラッグ、Quantize原版動的比較/複数Part、Add Tracks原版空OK、Count-In、Style015244、Q2、47native/29driver凍結と全体8受入の未達を維持する。

# Band Insertと埋込Band編集の限定統合

2026-10-10。SSP02の画像9〜11は候補131728で入口・編集・native保存・別プロセス復元まで限定成立した。原版との同一入力による動的比較と厳密な外観比較は未実施で、単位全体とSSP02は未完了、fullAcceptance=false。0735の限定採用と015244の独立Style未完は変更していない。

Band行の空白位置で右メニューInsertまたはInsertキーを使うと、その拍に埋込Bandを作りBand Propertiesを開く。名前、符号付きのStart Time、Belongs Toを編集でき、ダブルクリックで所有Segmentに結び付くMDI子画面を開く。16行の初期状態は無効、PCh10はStandard、他はPiano 1。16は新規画面の初期行数であり内部PChannelの上限ではない。子画面のCtrl+Z/Ctrl+YとPropertiesの変更は一つの所有Segment履歴を使う。

初期候補123249では短いBand名の右側の空白をクリックしても既存Bandが選択された。固定150論理pxのhit領域が原因だったため、描画とhitを同じフォントの実測幅にそろえ、次イベントと表示範囲で切り詰めた。修正候補131728で第5小節の空白が12288clocks・No event selectedとなり、Insertで新Bandをその位置へ追加できることを確認した。旧候補と失敗操作、初期のfixture名/保存先不一致、native relocationの明示guardは保存している。

| 検証 | 候補131728の結果 |
| --- | --- |
| 保存ソースと生成物 | 257保存ソースと作業木が一致、4出力不変。configure/build/installは各exit0 |
| 関連回帰 | 7モード520確認合格。Timeline117、Mute99、Sequence26、ComponentName70、Tempo19、Meter65、SegmentCreation124。Style/Bandの既知OS拒否2モードは起動せず障害を保持 |
| 本体操作 | Context Insert、Insertキー、自動Properties、名前b、子画面からUndo/Redo、空白位置hit、開始位置変更、占有論理位置でInsert無効を確認 |
| 保存と終了 | GuiBandRepair.pro/Seed.sgpを本体でnative保存。作者PID21632、別プロセス復元PID1772は双方通常exit0。復元後のファイルhash不変 |
| 復元 | bとBand1、Band1のStart6:1:-1/Belongs5:1、b子画面の16無効行を確認 |
| 独立native監査 | 2イベントの論理/物理時刻、独立GUID、全32楽器headerの無効状態、他Segment chunk/trackとProject metadata・参照・所有GUIDの保持をraw RIFFから確認 |
| 音声 | 今回は無効Bandの作成/名前/位置/子画面の範囲。発音の適用対象はなく未実行。楽器有効化/試聴/ミキサー音声はSSP03/05へ保持 |

HelpのInsert a New Band、Band Properties、Ensuring Timely Band Changesから、論理所属拍と発音開始拍−1tick、独立コピー、新規16無効PChannelを契約にした。GM/default metadataは公開SDK flagsによる実装で、原版native bytes一致の証拠ではない。pan64/volume100は無効値であり、原版の初期配置・byte値一致を主張しない。

実装はBandDocument::inactive_default、SegmentDocument::insert_default_band/update_band、埋込所有先のidentity/time adapter、Timelineコマンド、共有Properties、MDI子画面へ分けた。普通のBandDocumentコンストラクターは空のまま。不正/無変更の更新はbytes/dirty/Redoを変えず、未知データ・他group・他Bandを保持する。初期モデル診断5モード230確認は保存候補の正式結果と分離し、旧123249の7モード520確認も新候補へ転用していない。

残件は、原版同一入力/操作と外観の比較、SSP02画像44→46→47のLength再編集、SSP03楽器有効化とSSP05 pan/volume、完全同一論理時刻の複数Band/Cycle Bands、cut/copy/paste、全PChannel Properties。画面の論理Bar/Beat欄のUIAラベルがStart Time/Belongs Toになるアクセシビリティ差も残す。Deleteは実装/モデル確認があるが今回本体での削除検証はしていない。全97native/170driver一巡、凍結OS/Clipboard拒否、独立Style音声、Q2、全40/全8も未達を維持する。

次は画像44→46→47のSegment Properties→Lengthを既存の長さモデルと作成dialogへ接続する。Bandの原版/外観残差は削除せず継続管理する。

一次証拠: [開始](../../work/analysis/ui-segment-band/20261010T120126426Z/start-v2.json)、[ソース](../../work/analysis/ui-segment-band/20261010T120126426Z/source-changes-v6.json)、[保存build](../../work/build/product-snapshot/20261010T131728812Z/build-summary.json)、[ソース/出力照合](../../work/analysis/ui-segment-band/20261010T120126426Z/closing-build-identity-v2.json)、[関連回帰](../../work/acceptance/regression/20261010T132433872Z/run.json)、[本体保存/復元](../../work/analysis/ui-segment-band/20261010T120126426Z/gui-save-reload-proof-v2.json)、[独立RIFF監査](../../work/analysis/ui-segment-band/20261010T120126426Z/gui-native-audit-v2.json)、[作業状態](../../work/analysis/ui-segment-band/20261010T120126426Z/working-state-v4.json)。

再現は保存候補に対してscripts/Test-RegressionManifest.ps1の登録modeを実行し、scripts/Test-ProductProjectGui.ps1にGuiBandRepair.proとSeed.sgpを渡して別プロセスを起動する。独立監査はnode work/analysis/ui-segment-band/20261010T120126426Z/inspect-gui-native-v2.mjs。Git対象外workとscreenshotを別環境へ保全する際はhash照合も行う。旧proof-v1の共有actions.json参照は継続追記後に変化したため、完全一致する旧100操作のbytesを別ファイルに固定し、historical-gui-action-binding-v1.jsonで元hashへ結んだ。新proof-v2は専用の固定操作履歴を参照する。

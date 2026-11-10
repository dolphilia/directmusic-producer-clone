# 共通 Edit 履歴の本体検証

候補 `20261008T230634466Z`。限定採用0735は維持する。Edit > Undo/Redo と Ctrl+Z/Ctrl+Y を active Segment/Style/Band の既存履歴へ接続し、履歴が空ならメニューとボタンを無効にした。通常の文字入力、modal dialog、modeless Properties のキーを main 文書履歴へ送らない。対象は UI01.edit-history、DMUSProd.exe / SegmentDesigner.ocx / StyleDesigner.ocx / BandEditor.ocx。

原版基準は [Edit Menu](../../work/analysis/help/htm/editmenu.htm)（SHA256 779075fec5e22bef9a7d01ef7b6cbfe6b6bd8900cb0d605a8f2785b14033345c）。各編集画面が独立履歴を持つこととキー割当は help 由来。原版の動的操作・配置は approval timeout のため未観測。200%表示設定を維持し、各画面の実寸と状態は一次観測に保存した。原版と同条件の font/theme/DPI 比較は未成立。

[開始条件と七終了条件](../../work/analysis/ui-edit-history/20261008T225958147Z/unit-start.json)、[現行単位記録](../../work/analysis/ui-edit-history/20261008T225958147Z/unit-record.json)、[固定 GUI protocol](../../work/analysis/ui-edit-history/20261008T225958147Z/gui-protocol-v1.json)を正本とする。Clipboard/全選択/他編集画面、全 Toolbar/MDI/複数 Project はこの単位の完成範囲へ加えない。

| 終了条件 | 現候補の結果 |
| --- | --- |
| 保存ソース・build・install・生成物の結合 | 限定合格。229保存/作業ソースと4生成物を照合。configure/build/install各0 |
| 関連回帰 | 実行12合格/既知 jazp 障害1。実行12は各通常exit0、入力不変。新規全92/164一巡ではない |
| 本体メニュー/キー/無効表示/active editor | 限定合格。Segmentメニュー操作、三文書Ctrl+Z/Y、空履歴無効、Tree caretが別文書でもactive editorへ操作 |
| 履歴分離/分岐/保存/focus | 一部成立。Style分岐は自分のRedoだけを消し、Source/BandのRedo保持を別GUIで確認。保存は履歴を保持。文字の削除Undoとmodal/modeless隔離は確認。意図した非空文字列への置換は未観測 |
| native保存/通常終了/別process復元 | 限定合格。独立native監査8合格、指定三process通常exit0、補足processもexit0。別起動では履歴を保存せず値を復元 |
| 現候補のPlay/発音中Stop/無音/replay | 障害あり・未実行。録音先filenameのComputer Use cache障害の解除証拠なし。2137音声は転用しない |
| 原版の外観/操作比較 | 障害あり・未観測。既存approval timeoutを保持 |

[成功build](../../work/build/product-snapshot/20261008T230634466Z/build-summary.json) SHA256 3716fa2edbaa03a6407a143f1be3549488691d6c1767abb99281fe184ef3baf2、Producer.exe SHA256 28fe572961eb1b44924cd8a182aea549e05150d5e8ed50079c47432901d027d4。[関連13 run](../../work/acceptance/regression/20261008T231147380Z/run.json) SHA256 a91e37ba7f164e8a6b5c88e3ded45deb5140fc054e67ba9ccf7c298e4cc47293。component-name既存modeへ三文書履歴/保存/no-op/分岐の検証を追加し44 checks。登録数92/164は維持。2137全群の原始一巡とこの関連実行を分ける。

本体入力は保存済みFresh12 + MultiCapture5 と自分のEmpty1の18ファイル。Segment第一Tempo8→9（第二7.5、8notes、名前不変）、Style113→117（Pattern1/groove/length/name不変）、Band patch0→1（PChannel0/pan64/volume101/名前不変）のみを保存。Projectは三つのfilh保存時刻メタデータだけが変わり、他の参照と未知chunkを保つ。[独立native-proof](../../work/analysis/ui-edit-history/20261008T225958147Z/native-proof.json) SHA256 84c45ff7ef6142b9f4888db888cb8a851dcc706c2c85b32713060fbeb49ac9a1。audit期待値の対象件数5→指定三終了件数3は実行前に訂正し、訂正前スクリプトと理由を保全した。初回失敗processを通常終了の対象へ含めない。

| 自分のGUI session | PID | 結果 |
| --- | --- | --- |
| Empty | 21652 | 空履歴disabled、既存path Save、通常exit0 |
| author attempt1 | 17148 | 15分launcher timeoutと同時刻にexit4294967295。保存前18入力不変、通常終了は不成立 |
| author attempt2 | 11936 | 三文書既存path保存/保存後UndoRedo/Save As取消し/Project保存、通常exit0 |
| reload | 17896 | 9/7.5・117・patch1/参照/名前を復元、履歴disabled、18入力不変、通常exit0 |
| branch supplement | 5244 | Source9→10 Undo9、Band1→2 Undo1、Style117→118 Undo117→119分岐。Source/Band Redo保持を実操作し全三文書saved状態へ戻す。18入力不変、通常exit0 |

各PIDは当時のEXE/hash/start/windowから同定した履歴であり、以後の入力対象に固定しない。[actions](../../work/analysis/ui-edit-history/20261008T225958147Z/actions.json) と [observations](../../work/analysis/ui-edit-history/20261008T225958147Z/observations.json) に入力時刻・即時結果・更新結果・画像を保全。UIAのmenuクリックが別位置へ届いた観測も保持し、更新画像を基準に一度復旧した。silent no-opを保存成功へ数えない。

author1の15分終了はlauncher timeoutと相関するが原因未確定。driverにKillはない。observerのforcedTermination=false固定値は外部停止なしの証明にならない。driverへTimeoutSeconds（既定0、通常終了まで待つ）を追加し、二回目以降の通常終了を新証拠で確認した。driver-only変更は229製品ソースの対象外。2301の試験const-save compile失敗、誤ったdiagnostic path、置換入力の未観測とUIA cacheエラーは記録に保持した。SDK/OS/製品Properties不具合と断定しない。

[現候補の音声準備未達](../../work/analysis/ui-edit-history/20261008T225958147Z/playback/preparation-blocked-v1.json) と [文字入力観測](../../work/analysis/ui-edit-history/20261008T225958147Z/text-input-observation-v1.json) に解除条件を保存した。同条件のfilename cache操作や原版起動を反復しない。前候補でPlay/Stopが成立していてもこの候補の必要音声は未確認。

この単位は `completed=false / fullAcceptance=false`。全8は6作業中/2障害/0合格、全40のUI inventoryも未確定。Properties/File/Editの連続単位で入口、保存対象/無効表示、キー/historyの製品差分は減った。残る共通Edit clipboard選択操作を次の独立単位として固定し、既存Tempo/Pattern clipboardを活かす。未達音声/原版比較は必要証拠の解除まで保持する。

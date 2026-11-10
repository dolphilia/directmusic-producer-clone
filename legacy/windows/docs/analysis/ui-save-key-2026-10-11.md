# 保存キーとメニュー終了状態の限定調査（2026-10-11）

114844を再利用した画像1の保存キー調査。Edit→Escape一回→Ctrl+Sの無反応を新規/既存Segmentで再現し、原版の既存検証Segmentでも同じ反応を確認。二回目のEscape又は編集領域へのクリック後は保存成立。製品固有差を確認できずソース/動作変更なし。288ソース/4出力一致、GUI7条件・native9条件・5ファイル別復元成立、作者19964/別24796二通常exit0。143 modules=137x86+6WOW64、原版40hash一致0。内部メニュー状態の因果は推論、全操作/厳密外観未達。

## 観測と判断

| 対象 | 操作 | 結果 |
| --- | --- | --- |
| 再構成版・未保存SaveKey26 | Edit→Escape→Ctrl+S | Save Asは開かない |
| 同文書 | もう一度Escape→Ctrl+S | Save Asが開き、専用コピーへ保存成立 |
| 再構成版・保存済みSaveKey26にTempo追加 | Edit→Escape→Ctrl+S | 未保存印と保存前ハッシュが維持 |
| 同文書 | 編集領域クリック→Ctrl+S | 未保存印消失、Tempo120を保存 |
| 原版・検証用Segment2にTempo追加 | Edit→Escape→Ctrl+S | 保存前ハッシュ維持 |
| 同文書 | もう一度Escape→Ctrl+S | Tempo120を実ファイルに保存 |

この操作列で再構成版固有の差は確認できなかった。メニューバーの選択状態が残ることと整合するが、内部メニューループ/フォーカスを計測していないため因果は断定しない。保存キー実装を変更せず、以後の検証はメニューを完全に抜けるか編集領域へ戻った状態を明記する。原版はNew Files時点で実ファイルを作るため、未保存新規Save Asについて原版との完全同条件比較は成立していない。

原版で二回目のEscape後に保存した直後のCtrl+Zはトラックを取り除かなかった。編集領域を明示的にクリックしてからCtrl+ZするとTimeSig-onlyへ戻った。失敗を含む全操作と中間ファイルを保存した。最後のSegment2は開始時とバイト同一ではなく、変更leafはLIST:sgdl/segdの編集表示情報のみ。GUID、長さ、TimeSig、名称、版番号と他leafは一致する。既存Scenario.pro/Segment1.sgp/ScenarioTone.dlp、正式原版EXEとPID25548は保全し、原版Projectの未保存参照を破棄していない。

## 証拠と検証範囲

- [開始条件](../../work/analysis/ui-save-key/20261011T122028998Z/start-v1.json)
- [GUI・保存・別復元・終了](../../work/analysis/ui-save-key/20261011T122028998Z/gui-save-reload-proof-v1.json)：7条件。作者PID19964、別PID24796、ともに通常exit0。
- [native監査](../../work/analysis/ui-save-key/20261011T122028998Z/native-audit-v1.json)：9条件。新規24小節/73728clocks、TimeSig4/4、Tempo120、Project4参照。新規文書の最初の保存からの変更はTempo追加のみ。既存3Segmentは不変。
- [モジュール監査](../../work/analysis/ui-save-key/20261011T122028998Z/module-proof-v1.json)：作者の一点採取143件、x86 137/WOW64 6、原版40hash一致0/未解決0。連続監視やクリーンOS証明ではない。
- [closing identity](../../work/analysis/ui-save-key/20261011T122028998Z/closing-identity-v1.json)：候補20261011T114844643Zの288保存/作業ソースと4出力が一致。EXE SHA256 `71a4a0487835c07c183167205872fbf36fbd0d2b8fd090b43fa5d0ccec40b4b3`。
- [保全](../../work/analysis/ui-save-key/20261011T122028998Z/preservation-v1.json)、[再構成版操作](../../work/analysis/ui-save-key/20261011T122028998Z/candidate-actions.json)、[原版操作](../../work/analysis/ui-save-key/20261011T122028998Z/original-actions.json)。

ソース未変更のため新しいbuildを作らず、同じ114844の関連3モード370検査は[前単位](ui-add-tracks-2026-10-11.md)の証拠として保持した。今回、新しいnative100/driver172一巡を実行したとは扱わない。音声は非適用。全40責務・全8受入、厳密外観、全保存入口の一致は未達。限定採用0735、独立Style015244、Count-In15/16、旧Band停止失敗とOS凍結を維持し、fullAcceptance=false。

## 次の独立単位

画像44→46→47のSegment Properties→Length経路を原版と同じ保存済み検証入力で比較し、名前/Length入口/初期値/OK/Cancelの操作差を特定する。必要な差だけ修正し、Length変更/取消し/UndoRedo/native保存/別process復元/通常終了を固定する。45番はTimeSig誤撮影として除外。Add Tracks初期focus・原版空選択、全外観、旧Band停止失敗・Count-In15/16・Echo/SSP04/SSP08・Style015244/Q2/OS凍結は保持。

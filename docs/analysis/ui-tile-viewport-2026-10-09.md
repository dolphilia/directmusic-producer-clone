# Timelineのタイル配置と縦スクロール — 1021候補の限定検証

2026-10-09。4K・OS表示200%を変更せず、横タイルで下部ボタンが欠ける問題と、縦タイルで選択位置が折り返して欠ける問題を修正した。GUI7項目、native保存6項目、別プロセス復元、二通常終了、固定WASAPIと5故障対照が成立。原版の動的比較・全40責務・全8受入・Q2は未完のため、`completed=false` / `fullAcceptance=false`を維持する。0735限定採用と2137原始一巡は変更しない。

## 変更と根拠

変更は`src/producer/segment_timeline_window.cpp`のみ。選択位置と4ボタンを下部の別行に固定し、上部rulerと下部操作領域の間でstripを縦スクロールする。描画とクリック位置に同じscroll量を適用する。最小文書寸法を設定し、既存Sessionの履歴・保存・再生を使う。

原版helpの内部編集ウィンドウ、Windowメニュー、Segmentのruler/track構成を根拠とする。今回の伸縮寸法とスクロール設計は実装上の推論であり、原版の字体・アイコン・画素一致を証明しない。Win32のスクロールAPIは[Microsoft SetScrollInfo](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setscrollinfo)を参照した。

## 同じ候補に結び付けた結果

| 条件 | 結果と証拠 |
| --- | --- |
| 候補 | `20261009T102121693Z`、[build-summary](../../work/build/product-snapshot/20261009T102121693Z/build-summary.json)。234保存/作業ソース、4出力、472照合が一致 |
| Producer SHA-256 | `bed30a29a6e59caf5a0349223812da0334716de9584301a8fbec3f681f7cf839` |
| 変更ソース SHA-256 | `47137fa170676d23177655eb5f03f51360ce3540c3bf19c5710d6205c94ca8bd` |
| 関連回帰 | [native40](../../work/acceptance/regression/20261009T102653894Z/run.json)は36合格/4既知障害凍結。[4製品driverの結び付け](../../work/analysis/ui-tile-viewport/20261009T101806444Z/driver-binding-v1.json)も成立。原始一巡とは別の補足 |
| 表示と操作 | [GUI7](../../work/analysis/ui-tile-viewport/20261009T101806444Z/gui-proof-v1.json)、[表示確認](../../work/analysis/ui-tile-viewport/20261009T101806444Z/visual-layout-proof-v1.json)。横・縦タイルで両文書の4ボタンとSong位置が完全表示。scroll後のEnter選択/F11、canvasとFitボタンからCtrlZ/Y、CtrlTabで文書別履歴を確認 |
| 保存 | [native6](../../work/analysis/ui-tile-viewport/20261009T101806444Z/native-proof-v1.json)。Song Enterの10217→10233 clocksのみ変更。他のSong bytes、Other、Effectは同一。ProjectはSongのsize/write metadataのみ変更 |
| 復元と終了 | author PID23484 / reload PID6412、同じEXEで別起動し4:2:2:57を復元。両方通常exit0・強制終了なし。[終了後12入力照合](../../work/analysis/ui-tile-viewport/20261009T101806444Z/postexit-input-integrity-v1.json)一致 |
| 録音 | [固定protocol v2](../../work/analysis/ui-tile-viewport/20261009T101806444Z/audio-protocol-v2.json)、[実PCM判定](../../work/analysis/ui-tile-viewport/20261009T101806444Z/audio-proof-v2.json)。8音/3tempo、発音中Stop、無音、再開が成立 |
| 異常検出 | [5対照](../../work/analysis/ui-tile-viewport/20261009T101806444Z/audio-control-qualification-v2.json)。全無音、過大peak、停止後信号、入力hash不一致、早すぎるStopをそれぞれ意図したguardで拒否 |

初回Stop範囲は4.715〜4.958秒、再開後は4.705〜4.957秒で固定4.2〜6.3秒内。停止後・録音末尾・開始前RMSは0、peakは0.150338。録音は48kHz stereo float32、64秒。UTC/QPC driftは約4μs。6観測すべてで実際の再生対象Song.sgpを結び付けた。

## 保持した失敗と記録補正

[初回音声v1](../../work/analysis/ui-tile-viewport/20261009T101806444Z/audio-proof-v1.json)は操作セル間の遅延でStopが11秒/10秒となり、固定基準に不合格。PCM・操作時刻を保持し、成功へ転用しない。v2は一セル一入力と即時観測を保ち、別セルへの移行前に実Song/Stopの構造化観測を確認する。閾値は同一。

Undo直後に同一セル内で取得したaccessibility値はキャッシュが古く、別セルの新しい観測と画像で10217を確認した。[判定器の観測参照補正](../../work/analysis/ui-tile-viewport/20261009T101806444Z/gui-auditor-cache-correction-v1.json)は初回実行前で、期待値は不変。[保存コピーのpaired-row形式補正](../../work/analysis/ui-tile-viewport/20261009T101806444Z/input-pair-format-correction-v1.json)も元データ・hashを保持する。

## 残差と次の作業

原版の動的比較は既存approval timeoutで障害あり。同条件を再試行せず、help由来の期待と実装の推論を分ける。CtrlF11の全focus cycle、ShiftF11、Main dropdownと開いたMDIの活性化、AltW mnemonic、全module MDI、OLE/clipboard/strip管理、原版の字体・アイコン・配置・全40画面一覧は残る。

次は原版helpの共通UI契約からProject Tree表示/focusと文書活性化を実装・検証する。Q2独立Windows、Windows5/Clipboard5等の個別障害と各機能の残責務を維持する。

[封印記録](../../work/analysis/ui-tile-viewport/20261009T101806444Z/unit-1021-limited-v1.json) SHA-256：`ee4c4fee090102dedbaf2af600ee2811ef645ace5ebab32727b169a17bab4710`。

# 共通UIのフォーカス・Project Tree・文書活性化 — 1107候補の限定検証

2026-10-09。4K・OS表示200%を変更せず、Ctrl+F11のフォーカス移動、Shift+F11のTree表示切替、Mainの文書一覧から開いたMDIへの活性化を接続した。GUI8、native6、二通常終了、別プロセス復元、固定WASAPIと5故障対照が成立。原版の動的比較・全40責務・全8受入・Q2は未完。0735限定採用、2137原始一巡、`completed=false` / `fullAcceptance=false`を保持する。

## 根拠と変更

原版helpのkeyboardshortcuts.htmにあるF11・Ctrl+F11・Shift+F11・Ctrl+Tabと、Project Tree/Viewの記述を静的契約とした。閉じたPropertiesを開かずに飛ばす、隠したTreeから編集画面へフォーカスを移す、Tree再表示時にcaretを保持する、開いているSegment MDIを文書一覧から活性化する方針は実装上の推論。原版の動的動作と同じとは判定していない。

変更はmain、project_tree、properties_window、segment_timeline_windowの7ソース。修飾キー付きF11をplain F11から分け、Tree→Editor→表示中Properties→Treeを実装した。Propertiesを新しく開いたり選択を変更したりせず、現在の入力欄へ移る。文書一覧の実CBN_SELCHANGEで該当する既存MDIを活性化する。Win32技術の確認には[SetFocus](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setfocus)と[Microsoft MDI文書](https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/winmsg/about-the-multiple-document-interface.md)を使用した。

## 同じ候補の証拠

| 条件 | 結果 |
| --- | --- |
| build/install | [1107 build](../../work/build/product-snapshot/20261009T110740409Z/build-summary.json)、234保存/作業ソース・4出力・472照合一致。Producer SHA-256 `e11c745a873444e8feb3898635fc38251f438448da710d2f53426f06c8279312` |
| 関連回帰 | 初回native40は28合格/4既知障害凍結/8失敗。失敗8だけをレビュー済み通常hostで実行し全8合格。実効36合格/4凍結。[結び付け](../../work/analysis/ui-common-focus/20261009T110626261Z/regression-binding-v1.json) |
| 製品driver | 同じ候補の[4driver](../../work/analysis/ui-common-focus/20261009T110626261Z/driver-final-binding-v1.json)が合格。driverコピーと入力を照合 |
| 本体操作 | [GUI8](../../work/analysis/ui-common-focus/20261009T110626261Z/gui-proof-v1.json)。閉じたPropertiesのskip、表示中3pane cycle、Tree hide/showとhidden pane skip、実dropdown Song/Other活性化、F11編集、canvas CtrlZ/Y、CtrlTab、別process復元 |
| 現在の表示 | [4画面](../../work/analysis/ui-common-focus/20261009T110626261Z/visual-layout-proof-v1.json)。Tree表示/非表示と横/縦タイルで位置・4ボタンを表示。原版parityは未確認 |
| native保存 | [6項目](../../work/analysis/ui-common-focus/20261009T110626261Z/native-proof-v2.json)。Song Enterだけ10233→10249 clocks（Tick57→73）。他のSong bytes、Other、Effect不変。ProjectはSong size/write metadataのみ変更 |
| 復元・終了 | author PID19160 / reload PID21132、同じEXEで別起動し4:2:2:73と履歴空を確認。二通常exit0・強制終了なし。[終了結び付け](../../work/analysis/ui-common-focus/20261009T110626261Z/normal-exit-binding-v1.json)、[12入力照合](../../work/analysis/ui-common-focus/20261009T110626261Z/postexit-input-integrity-v1.json) |
| 音声 | [固定protocol](../../work/analysis/ui-common-focus/20261009T110626261Z/audio-protocol-v2.json)、[実PCM](../../work/analysis/ui-common-focus/20261009T110626261Z/audio-proof-v2.json)。8音・3tempo・発音中Stop・無音・再開成立。64秒48kHz stereo float32、peak0.149052、baseline/停止後/末尾RMS0 |
| 異常検出 | [5対照](../../work/analysis/ui-common-focus/20261009T110626261Z/audio-control-qualification-v2.json)。全無音、過大peak、停止後信号、誤入力hash、早すぎるStopを意図したguardで拒否 |

初回Stop範囲4.715〜4.959秒、再開後4.714〜4.966秒で固定4.2〜6.3秒内。UTC/QPC drift約4.4μs。6つの実Main観測すべてでSong.sgpを結び付けた。閾値は1021と同一。

## 保持した失敗と記録補正

既定hostのcanonical Access is deniedを含むnative8失敗と、空stdout/stderrでexit1のidentity/motif旧runを保持した。レビュー済み通常hostで実行条件を変更し、該当8と2driverが合格。OS設定・COM登録・セキュリティ設定は変更していない。Send module physical pathの旧失敗も消していない。

Get-Processで取得した独立watcherのExitCodeはnullだったためv1を保存し、同じPID・EXE・hash・開始時刻の終了を確認した上で、元のProcess handleを保持したlauncherのexit0へ結び付けた。native判定器v1を保持し、v2で結び付け済みの終了記録を参照した。期待値・製品ソース・入力・音声閾値は変更していない。UIAの古いindex/負座標エラーも操作ログに保持し、新しい画像/観測で操作した。

## 残差と次の作業

原版の動的比較は既存approval timeoutで障害あり。同条件再試行をせず、help由来と推論を分けたまま比較未確認を残す。他moduleのmodeless focus、Alt+W/menu、全module MDI/OLE/clipboard、原版font/icon/配置、全40必要画面一覧、Q2および個別Windows5障害は未完。

3つの共通UI単位で実際の不足は減った。次は別責務のWave Snap to Zero/Clone Loopを原版helpと既存PCM coreから照合して、機能・画面・操作の不足を進める。全体受入は6作業中/2障害/0合格のまま。

[封印記録](../../work/analysis/ui-common-focus/20261009T110626261Z/unit-1107-limited-v1.json) SHA-256：`f453dc90956b39fddded40fd02737814613a0e242ca57e4ea51196591dfb6e8e`。

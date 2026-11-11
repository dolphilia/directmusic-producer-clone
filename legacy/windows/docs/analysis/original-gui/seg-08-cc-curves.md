# SEG-08　CC レーン・PBend・Expression

[調査計画](README.md) · [Sequence Part](seg-06-sequence-parts.md) · [ノートと Snap](seg-07-note-gestures.md)

2026-11-11。DirectMusic Producer 5.3.0.900 の調査用コピー `DmusGuiCc.exe`（SHA-256 `c330bb2ca2c885f4157ec9dce060f64957e33e88192b6547630c2db5f1a65a22`）で、[3パート入りの固定素材](../../../work/original-gui/SEG-06-07/20261111T023012247347Z/inputs/three-parts/manifest.json) を複製した。TimeSig 4/4、約150 px/小節、停止中。ツールバーは [Segment 検証用の2段](ui-01-view-toolbars.md)。Windows 11 10.0.26200、1280×800、96 DPI。親子ウィンドウ最大化、ウィンドウ全体の画像は1280×752 px。座標は画像左上から。青い光と黒い矢印は操作ツールの表示。

[実行記録](../../../work/original-gui/SEG-08/20261111T031027800626Z/run.json) · [撮影環境](../../../work/original-gui/SEG-08/20261111T031027800626Z/environment.json) · [画像対応表](../../../work/original-gui/SEG-08/20261111T031027800626Z/published-images.json) · [保存値の照合](../../../work/original-gui/SEG-08/20261111T031027800626Z/saved-curves.json)

## 空の CC と種別追加

1. 第1パートを開く。鍵盤とピアノロールの下に `CC`、時間領域の左端に `No CC Tracks`。右端に小さな四角い開閉ボタンがある。空行を右クリックすると `Add New CC Track...` が選べ、`Draw Curve` と `Properties` は灰色。
2. `Add New CC Track` ダイアログは `PBend - create` が初期選択で、右に OK と Cancel。種別一覧の先頭は PBend、Mono AT、Poly AT、RPN、NRPN。その下に番号付きのコントローラーが並び、11 は `Expression Controller`。追加されていない種別の末尾には `- create` が付く。
3. RPN を選ぶと下段に `RPN/NRPN Type` のスピン付き数値欄が現れ、値0で `Pitch Bend Range` と表示される。Cancel を押すとダイアログが閉じ、淡黄色になった CC 行には `No CC Tracks` が残る。次に同じダイアログを開くと `PBend - create` に戻る。
4. PBend を選んで OK。CC 行は閉じたまま、表示が `PB` に変わる。右端の四角いボタンで展開すると左が `CC: PBend` になり、約82 px の高さに広がる。レーンに薄い水平中心線があり、赤い位置線と縦の時間目盛りはピアノロールとそろう。

![第1パートに空の CC 行がある初期状態](figures/cc-06-expand-first.jpg)

![空の CC 行のメニュー。Add New CC Track が有効、Draw Curve は灰色](figures/cc-07-empty-menu.jpg)

![Add New CC Track の初期状態。PBend - create、OK、Cancel](figures/cc-08-add-dialog.jpg)

![種別の一覧。先頭5種と番号付きコントローラー](figures/cc-09-type-list.jpg)

![RPN を選ぶと下に RPN/NRPN Type と Pitch Bend Range が現れる](figures/cc-10-rpn-fields.jpg)

![Cancel 後。CC 行が淡黄色で No CC Tracks のまま](figures/cc-11-cancel-rpn.jpg)

![PBend 追加後、折りたたみ状態の要約は PB](figures/cc-16-pbend-summary.jpg)

![PBend 展開後。薄い水平中心線とピアノロールと共通の時間目盛り](figures/cc-17-pbend-expanded.jpg)

## PBend の描画とハンドル

1. 展開した PBend の空セル (330, 425) を右クリック。Draw Curve と F11 の Properties が有効。Draw Curve を選ぶと背景が淡黄色になり、下のステータスに `Start, End` が出る。
2. 中心線付近 (340, 403) から右上 (413, 374) へドラッグして離す。第1小節に中心線から上へ盛り上がる赤い曲線ができ、外接する矩形に黒いハンドルが並ぶ。
3. F11 の `PBend Curve Properties` で、曲線の時刻は Bar、Beat、Grid、Tick の4列。初期の Type は `Sine`、Start Value は0、End Value は5938。Reset タブは `Enable Reset` にチェックが付き、Reset Value は0、Reset Duration は 19/2/2/0。閉じて保存する。
4. 選択中の右上ハンドル (402, 376) を (445, 390) へドラッグ。赤い曲線は右へ伸び、上端が中心線に近づく。F11 を開くと直前に見ていた Reset タブが表示される。Curve(s) タブでは Start Time の Tick が0から−5、End Time が1/4/3/−41から2/1/3/67、End Value が5938から3071に変わる。Start Value は0。
5. Type のドロップダウンは Sine、Logarithmic、Exponential、Instant、Linear の順。Linear を選ぶと曲線の上辺が直線になる。始点・終点の値は選択前と同じ。

![展開した PBend の右クリックメニュー。Draw Curve と Properties が有効](figures/cc-18-pbend-menu.jpg)

![Draw Curve 選択後。背景が淡黄色になり、下に Start, End](figures/cc-19-draw-mode.jpg)

![ドラッグを離した後。選択された Sine が赤く描かれ、黒いハンドルが出る](figures/cc-20-pbend-created.jpg)

![描画直後の Curve(s) タブ。Start 0、End 5938、Type Sine](figures/cc-21-pbend-properties.jpg)

![PBend の Reset タブ。Enable Reset が選択済み](figures/cc-22-pbend-reset.jpg)

![右上のハンドルを右下にドラッグした後。曲線の幅が広がり、上端は中心線に近づく](figures/cc-26-pbend-handle-drag.jpg)

![ハンドル操作後の時刻と値。End Value は3071](figures/cc-28-pbend-resized-values.jpg)

![選択可能な5種類の曲線タイプ](figures/cc-29-curve-types.jpg)

![Linear を選択した後。上辺は直線で、選択された曲線は赤い](figures/cc-31-linear-visible.jpg)

## Expression、選択と開閉

1. PBend を開いたまま `Add New CC Track...` を開くと、追加済みの `PBend` から `- create` が消える。`11 Expression Controller - create` を選び OK。PBend の下に展開済みの Expression が追加され、左のラベルは欄の幅で `CC: 11 Express` まで見える。
2. Expression の空セルをクリックするとその行が淡黄色になり、PBend の曲線は青に戻る。Insert で描画モードになり、下に `Start 84, End` と表示される。第2小節の (480, 463) から (565, 502) へドラッグし、下降する赤い曲線を作る。塗りつぶしはレーン下端まで続く。
3. F11 で `11 Expression Controller Curve Properties` が開く。Start 98、End 36、Type Sine。Time は Start 2/2/2/0、End 2/4/3/13。
4. Expression の右端のマイナスを押すと、PBend も同時に畳まれ、1行の `PB, Expr` 要約になる。要約の時間領域をダブルクリックすると2レーンとも開く。
5. D5 をクリックするとノートが赤くなり、2曲線は青。続けて Expression 曲線内をクリックすると、Expression に淡黄色の背景と赤い曲線・黒いハンドルが付き、D5 は青に戻る。

![PBend 追加後の種別一覧。PBend には create が付かず、CC11 は create](figures/cc-34-expression-in-list.jpg)

![Expression 追加直後。PBend の下に2本目の CC レーンが現れる](figures/cc-36-expression-lane.jpg)

![Expression にフォーカスして Insert を押した状態。下に Start 84, End](figures/cc-38-expression-draw-mode.jpg)

![Expression 曲線の描画直後。下降する赤い曲線とハンドル](figures/cc-39-expression-created.jpg)

![Expression の Curve(s) タブ。Start 98、End 36](figures/cc-40-expression-properties.jpg)

![Expression 行の右端ボタンを押した後。2本とも折りたたまれて PB, Expr と表示](figures/cc-42-expression-minus.jpg)

![PB, Expr をダブルクリックすると2レーンとも展開される](figures/cc-43-double-click-summary.jpg)

![D5 をクリックした状態。ノートは赤、曲線は青](figures/cc-45-note-selection.jpg)

![Expression をクリックした状態。曲線が赤になり、D5 は青に戻る](figures/cc-46-curve-selection.jpg)

## 取消、保存、別プロセスでの確認

Expression の空セル (624, 472) で Insert の後、描き始める前に Esc を押した。ステータスの描画待機表示は残り、その後の (624, 472) → (698, 500) で3本目ができた。Ctrl+Z で3本目が消え、再保存した SGP は2本の保存時と全バイト一致した。

![Insert、Esc の順に押した後のドラッグ。3本目が赤く選択される](figures/cc-50-post-escape-drag.jpg)

![Ctrl+Z 直後。PBend と最初の Expression だけが残る](figures/cc-51-undo-extra.jpg)

第1パートを畳み、第2パートを展開すると、第2パートの CC は `No CC Tracks`。この状態で親ウィンドウを閉じると、Segment1 の保存確認が出た。Yes で保存して終了。別プロセスで Project2 と Segment1 を開くと、第2パートが展開された状態が復元された。第1パートを展開すると、以前の PBend と Expression が開いた状態で戻る。両方の F11 を照合し、保存時の時刻、値、Type を再確認した。

![第2パートの展開時。第2パートの CC 要約は No CC Tracks](figures/cc-54-part-two-empty.jpg)

![表示状態を変えて終了すると出る保存確認。Yes、Yes All、No、No All、Cancel](figures/cc-55-close-state.jpg)

![再起動して開いた直後。最後に保存した第2パートの開閉状態が戻る](figures/cc-58-reopen-segment.jpg)

![再起動後に第1パートを開いた状態。元の2曲線と第2パートのロールが見える](figures/cc-59-reopen-first-part.jpg)

![別プロセスで確認した Expression。Start 98、End 36、Sine](figures/cc-61-reopened-values.jpg)

![別プロセスで確認した PBend。Start 0、End 3071、Linear](figures/cc-62-reopened-pbend-values.jpg)

| SGP と対象 | 開始 clocks | 長さ clocks | 画面の始点 → 終点 | 保存した始点 → 終点 | 曲線タイプ |
| --- | ---: | ---: | --- | --- | --- |
| 初回保存 PBend | 1152 | 1495 | 0 → 5938 | 8192 → 14130 | Sine |
| 2曲線保存 PBend | 1147 | 2376 | 0 → 3071 | 8192 → 11263 | Linear |
| 2曲線保存 Expression | 4032 | 1741 | 98 → 36 | 98 → 36 | Sine |

2レコードとも最初の Sequence の `seqt/curl`、PChannel フィールド0。元の4音の `evtl` はすべての保存で一致した。終了時に Yes で再保存した SGP とその前の SGP を比較すると、RIFF のデータ部分で変わったのは第1・第2パートの `psqc`。曲線2本を含む [次回用素材](../../../work/original-gui/SEG-08/20261111T031027800626Z/inputs/two-curves/manifest.json) は両レーンを開いた段階の保存を使う。

## 追試する条件

押下中の破線プレビューとポインター、中心ドラッグ、複数曲線の選択、値の上下限、レーンの高さ変更、削除と復元、音への効果を調べる。Esc の試験は描画待機中の1例で、押下中の取消は未観測。対応する画像要件は [UI 互換性台帳](../ui-compatibility.json) の `UI02.priority-29`〜`37`。

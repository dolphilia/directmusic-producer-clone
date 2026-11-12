# SEG-14　Wave 文書のループ端と Segment 項目への継承

2026-11-12、DirectMusic Producer 5.3.0.900 の作業コピーで確認した。元の **SfxCow.wvp** は44.1 kHz、16 bit、Mono、63,868サンプル。参照先は Clock Time の **SfxCow.sgp** にある既存項目と、音楽時刻の **Segment1.sgp** に追加した項目。Wave 文書の単独試聴と各 Segment の再生を別々に開始して、2本の WAV に収録した。画像は1280 × 752 px の31枚。太い黒矢印と青い光は操作ツールの位置表示。 [比較素材と凍結入力](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/run.json) · [掲載画像と操作記録](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/published-wave-document-loop-figures.json)

## Wave エディターで選択からループを作る

1. `SfxCow.sgp` を開き、項目の **Wave Properties → Loop** を表示。Loop はオフで、Samples の **Start 0、End 63,867、Length 63,868** は灰色。
2. Project Tree の `SfxCow.wvp` を展開して **SfxCow** Wave 子ノードを開く。Transport の対象が **SfxCow** になり、エディターのタイトルが **[Wave: SfxCow]** に切り替わる。黒の波形、中心線、下端の横スクロール、右下の虫眼鏡が見える。ステータスバーは **Zoom 1:64**。Wave 文書の Properties は **Wave、Info、Compression/Streaming** の3タブ。
3. 波形面の `(527, 360) → (699, 360)` を左ドラッグする。選択範囲の背景が赤になり、黒い波形が上に描かれる。Properties の **Selection** は **22,008〜33,010**、**11,003 samples**。両端とも隣接する元 PCM サンプルで符号が変わる位置だった。
4. 赤い範囲を右クリックすると **Snap To Zero** にチェック。**Set Loop From Selection** を選ぶと **Loop** がオン、方式は **Forward**。選択と同じ範囲が上のループ欄に入る。波形の先頭付近をクリックして選択を外すと、赤い面が消え、緑の始点と青の終点が残る。
5. **File → Save SfxCow.wvp** で保存する。元の PCM、形式、GUID は保存前と一致した。

![編集前の既存 Segment。項目の Loop はオフ、全サンプル範囲が灰色](figures/waveloop-11-existing-loop-before.jpg)
![ファイル行を開いて子ノードを選んだ状態。Segment は表示されたまま](figures/waveloop-12-wave-document.jpg)
![Wave 子ノードから開いた Wave Editor。Zoom 1:64、黒い波形と緑・青の端](figures/waveloop-13-wave-editor.jpg)
![範囲選択。赤地の区間と Selection の Start、End、Length](figures/waveloop-14-wave-selection.jpg)
![Wave の右クリックメニュー。Snap To Zero にチェック、Set Loop From Selection が有効](figures/waveloop-15-selection-menu.jpg)
![Set Loop From Selection 後。Loop と Forward、22,008〜33,010](figures/waveloop-16-set-loop-from-selection.jpg)
![赤い選択を外した状態。緑の始点と青の終点が残る](figures/waveloop-17-wave-loop-lines.jpg)
![File メニューの Save SfxCow.wvp](figures/waveloop-18-wave-save-menu.jpg)

**Play From Start** で Wave 文書を単独試聴すると、黒い縦の再生位置線が出た。録音では、元のサンプル長を過ぎてもループ区間の音が続いた。**Stop Immediately** で再生位置線が消える。 [同梱ヘルプのループ端操作](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/settinglooppoints.htm)

![Wave 文書の単独試聴中。緑と青の間に黒い再生位置線](figures/waveloop-20-document-loop-play.jpg)
![停止後。再生位置線が消えてループ端は残る](figures/waveloop-21-document-stop.jpg)

## 既存項目と新規項目

**既存の SfxCow.sgp** に戻ると、Loop のチェックはオフのまま、灰色の Samples に **22,008〜33,010** が表示された。Wave 文書を保存した時点では `SfxCow.sgp` は変更前とバイト単位で一致した。その後、項目がオフの状態で Segment を保存すると、表示された範囲は編集用 `wvcu` に入り、再生用 `waih` は **0 / 0**。項目の Loop をオンにして保存すると、同じ範囲が `waih` にも入った。タイムライン後半の波形も反復する形に変わった。

**新規の Segment1 項目**は、Variation 1 を単独で選び、Tree の .wvp ファイル行 `(95, 470)` から PChannel 1 のレイヤー a `(282, 211)` へドロップした。開始は **1:1:2:0**（192クロック、125 ms）。追加直後から Loop がオンで、Samples は **22,008〜33,010**。項目の Loop を編集せず保存し、`waih` と `wvcu` の両方でこの範囲を確認した。

![既存項目に戻った直後。Loop オフのまま灰色の範囲が変わる](figures/waveloop-22-existing-after-return.jpg)
![項目の Loop をオンにした直後。タイムライン後半が反復波形になる](figures/waveloop-25-existing-enable-loop.jpg)
![Wave ファイル行から Segment1 にドロップした直後](figures/waveloop-31-add-wave-item.jpg)
![新規追加項目。最初から Loop がオンで文書の範囲を保持](figures/waveloop-32-new-item-loop.jpg)
![新規項目の再生。PChannel 1 の Variation 1 が赤い](figures/waveloop-34-new-item-play.jpg)

## ループ端のドラッグ、Undo、参照元の再編集

Wave 文書を開き、Length の Lock を外した状態で比較した。端の線から左ドラッグすると、ループ範囲の数値が変わる。解放直後、端の上では緑または青の**左右両矢印カーソル**を確認した。押下を保持した途中のカーソルは未取得。

| 操作 | Start | End | Length |
| --- | ---: | ---: | ---: |
| 選択から作った範囲 | 22,008 | 33,010 | 11,003 |
| 緑の始点を `(526, 143) → (458, 143)` | 17,600 | 33,010 | 15,411 |
| 青の終点を `(698, 143) → (754, 143)` | 17,600 | 36,543 | 18,944 |
| 波形面で Ctrl+Z 1回 | 17,600 | 33,010 | 15,411 |

![緑の始点を左へ移動。緑の左右矢印、Start は17,600](figures/waveloop-37-wave-loop-start-drag.jpg)
![青の終点を右へ移動。青の左右矢印、End は36,543](figures/waveloop-38-wave-loop-end-drag.jpg)
![Ctrl+Z 後。青の終点は33,010、緑の始点は17,600](figures/waveloop-39-undo-wave-end.jpg)

終点の操作を取り消した状態で Wave 文書を保存した。既存項目、新規項目とも Loop はオンで、**編集欄の Start は22,008**。下段に灰色で表示する参照元の **Start は17,600**。どちらの Segment も、この Wave 保存の前後でファイルのハッシュが一致した。

録音用コピーで始点だけをドラッグし、Wave を保存して既存項目を開き直した回には、灰色の参照元にも旧値22,008が残った。始点と終点を動かして終点を Undo した試行では、同じプロセスで新しい灰色の値が出た。追試では、両端のドラッグ後に旧値が残ることと、Undo・Redo または数値確定で保存前に更新されることを確認した。参照する Segment を開き直すと、未保存の Wave 文書の値も表示された。 [参照元ループ値の更新の追試](seg-14-wave-reference-refresh.md)

![既存項目の Start は22,008、灰色の参照元は17,600](figures/waveloop-41-existing-after-second-source-edit.jpg)
![新規項目も22,008を保持し、灰色の参照元は17,600](figures/waveloop-43-new-after-second-source-edit.jpg)
![始点ドラッグだけを反復した回。項目値と灰色の参照元表示が22,008](figures/waveloop-91-compare-existing-after-save-b.jpg)

**Close Project** で Wave と2つの Segment に保存確認が出た。手動保存したファイルを照合するため、それぞれ **No** で閉じた。同じプロセスで Project を開き直すと、Wave 文書は **17,600〜33,010**、既存と新規の項目は **22,008〜33,010** のまま。2項目の灰色の参照元欄には **17,600〜33,010** が表示された。

![最初に Project を閉じる際の Wave 保存確認](figures/waveloop-45-project-closed.jpg)
![再読込した Wave 文書。17,600〜33,010](figures/waveloop-53-wave-file-reopen.jpg)
![再読込した既存項目。22,008と17,600が上下に並ぶ](figures/waveloop-58-existing-loop-tab-restored.jpg)
![再読込した新規項目も異なる項目値とソース値を表示](figures/waveloop-60-new-item-reopened.jpg)

## 保存値

| ファイルと条件 | Wave の範囲（Start / End / Length） | 項目の編集用 `wvcu` | 項目の再生用 `waih` |
| --- | --- | --- | --- |
| [元の WVP](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/inputs/FarmGameProject/SfxCow.wvp) | ループ件数0 | — | — |
| [選択から設定した WVP](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/captures/19-document-save-SfxCow.wvp) | 22,008 / 33,010 / 11,003 | — | — |
| [始点を動かした WVP](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/captures/40-document-second-SfxCow.wvp) | 17,600 / 33,010 / 15,411 | — | — |
| [WVP のみ保存時の既存 SGP](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/captures/19-document-save-SfxCow.sgp) | — | 0 / 63,867 | 0 / 0 |
| [項目オフを保存した既存 SGP](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/captures/24-existing-off-with-source-loop-SfxCow.sgp) | — | 22,008 / 33,010 | 0 / 0 |
| [項目オンを保存した既存 SGP](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/captures/26-existing-item-loop-on-SfxCow.sgp) | — | 22,008 / 33,010 | 22,008 / 33,010 |
| [新規項目を追加した SGP](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/captures/33-new-item-inherited-Segment1.sgp) | — | 22,008 / 33,010 | 22,008 / 33,010 |

WVP の `wsmp` は始点と長さ、`smpl` は始点と終点を持つ。表の **Length = End − Start + 1**。設定した2版とも Forward のループが1件。既存 SGP の項目長は **14,483,039 × 100 ns**、新規項目は **2,225音楽クロック**。元の WAV サンプル列は3版で SHA-256 が一致した。 [保存値、PCM、参照 GUID と SHA-256 の照合](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/wave-document-loop-audit.json) · [照合スクリプト](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/analyze-document-loop.py)

## 実際の再生音

既定の Windows 出力を **48 kHz、32-bit float、ステレオ**でループバック録音した。元 PCM の冒頭130 msで各再生の開始を合わせ、**0.550〜0.650秒**の波形を、各ループ長から計算した2回分の反復位置で探した。参照用に、元 PCM の **0.985〜1.085秒**の後半波形も照合した。表の時刻は録音内で元 PCM の冒頭と一致した位置。反復周期は2回の検出位置の差。

| 条件 | 録音と開始位置 | 反復周期 | 反復の正規化相関 | 元の後半波形との相関 |
| --- | ---: | ---: | ---: | ---: |
| Wave 文書、22,008〜33,010 | 1本目 10.369秒 | 249.5 ms | 0.987〜0.994 | 0.142 |
| Wave 文書オン、既存項目オフ | 1本目 27.453秒 | 反復を検出せず | 0.123〜0.134 | 0.990 |
| Wave 文書オン、既存項目オン | 1本目 61.804秒 | 249.5 ms | 0.994〜0.995 | 0.142 |
| 新規項目、追加時の Loop を使用 | 1本目 86.904秒 | 249.5 ms | 0.893〜0.903 | 0.153 |
| Wave 文書、17,600〜33,010 | 1本目 118.069秒 | 349.5 ms | 0.989〜0.995 | 0.203 |
| Wave 文書の始点変更後、保存済みの既存項目 | 2本目 9.975秒 | 249.5 ms | 0.995〜0.996 | 0.140 |
| Wave 文書の始点変更後、保存済みの新規項目 | 2本目 40.284秒 | 249.5 ms | 0.902〜0.906 | 0.150 |

1本目は、保存済みの WVP と既存 SGP のオフ状態を別の録音用コピーで再現し、その場で既存項目の Loop をオンにした。2本目は Wave の始点変更を保存した元の作業コピーを再読込した。どちらも再生を選んだ直後の操作時刻を記録した。新規項目の比較では、**Part をアクティブにして波形を選んだ状態**で Play From Start を押すと、再生中に Variation 1 が赤く表示された。Part が灰色のまま再生した別試行では Variation 2 が赤かったため、その区間は比較から除いた。

最初の7回はレコーダーの起動後に WAV が残らなかった。録音プロセスの終了を待つコマンドで再収録し、完了記録と WAV を照合した。1本目の録音時間を過ぎた追加操作は2本目で取り直した。2本の録音の不連続フラグは、どちらも最初の0.1秒以内で、比較対象の区間より前だった。 [1本目の録音・125秒](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/captures/audio-comparison-complete/output.wav) · [1本目の測定](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/captures/audio-comparison-complete/analysis.json) · [2本目の録音・42秒](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/captures/audio-saved-after-change/output.wav) · [2本目の測定](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/captures/audio-saved-after-change/analysis.json) · [波形照合コード](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/measure-document-loop.py)

![録音用コピーの既存項目。文書 Loop オン、項目 Loop オフ](figures/waveloop-77-compare-loop-off.jpg)
![始点を変更した Wave 文書の録音中](figures/waveloop-86-record-document-b.jpg)
![Part が非アクティブなまま再生。Variation 2 が赤い](figures/waveloop-109-record-new-with-saved-b.jpg)
![波形を選んで再生。PChannel 1 の Variation 1 が赤い](figures/waveloop-112-record-new-v1-with-saved-b.jpg)

## 終了時の状態と追試

調査用と録音用の各 Project で変更されたファイルは `FarmGameProject.pro`、`SfxCow.wvp`、`SfxCow.sgp`、`Segment1.sgp`。複製元と凍結入力28件のハッシュを照合した。Project を閉じ、同じ Producer 1件と View の標準4項目を確認した。後続の試験では Wave 文書の Loop and Release、2つの Lock、Snap To Zero、Best Loop End を確認した。別の試験では灰色の参照元値の更新を、ドラッグ、Undo・Redo、数値確定、参照する Segment の開き直しで比較した。 [参照元ループ値の更新](seg-14-wave-reference-refresh.md) [Wave エディターの設定と単独試聴](seg-14-wave-editor-loop-options.md) [終了時のウィンドウ](../../../work/original-gui/SEG-14-wave-document-loop/20261111T235846137158Z/captures/117-actual-final-windows.json) · [Wave エディターの基礎調査](ui-07-dls-wave-instrument.md) · [Segment 項目の Loop の前回試験](seg-14-wave-track-followup.md#clock-time-のループ表示と録音)

![終了時の View。標準4項目が有効](figures/waveloop-116-actual-final-view.jpg)
![Project を閉じた後の作業領域](figures/waveloop-117-actual-final-workspace.jpg)

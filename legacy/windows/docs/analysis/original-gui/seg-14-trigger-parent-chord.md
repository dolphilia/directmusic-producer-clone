# SEG-14　親の Chord と子の条件付き Variation Enter

2026-11-12。DirectMusic Producer 5.3.0.900 の作業コピーで、Segment Trigger の **Align to Barline** から子の Pattern に入るときの挙動を調べた。親の Chord の時刻と Flags、子の Enter の条件を変え、MIDI 8本を比較した。1280×752 px の撮影237枚から25枚を掲載。青い光と太い黒矢印は操作ツールの表示。 [凍結した Project](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/run.json) · [保存値と MIDI の照合](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/trigger-parent-chord-audit.json) · [画像の出典とハッシュ](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/published-trigger-parent-chord-figures.json)

## 親と子の設定

親 **BGDawn** は4小節、4/4、100 BPM、Repeat 0。比較用の親に、子と同じ Group 1 の Chord トラックを複製し、先頭の **1CM7** から始めた。親の Segment Trigger は1件。Start Time は **2:2:300（4140 clocks）**、**Secondary / Quick Response**、**Controlling Segment** はオフ。Boundary は **Align to Barline**、代替境界は **on any Beat**。親の PChannel 1 の基準音は192と7680 clocksにある。 [親の凍結ファイル](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/inputs/FarmGameProject/BGDawn.sgp) · [比較用 Project の作成手順](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/prepare-fixture.py)

子 **SSBird** は2小節、4/4。先頭に **1CM7** が1件あり、PChannel 2 の Sequence で開始位置を、PChannel 13 の Pattern で Variation を識別した。Pattern の各 Grid に2つの Variation 用ノートがあり、ベロシティは **1 が61、2 が86**。Part の Play は **in order from first**、Default Play Mode は **Chord/Scale**。この子は前回の「第4拍、条件付き、子に Chord 変更なし」の保存版。 [子の凍結ファイル](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/inputs/FarmGameProject/SSBird.sgp) · [子自身の Chord を変えた前回の比較](seg-14-trigger-variation-enter.md)

子の **Var Switch Point** 行には2つの緑の点がある。早い点はローカル960 clocksの Variation 1 の Enter。第1小節第4拍の点はローカル2304 clocksの Variation 2 の Enter で、**Trigger only with Chord Change** がオンの間は点に暗い枠がある。Variation **1** を非表示にして第4拍の点を選ぶと、Properties に **Enter**、条件のチェック、Variation **2** が表示され、ステータスバーに **Switch Pt 00:01.500** が出た。

![基準の親。Chord 行の先頭に 1CM7、下の行に SSBird](figures/trigchord-01-parent-baseline.jpg)
![子 SSBird。展開した Part、Var Switch Point の2点と下段の Sequence](figures/trigchord-02-child-points.jpg)
![Variation 2 のみを表示して第4拍の点を選択。条件がオン](figures/trigchord-03-conditional-selected.jpg)
![親のトリガーを選択。Secondary と Start Time 2:2:300](figures/trigchord-04-trigger-reference.jpg)
![Boundary。Align to Barline、on any Beat、Quick Response](figures/trigchord-05-trigger-boundary.jpg)

親の直前の小節線 **3072 clocks** に子のローカル0を合わせると、Variation 1 の点は **4032** でトリガー時刻より前、Variation 2 の点は **5376** で後になる。各試行で Transport の対象 **BGDawn.sgp: BGDawn** を確認し、Wave/MIDI Export Toolbar で MIDI を待機して **Play From Start** から自然終了まで収録した。

![先頭に 1CM7 だけを置いた親の再生中](figures/trigchord-06-baseline-playing.jpg)

## 親の 2C#M を前・同時・後に置く

1. 親の Chord 行で **第2小節第3拍** を右クリックする。空セルに濃い青の選択が出て、メニューの **Insert** が有効、Cut・Copy・Paste・Delete は灰色。**Insert** を選ぶと **2CM** が入り、Chord Properties が開く。
2. **Chord/Scale** のルート横の上スピンを1回押す。欄は **2C#**、タイムラインのラベルは **2C#M**。この状態を保存し、子の開始より1拍前（親4608 clocks）の条件を収録する。
3. 2C#M をクリックして **Ctrl+X**。ラベルが消え、Properties の入力欄と鍵盤からコードの表示が消える。第2小節第4拍の空セルをクリックして **Ctrl+V**。同じラベルとルートが戻る。今度は子の開始と同じ **5376 clocks**。
4. 同じ操作で第3小節第1拍に移す。子の開始より1拍後の **6144 clocks**。親の先頭の 1CM7 と子の保存ファイルは共通。

![第2小節第3拍の空セルを右クリック。Insert が有効](figures/trigchord-07-chord-context.jpg)
![2CM を挿入した直後。Chord Properties にルートと鍵盤](figures/trigchord-08-chord-insert.jpg)
![ルートを1段上げて 2C#M に変更](figures/trigchord-09-chord-root.jpg)
![子の開始より1拍前に 2C#M を置いた再生](figures/trigchord-10-before-playing.jpg)
![2C#M を切り取った直後。Properties のルートと鍵盤の選択が空](figures/trigchord-11-cut-clears-chord.jpg)
![第2小節第4拍に貼り付け。元と同じ 2C#M](figures/trigchord-12-same-beat-paste.jpg)
![子の開始と同じ拍に 2C#M を置いた再生](figures/trigchord-13-same-beat-playing.jpg)
![第3小節先頭に貼り付け。Properties は 2C# と M](figures/trigchord-14-after-paste.jpg)
![子の開始より1拍後に 2C#M を置いた再生](figures/trigchord-15-after-playing.jpg)

## Chord Track Properties の対照

親の **Chord Track Properties > Flags** では **Enable Control** と **Send Notifications** にチェックがあり、**Enable Play** は灰色。2C#M を第3小節先頭に置いたまま、まず **Send Notifications** だけオフ、次にこれをオンへ戻して **Enable Control** だけオフにした。チェックを変えても Chord 行のラベルは残る。保存値 `trkx` の先頭は既定 **0x28**、通知オフ **0x08**、Control オフ **0x20**。

![Chord トラックの Flags。Control と Notifications がオン](figures/trigchord-16-flags-default.jpg)
![Send Notifications のみオフで保存](figures/trigchord-17-flags-no-notify.jpg)
![Enable Control のみオフで保存](figures/trigchord-18-flags-no-control.jpg)

トラックを外す対照では、親の Chord 行の見出しを右クリックし、**Delete Track(s)** を選んだ。確認ダイアログの **OK** で Chord 行が消え、Segment Trigger 行が TimeSig の直下へ移った。開いていた Properties は **No properties available** に変わった。子の条件付き Enter のまま再生し、次に子の条件を外した場合も同じ親で再生した。

![Chord トラック削除の確認。OK とキャンセル](figures/trigchord-19-delete-confirmation.jpg)
![削除直後。Chord 行が消え、Properties は No properties available](figures/trigchord-20-no-chord-track.jpg)

無条件の対照は、子で Variation **1** を非表示にし、第4拍の点の Properties で **Trigger only with Chord Change** だけオフ。**Enter** と Variation **2** のチェックを保った。暗い枠が消え、**1** を再表示すると緑の2点が見える。子を保存して親から再生した。保存上は第4拍の `mrkr` が **(Grid 12, mask 2, flags 5) → (12, 2, 1)**。Part と参照先の GUID は更新された。64件のノートと時刻、ベロシティ、Sequence、子のコードの値は一致した。

![Variation 2 の Enter で条件だけオフ。点の暗い枠が消える](figures/trigchord-21-unconditional-selected.jpg)
![Variation 1・2 を再表示。Var Switch Point の緑の2点](figures/trigchord-22-unconditional-both-visible.jpg)
![親に Chord がなく、子の Enter が無条件の再生](figures/trigchord-23-unconditional-playing.jpg)

## MIDI 8本の照合

全試行で **768 ticks/四分音符、100 BPM**。子の Sequence は **5376 ticks / キー60** から21音、Pattern は同じ **5376 ticks** から192 ticks刻みで20音。親の2音も192・7680 ticksで一致した。表の音高は子の **Ch. 13 の MIDI キー**。最初の4音は5376〜5952 ticks、残り16音は6144〜9024 ticks。

| 親の Chord と子の Enter | 最初の4音 | 残り16音 | Variation（ベロシティ） | 保存・収録 |
| --- | ---: | ---: | --- | --- |
| 先頭の 1CM7 のみ、条件付き | 72 | 72 | 2（86） | [親 SGP](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/12-baseline-BGDawn.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/12-baseline.mid) |
| 2C#M が1拍前（4608）、条件付き | 85 | 85 | 2（86） | [親 SGP](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/20-before-BGDawn.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/20-before.mid) |
| 2C#M が同時（5376）、条件付き | 85 | 85 | 2（86） | [親 SGP](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/28-at-BGDawn.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/28-at.mid) |
| 2C#M が1拍後（6144）、条件付き | 72 | 85 | 2（86） | [親 SGP](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/36-after-BGDawn.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/36-after.mid) |
| 1拍後、Send Notifications オフ、条件付き | 72 | 85 | 2（86） | [親 SGP](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/43-no-notify-BGDawn.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/43-no-notify.mid) |
| 1拍後、Enable Control オフ、条件付き | 96 | 96 | 1（61） | [親 SGP](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/50-no-control-BGDawn.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/50-no-control.mid) |
| Chord トラックなし、条件付き | 96 | 96 | 1（61） | [親 SGP](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/58-no-chord-BGDawn.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/58-no-chord.mid) |
| Chord トラックなし、無条件 | 84 | 84 | 2（86） | [子 SGP](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/72-unconditional-SSBird.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/72-unconditional.mid) |

親の先頭に **1CM7** が1件あり **Enable Control** がオンの時点で、条件付き Enter は Variation 2 で始まった。2C#M を置く拍に応じて音高は変わり、1拍後の条件では **6144 ticks ちょうど**からキー85になった。**Enable Control** を外すと Variation 1 になり、Chord トラックを削除した結果と一致した。同じ親で子の Enter を無条件にすると Variation 2 に戻った。

この比較で開始位置は一定だった。親の Chord の位置は子の音高に反映され、親の Enable Control の切り替えで条件付き Enter が選ぶ初回の Variation が変わった。観測範囲は **Group 1、Secondary / Align to Barline、親・子とも4/4** の構成。親の先頭のコードが条件を満たした内部のタイミングは、この MIDI からは特定できない。 [全 Note On と親子の保存値](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/trigger-parent-chord-audit.json) · [照合スクリプト](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/analyze-parent-chord.py)

## 終了時の状態

Wave/MIDI Export Toolbar を非表示に戻して Project を閉じた。親と子の保存確認では **No** を選び、収録時に保存した版を保持した。凍結入力28件のハッシュは一致し、開始時と終了時の Producer は同じ1件。View は **Project Tree、Standard Toolbar、Status Bar、Transport Controls** の4項目。 [終了時のウィンドウ一覧](../../../work/original-gui/SEG-14-trigger-parent-chord/20261111T210106012671Z/captures/81-final-windows.json)

![終了時の View。標準4項目だけにチェック](figures/trigchord-24-final-view.jpg)
![Project を閉じた後の作業領域](figures/trigchord-25-final-workspace.jpg)

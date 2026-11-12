# SEG-14　同じ Tick にある Segment と Motif の再生

2026-11-12。DirectMusic Producer 5.3.0.900 の作業コピーで、FarmGameProject の **BGDawn** にある **SSBird** と **accordion (Heartland)** を調べた。各参照を MIDI で識別できるよう、作業用 Project のノートを差し替えた。1280×752 px の撮影204枚から26枚を掲載し、6本の MIDI と対応する保存ファイルを照合した。画像の太い黒矢印と青い光は操作ツールの表示。 [比較用 Project と凍結入力](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/run.json) · [画像の出典とハッシュ](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/published-trigger-motif-figures.json) · [保存値と MIDI の照合記録](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/trigger-motif-overlap-audit.json)

## 比較用 Project と参照先

親の BGDawn は4小節、4/4、100 BPM、Repeat 0。Group 1 の Segment Trigger に、同じ **Start Time 2:2:300** で2件を置いた。両方とも **Secondary / Immediate / Quick Response**、**Controlling Segment** と **Start at next marker** はオフ。保存した `sgih` は所属3840、開始4140 clocks、フラグ `0x80`。最後の値は Segment が0、Motif が1。親には PChannel 1 の基準音2つ（192・7680 clocks、キー44、ベロシティ100、長さ192）と、Marker 3点（0・5184・7104 clocks）がある。 [元の重なりを保存した BGDawn](../../../work/original-gui/SEG-14-trigger-followup/20261111T150559451685Z/captures/overlap-BGDawn.sgp) · [親の凍結ファイル](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/inputs/FarmGameProject/BGDawn.sgp) · [作成スクリプト](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/prepare-fixture.py)

Project Tree で `Heartlnd.stp` を開くと、Style 画面の下段に **accordion、fiddle、guitar、organ** が並ぶ。ツリーの **Heartland > Motifs > accordion** をダブルクリックすると、タイトルが **[Motif: Heartland - accordion]** になり、Transport に **Heartlnd.stp: accordion**、テンポ欄に **112.00** が出る。エディターは4/4、1小節。**2 (Strings): 2** は折りたたみ時にノートの線と四角い展開ボタンを表示する。ボタンを押すと、鍵盤と3つの青いノート、Variation 1–16、Var Switch Point、CC の **No CC Tracks** が見える。

![Heartland の Style 画面と Motifs フォルダー](figures/trigmot-01-heartland-tree.jpg)
![accordion の Motif Editor と折りたたまれた Part](figures/trigmot-02-motif-editor.jpg)
![Part を展開。鍵盤、Variation 1–16、3つの青いノート](figures/trigmot-03-motif-notes.jpg)

accordion は既存の PChannel 2 の Part を使い、その `note` だけを3件に差し替えた。各ノートの長さ192、ベロシティ90。先頭から0、192、576 clocks の位置に置き、同じ3音を元から有効な16の Variation で使用する。Style の GUID、Band と他の Part の保存値を照合した。SSBird は参照 GUID を維持し、2小節の Sequence に PChannel 3、キー76、ベロシティ100、長さ192 のノートを0・384 clocks に置いた。 [比較用の Heartland](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/inputs/FarmGameProject/Heartlnd.stp) · [変更前の Heartland](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/inputs/variants/original-Heartlnd.stp) · [比較用の SSBird](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/inputs/FarmGameProject/SSBird.sgp)

![SSBird の Sequence。第1小節の先頭付近にある同じ高さの2音](figures/trigmot-04-segment-notes.jpg)

## 同じセルの表示と選択

1. BGDawn を開くと、Segment Trigger の第2小節第2拍のセルには斜体で **SSBird** が見える。ラベルをクリックするとストリップの背景が淡い黄色になり、ラベルは濃い背景に白い文字で選択表示される。**F11 > Segment** では Segment が選択され、参照欄に **FarmGameProject: SSBird**、Start Time に **2 / 2 / 300** が出る。
2. 同じセルを右クリックすると、**Cycle Items** が有効、**Insert** が灰色。**Cycle Items** を選ぶと斜体の **accordion (Heartland)** に替わり、選択表示は同じセルに残る。**Boundary** タブを開いていると対象名も accordion に替わり、境界は **Immediate** と表示された。
3. **Segment** タブに戻すと **Motif** が選択され、上のドロップダウンが **Style**、下のドロップダウンが **Motif** になる。値は **FarmGameProject: Heartland 4/4** と **accordion**。Type と Start Time は SSBird 選択時と一致した。

![初期表示。重なった拍には斜体の SSBird が見える](figures/trigmot-05-parent-overlap.jpg)
![SSBird を選択した Segment タブ。Secondary と Start Time 2:2:300](figures/trigmot-06-segment-properties.jpg)
![重なったセルの右クリック。Cycle Items が有効で Insert は灰色](figures/trigmot-07-cycle-menu.jpg)
![Cycle Items で accordion に切り替えた直後の Boundary タブ](figures/trigmot-08-motif-boundary.jpg)
![選択した accordion の Style と Motif の参照欄](figures/trigmot-09-motif-properties.jpg)

## 単独の状態を作る操作と Undo

選択した SSBird を **Ctrl+X** で切り取ると、同じセルの accordion は立体の黒文字になり、Properties の入力欄は空で灰色になった。残ったラベルをクリックすると選択表示と Motif の値が戻る。この状態を保存し、Transport の親が **BGDawn.sgp: BGDawn** であることを確認して先頭から再生した。

![SSBird を切り取った直後。立体の accordion と灰色の入力欄](figures/trigmot-10-motif-after-cut.jpg)
![残った accordion を選択し、Motif の値が表示された状態](figures/trigmot-11-motif-selected.jpg)
![Motif だけの親を再生。カーソルがトリガーを通過した状態](figures/trigmot-12-motif-only-playing.jpg)

録音後、Transport を操作した直後の **Ctrl+Z** では表示に変化がなかった。残ったラベルをクリックしてストリップへフォーカスを戻し、**Ctrl+Z** で重なりを復元できた。次に **Cycle Items** で accordion を前面にして **Ctrl+X**。今度は SSBird だけが立体で残った。その状態も保存して親から再生し、同じ手順で Undo すると2件に戻った。

![Transport 操作後の Ctrl+Z。accordion だけの状態が続く](figures/trigmot-13-undo-focus.jpg)
![トラックへフォーカスを戻して Undo。SSBird が斜体に戻る](figures/trigmot-14-undo-restored.jpg)
![accordion を切り取った直後。立体の SSBird が残る](figures/trigmot-15-segment-after-cut.jpg)
![Segment だけの親を再生。再生カーソルは第2小節を進む](figures/trigmot-16-segment-only-playing.jpg)

## 前面の切り替え、Enable Play と保存順

SSBird を前面にしたまま保存・再生し、次に同じセルで **Cycle Items** を選んで accordion を前面にして保存・再生した。両方の保存ファイルはハッシュが一致し、Segment Trigger の項目順は **SSBird → accordion**。表示と Properties はそれぞれ前面にした項目を示す。MIDI のノートとテンポは両試行で一致した。

![両方ある状態の再生。前面は SSBird](figures/trigmot-17-segment-front-playing.jpg)
![両方ある状態の再生。前面は accordion (Heartland)](figures/trigmot-18-motif-front-playing.jpg)

MIDI は **View > Wave/MIDI Export Toolbar** を表示し、MIDI 側の **…** から **Export to MIDI: Set Filename** を開いて試行ごとに出力先を設定した。ファイル名の右の MIDI ボタンで記録を待機し、Transport の **Play From Start** で毎回 BGDawn を再生した。

![MIDI 側の … で開くファイル名ダイアログ](figures/trigmot-19-export-filename.jpg)

Segment Trigger の左のトラック名をクリックして **F11 > Flags** を開くと、ウィンドウのタイトルは **Segment Trigger Track Properties**。**Playback > Enable Play** のチェックを外すと、斜体のラベルと2件の保存データは残り、`trkx` の先頭値は `0x10 → 0x00`。再生中もラベルは残って親のカーソルは進み、MIDI には親の基準音2つだけが記録された。

![トラックの Flags。Playback の Enable Play にチェック](figures/trigmot-20-track-flags.jpg)
![Enable Play オフ。斜体の accordion は表示されたまま](figures/trigmot-21-track-disabled.jpg)
![Enable Play オフの親を再生。親の再生カーソルが進む](figures/trigmot-22-track-disabled-playing.jpg)

次に Project を閉じ、Enable Play がオンの保存版から `lseg` の2件の順序だけを **accordion → SSBird** に入れ替えた。各イベントと他のトラックの内容はバイト単位で一致した。Project を開き直し BGDawn を表示すると、選択していない状態で斜体の **accordion (Heartland)** が見えた。そのまま親を再生すると、両参照から前と同じ5音が出力された。 [順序だけを変えるスクリプト](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/prepare-reversed.py)

![逆順の保存版を開いた直後。前面は斜体の accordion](figures/trigmot-23-reversed-reopened.jpg)
![逆順の保存版で再生。Cycle Items を使う前の前面とカーソル](figures/trigmot-24-reversed-playing.jpg)

## MIDI 6本の照合

各セルは MIDI の **開始 tick / キー / ベロシティ**。全6本で分解能は **768 ticks/四分音符**、テンポは **tick 0 の 600,000 µs/四分音符（100 BPM）**。親の Ch. 1 には毎回 **192 / 44 / 100** と **7680 / 44 / 100** が出た。

| BGDawn での状態 | Motif の Ch. 2 | Segment の Ch. 3 | 保存ファイル・MIDI |
| --- | --- | --- | --- |
| accordion のみ | 4140 / 79 / 90、4332 / 84 / 90、4716 / 72 / 90 | なし | [SGP](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/captures/17-motif-only-BGDawn.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/captures/21-motif-only.mid) |
| SSBird のみ | なし | 4140 / 76 / 100、4524 / 76 / 100 | [SGP](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/captures/31-segment-only-BGDawn.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/captures/32-segment-only.mid) |
| 両方、SSBird が前面 | 単独と同じ3音 | 単独と同じ2音 | [SGP](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/captures/36-both-bird-front-BGDawn.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/captures/37-both-bird-front.mid) |
| 両方、Cycle Items で accordion が前面 | 単独と同じ3音 | 単独と同じ2音 | [SGP](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/captures/41-both-motif-front-BGDawn.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/captures/42-both-motif-front.mid) |
| 両方、Enable Play オフ | なし | なし | [SGP](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/captures/47-disabled-BGDawn.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/captures/48-disabled.mid) |
| 両方、保存順を逆転し再読込 | 単独と同じ3音 | 単独と同じ2音 | [SGP](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/captures/54-reversed-gui-BGDawn.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/captures/61-reversed.mid) |

両方が有効な3試行で、最初の Note On は両方とも **4140 ticks（100 BPM で先頭から約3.234秒）**。Cycle Items と逆順での再読込は前面のラベルを変えた。発音の有無を切り替えたのは、今回の条件では切り取りとトラックの Enable Play だった。確認範囲は上記の比較用素材と **Secondary / Immediate**。後続の試験では、元の内蔵 Pattern を持つ SSBird と加工前の accordion を同じ親で再生し、音声で両方の重なりを確認した。 [元のサンプルを使った録音と当時の判定](seg-14-segment-trigger-followup.md#ssbird-と-accordion-の混合音単独試聴) · [2つの Segment を重ねた試験](seg-14-trigger-simultaneous.md) · [元の SSBird と accordion の同時発音](seg-14-trigger-original-audio.md)

## 終了時の表示

**Wave/MIDI Export Toolbar** を非表示に戻して Project を閉じた。View でチェックがあるのは **Project Tree、Standard Toolbar、Status Bar、Transport Controls** の4項目。調査の前後で Producer は同じ1件。作業中に保存したファイルは調査用 Project にあり、凍結入力28件のハッシュを照合した。 [終了時のウィンドウ一覧](../../../work/original-gui/SEG-14-trigger-motif-overlap/20261111T202324536712Z/captures/67-final-windows.json)

![調査終了時の View。標準の4項目にチェック](figures/trigmot-25-final-view.jpg)
![Project を閉じた Producer。空の Project Tree と作業領域](figures/trigmot-26-final-workspace.jpg)

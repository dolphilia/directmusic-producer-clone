# SEG-14 Wave Track の Project Tree ドロップと複数 Variation での挿入

## 試験素材

起動済みの Producer コピーを1件使い、FarmGameProject の試験コピーで **Segment1.sgp** を開いた。Segment は7小節、4/4、音楽時刻。PChannel 1 の **a** に4小節目の SfxAlarm（Variation 1）と6小節目の SfxAlarm（Variation 2）、**b** に2小節目の SfxAlarm（Variation 2）がある。PChannel 2 は空。Project Tree、Standard Toolbar、Status Bar、Transport Controls を表示した。以下の座標は 1280 × 752 の Producer ウィンドウ内。 [試験条件と素材のハッシュ](../../../work/original-gui/SEG-14-wave-tree-variation-drop/20261112T020004047285Z/run.json) · [開始時の SGP](../../../work/original-gui/SEG-14-wave-tree-variation-drop/20261112T020004047285Z/inputs/FarmGameProject/Segment1.sgp)

![Variation 1 単独。PChannel 1 の a に4小節目の Alarm だけが見える](figures/wavetree-08-current-after-zoom.jpg)
![Variation 2 を加えると、b の2小節目と a の6小節目にも Alarm が表示される](figures/wavetree-09-both-visible.jpg)

## Project Tree からのドラッグ

Properties を閉じて試した。空き位置は **a の1小節目** `(309, 211–213)`。4小節目の比較では、既存の Alarm の範囲内にある同じ X 座標を使った。

| ドラッグ元 | 選択中の Variation | ドロップ先 | 操作後 |
| --- | --- | --- | --- |
| `SfxCow.wvp` ファイル行、Wave を開く前 | 1・2、または2のみ | a の1小節目の空き | いずれも追加なし。Tree のファイル名が選択状態。 |
| `SfxCow` 子ノード、Wave を開いた後 | 2のみ | a の1小節目の空き | 追加なし。Tree の子ノードが選択状態で、Transport は SfxCow。 |
| `SfxAlarm.wvp` ファイル名とアイコン | 2のみ | a の1小節目または3小節目の空き | 追加なし。3小節目は、位置線を合わせた再試行でも同じ。 |
| `SfxCow.wvp` ファイル行、SfxCow を Segment で参照した後 | 1のみ | a の1小節目の空き、または4小節目の表示中の Alarm 上 `(687, 211)` | 追加なし。 |
| 同じファイル行 | 2のみ | a の1小節目の空き、4小節目の非表示の Alarm 上 `(687, 211)`、または同じ時刻の空きレイヤー b `(687, 251)` | 追加なし。b はファイル行を選択してから再試行しても同じ。 |
| 同じファイル行、および展開した `SfxCow` 子ノード | 1・2 | a の1小節目の空き | 追加なし。子ノードを選ぶと Transport は SfxCow。ファイル行を選び直しても、クリックで Segment に戻るまで Transport は SfxCow のまま。 |

Tree で最初に `SfxCow.wvp` の左側をクリックした時点では子ノードが現れなかった。ファイル行をダブルクリックすると Wave エディターが開き、子ノードが選択された。Segment に戻って子ノードを単独クリックすると、表示は Segment のまま Transport の対象が SfxCow に切り替わった。後に Segment 項目から SfxCow を参照した状態では、Tree のプラスをクリックして子ノードを展開できた。

単独選択時と両方を表示した時の後半のドラッグ試行について、直前と直後の保存済み Wave 項目が同じだったことを確認した。前回はファイル行からの追加に成功したが、今回の試行では空き位置での成功も再現できなかった。複数選択、子ノード、非表示の波形がドロップの可否に与える影響は未確定。撮影したのはドラッグ終了後で、押下中の実カーソルとドロップ可否表示は未観測。 [前回のドロップ試験](seg-14-wave-drop-overlap.md) · [同梱ヘルプの Wave 挿入手順](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/insertingwavesinthewavetrack.htm)

![1・2 の両方を表示。ファイル行を空きレイヤー a にドラッグした直後](figures/wavetree-11-both-file-empty.jpg)
![2 のみ表示。同じファイル行と同じ空き座標](figures/wavetree-15-two-cow-file-empty.jpg)
![SfxCow.wvp を開くと子ノードが現れ、Wave エディターと Transport が SfxCow を表示](figures/wavetree-16-open-cow.jpg)
![2 のみ表示。選択済みの子ノードから空きレイヤー a へドラッグした直後](figures/wavetree-18-two-cow-child-empty.jpg)
![1 のみ表示。4小節目の表示中の Alarm 上でファイル行のドラッグを終了](figures/wavetree-35-one-cow-file-visible-a.jpg)
![2 のみ表示。4小節目の非表示の Alarm 上でファイル行のドラッグを終了](figures/wavetree-37-two-cow-file-hidden-a.jpg)
![2 のみ表示。同じ時刻で空いているレイヤー b へドラッグした直後](figures/wavetree-38-two-cow-file-empty-b.jpg)
![1・2 の両方を表示。参照済みの子ノードから1小節目の空き位置へドラッグした直後](figures/wavetree-55-both-cow-child-empty.jpg)
![同じ位置で参照済みのファイル行からのドラッグを終了](figures/wavetree-56-both-cow-file-empty.jpg)

## Variation 1・2 を表示した状態での Insert

PChannel 1 で **Variation 1・2** を選び、a の空き `(551, 211)` をクリックして **Insert** キーを押した。開始位置 **3:2:1:0** に赤い **Empty** が現れ、**Wave Properties** が開いた。ソース欄は空で、Start の各欄には挿入位置が入り、End と Length の欄は灰色だった。Wave 欄のドロップダウンは `FarmGameProject: SfxCow` のように Project 名と Wave 名を並べる。SfxCow を選ぶと、同じ場所のラベルと波形が赤く表示され、End と Length が埋まった。

**Variations** タブでは1と2にチェックが入り、保存した項目の所属マスクは **`0x00000003`**。タイムラインを2のみ、1のみに切り替えると、Alarm は所属に従って入れ替わり、SfxCow は両方の画面に残った。保存した表示マスクは順に `0x00000002`、`0x00000001`。SfxCow の所属はどちらも `0x00000003` だった。

![Insert 直後の赤い Empty。Wave は未割り当てで Start に 3:2:1:0](figures/wavetree-26-insert-both.jpg)
![Wave Properties のドロップダウンに Project 内の Wave が並ぶ](figures/wavetree-27-wave-selector.jpg)
![割り当て直後。Empty が SfxCow の名前と赤い波形に切り替わる](figures/wavetree-28-insert-cow-assigned.jpg)
![Variations タブで1・2にチェックが入る](figures/wavetree-29-insert-cow-variations.jpg)
![2 のみ表示。SfxCow と、2・6小節目の Alarm が残る](figures/wavetree-31-insert-visible-only-two.jpg)
![1 のみ表示。同じ SfxCow と、4小節目の Alarm が残る](figures/wavetree-33-insert-visible-only-one.jpg)

## 既存波形と重なる位置への Insert

レイヤー a の `(687, 211)` を、4小節目の SfxAlarm を表示している状態と、同じ Alarm を隠した状態で右クリックした。どちらも赤い位置線が右クリックした X 座標に移り、コンテキストメニューの **Insert** が有効だった。表示中は SfxAlarm が赤く選択され、**Cut、Copy、Delete** が有効になる。非表示時は SfxAlarm が選択されず、3項目が灰色だった。

両条件でメニューから **Insert** を選ぶと、**4:2:4:0** に赤い Empty が現れた。Wave Properties で SfxCow を割り当てると、既存の Alarm を残して同じレイヤーに SfxCow が加わった。Variation 1 のみの試行では両項目が1に属し、Variation 2 のみの試行では追加した SfxCow だけが2に属した。1の試行で追加した SfxCow は、保存値を記録した後、選択されたまま Delete で取り除いてから2の試行へ進んだ。

| 保存時の試行 | Cow の物理開始 | Cow の論理開始 | 長さ | Cow の所属 | 保存 |
| --- | ---: | ---: | ---: | --- | --- |
| 空き位置、1・2選択 | 6,912 | 6,912 | 2,225 | 1・2 | [SGP](../../../work/original-gui/SEG-14-wave-tree-variation-drop/20261112T020004047285Z/captures/30-insert-both-Segment1.sgp) |
| Alarm 表示中、1のみ | 10,560 | 9,984 | 2,225 | 1 | [SGP](../../../work/original-gui/SEG-14-wave-tree-variation-drop/20261112T020004047285Z/captures/46-overlap-visible-save-Segment1.sgp) |
| Alarm 非表示、2のみ | 10,560 | 9,984 | 2,225 | 2 | [SGP](../../../work/original-gui/SEG-14-wave-tree-variation-drop/20261112T020004047285Z/captures/51-overlap-hidden-save-Segment1.sgp) |

単位は音楽クロック。4小節目の Alarm の物理範囲は **9,600〜11,848**。両試行とも追加した Cow と **1,288クロック**重なり、元の Alarm 3件の所属・開始・長さは保存値でも同じだった。最後に1と2を同時表示して保存し、同じ Producer で Project を閉じて開き直した。両方の表示と重なりが復元され、Cow のみが描かれている後半をクリックして Properties を開くと、所属は2のみだった。 [最終 SGP](../../../work/original-gui/SEG-14-wave-tree-variation-drop/20261112T020004047285Z/captures/57-after-both-drag-Segment1.sgp)

![Alarm 非表示時の右クリック。Insert は有効で Cut、Copy、Delete は灰色](figures/wavetree-42-hidden-alarm-context.jpg)
![同じ座標で Alarm 表示中の右クリック。Alarm が赤く選択され Cut、Copy、Delete が有効](figures/wavetree-43-visible-alarm-context.jpg)
![表示中の Alarm に重なる位置で Insert。元の Alarm は青く、Empty が赤い](figures/wavetree-44-insert-on-visible-alarm.jpg)
![Cow 割り当て後。青い Alarm と赤い Cow が同じレイヤーで重なる](figures/wavetree-45-overlap-visible-assigned.jpg)
![非表示の Alarm と同じ位置でも赤い Empty が現れ、Start は 4:2:4:0](figures/wavetree-48-insert-on-hidden-alarm.jpg)
![Variation 2 のみで追加した Cow の所属。2だけにチェック](figures/wavetree-50-overlap-hidden-mask.jpg)
![Variation 1 を再表示すると、元の Alarm と追加した Cow の重なりが見える](figures/wavetree-52-overlap-show-both.jpg)
![Project 再読込後。両 Variation が表示され、4小節目で両波形が重なる](figures/wavetree-62-reopened-segment.jpg)
![再読込後の Cow の所属は Variation 2 のみ](figures/wavetree-64-reopen-overlap-mask.jpg)

## 記録と終了状態

開始素材28件のハッシュ、保存した8状態の SGP と Wave GUID、掲載した27枚の寸法とハッシュを照合した。試験用 Project で変更されたのは `FarmGameProject.pro` と `Segment1.sgp`。Project を閉じ、開始時と同じ Producer ウィンドウ1件と View の標準4項目を確認した。 [保存値と照合結果](../../../work/original-gui/SEG-14-wave-tree-variation-drop/20261112T020004047285Z/wave-tree-variation-audit.json) · [照合スクリプト](../../../work/original-gui/SEG-14-wave-tree-variation-drop/20261112T020004047285Z/analyze-wave-tree-variation.py) · [スクリーンショット一覧](../../../work/original-gui/SEG-14-wave-tree-variation-drop/20261112T020004047285Z/published-figures.json) · [終了時のウィンドウ](../../../work/original-gui/SEG-14-wave-tree-variation-drop/20261112T020004047285Z/final-windows.json)

![終了時の View。Project Tree、Standard Toolbar、Status Bar、Transport Controls のみ有効](figures/wavetree-65-final-view.jpg)

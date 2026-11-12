# SEG-14　Wave Track の重なりと Variation の発音

2026-11-12。DirectMusic Producer 5.3.0.900 の作業コピーで、同じ位置に重なる SfxAlarm と SfxCow を使った。Variation 1・2 の単独再生、両方を表示した状態のループ、再生中の表示切り替えを比較した。録音5本を照合し、撮影記録118枚から30枚を掲載する。画像は1280×752 px。青い光と黒い矢印は操作ツールの表示。 [入力、配置と実行ファイル](../../../work/original-gui/SEG-14-wave-variation-audio/20261112T013800560194Z/run.json) · [掲載画像の出典とハッシュ](../../../work/original-gui/SEG-14-wave-variation-audio/20261112T013800560194Z/published-figures.json)

## 同じレイヤーに配置した6区間

前回の重なり試験の SGP から Wave 項目を複製し、**Segment1、7小節、4/4、PChannel 1、レイヤー a** に配置した。各小節の先頭から384クロック後に開始し、重なる組は両音の物理開始が一致する。Wave 自体の Loop はオフ、Offset は0。PChannel 2 は空。Wave の所属はマスク1が Variation 1、2が Variation 2、3が両方。 [前回の重なり試験](seg-14-wave-overlap-audio.md) · [素材作成コード](../../../work/original-gui/SEG-14-wave-variation-audio/20261112T013800560194Z/prepare.py) · [入力 SGP](../../../work/original-gui/SEG-14-wave-variation-audio/20261112T013800560194Z/inputs/FarmGameProject/Segment1.sgp)

| 小節 | 物理開始（クロック） | Alarm の所属 | Cow の所属 |
| ---: | ---: | --- | --- |
| 1 | 384 | 1・2 | — |
| 2 | 3456 | — | 1・2 |
| 3 | 6528 | 1 | 1 |
| 4 | 9600 | 2 | 2 |
| 5 | 12672 | 1 | 2 |
| 6 | 15744 | 2 | 1 |

## Variation の表示と選択

Segment1 を開き、右下の横ズーム拡大を2回押した。PChannel 1 の空白部分をクリックすると背景が薄黄色になる。**Variation 1 のみ**の表示では第4小節が空で、第5小節に Alarm、第6小節に Cow。上段の **2** をダブルクリックすると **Variation 2 のみ**になり、第3小節が空、第5小節に Cow、第6小節に Alarm。次に **1** を1回クリックすると両方の Variation が表示され、第3〜6小節はすべて重なった位置にラベルと波形が出る。 [同梱ヘルプの Variation 操作](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/workingwithmultiplevariations.htm)

![Variation 1 のみ。第4小節が空、第5小節に Alarm、第6小節に Cow](figures/wavevar-01-var1-edit.jpg)

![2をダブルクリックした直後。第3小節が空、第5小節に Cow、第6小節に Alarm](figures/wavevar-06-var2-edit.jpg)

![両方を表示。第3〜6小節に波形が並ぶ](figures/wavevar-10-both-edit.jpg)

両方を表示した第5小節の重なりをクリックした試行では **SfxCow** が赤く選択され、Wave Properties の **Variations** には2だけチェック。上段の2をクリックして表示を外すと、Cow の選択が解除され、Properties は **Wave Part** に切り替わる。同じ位置をクリックすると Alarm が赤くなり、Variations には1だけチェック。第1小節の基準 Alarm は1・2の両方にチェックがあり、編集画面で Variation 1 のみを表示していても両方の所属が読める。

![第5小節の Cow を選択。Variations の2だけにチェック](figures/wavevar-11-cow-mask.jpg)

![上段の2をオフ。Cow が消え、第5小節に Alarm が見える](figures/wavevar-12-hide-cow.jpg)

![同じ位置で Alarm を選択。Variations の1だけにチェック](figures/wavevar-13-alarm-mask.jpg)

![第1小節の基準 Alarm。Variations の1と2にチェック](figures/wavevar-14-shared-mask.jpg)

## 単独再生と両方を表示したループ

Transport の左側の **Play From Start** で単独再生すると、上段の該当番号が赤くなり、赤い位置線は空の小節も通過する。停止すると番号は黒に戻った。第1・2小節の単独音は両条件で鳴った。下段の空の PChannel 2 でも別の番号が赤くなったため、番号は Part ごとに確認した。 [Variation 1 の録音](../../../work/original-gui/SEG-14-wave-variation-audio/20261112T013800560194Z/captures/audio-variation-1/output.wav) · [Variation 2 の録音](../../../work/original-gui/SEG-14-wave-variation-audio/20261112T013800560194Z/captures/audio-variation-2/output.wav)

![Variation 1 を再生。第3小節に位置線、上段の1が赤](figures/wavevar-02-var1-same.jpg)

![Variation 1 を再生。第4小節は空のまま位置線が通過](figures/wavevar-03-var1-empty.jpg)

![Variation 1 の第5小節。Alarm の位置を通過](figures/wavevar-04-var1-split-alarm.jpg)

![Variation 1 の第6小節。Cow の位置を通過](figures/wavevar-05-var1-split-cow.jpg)

![Variation 2 を再生。上段の2が赤、第4小節に位置線](figures/wavevar-07-var2-same.jpg)

![Variation 2 の第5小節。Cow の位置を通過](figures/wavevar-08-var2-split-cow.jpg)

![Variation 2 の第6小節。Alarm の位置を通過](figures/wavevar-09-var2-split-alarm.jpg)

両方を表示した状態で、PChannel 1 のヘッダーをクリックして **F11 > Wave Part** を開いた。**Play** は初期値 **in random order**。一覧から **in order from first** を選択した。Segment の **Loop** タブで **Repeat** を0から1にすると、Music Time Looping が有効になり、範囲は **1:1:1:0〜8:1:1:0**。この状態を保存して先頭から再生すると、1周目は赤い1、2周目は赤い2になり、波形の表示範囲は両周で同じだった。1周目の末尾から2周目の冒頭に移ると、Elapsed は進み、Offset と赤い位置線が先頭側へ戻った。 [同梱ヘルプの試聴](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/auditioningvariations.htm) · [両方を表示したループの録音](../../../work/original-gui/SEG-14-wave-variation-audio/20261112T013800560194Z/captures/audio-both-loop/output.wav)

![Wave Part の Play の候補5件](figures/wavevar-15-play-order-menu.jpg)

![in order from first の選択後](figures/wavevar-16-play-order.jpg)

![Repeat 1、Music Time の端点は1小節目と8小節目の先頭](figures/wavevar-17-repeat-one.jpg)

![1周目の第5小節。赤い1と、両方を表示した波形](figures/wavevar-18-loop-first.jpg)

![折り返し前。Elapsed 13.242秒、赤い1](figures/wavevar-19-before-wrap.jpg)

![折り返し後。Elapsed 14.513秒、Offset 0.511秒、赤い2](figures/wavevar-20-after-wrap.jpg)

![2周目の第5小節。表示範囲は同じで赤い番号は2](figures/wavevar-21-loop-second.jpg)

| 小節 | 1のみ表示・単独再生 | 2のみ表示・単独再生 | 両方表示・1周目 | 両方表示・2周目 |
| ---: | --- | --- | --- | --- |
| 3 | Alarm ＋ Cow | 検出なし | Alarm ＋ Cow | 検出なし |
| 4 | 検出なし | Alarm ＋ Cow | 検出なし | Alarm ＋ Cow |
| 5 | Alarm | Cow | Alarm | Cow |
| 6 | Cow | Alarm | Cow | Alarm |

録音は既定の Windows 出力のループバック、48 kHz、32-bit float、ステレオ。各周の第1・2小節を原 WVP の120 msの波形で照合し、実測開始差からクロック速度を求めた。第3〜6小節は開始50〜1300 msの左右平均波形を、同じ周の単独音2本の線形和に当てはめた。発音を検出しなかった区間の RMS は **0.000096以下**。発音した成分の倍率は **0.938〜0.996**、他方の成分は絶対値 **0.0013以下**。2周録音の基準音の再出現間隔は **約14.000秒**。この入力は Tempo Track を含まず、単独音の開始差から **1536クロック/秒（120 BPM相当）** と測定された。前回の別試行では125 BPM相当だった。後続の対照では、同一の Tempo Track なしの Segment が、直前に再生した Tempo Track に応じて90・125 BPM相当へ変わることを確認した。 [先行テンポと Alternate Tempo の比較](seg-14-wave-tempo-context.md) [波形照合の結果](../../../work/original-gui/SEG-14-wave-variation-audio/20261112T013800560194Z/variation-audio-measurements.json) · [測定コード](../../../work/original-gui/SEG-14-wave-variation-audio/20261112T013800560194Z/measure.py)

## 再生途中で Variation 1 の表示を外す

両方を表示して先頭から再生し、1が赤い第2小節の後半で **1** を1回クリックした。画面は Variation 2 の表示になり、上段の赤い番号も2に変わった。録音を2回行い、2回目は Project を閉じて保存済み Segment を開き直した。録音の基準音に合わせたクリック前後の時刻は1回目 **3.010〜3.140秒**、2回目 **3.383〜3.513秒**。2回とも結果は同じ。録音の2回目と撮影の追試では、上部の空白へポインターを移して赤い2を確認した。 [最初の途中切り替えの録音](../../../work/original-gui/SEG-14-wave-variation-audio/20261112T013800560194Z/captures/audio-live-toggle/output.wav) · [再読込後の録音](../../../work/original-gui/SEG-14-wave-variation-audio/20261112T013800560194Z/captures/audio-live-toggle-reopened/output.wav)

| 小節 | 切り替えを行った1周目の計測窓 | 2周目の同じ位置 |
| ---: | --- | --- |
| 3 | Alarm ＋ Cow。波形は画面から消えていた | 検出なし |
| 4 | Alarm。録音の波形照合で Cow 成分は絶対値0.0004以下 | Alarm ＋ Cow |
| 5 | Cow | Cow |
| 6 | Alarm | Alarm |

この操作では、赤い番号の切り替え後にも第3小節の音が鳴り、第4小節の2音が揃ったのは2周目だった。再生中の表示だけから切り替え直後の発音を判定できない。検証範囲は **in order from first**、1・2を表示した開始、1周目の第2小節後半で1を外す操作。後続の試行では、第3・5小節の発音中に1を外した後も鳴り始めた成分が続いた。第5小節の発音前に外した試行では、クリック時刻に応じて Cow、発音の検出なし、Alarm を観測した。 [発音前・発音中・折り返し前後の比較](seg-14-wave-live-variation-boundary.md)

![切り替え前。第1小節、赤い1、両方の Variation を表示](figures/wavevar-22-live-before.jpg)

![最初の収録で1を外した後。第3小節の波形が消えた区間を通過](figures/wavevar-23-live-hidden-three.jpg)

![再読込後の収録、1周目の第4小節。ポインターを離した状態でも赤い2](figures/wavevar-24-live-first-four.jpg)

![同じ1周目の第5小節。赤い2と Cow の波形](figures/wavevar-25-live-first-five.jpg)

![同じ収録の2周目、第4小節。赤い2を維持](figures/wavevar-26-live-second-four.jpg)

## 保存と開き直し

収録前の Ctrl+S で `waph` の Play 値は **1→0**、`wpcu` の先頭の表示マスクは **1→3**、`segh` は Repeat 1、ループ0〜21504クロックになった。10件の Wave 所属マスク、物理・論理時刻、レイヤー、長さと参照 GUID は入力と一致した。収録と表示切り替えの後に Project を閉じると Segment1 の変更確認が出たので **No** を選択。開き直した画面で、両方を表示した波形、横ズームと **in order from first** が復元された。 [原版保存の SGP](../../../work/original-gui/SEG-14-wave-variation-audio/20261112T013800560194Z/captures/23-save-both-loop-Segment1.sgp) · [入力と保存値の監査](../../../work/original-gui/SEG-14-wave-variation-audio/20261112T013800560194Z/variation-audit.json)

![切り替え後の Close Project。Segment1.sgp の保存確認](figures/wavevar-27-save-prompt.jpg)

![開き直した Segment1。両方の波形を表示した7小節が復元](figures/wavevar-28-reopened-variations.jpg)

![開き直した Wave Part の Play に in order from first が出る](figures/wavevar-29-reopened-play-order.jpg)

凍結した28ファイルは SHA-256 が一致した。作業コピーでは Segment1.sgp と FarmGameProject.pro を更新した。最後に Project を閉じ、同じ Producer 1件で **Project Tree、Standard Toolbar、Status Bar、Transport Controls** を表示した。 [最終ウィンドウ一覧](../../../work/original-gui/SEG-14-wave-variation-audio/20261112T013800560194Z/captures/final-windows.json)

![最後の View メニュー。標準4項目の表示](figures/wavevar-30-final-view.jpg)

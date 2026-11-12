# SEG-14　Pattern Part の Variation の Enter と Align

2026-11-12。DirectMusic Producer 5.3.0.900 の作業コピーで、Segment Trigger の参照先に **Pattern Part の Variation の Enter** を置いた。子全体の発音開始と、Part が選んだ Variation をそれぞれ MIDI で観測した。1280×752 px の撮影261枚から23枚を掲載し、8本の MIDI と対応する SGP を照合した。青い光と太い黒矢印は操作ツールの表示。 [Marker トラック下段での比較](seg-14-trigger-enter.md) · [凍結した Project と素材の出典](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/run.json) · [撮影画像の出典とハッシュ](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/published-trigger-variation-figures.json)

## 比較条件

親 **BGDawn** は4小節、4/4、100 BPM、Repeat 0。トリガー1件の **Start Time は 2:2:300**、実時刻は **4140 clocks**。**Secondary、Controlling Segment オフ、Quick Response、Align to Barline / on any Beat** を固定した。子のローカル0は親の直前の小節線 **3072 clocks** に対応し、親の次の Beat は **4608 clocks**。親自身の PChannel 1 には192と7680 clocksにキー44を置いた。

子 **SSBird** は2小節、4/4。Marker トラックを省き、前回と同じ **Sequence（PChannel 2）** と、単独試聴に使った Part を拡張した **Pattern（PChannel 13）** を置いた。Sequence は35音の音高でローカル時刻を識別できる。Pattern は各 Grid に長さ96 clocks の音を Variation ごとに1つずつ置いた。Variation 1 のベロシティは **61**、Variation 2 は **86**。Part の **Play** は **in order from first**、**Default Play Mode** は **Chord/Scale**。両 Variation を表示した状態で親を再生した。 [素材の作成手順](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/prepare-fixture.py) · [親 SGP](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/inputs/FarmGameProject/BGDawn.sgp) · [基準の子 SGP](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/inputs/FarmGameProject/SSBird.sgp) · [元の単独試聴](seg-14-variation-order.md)

![子 SSBird。Chord、Pattern Part、空の Var Switch Point、Sequence が見える](figures/trigvar-01-baseline-child.jpg)
![Part タブ。PChannel 13、2 Bars、in order from first、Chord/Scale](figures/trigvar-02-part-settings.jpg)
![親の Segment タブ。参照先 SSBird、Secondary、Start Time 2:2:300](figures/trigvar-03-parent-trigger.jpg)
![親の Boundary タブ。Align to Barline、代替は on any Beat](figures/trigvar-04-parent-align.jpg)

## Enter の位置と表示する Variation

1. 子の **2** ボタンをクリックして Variation 1 のみにする。**Var Switch Point** 行でローカル960 clocksの Grid を右クリックし、**Insert Switch Point > Enter** を選ぶ。行に緑の小矩形が現れる。
2. **2** を再表示して保存し、親を再生する。次の条件では **1** を一時的に非表示にし、同じメニューで Variation 2 のローカル1728 clocksに Enter を置く。Variation 1 の点は Variation 2 だけの表示中は隠れ、**1** を再表示すると緑の2点が並ぶ。
3. 親で **Wave/MIDI Export Toolbar** の MIDI 出力先と記録ボタンを設定し、**Play From Start** から自然終了まで収録する。Sequence と Pattern の先頭の Note On を独立に確認する。

![Var Switch Point の Insert Switch Point サブメニュー。Enter、Exit、Chord Trigger 付きが並ぶ](figures/trigvar-05-insert-menu.jpg)
![Variation 1 だけを表示して過去の Enter を追加](figures/trigvar-06-past-var1.jpg)
![Variation 2 だけを表示して後続の Enter を追加。過去の点は隠れている](figures/trigvar-07-future-var2.jpg)
![両 Variation を表示すると緑の2点が並ぶ](figures/trigvar-08-both-points.jpg)
![MIDI 出力先を設定し、ファイル名の右の記録ボタンを押した状態](figures/trigvar-09-armed-midi.jpg)
![後続 Enter の条件で BGDawn を再生中](figures/trigvar-10-playing-parent.jpg)

| Enter の位置（子のローカル clocks） | 対象 | 起点3072に重ねた親時刻 | Grid index |
| ---: | --- | ---: | ---: |
| 960 | Variation 1、トリガーより前 | 4032 | 5 |
| 1728 | Variation 2、最初の後続点 | 4800 | 9 |
| 2304 | 第1小節第4拍に移した点 | 5376 | 12 |

Enter がない場合と、Variation 1 の過去の Enter だけの場合は、子の **Sequence と Pattern が同じ4608 ticks** から発音し、Pattern は Variation 1。Variation 2 の後続 Enter を追加すると、**両トラックが4800 ticks** から発音し、Pattern の全 Note On はベロシティ86の Variation 2 に切り替わった。Sequence の音高と時刻は、子のローカル0が親3072に対応することを示した。

## Trigger only with Chord Change

後続の点を選択して **F11** で **Var Switch Point Properties** を開いた。Variation 1・2 が表示された状態では **Trigger only with Chord Change** が灰色だった。この状態から **1** を非表示にすると、Properties の対象が **2** のみになり、Enter 側の条件を編集できた。オンにすると緑の点に暗い枠が加わった。両 Variation を再表示して保存し、子の Chord 行が先頭の **1CM7** だけのまま再生すると、子全体の発音は **4800 ticks** のまま、Pattern は **Variation 1** に戻った。 [同梱ヘルプの Variation Switch Points](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/variationswitchpoints.htm)

![両 Variation を表示。Chord Change のチェック欄が灰色](figures/trigvar-11-both-chord-disabled.jpg)
![Variation 2 だけを表示。Enter 側の条件を操作できる](figures/trigvar-12-var2-chord-enabled.jpg)
![Enter の Trigger only with Chord Change をオン](figures/trigvar-13-chord-flag-checked.jpg)
![両方を再表示。後続の点に暗い枠がある](figures/trigvar-14-conditional-outline.jpg)

コード変更を同じ拍に置くため、Variation 2 のみ表示した状態で、点を第4拍の先頭へ移した。通常の左ドラッグを2回試した範囲では、ポインターは移動したが点と **Switch Pt 00:01.125** の表示は元の位置に残った。対象点を選んで **Ctrl+X**、第4拍のセルを選んで **Ctrl+V** とすると、**Switch Pt 00:01.500** に移り、Properties は Enter・Chord Change・Variation 2 を保持した。保存上の Grid index は9から12に変わり、Variation 1 の過去の点は残った。

![選択済みの条件付き Enter を左ドラッグした後。点と時刻は元の位置](figures/trigvar-15-drag-retains-position.jpg)
![Ctrl+X 後。Variation 2 の点が消え、Properties は空](figures/trigvar-16-cut-clears-point.jpg)
![第4拍で Ctrl+V。点と条件が復元し、Switch Pt が1.500秒](figures/trigvar-17-paste-at-beat4.jpg)

## 第4拍にコード変更を置いた比較

条件付き Enter を第4拍に置いた状態で、まず子にコード変更を追加せずに録音した。次に Chord 行の第4拍で **Insert** を押し、**Chord Properties > Chord/Scale** のルートを上矢印で1段上げて **2C#M** にした。保存ファイルでは子のローカル2304 clocksにコードが増え、4レベルのルートは13。コード編集時に、第4拍以後の Pattern の40音で保存上の `wMusicValue` が書き換わった。Variation 1 は `0x8000 → 0x6220`、Variation 2 は `0x7000 → 0x5220`。時刻、Variation mask、長さ、ベロシティなど他のノート値は一致した。この書き換え後の3条件ではコードとノート値をそろえ、後続 Enter の Variation mask または条件フラグを変えた。

![Chord 行の第4拍に 2C#M。真下の Var Switch Point に条件付き Enter](figures/trigvar-18-same-beat-chord.jpg)
![同じコードの対照。第4拍の条件付き Enter を Variation 1 に割り当てた Properties](figures/trigvar-19-same-chord-var1.jpg)
![同じコードの対照。第4拍の点を無条件にし、Variation 2 のみに割り当て](figures/trigvar-20-same-chord-var2.jpg)

| 条件 | 子 Sequence の先頭（ticks / キー） | Pattern の先頭（ticks / キー / ベロシティ） | Pattern の Variation | SGP と MIDI |
| --- | --- | --- | ---: | --- |
| Enter なし | 4608 / 56 | 4608 / 96 / 61 | 1 | [SGP](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/inputs/FarmGameProject/SSBird.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/20-no-enter.mid) |
| 過去の Enter だけ | 4608 / 56 | 4608 / 96 / 61 | 1 | [SGP](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/27-past-SSBird.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/29-past-enter.mid) |
| 後続1728、Variation 2、無条件 | 4800 / 57 | 4800 / 84 / 86 | 2 | [SGP](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/36-future-SSBird.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/38-future-enter.mid) |
| 後続1728、Variation 2、条件付き、コード変更なし | 4800 / 57 | 4800 / 96 / 61 | 1 | [SGP](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/49-chord-condition-SSBird.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/51-chord-no-change.mid) |
| 後続2304、Variation 2、条件付き、コード変更なし | 5376 / 60 | 5376 / 96 / 61 | 1 | [SGP](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/60-beat4-no-chord-SSBird.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/62-beat4-no-chord.mid) |
| 後続2304、Variation 2、条件付き、同じ拍にコード変更 | 5376 / 60 | 5376 / 83 / 61 | 1 | [SGP](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/69-beat4-with-chord-SSBird.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/71-beat4-with-chord.mid) |
| 後続2304、Variation 1、条件付き、同じ拍にコード変更 | 5376 / 60 | 5376 / 83 / 61 | 1 | [SGP](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/80-beat4-var1-control-SSBird.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/82-beat4-var1-control.mid) |
| 後続2304、Variation 2、無条件、同じ拍にコード変更 | 5376 / 60 | 5376 / 71 / 86 | 2 | [SGP](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/92-beat4-var2-unconditional-SSBird.sgp) · [MIDI](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/94-beat4-var2-unconditional.mid) |

第4拍にコード変更を置いた3条件は、すべて子の Sequence と Pattern が5376 ticksで開始した。同じ拍のコード変更は条件付きの2回で音高83に反映され、ベロシティ61の Variation 1 が出た。後続の点を Variation 2 に割り当てたまま条件フラグをオフにした回では、ベロシティ86の Variation 2 が出た。この Secondary の試験では、条件付きの点も Align の開始位置に使われ、**子の同時刻にコードを追加しても、条件付き Enter の割り当て先が初回の Variation として選ばれることは確認できなかった**。親側のコードの位置と Enable Control は後続の試験で比較した。再生途中の Variation 切替には別の条件が残る。 [保存フラグ、コード、全 Note On の照合](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/trigger-variation-enter-audit.json) · [再生途中の Exit の比較](seg-14-variation-playback.md) · [親の Chord と子の条件付き Variation Enter](seg-14-trigger-parent-chord.md)

保存された `mrkr` のフラグは無条件の Enter が **1**、Chord Change 付きが **5**。全試行で子の2トラックは親3072を起点とし、親自身の2音、MIDI の768 ticks/拍と600,000 µs/拍は一致した。Pattern の Note On は開始時刻ごとに24・23・20件で、各試行の最初から最後まで同じベロシティだった。

## 終了時の状態

MIDI 書き出しツールバーを非表示にし、Project を保存して閉じた。開始時と同じ Producer 1件で、View は Project Tree、Standard Toolbar、Status Bar、Transport Controls の4項目。凍結入力28件のハッシュは一致した。 [終了時のウィンドウ一覧](../../../work/original-gui/SEG-14-trigger-variation-enter/20261111T193612664571Z/captures/105-final-windows.json)

![最後の MIDI 収録の自然終了。出力ファイル名は空欄](figures/trigvar-21-natural-end.jpg)
![終了時の View。標準4項目がオン](figures/trigvar-22-final-view.jpg)
![Project を閉じた停止状態](figures/trigvar-23-final-workspace.jpg)


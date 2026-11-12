# UI13　Synth Status と Output Status の実機操作

2026-11-11。Windows 上の DirectMusic Producer 5.3.0.900 の調査用コピーで、FarmGame の `SSBird.sgp` と `BGNight.sgp` を使った。親ウィンドウは1280×752 px。Project Tree、Standard Toolbar、Transport Controls、Status Bar を基準に、調べるツールバーだけを追加した。採取した画像123枚から46枚を掲載する。青い光と黒い矢印は操作ツールの表示。 [実行記録](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/run.json) · [画像の出典・寸法・ハッシュ](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/published-transport-status-live-figures.json) · [数値とファイルの照合](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/transport-status-live-audit.json)

## 調査開始時の終了と復帰

前の書き出し検証から開いていた Producer で **View > Synth Status** を表示し、Project Tree の `SSBird.sgp` をダブルクリックした。直後の画像では Tree が白くなり、続く取得時点で Producer ウィンドウがなくなった。Windows の Application Error（イベント1000、2026-11-11 21:09:03 JST）に `SegmentDesigner.ocx`、例外コード `0xc0000005`、オフセット `0x0002b8b3` が記録された。 [Windows イベントの記録](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/transport-status-segment-crash.json)

![エディターを開く前の Synth Status](figures/meter-04-synth-empty.jpg)

![SSBird を開く操作の直後。Tree が一時的に空白](figures/meter-05-open-bird.jpg)

Producer のウィンドウとプロセスが0件になったことを確認し、同じ調査用コピーを1件起動した。File の履歴から FarmGameProject を開き、標準4項目の状態で SSBird と BGNight を先に開くと、Synth Status を表示して両方を再生できた。終了は1回の観測で、Synth Status の表示とアクセス違反の因果関係は未確定。 [起動時のプロセス確認](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/transport-status-relaunch.json)

![同じ実行ファイルを再起動した直後](figures/meter-07-after-crash-relaunch.jpg)

![標準4項目で SSBird を先に開いた状態](figures/meter-12-bird-stable.jpg)

## Synth Status の表示と値

**View > Synth Status** で Transport Controls の直下に行が増える。左から **CPU**、**Voices**、**Memory Use**、**Latency**、**Reset**。CPU と Voices の枠には現在値、Peak、`at` に続く時刻が並ぶ。表示直後の画像では CPU 欄が空だった。更新後は数値が入った。Timeline の上端は y=105 から y=135 へ移動し、隠すと y=105 へ戻った。 [同梱ヘルプの各欄の定義](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/synthesizerstatuswindow.htm)

![SSBird を開いた後に Synth Status を表示。CPU 枠は更新前](figures/meter-17-synth-bird-idle.jpg)

`SSBird.sgp` は PChannel 11 の Pattern、`BGNight.sgp` は複数の PChannel を使う Style の演奏。表の **Elapsed** は Status Bar、**Peak at** は Synth Status の Voices 欄に表示された時刻。Memory Use はこの表の全画像で **3166K**。

| 操作後の状態 | Elapsed | Voices | Peak / Peak at | Latency |
| --- | --- | --- | --- | --- |
| SSBird の再生中 | 1.977 s | 1 | 1 / 00:00:01.823 | 118 ms |
| SSBird の自然終了 | 3.604 s | 0 | 1 / 00:00:01.823 | 118 ms |
| BGNight の再生開始後 | 1.297 s | 2 | 2 / 00:00:00.581 | 114 ms |
| BGNight の再生中 | 3.729 s | 6 | 6 / 00:00:02.575 | 112 ms |
| BGNight の Stop Immediate 直後 | 4.127 s | 6 | 6 / 00:00:02.575 | 112 ms |
| 直後から約0.65秒後 | 4.127 s | 3 | 6 / 00:00:02.575 | 112 ms |
| 停止中に Reset | 4.127 s | 0 | 0 / 00:00:00.000 | 109 ms |

Transport が緑の再生ボタンに戻った後も、Voices が6から3へ変化する間は Peak 6 とその時刻を保持した。Reset 後、Voices の Peak と時刻がゼロになった。CPU の Peak は0.4%から0.1%へ、時刻は00:00:00.000に変わった。Memory Use は同じで Latency は更新を続けた。画面上の数値と変化を記録しており、発音数の内部算出や停止時のオーディオ信号は別試験。

![SSBird の再生中。Voices 1、Peak 1](figures/meter-20-bird-playing-later.jpg)

![SSBird の自然終了後。Voices 0、Peak 1](figures/meter-21-bird-ended.jpg)

![BGNight の再生開始後。Voices 2](figures/meter-23-night-playing.jpg)

![BGNight の再生中。Voices 6、Peak 6](figures/meter-24-night-playing-later.jpg)

![Stop Immediate 直後。Transport は停止状態、Voices の表示は6](figures/meter-25-night-stopped.jpg)

![同じ停止位置で Voices は3に減り、Peak 6 を保持](figures/meter-26-night-tail-ended.jpg)

![停止中の Reset 後。Peak と時刻が再設定された状態](figures/meter-27-synth-reset-after.jpg)

![Synth Status を隠すと Timeline 上端が元の位置に戻る](figures/meter-30-synth-hidden.jpg)

### Latency と設定画面

**View > Transport Options** の右端から **MIDI/Performance Options** を開くと、この環境の **Adjust Latency** は55 ms。**Number of Voices** は64、**Sample Rate** は22 kHz、**Default Synthesizer** は **Not Specified (Use DirectMusic Default)**。上の実行で Synth Status に表示された109〜118 msの値とは別の欄で、同梱ヘルプは Synth Status の Latency を直近10秒の平均と説明している。設定画面は変更せず **Cancel** で閉じた。 [MIDI/Performance Options のヘルプ](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/midiperformanceoptionsdialogbox.htm)

![MIDI/Performance Options。Adjust Latency 55 ms、Voices 64、22 kHz](figures/meter-93-midi-performance-readback.jpg)

## Output Status の一覧とメーター

Synth Status を隠して **View > Output Status** を選ぶ。Transport Controls の直下に赤い **All Notes Off** と、PChannel のボタンが横に並ぶ。既に SSBird と BGNight を再生した後で初めて表示したところ、**1 Oboe**、**4 Fantas…**、**11 Ocari…** の3枠があった。実画面では長い名前の右端が枠に収まらず切れる。PChannel の枠はこの画面で約52 px幅、番号の近くに黄色のレベルが描かれる。BGNight の再生中は1と4の両方に同時に黄色が見えた。 [同梱ヘルプの Output Status](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/outputstatustoolbar.htm)

![再生後に Output Status を初めて表示。1、4、11 のボタン](figures/meter-32-output-shown.jpg)

![BGNight の再生中。PChannel 1 と4に黄色のレベル表示](figures/meter-35-output-night-meters.jpg)

再生途中に **3 Pizz…** が1と4の間に追加され、4と11のボタンが右へ移動した。1、4、11から1、3、4、11に変わるため、前の画像で4だった位置を次に使うと3が対象になる。最初の試行では実際にこの位置の変化をまたいだ。停止後も4枠が残り、その時点でソロだった3の再ダブルクリックで全枠を有効に戻した。

![初回のミュート中。PChannel 1 のラベルが薄く、4は黄色](figures/meter-37-output-muted-later.jpg)

![PChannel 3 が途中で追加。4と11が右へ移動した状態](figures/meter-39-output-fantasia-solo.jpg)

![停止後も4枠が残り、PChannel 3 のみ有効](figures/meter-43-output-stopped.jpg)

![停止中にソロ中の PChannel 3 を再ダブルクリック。全枠が有効](figures/meter-44-clear-pizz-solo.jpg)

### ミュートとソロ

4枠がそろった状態でもう一度 BGNight を再生し、同じ位置のボタンで操作した。

| 操作 | 画面の変化 |
| --- | --- |
| PChannel 1 を単クリック | ボタンの色が沈み、ラベルが白っぽくなる。4の黄色いメーターと Primary の再生が続く |
| 同じボタンを再度単クリック | 1のラベルが黒く戻り、黄色のレベルが再び表示される |
| PChannel 4 をダブルクリック | 1、3、11のラベルが白っぽくなり、4は有効表示のまま。Primary の Elapsed は進み続ける |
| PChannel 4 を再度ダブルクリック | 全枠のラベルが黒く戻る |

これはツールバー表示と Transport の観測。ミュートやソロ中の出力音声はこの操作と同時に収録していない。

![PChannel 1 を単クリックしてミュート。4のメーターと再生は継続](figures/meter-59-stable-muted.jpg)

![PChannel 1 を再度単クリック。1と4に黄色のレベル表示](figures/meter-60-stable-oboe-restored.jpg)

![PChannel 4 をダブルクリック。1、3、11がミュート表示](figures/meter-62-stable-fantasia-solo.jpg)

![同じ PChannel 4 を再ダブルクリック。全枠が有効](figures/meter-63-stable-solo-cleared.jpg)

### All Notes Off と一覧の消去

左端のアイコンで Status Bar に `Send an All Notes Off event. Right click to remove all status buttons.` が出た。BGNight の再生中に左クリックすると、黄色の表示が消え、Transport も停止表示に戻った。直後の Elapsed は8.448秒で、次の撮影でも同じだった。PChannel ボタンは残った。その後、停止中にアイコンを右クリックすると PChannel ボタンがすべて消え、赤いアイコンだけになった。 [同梱ヘルプの All Notes Off](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/allnotesoffbutton.htm)

![再生中に All Notes Off を左クリック。停止状態になり、一覧は残る](figures/meter-64-all-notes-off.jpg)

![All Notes Off を右クリック。一覧が空になる](figures/meter-68-remove-status-buttons.jpg)

一覧を空にして SSBird を選んだだけでは枠は空。再生すると11だけが追加され、黄色のレベルが出た。同じ演奏中に黄色がなくなる時点があり、自然終了後も枠自体は残った。View で隠して再表示しても、再生操作なしで11の枠を保持していた。

![一覧を空にした後、SSBird を選んだ停止状態](figures/meter-69-bird-only-ready.jpg)

![SSBird を再生。PChannel 11 が追加され、レベルが表示された](figures/meter-70-bird-only-output.jpg)

![同じ再生の途中で11の黄色表示が消えた時点](figures/meter-71-bird-level.jpg)

![自然終了後も11のボタンが残る](figures/meter-72-bird-output-end.jpg)

![View で Output Status を非表示にした状態](figures/meter-74-output-hidden.jpg)

![再生せず再表示しても11のボタンが残る](figures/meter-76-output-preserved.jpg)

## チャンネル名の変更

停止中に PChannel 11 を右クリックすると **Edit PChannel Name** が開いた。タイトルは PChannel 共通で、入力欄の左に **11:**、右に **OK** と **Cancel**。初期値 **Ocarina** は選択済みだった。**Bird** を入力して **Cancel** を押し、開き直すと **Ocarina** が残っていた。改めて **Bird** を入力して **OK** を押すと、ボタンの表示は直ちに **11 Bird** になった。別のエディター **SSBird** に切り替えると、同じ PChannel の Pattern 行も **11 (Bird)** になった。名前を **Ocarina** に戻すと、ボタンと Pattern 行の両方に反映された。

![右クリックした直後。PChannel 11 の Ocarina が選択済み](figures/meter-45-edit-pchannel-name.jpg)

![入力欄の名前を Bird に変更した状態](figures/meter-46-name-edit-preview.jpg)

![Cancel 後に開き直すと Ocarina](figures/meter-48-name-reopen.jpg)

![OK 後。ボタンの表示は11 Bird](figures/meter-50-name-committed.jpg)

![別の Segment に切り替えると Pattern 行も11 (Bird)](figures/meter-52-name-visible-in-strip.jpg)

![Ocarina を復元した後。ボタンと Pattern 行が元の表示に戻る](figures/meter-55-name-restored.jpg)

## ドッキングと浮動時の操作

Output Status の右端にある最後のボタン外側の余白を、Timeline 側へドラッグすると **Output Status** と題した浮動枠になった。1枠のときは116×60 pxで、All Notes Off と11のボタンが横並び、右上に赤い閉じるボタン。タイトルバーのドラッグで位置が変わる。右下（外枠の内側から3 pxほど）を斜めにドラッグした試行では枠ごと移動し、寸法は116×60 pxだった。タイトルバーをダブルクリックすると元の行へ再ドッキングした。

![Output Status の浮動枠。1つの PChannel ボタンを表示](figures/meter-77-output-drag.jpg)

![浮動枠のタイトルバーをドラッグした後](figures/meter-78-output-float-moved.jpg)

![右下をドラッグした後。外枠の位置が変わり、幅と高さは同じ](figures/meter-79-output-resize-probe.jpg)

![タイトルバーをダブルクリックして再ドッキング](figures/meter-80-output-redocked.jpg)

Synth Status も Reset の外側にある右端の余白からドラッグすると浮動枠になった。726×60 pxで、ドッキング時と同じ横並び。浮動中も数値を表示し、Reset を押すと Voices の Peak が13から0に戻った。タイトルバーのダブルクリックで元の行へ戻った。どちらも浮動中は Timeline 上端が y=105、元の行に戻すと y=135。 [浮動枠の採取位置と寸法](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/transport-status-live-audit.json)

![Synth Status の浮動枠](figures/meter-86-synth-floated.jpg)

![浮動中に Reset を押した後](figures/meter-87-synth-reset-floating.jpg)

![再ドッキングした Synth Status](figures/meter-88-synth-redocked.jpg)

## 復元と次の確認

PChannel の名前を Ocarina に復元し、Output Status の一覧を消去した。Synth Status と Output Status は再ドッキング後に非表示、設定比較のために表示した Transport Options も非表示にした。終了時の Producer は1ウィンドウで、SSBird を開いて停止中。作業プロジェクトのファイルは前回記録のハッシュと一致し、凍結した原版素材21ファイルも照合した。

![終了時の View。標準4項目だけにチェック](figures/meter-97-final-view.jpg)

続く試行で Transport Options のメトロノーム、カウントイン、MIDI 切替、設定の有効条件を観測した。ミュートとソロは音声への反映、Output Status は多チャンネル時の浮動枠の折り返しとサイズ、アクセス違反は再現条件の切り分けが残る。 [Transport Options の実機記録](ui-13-transport-options-live.md)

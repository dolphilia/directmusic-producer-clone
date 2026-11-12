# SEG-14　Segment Trigger の同一 Tick にある2件の発音

2026-11-12。DirectMusic Producer 5.3.0.900 の作業コピーで、FarmGameProject の **BGDawn** の同時刻トリガーを再検証した。参照先を別々の MIDI チャンネルと音高で識別できる短い Sequence にした。1280×752 px の撮影186枚から29枚を掲載し、MIDI 6本とループバック WAV 4本を照合した。青い光と太い黒矢印は操作ツールの表示。 [同時刻の選択と保存の前回調査](seg-14-segment-trigger-followup.md) · [凍結入力28件と実行ファイル](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/run.json) · [画像の出典とハッシュ](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/published-trigger-simultaneous-figures.json)

## 比較用の親 Segment と参照先

[前回の同時刻 BGDawn](../../../work/original-gui/SEG-14-trigger-followup/20261111T150559451685Z/captures/overlap-BGDawn.sgp) と [SfxCow 参照入りの保存版](../../../work/original-gui/SEG-14-trigger-followup/20261111T150559451685Z/captures/immediate-BGDawn.sgp) を使用。2つの **Segment** 参照が **3:1:1** にある12小節の親を作り、親は **1/4・100 BPM・Repeat 0**。参照先のファイル名と GUID を保ったまま、コピー内の **SSBird** と **SfxCow** を各2小節、1つの Sequence の識別音に差し替えた。親の Band1 と共通の再生先を使い、親と参照先に埋め込み AudioPath はない。親の基準音をトリガーより前と後ろに置いた。 [素材作成のスクリプト](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/prepare-fixture.py) · [親 BGDawn](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/inputs/FarmGameProject/BGDawn.sgp) · [SSBird](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/inputs/FarmGameProject/SSBird.sgp) · [SfxCow](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/inputs/FarmGameProject/SfxCow.sgp)

| Segment | Sequence の PChannel | MIDI キー | 各 Segment 内の Note On 時刻 |
| --- | ---: | --- | --- |
| 親 BGDawn | 1 | 48 | 192、5376 clocks |
| SSBird | 2 | C 5 / 60 | 96、480 clocks |
| SfxCow | 3 | E 6 / 76 | 96、480 clocks |

![親 BGDawn の12小節。第3小節の重なりは斜体の SSBird、下段に Marker](figures/trigsim-01-parent.jpg)

### 同じセルで前面の項目を切り替える

BGDawn を開くと第3小節に斜体の **SSBird**。ラベルを選択して F11 を押すと **Segment/Motif Trigger Properties**。**Segment** が選択され、参照先は **FarmGameProject: SSBird**。**Secondary**、**Controlling Segment** はオフ、Start Time は **3:1:1**。同じセルの右クリックから **Cycle Items** を選ぶと、前面のラベルと Properties が **SfxCow** に切り替わり、時刻は同じ **3:1:1** だった。**Boundary** には **Immediate**、**Quick Response**、**Start at next marker** はオフ。保存された両項目の `sgih` は所属 **1536**、実時刻 **1537**、フラグ **0x80**。

![第3小節を選択。前面の SSBird と Start Time 3:1:1](figures/trigsim-02-bird-trigger.jpg)
![右クリック。Cycle Items が有効](figures/trigsim-03-cycle-menu.jpg)
![Cycle Items 後の SfxCow。Secondary、Controlling Segment オフ、3:1:1](figures/trigsim-04-cow-trigger.jpg)
![SfxCow の Boundary。Immediate、Quick Response、Start at next marker オフ](figures/trigsim-05-immediate.jpg)

### 参照先を開いてノートを確認する

Project Tree の各ファイルの下の Segment ノードをダブルクリックすると子エディターが開き、Transport の対象がその Segment に切り替わった。SSBird のヘッダーは **2 (Strings)**、SfxCow は **3 (Pizz.)**。SfxCow の開いた直後の鍵盤は C 5 付近で、ノートは表示範囲の上。鍵盤横のつまみを約14 px 上へドラッグすると、E 6 の2音が現れた。

各1音目を選んで F11 を押すと **Sequence Properties**。両方とも Velocity **100**、Start Time は **1:1:2:-96**、End Time は **1:1:3:-96**。保存されている開始時刻は96、長さは192 clocks。第2グリッド先頭の192 clocksから96 clocks手前に表示上の開始がある。

![SSBird の PChannel 2 と C 5 の2音](figures/trigsim-06-bird-editor.jpg)
![SSBird の1音目の Sequence Properties](figures/trigsim-07-bird-note.jpg)
![SfxCow を開いた直後。鍵盤の表示範囲に対象音が見えていない](figures/trigsim-08-cow-viewport.jpg)
![鍵盤のつまみを上へ移動した後。PChannel 3 の2音を表示](figures/trigsim-09-cow-editor.jpg)
![SfxCow の1音目の Sequence Properties。E 6](figures/trigsim-10-cow-note.jpg)

## 同時再生、単独、Enable Play オフ、保存順の逆転

**View > Wave/MIDI Export Toolbar** を必要な間だけ表示。右側の **…** で出力先を選び、隣のボタンで MIDI 記録を待機。Transport の対象を **BGDawn.sgp: BGDawn** と確認し、**Play From Start** から自然終了まで収録した。

![比較に必要な書き出しツールバーを追加](figures/trigsim-11-export-toolbar.jpg)
![出力先を設定して MIDI 記録を待機](figures/trigsim-12-export-armed.jpg)
![SfxCow が前面の状態で同時再生。親のカーソルが第3小節を通過](figures/trigsim-13-cow-front-play.jpg)
![Cycle Items で SSBird を前面に戻す](figures/trigsim-14-cycle-bird-front.jpg)
![SSBird が前面の状態で同時再生](figures/trigsim-15-bird-front-play.jpg)

単独試験は親のトリガー行で対象の前面ラベルをクリックして **Delete**。SSBird を削除すると同じセルに SfxCow だけが表示される。その状態を保存して記録後、**Ctrl+Z** で削除を取り消した。**Cycle Items** で SfxCow を選び直して削除すると SSBird だけが残った。各保存版で残った参照名、時刻、フラグを照合した。 [SfxCow だけの保存版](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/captures/gui-cow-only-BGDawn.sgp) · [SSBird だけの保存版](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/captures/gui-bird-only-BGDawn.sgp)

![SSBird を削除した後。同じセルに SfxCow が表示](figures/trigsim-16-cow-only.jpg)
![SfxCow だけを残して再生](figures/trigsim-17-cow-only-play.jpg)
![Undo で重なりと斜体のラベルが復元](figures/trigsim-18-undo-restore.jpg)
![SfxCow を削除した後。SSBird だけが表示](figures/trigsim-19-bird-only.jpg)
![SSBird だけを残して再生](figures/trigsim-20-bird-only-play.jpg)

削除を取り消して両方を戻し、**Segment Trigger** のヘッダーで **F11 > Flags**。**Enable Play** をオフにして保存した。重なったラベルは残り、Transport の親 Segment は同じように進んだ。保存値はイベント2件を保持し、トラックの `trkx` の先頭値が **0x10 → 0x00**。 [Enable Play オフの保存版](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/captures/gui-disabled-BGDawn.sgp)

![ヘッダーで開いた Segment Trigger Track Properties。グループ1](figures/trigsim-21-track-properties.jpg)
![Flags タブ。Enable Play オン](figures/trigsim-22-track-enabled.jpg)
![Enable Play をオフにした状態](figures/trigsim-23-track-disabled.jpg)
![無効で再生中。ラベルを表示したまま親の位置線が進む](figures/trigsim-24-disabled-play.jpg)

別の試行では Project を閉じ、作業コピーの BGDawn を **SfxCow → SSBird** の順に記録した RIFF に置き換えて開き直した。読み込み直後は斜体の **SfxCow** が前面。項目の時刻とフラグは同じ。再生した MIDI では元の保存順と同じ2件が出力された。 [保存順を逆にした BGDawn](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/inputs/variants/reversed-BGDawn.sgp)

![逆順の BGDawn を再読込。第3小節の前面は SfxCow](figures/trigsim-25-reverse-open.jpg)
![逆順のまま再生。親の第3小節付近](figures/trigsim-26-reverse-play.jpg)
![自然終了後。MIDI 欄が空、Offset 0.000秒](figures/trigsim-27-natural-stop.jpg)

| 再生条件 | Ch. 2 / キー60 | Ch. 3 / キー76 | MIDI |
| --- | --- | --- | --- |
| 同時刻、SfxCow が前面 | 1633、2017 | 1633、2017 | [収録](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/captures/22-both-cow-front.mid) |
| 同時刻、SSBird が前面 | 1633、2017 | 1633、2017 | [収録](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/captures/25-both-bird-front.mid) |
| SfxCow だけ | なし | 1633、2017 | [収録](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/captures/30-cow-only.mid) |
| SSBird だけ | 1633、2017 | なし | [収録](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/captures/38-bird-only.mid) |
| 両方を置いたまま Enable Play オフ | なし | なし | [収録](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/captures/49-track-disabled.mid) |
| 保存順を逆にして再読込 | 1633、2017 | 1633、2017 | [収録](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/captures/61-reversed-order.mid) |

表の時刻は **768 ticks/拍** の MIDI Note On。**1537 + 96 = 1633**、**1537 + 480 = 2017** で、各参照先の開始と元ノートのオフセットに一致した。親の Ch. 1、キー48は全試行で **192、5376 ticks**。親のテンポイベントも全試行で **600,000 µs/拍** だった。 [保存値と MIDI 全イベント](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/trigger-simultaneous-audit.json)

## ループバック録音で2つの音高を照合

既定の再生先から 48 kHz、32-bit float、ステレオで各13秒を録音した。親の先頭の基準音の周波数ピークで各録音を合わせ、子の2つの発音位置付近で **261.6 Hz（キー60）** と **659.3 Hz（キー76）** の成分を80 msの窓で測った。表は2つの位置における最大振幅の範囲。残した子だけで出る成分が、同時再生では両方に現れた。

| 条件 | 261.6 Hz | 659.3 Hz | WAV |
| --- | ---: | ---: | --- |
| 両方、SSBird が前面 | 0.0434〜0.0492 | 0.0351〜0.0356 | [録音](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/captures/audio-25-both-bird-front/output.wav) |
| SfxCow だけ | 0.00022〜0.00029 | 0.0320〜0.0321 | [録音](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/captures/audio-30-cow-only/output.wav) |
| SSBird だけ | 0.0437〜0.0499 | 0.00016〜0.00026 | [録音](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/captures/audio-38-bird-only/output.wav) |
| Enable Play オフ | 0.00003 未満 | 0.00003 未満 | [録音](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/captures/audio-49-track-disabled/output.wav) |

SfxCow を前面にした最初の試行では WAV が生成されず、MIDI を採用した。次の収録からは録音プロセスの終了まで待機し、4本の完了メタデータと WAV を確認した。録音は既定出力のミックス。サンプル単位の発音時刻は機器の遅延を含むため、2件が同じ Tick に出たことは MIDI の時刻で確認した。 [録音と周波数成分の照合](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/trigger-simultaneous-audit.json)

## 終了時の状態と次の比較

MIDI 書き出しを隠して Project を閉じた。開始時から同じ Producer が1件で、View の **Project Tree、Standard Toolbar、Status Bar、Transport Controls** がオン。作業コピーの BGDawn は逆順版、凍結入力には元の順序と対照試験を保存してある。 [終了時のウィンドウ](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/captures/89-final-windows.json)

![終了時の View。標準の4項目だけにチェック](figures/trigsim-28-compact-view.jpg)
![Project を閉じた停止状態](figures/trigsim-29-closed-project.jpg)

発音を確定した範囲は、識別用 Sequence を参照する **Secondary / Immediate / Controlling Segment オフ** の2件。元の音源の SSBird と accordion の混合録音では、前回資料のとおり各発音の特定は保留。続く試験では、この入力を複製して **Align** と **Start at next marker** の発音時刻を比較した。この同時発音試験の親に含まれる Marker の位置は **0、2304、4608、7296 clocks**。保存した各版で位置は一致した。 [凍結した比較用 Project](../../../work/original-gui/SEG-14-trigger-simultaneous/20261111T183257234944Z/run.json) · [Align と Start at next marker](seg-14-trigger-alignment.md)

後続の試験では、PChannel 3 の Sequence を使う Segment と PChannel 2 の Motif の3音を同じ Tick に置き、単独時の音が同時再生でも出ることを MIDI で照合した。元の内蔵 Pattern を持つ SSBird と加工前の accordion は、同じ親から再生して MIDI と音声で重なりを照合した。 [同じ Tick にある Segment と Motif](seg-14-trigger-motif-overlap.md) · [元の SSBird と accordion の同時発音](seg-14-trigger-original-audio.md)

# SEG-14 Producer 再起動後の再生テンポと Wave 選択

DirectMusic Producer 5.3.0.900 の調査用コピーで、**Tempo Track のない Segment1** を、先行再生と Producer の再起動を挟んで比較した。終了前に操作した **Alternate Tempo** と **Snap To Zero** の再起動後の状態も記録した。録音5本と1280×752 pxのスクリーンショット36枚を掲載する。 [入力と実行ファイルのハッシュ](../../../work/original-gui/SEG-14-wave-tempo-restart/20261112T032849570120Z/run.json) · [同じ Producer 内での先行再生と倍率の試験](seg-14-wave-tempo-context.md)

## 試験条件

- `Segment1.sgp` は4/4、3小節、Repeat 0。TimeSig と Wave があり、Tempo Track はない。PChannel 1、レイヤー a、Variation 1 の **SfxAlarm** 3件は、物理開始384、3,456、6,528クロック。論理開始は0、3,072、6,144。すべてループなし。 [入力 Segment1](../../../work/original-gui/SEG-14-wave-tempo-restart/20261112T032849570120Z/inputs/FarmGameProject/Segment1.sgp)
- `BGDawn.sgp` は4/4の1小節で、先頭に Tempo **90.00**。TimeSig と Tempo だけを含む。BGDawn と Segment1 はどちらも **Play From Start** で自然終了まで再生した。再起動後の初回録音では、自動で読み込まれた Project の中で Segment1 だけを開き、先に再生した文書はない。 [入力 BGDawn](../../../work/original-gui/SEG-14-wave-tempo-restart/20261112T032849570120Z/inputs/FarmGameProject/BGDawn.sgp)
- Producer のタイトルバー右上の **閉じる** を使って通常終了した。毎回、アプリ操作ツールの一覧で Producer のウィンドウが0件になった後に同じ EXE を起動し、新しいメインウィンドウが1件であることを確認した。ウィンドウ ID は最初から **121966676 → 30347256 → 11734112 → 9113166**。 [起動・終了と画面の照合](../../../work/original-gui/SEG-14-wave-tempo-restart/20261112T032849570120Z/audit.json)

## x100%で90 BPMを先に再生してから再起動

BGDawn では Tempo 行と Current Tempo に **90.00**。自然終了時の Elapsed は **2.678秒**。直後に同じ Producer で Segment1 を再生すると、Wave の間隔は約 **2.667秒**、自然終了は **8.011秒**だった。第3小節の Wave をクリックして選ぶと、赤い波形と **Wave 00:04.250** が出る。Current Tempo は空欄で、再生中の Elapsed 5.950秒に対する Offset は4.462秒。

![無音の BGDawn。先頭の Tempo 90.00、Current Tempo 90.00、x100%](figures/waverestart-05-prime90.jpg)
![90.00 を先行再生。赤い位置線が進む](figures/waverestart-06-prime90-playing.jpg)
![先行再生の自然終了。Elapsed 2.678秒](figures/waverestart-07-prime90-ended.jpg)
![Segment1 の第3小節の Wave を選択。Wave 00:04.250、Current Tempo は空欄](figures/waverestart-09-before-exit-selected.jpg)
![再生中。Elapsed 5.950秒、Offset 4.462秒](figures/waverestart-before-exit-100-middle.jpg)
![自然終了。Elapsed 8.011秒、Offset 0](figures/waverestart-before-exit-100-ended.jpg)

Producer を閉じて同じコピーを起動し直すと、Project Tree に FarmGameProject が折りたたまれた状態で出た。中央にエディターはなく、Transport の Tempo 欄は灰色の **120**、右端は **x100%**。Segment1 を開くと Tempo 欄は空欄、第3小節を選んだときの **Wave 00:04.250** は同じ。初回再生の発音間隔は **2.000秒**、自然終了は **6.006秒**。Elapsed 3.746秒のフレームでは Offset も3.746秒だった。

![x100%の再起動直後。Project が折りたたまれ、Tempo 欄は灰色の120](figures/waverestart-12-first-start.jpg)
![新しい Producer で Segment1 を開く。Current Tempo は空欄](figures/waverestart-14-restart-target.jpg)
![再起動後に同じ第3小節を選択。Wave 00:04.250](figures/waverestart-15-restart-selected.jpg)
![再起動後の初回再生。赤い位置線と Variation 1](figures/waverestart-first-new-100-playing.jpg)
![初回再生の途中。Elapsed、Offset ともに3.746秒](figures/waverestart-first-new-100-middle.jpg)
![再起動後の自然終了。Elapsed 6.006秒](figures/waverestart-first-new-100-ended.jpg)

## x150%の設定と初回再生

Transport Controls の倍率ボタンを右クリックして **Alternate Tempo** に150を入れ、**OK**。x150%を表示したまま BGDawn を再生すると、Current Tempo は90.00、自然終了は **1.780秒**。続けて再生した Segment1 は **135 BPM相当**、自然終了は **5.336秒**だった。

![倍率ボタンの右クリックで開く Alternate Tempo](figures/waverestart-20-alt-dialog.jpg)
![倍率欄に150を入力](figures/waverestart-22-alt-value.jpg)
![確定後の Transport Controls。x150%](figures/waverestart-23-alt-150.jpg)
![x150%の先行再生中。Current Tempo と Tempo 行は90.00](figures/waverestart-31-prime90-alt150-playing.jpg)
![先行再生の自然終了。Elapsed 1.780秒](figures/waverestart-32-prime90-alt150-ended.jpg)
![先行再生後の Segment1。再生中の Current Tempo は空欄](figures/waverestart-before-exit-150-playing.jpg)
![135 BPM相当の自然終了。Elapsed 5.336秒](figures/waverestart-before-exit-150-ended.jpg)

Project を開いたまま再び Producer を閉じ、起動し直すと **x150%** が残った。エディターを開く前の Tempo 欄は灰色の **120**。Segment1 を最初に再生すると、発音間隔は **2.000秒**、自然終了は **6.006秒**だった。その後、x150%のボタンに触れずに BGDawn を再生すると、BGDawn の自然終了は **1.789秒**。その次の Segment1 は再び **135 BPM相当** になった。終了前の有効な倍率は新しい Producer でも Tempo Track を再生した時に働き、Tempo Track のない Segment の初回再生は両倍率とも120 BPM相当だった。補正を無効にした状態での再起動は未確認。

![150%で終了した後の再起動直後。灰色の120と x150%](figures/waverestart-35-second-start.jpg)
![再起動後の x150%。Segment1 の Current Tempo は空欄、Wave 00:04.250](figures/waverestart-38-second-selected.jpg)
![x150%を表示した初回再生。Elapsed 1.497秒、Offset 1.496秒](figures/waverestart-first-new-after-150-playing.jpg)
![x150%を表示した初回の自然終了。Elapsed 6.006秒](figures/waverestart-first-new-after-150-ended.jpg)
![倍率を操作せず再起動後の BGDawn を再生。90.00 と x150%](figures/waverestart-44-prime-new-alt150-playing.jpg)
![再起動後の BGDawn の自然終了。Elapsed 1.789秒](figures/waverestart-45-prime-new-alt150-ended.jpg)
![BGDawn の後の Segment1。Elapsed と Offset の進み方が変わる](figures/waverestart-new-after-prime-150-playing.jpg)
![再起動後にテンポを先行再生した条件。自然終了5.345秒](figures/waverestart-new-after-prime-150-ended.jpg)

## 録音による照合

| Segment1 の再生条件 | Alternate Tempo | 1小節の発音間隔 | 実測 BPM 相当 | 自然終了時の Elapsed | 録音 |
| --- | --- | ---: | ---: | ---: | --- |
| BGDawn 90.00 の直後、再起動前 | x100% | 2.667秒 | 90.00 | 8.011秒 | [WAV](../../../work/original-gui/SEG-14-wave-tempo-restart/20261112T032849570120Z/captures/before-exit-100/output.wav) |
| x100%で再起動した後の初回 | x100% | 2.000秒 | 120.00 | 6.006秒 | [WAV](../../../work/original-gui/SEG-14-wave-tempo-restart/20261112T032849570120Z/captures/first-new-100/output.wav) |
| BGDawn 90.00 の直後、再起動前 | x150% | 1.778秒 | 135.00 | 5.336秒 | [WAV](../../../work/original-gui/SEG-14-wave-tempo-restart/20261112T032849570120Z/captures/before-exit-150/output.wav) |
| x150%で再起動した後の初回 | x150% | 2.000秒 | 120.00 | 6.006秒 | [WAV](../../../work/original-gui/SEG-14-wave-tempo-restart/20261112T032849570120Z/captures/first-new-after-150/output.wav) |
| x150%で再起動し、BGDawn 90.00 を再生した後 | x150% | 1.778秒 | 135.00 | 5.345秒 | [WAV](../../../work/original-gui/SEG-14-wave-tempo-restart/20261112T032849570120Z/captures/new-after-prime-150/output.wav) |

既定の Windows 出力を48 kHz、32-bit float、ステレオで取得した。原 WVP の冒頭180 msとの相関で各録音の3音を同定し、3,072クロック離れた2つの発音間隔から BPM 相当を算出した。15音すべてで相関は0.973以上。開始後300〜540 msの主な周波数は488.426〜488.492 Hzで、原 PCM は488.409 Hz。記録開始直後のパケット不連続は各 WAV に1件あり、発音の開始前から最後の音の終了までの区間にはなかった。 [測定コード](../../../work/original-gui/SEG-14-wave-tempo-restart/20261112T032849570120Z/measure.py) · [発音開始フレームと周波数](../../../work/original-gui/SEG-14-wave-tempo-restart/20261112T032849570120Z/wave-tempo-audio-measurements.json)

## Snap To Zero をオフにして再起動

x150%の再起動前に、Project Tree の **SfxCow.wvp** をダブルクリックし、波形面で右クリックした。**Snap To Zero** の初期チェックを確認してオフにした。同じ **Zoom 1:64** の画面で `(527, 360) → (699, 360)` をドラッグすると、赤い範囲が出て **Samples 22016 thru 33024**。アプリ終了前に Wave 文書を保存する操作は行っていない。

再起動後、Segment1 の初回再生を終えてから同じ SfxCow を開くと、赤い範囲は消えており、右クリックメニューの **Snap To Zero** はチェックあり。同じドラッグの結果は **Samples 22008 thru 33010**。ステータスバーの表示時刻は両方とも0.500〜0.750秒だが、サンプル位置は異なる。

| 状態 | 選択始点と直前の PCM | 選択終点と直前の PCM | 選択長 |
| --- | --- | --- | ---: |
| 終了前、Snap To Zero オフ | 22,016、−2377 → −1342 | 33,024、−1009 → 692 | 11,009 |
| 再起動後、Snap To Zero オン | 22,008、103 → −1077 | 33,010、774 → −99 | 11,003 |

両端のサンプル値は16-bit PCM から確認した。前回は同じ Producer の中で Project を閉じて開き直してもオフを保持した。今回の通常終了と別起動ではチェックがオンに戻り、選択にも適用された。 [同じ Producer 内の再読込](seg-14-wave-editor-loop-options.md#保存で復元される値とエディターの状態) · [SfxCow.wvp の入力](../../../work/original-gui/SEG-14-wave-tempo-restart/20261112T032849570120Z/inputs/FarmGameProject/SfxCow.wvp)

![SfxCow の右クリック。終了前の初期値はチェックあり](figures/waverestart-25-snap-initial-menu.jpg)
![Snap To Zero をオフにした後のメニュー](figures/waverestart-27-snap-off-menu.jpg)
![オフの選択。ステータスバーは Samples 22016 thru 33024](figures/waverestart-29-snap-off-selection.jpg)
![再起動後の SfxCow。赤い選択範囲が消え、Snap To Zero にチェックあり](figures/waverestart-40-snap-after-restart-menu.jpg)
![再起動後の同じドラッグ。Samples 22008 thru 33010](figures/waverestart-42-snap-on-selection.jpg)

## Project を閉じてから終了

x100%に戻して **File > Close Project FarmGameProject** を選び、Project Tree が空になった状態で Producer を終了した。再起動後もツリーは空で、x100%と灰色の120を表示した。Project を開いたまま終了した2回では、起動時に Project のルート行が復元され、Segment や Wave の編集画面は閉じていた。

凍結した28ファイルを照合し、作業コピーは `FarmGameProject.pro` のみハッシュが変わった。最後は Project を閉じた Producer 1件で **Project Tree、Standard Toolbar、Status Bar、Transport Controls** を表示した。青い光と黒い矢印は操作ツールの表示。 [掲載画像の出典とハッシュ](../../../work/original-gui/SEG-14-wave-tempo-restart/20261112T032849570120Z/published-figures.json) · [最終ウィンドウ一覧](../../../work/original-gui/SEG-14-wave-tempo-restart/20261112T032849570120Z/captures/57-final-windows.json)

![倍率を x100%に戻す](figures/waverestart-50-alt-restored.jpg)
![Close Project 後の空の Project Tree](figures/waverestart-53-project-closed.jpg)
![Project を閉じて終了した後の再起動直後。Project Tree は空](figures/waverestart-55-clean-start.jpg)
![最後の View メニュー。標準の4項目にチェック](figures/waverestart-56-final-view.jpg)

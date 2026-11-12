# SEG-14 Tempo Track を含まない Wave Segment の再生速度

同じ Producer 5.3.0.900 の作業コピー1件で、試験 Project の4文書を切り替えた。既定の Windows 出力を録音し、Wave の発音間隔と画面の **Current Tempo、Wave、Elapsed、Offset** を比較した。すべての試聴は **Play From Start** から自然終了まで。最初の測定前に試験用の有テンポ Segment を開いたが、再生はしていなかった。 [試験条件と入力のハッシュ](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/run.json)

## 試験素材と初期表示

| 文書 | 長さ | トラックと用途 |
| --- | --- | --- |
| `Segment1.sgp` | 4/4、3小節 | TimeSig と Wave。Tempo Track を含まない測定対象 |
| `BGDawn.sgp` | 4/4、1小節 | 冒頭の Tempo 90.00 と TimeSig。音のない先行再生 |
| `SSBird.sgp` | 4/4、1小節 | 冒頭の Tempo 125.00 と TimeSig。音のない先行再生 |
| `BGNight.sgp` | 4/4、3小節 | 冒頭の Tempo 90.00、TimeSig、Segment1 と同一の Wave Track |

両測定対象の PChannel 1、レイヤー a、Variation 1 に同じ **SfxAlarm** を3回置いた。物理開始は **384、3,456、6,528** 音楽クロック、論理開始は **0、3,072、6,144**。3件とも長さは2,248クロック、ループなし。発音開始の間隔は1小節、3,072クロック。2つ目の Wave Part は空。素材は調査用コピー内で生成し、開始値は保存ファイルから読み直した。 [Tempo Track なしの入力](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/inputs/FarmGameProject/Segment1.sgp) · [明示90 BPM の入力](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/inputs/FarmGameProject/BGNight.sgp) · [試験素材の生成スクリプト](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/prepare.py)

Segment1 では **Current Tempo** の欄が空で、TimeSig が先頭に出る。項目を選ぶと赤くなり、1件目は **Wave 00:00.250**、3件目は **Wave 00:04.250**。Tempo 90.00 の BGNight では Tempo 行と Current Tempo に **90.00** が出て、同じ音楽クロック位置の3件目は **Wave 00:05.666**。文書ごとの横ズームは開始素材のまま使った。

![Segment1。Current Tempo は空。3小節に SfxAlarm が1つずつ並ぶ](figures/wavetempo-03-no-tempo-target.jpg)
![Segment1 の1件目を選択。Wave 00:00.250](figures/wavetempo-04-first-item-static.jpg)
![Segment1 の3件目を選択。Wave 00:04.250](figures/wavetempo-05-third-item-static.jpg)
![無音の BGDawn。先頭の Tempo 90.00 と TimeSig](figures/wavetempo-06-open-prime90.jpg)
![無音の SSBird。先頭の Tempo 125.00 と TimeSig](figures/wavetempo-08-open-prime125.jpg)
![Tempo 90.00 の対照 BGNight。3件目は Wave 00:05.666](figures/wavetempo-16a-explicit-wave-static.jpg)

## 直前に再生した Tempo Track

**x100%** の状態で、先行再生なし、無音90 BPM の自然終了後、無音125 BPM の自然終了後を比較した。先行文書では再生中も Tempo 行と Current Tempo に指定値が出た。各文書の終了を緑の再生ボタンで確認してから **Window** メニューで Segment1 に切り替え、同じ操作で試聴した。Segment1 は同じ SGP を使用した。

| Segment1 の直前の条件 | 1小節の発音間隔 | 実測 BPM 相当 | 自然終了時の Elapsed |
| --- | ---: | ---: | ---: |
| この試験内で先行再生なし | 2.000秒 | 120.00 | 6.010秒 |
| BGDawn 90.00 の終了後 | 2.667秒 | 90.00 | 8.009秒 |
| SSBird 125.00 の終了後 | 1.920秒 | 125.00 | 5.768秒 |

3件目を選択したときの **Wave 00:04.250** と空欄の **Current Tempo** は、先行再生が変わっても同じだった。再生中の赤い位置線と **Offset** は実際の進行に追従し、**Elapsed** と進み方が異なった。自然終了では Transport が緑の再生ボタンに戻り、Offset は0に戻る。 [先行再生なしの録音](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/captures/baseline-verified/output.wav) · [90 BPM と125 BPM の比較録音](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/captures/prime-comparison/output.wav)

![初期状態の Alternate Tempo ダイアログ。100%](figures/wavetempo-10-alternate-initial.jpg)
![先行再生なし。Elapsed 3.649秒、Offset 3.649秒](figures/wavetempo-11-initial100-target-late.jpg)
![先行再生なしの自然終了。Elapsed 6.010秒](figures/wavetempo-11-initial100-target-ended.jpg)
![90 BPM の準備 Segment を再生中](figures/wavetempo-12-after90-100-prime-playing.jpg)
![準備 Segment の自然終了。Elapsed 2.670秒](figures/wavetempo-12-after90-100-prime-ended.jpg)
![90 BPM の終了後に Segment1 を再生。Elapsed 3.681秒、Offset 2.761秒](figures/wavetempo-12-after90-100-target-late.jpg)
![90 BPM の終了後。Segment1 は Elapsed 8.009秒](figures/wavetempo-12-after90-100-target-ended.jpg)
![125 BPM の準備 Segment を再生中](figures/wavetempo-13-after125-100-prime-playing.jpg)
![準備 Segment の自然終了。Elapsed 1.930秒](figures/wavetempo-13-after125-100-prime-ended.jpg)
![125 BPM の終了後に Segment1 を再生。Elapsed 3.665秒、Offset 3.816秒](figures/wavetempo-13-after125-100-target-late.jpg)
![125 BPM の終了後。Segment1 は Elapsed 5.768秒](figures/wavetempo-13-after125-100-target-ended.jpg)

## Alternate Tempo を切り替える順序

Transport Controls 右端の **x100%** を右クリックすると **Alternate Tempo** が開く。数値欄に150を入力して **OK** を押すと、ボタンは **x150%** になった。左クリックで有効・無効を切り替えられ、無効時もラベルは **x150%** のまま。以下では同じ **125.00 BPM、1小節** の SSBird を先行再生した。

| SSBird の再生時 | SSBird の終了後、Segment1 の前 | SSBird 終了時の Elapsed | Segment1 の発音間隔 | 実測 BPM 相当 | Segment1 終了時の Elapsed |
| --- | --- | ---: | ---: | ---: | ---: |
| x100% | Segment1 側で x150% を入力し OK | 1.930秒 | 1.920秒 | 125.00 | 5.763秒 |
| x150% 無効 | 同じ表示のまま | 1.934秒 | 1.920秒 | 125.00 | 5.764秒 |
| x150% 有効 | 同じ表示のまま | 1.290秒 | 1.280秒 | 187.50 | 3.846秒 |
| x150% 有効 | Segment1 側で左クリックして無効 | 1.289秒 | 1.280秒 | 187.50 | 3.843秒 |

125 BPM の再生が終わってから倍率を入力または無効化した場合、次の Tempo Track なしの再生には、先に鳴らしたときの速度が残った。125 BPM の Tempo Track を **x150% 有効**で再生した場合は、その後の Segment1 も187.5 BPM相当。SSBird の **Tempo 125.00** と Segment1 の **Wave 00:04.250** は、どちらの実測速度でも同じ表示だった。 [倍率を後から入力した試行と無効で再生した試行の録音](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/captures/modifier-retry/output.wav) · [倍率を先に有効にした録音](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/captures/modifier-before-tempo/output.wav) · [有効で再生した後に無効化した録音](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/captures/modifier-after-tempo/output.wav) · [Alternate Tempo の基本操作](ui-13-transport-options-live.md)

![125 BPM の終了後、Segment1 側で150%を入力](figures/wavetempo-14a-set150-dialog.jpg)
![OK 後に x150%。Wave の位置と表示時刻は同じ](figures/wavetempo-14a-set150-applied.jpg)
![125 BPM を鳴らした後で x150% を適用。Elapsed 3.640秒、Offset 3.791秒](figures/wavetempo-14-after125-150-target-late.jpg)
![終了時は5.763秒](figures/wavetempo-14-after125-150-target-ended.jpg)
![左クリックで無効にしてもラベルは x150%](figures/wavetempo-15a-150-disabled.jpg)
![x150% を無効にした状態で125 BPM を再生。自然終了1.934秒](figures/wavetempo-15-after125-disabled150-prime-ended.jpg)
![x150% 無効で先行再生した後。Segment1 の自然終了5.764秒](figures/wavetempo-15-after125-disabled150-target-ended.jpg)
![先行再生する前に x150% を有効にする](figures/wavetempo-16b-150-enabled-before-prime.jpg)
![x150% 有効で125 BPM の準備 Segment を再生。自然終了1.290秒](figures/wavetempo-16-prime125-enabled150-prime-ended.jpg)
![同じ x150% で Segment1。Elapsed 3.619秒、Offset 5.654秒](figures/wavetempo-16-prime125-enabled150-target-late.jpg)
![先に倍率を有効にした場合は自然終了3.846秒](figures/wavetempo-16-prime125-enabled150-target-ended.jpg)
![x150% 有効で125 BPM を再生した後に無効へ切り替え](figures/wavetempo-18a-disabled-after-prime.jpg)
![後から無効にしても Elapsed 3.659秒、Offset 5.716秒](figures/wavetempo-18-prime150-then-disable-target-late.jpg)
![後から無効にした場合も自然終了3.843秒](figures/wavetempo-18-prime150-then-disable-target-ended.jpg)

## 明示した Tempo、Project の再読込、数値編集

SSBird 125 BPM の直後、Tempo **90.00** を自分で持つ BGNight を再生した。x100% では Wave の間隔は **2.667秒（90 BPM）**、x150% 有効では **1.778秒（135 BPM）**。どちらも再生中の Current Tempo と Tempo 行は **90.00**。x150% では Elapsed 3.617秒に対して Offset 5.423秒だった。x100% に戻した比較では Elapsed 3.632秒、Offset 3.631秒。

BGNight を x100% で自然終了させて **Close Project**。同じ Producer で Project を開き直し、Segment1 だけを再生した。録音は2.667秒間隔で、再読込前に明示した90 BPM相当だった。次に BGDawn の先頭の **Tempo Properties** で120.00を入力して Tab で確定した。Tempo 行と Current Tempo は120.00になった。ここで BGDawn を再生せず Segment1 を試聴すると、まだ2.667秒間隔だった。BGDawn を実際に120 BPMで再生してから Segment1 を再試聴すると、開始時と同じ2.000秒間隔に戻った。閉じる際の保存確認で **No All** を選び、BGDawn の120.00編集は破棄した。

| 測定対象と条件 | 発音間隔 | 実測 BPM 相当 | 自然終了時の Elapsed |
| --- | ---: | ---: | ---: |
| BGNight、先行125 BPM、x150% 有効 | 1.778秒 | 135.00 | 5.348秒 |
| BGNight、先行125 BPM、x100% | 2.667秒 | 90.00 | 8.002秒 |
| Segment1、同じ Producer で Project を再読込 | 2.667秒 | 90.00 | 8.004秒 |
| Segment1、BGDawn の Tempo を120.00に編集した後 | 2.667秒 | 90.00 | 8.007秒 |
| Segment1、BGDawn を120.00で再生した後 | 2.000秒 | 120.00 | 6.007秒 |

後続の通常終了と別起動では、x100%とx150%のどちらも、最初に再生した Segment1 は120 BPM相当だった。x150%の設定は残り、同じ起動内で明示 Tempo Track を再生すると倍率が効いた。 [Producer 再起動後の比較](seg-14-wave-tempo-restart.md) · [Project 再読込後の録音](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/captures/reopen-context/output.wav) · [120.00の入力前後の比較録音](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/captures/restore-context/output.wav)

![明示90 BPM の BGNight を x150% で再生。Tempo は90.00、Offset 5.423秒](figures/wavetempo-17-explicit90-enabled150-target-late.jpg)
![BGNight、x150% の自然終了5.348秒](figures/wavetempo-17-explicit90-enabled150-target-ended.jpg)
![対照のため Alternate Tempo を100%に戻す](figures/wavetempo-19a-set100-dialog.jpg)
![明示90 BPM、x100%。Elapsed 3.632秒、Offset 3.631秒](figures/wavetempo-19-explicit90-100-target-late.jpg)
![明示90 BPM、x100% の自然終了8.002秒](figures/wavetempo-19-explicit90-100-target-ended.jpg)
![明示90 BPM の再生後に Project を閉じた同じ Producer](figures/wavetempo-20-close-project-after90.jpg)
![Project を再読込。Tempo Track のない Segment1 のみを開く](figures/wavetempo-21-reopen-no-tempo-target.jpg)
![再読込直後の試聴。Elapsed 3.644秒、Offset 2.732秒](figures/wavetempo-22-reopen-after90-target-late.jpg)
![再読込直後の自然終了8.004秒](figures/wavetempo-22-reopen-after90-target-ended.jpg)
![試験用 BGDawn の Tempo Properties。先頭は90.00](figures/wavetempo-23-restore120-tempo-properties.jpg)
![120.00を確定。Tempo 行と Current Tempo が120.00](figures/wavetempo-24-tempo-restored120-unsaved.jpg)
![120.00の文書をまだ再生していない場合。Segment1 は8.007秒](figures/wavetempo-25-edited120-before-play-target-ended.jpg)
![120.00に編集した無音 BGDawn を再生](figures/wavetempo-26-after120-100-prime-playing.jpg)
![120.00の再生後。Segment1 は6.007秒](figures/wavetempo-26-after120-100-target-ended.jpg)
![Close Project で未保存の BGDawn.sgp の保存確認](figures/wavetempo-27-close-restore-tempo-prompt.jpg)

## 録音、画面の時刻、保存値の照合

録音は Windows の既定出力を **48 kHz、32-bit float、ステレオ** で取得した。元 WVP の冒頭180 msに正規化相関で合わせて3音の開始を特定し、間隔2つの平均と保存値の3,072クロックから BPM 相当を求めた。冒頭の相関は36音すべてで0.9709以上。開始後300〜540 msの主な周波数は元のPCMで488.409 Hz、録音した36音で488.425〜488.521 Hzだった。発音間隔が1.280〜2.667秒に変わった条件でも、同じ波形の音高を確認できた。

自然終了までの Elapsed は12拍を実測 BPM 相当で換算した時刻と約15 ms以内で一致した。選択項目の **Wave** は文書内の時刻、**Elapsed** は試聴の実時間、**Offset** は再生カーソル位置の文書内時刻として振る舞った。今回の値では、Tempo Track なしの表示時刻は120 BPM、Tempo 90.00 の文書では90 BPMで音楽クロックを換算すると画面と一致する。

比較に使った録音は7本、測定は12条件。最初の録音手順とダイアログ操作の一部は途中で止まったため、完了して3音を照合できた録音を表に載せた。開始素材28件を照合し、試験コピーでは `FarmGameProject.pro` のハッシュだけが変わった。試聴速度は120 BPM相当に戻し、Alternate Tempo は開始時の x100% 有効、Project を閉じた Producer 1件、View は標準4項目を確認した。 [音の開始と周波数の測定](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/wave-tempo-audio-measurements.json) · [測定スクリプト](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/measure-wave-tempo.py) · [入力・画面・録音の照合](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/wave-tempo-context-audit.json) · [掲載画像の寸法とハッシュ](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/published-figures.json) · [終了時のウィンドウ](../../../work/original-gui/SEG-14-wave-tempo-context/20261112T024658340336Z/final-windows.json)

![終了時の View。Project Tree、Standard Toolbar、Status Bar、Transport Controls のみ有効](figures/wavetempo-28-final-view.jpg)

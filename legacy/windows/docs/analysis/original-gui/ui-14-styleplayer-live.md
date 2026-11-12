# UI14　StylePlayer の実機操作と音声

2026-11-11。配布版 `StylePlayer.exe` を作業フォルダーに複製し、同じディレクトリに Style 187件、ChordMap 27件と説明書を置いた。実行ファイルを含む216ファイルの SHA-256 を元資料と照合した。操作・観測ログ94件、採取画像105枚から54枚を掲載する。青い光と黒い矢印は操作ツールの表示。 [実行記録と入力ハッシュ](../../../work/original-gui/UI14-styleplayer/20261111T132150928505Z/run.json) · [画像の出典・寸法・ハッシュ](../../../work/original-gui/UI14-styleplayer/20261111T132150928505Z/published-styleplayer-figures.json) · [配布資料の構成](ui-14-companion-apps.md#styleplayer)

## 起動、自動再生、配置

タイトルは **StylePlayer**、親ウィンドウは247×437 px。この環境では通常のメニューバーはなく、タイトル左端にアイコン、右端に閉じるボタンがある。

| 位置 | 項目と配置 |
| --- | --- |
| Create Segment | Style、ChordMap、Band、Shape の4行。左に項目名、右に同幅のコンボ |
| 同じ枠の中段 | 左に Intro、右に End のチェック。下に Length の数値欄と Measures |
| 同じ枠の下段 | Re-Compose |
| Play Segment & Style Motifs | Play と Stop が横並び。その下に緑の横長の進捗バー、さらに下に2列2行の Motif ボタン |

素材ありの1回目は **Rock I / Boogie / Default**。最初に全面を撮れた時点では Stop は灰色で、進捗は0。起動前からの録音では、Play を操作する前に音が出た。2回目は **Shuffle / Funk parallel / Default**。起動直後に Stop は有効、進捗の取得値は565から3,392へ増え、Play を押さずに再生が進んだ。操作せず待つと Stop が灰色に戻り、進捗は0になった。両回で **Shape は Song、Length は12、Intro と End はオン**。前の実行で最後に選んだ Style、Shape、Length は次の起動に引き継がれなかった。 [1回目の起動音の録音](../../../work/original-gui/UI14-styleplayer/20261111T132150928505Z/captures/audio-initial/output.wav) · [起動時刻と録音の照合](../../../work/original-gui/UI14-styleplayer/20261111T132150928505Z/captures/audio-initial/analysis.json) · [同梱の説明書](../../../../../work/producer/style-library/StylePlayer.txt)

![1回目。Rock I の表示と停止時の画面](figures/sp-02-visible.jpg)

![2回目の起動直後。Shuffle が自動再生](figures/sp-91-repeat-startup.jpg)

![Play を押さずに進捗が前へ進む](figures/sp-92-repeat-startup-progress.jpg)

![自然終了後。Stop は灰色、進捗は0](figures/sp-93-repeat-natural-stop.jpg)

## 選択リストと Style の切替

Style リストには縦スクロールバーが出る。Rock I の位置から開くと Rock I、Rock II、Rock III などが見える。ChordMap は **Boogie** から **tension** まで27候補が1つのポップアップに収まった。Shape の順序は **Falling、Level、Loopable、Loud、Peaking、Quiet、Random、Rising、Song**。矢印をクリックして開き、値を変えずに Esc を押すと、各リストは閉じた。

![Style リストの Rock I 周辺](figures/sp-03-style-list.jpg)

![ChordMap の候補](figures/sp-05-chordmap-list.jpg)

![Shape の9候補](figures/sp-09-shape-list.jpg)

**Rock I** の Band は **Appalachian、Default、Electric Buzz、Gnarley Dude、Honky Tonk、Lounge、Lush、Organ、Synth** の9件。4つの Motif は **Guitar Riff、Brassy、Piano Riff、Baritone**。**Rumbling Point** に変えると、Band は **Custom5** だけ、Motif は4つとも **No Motif** の灰色表示になった。ChordMap は選択していた **Classical** から **minor pedal poin** に変わり、Shape の **Rising**、Length の **32**、Intro と End がオフの状態は残った。Style 変更の後に Stop が有効になり、進捗が増えた。配布素材の旧形式 `AASY` 内の Band も、Rock I が9件、Rumbling Point が1件だった。 [素材の Band 名とチャンクの一覧](../../../work/original-gui/UI14-styleplayer/20261111T132150928505Z/style-inventory.json)

![Rock I の Band 9件](figures/sp-07-band-list.jpg)

![Rumbling Point に切替。ChordMap と Band が更新され、4つとも No Motif](figures/sp-45-rumbling-playing.jpg)

![Rumbling Point の Band は Custom5 の1件](figures/sp-46-rumbling-band-list.jpg)

### キーボードで選択した場合

停止した Rumbling Point で Style リストを開き、**Home** を押すと先頭の **20th C. Chorale** がハイライトされた。リストが開いたまま ChordMap は **classical colors**、Band は **Default**、Motif は **Arpeggio、Twinkle、Crash Boom、Melodic** になり、Stop と進捗も動き始めた。Esc で閉じた後も **20th C. Chorale** のまま再生が続いた。キーでリスト内を移動した時点で、依存する項目と演奏が更新される例。

![Style リストを開いたまま Home で先頭へ移動](figures/sp-81-style-list-home.jpg)

![Esc で閉じた後。選択と演奏が切替先のまま](figures/sp-83-style-cancel-settled.jpg)

## Length の入力と境界

Length にフォーカスし、Home と Shift+End で値全体を選べた。停止中の Rock I で **12 → 32** と入力した直後の画像では Stop は灰色、進捗0。その後 Tab で離れると Intro に点線のフォーカス枠が付き、Stop が有効になり、進捗が935から18,472へ進んだ。

![Length の既存値を選択](figures/sp-13-length-selection.jpg)

![32を入力した時点では停止表示](figures/sp-15-length-32-edit.jpg)

![Tab で Length を離れた後。Intro にフォーカス枠、Stop 有効](figures/sp-16-length-commit.jpg)

![しばらくして進捗バーが伸びた状態](figures/sp-17-length-playing-later.jpg)

![Stop を押して進捗が消えた状態](figures/sp-18-stop-before-audio.jpg)

Rumbling Point の停止中に **0** を入力して Tab で離れると、表示は0を保持し、警告は出なかった。自動再生は始まらなかったが、**Play** を押すと0の表示のまま演奏が始まり、進捗は3,581まで増えた。その後 **Re-Compose** を押しても、進捗は21,757から24,395へ進み、先頭へ戻らなかった。この操作で0小節の Segment が生成されたかは確認できていない。**24** に戻して入力欄を離れた後は、進捗が先頭付近から増えた。

![0を入力した編集中の状態](figures/sp-55-length-zero-edit.jpg)

![0のまま欄を離れ、時間をおいても停止](figures/sp-57-length-zero-settled.jpg)

![Length 0のまま Play を押した後](figures/sp-59-zero-play-result.jpg)

![0のまま再生中。Re-Compose を押す前](figures/sp-60-zero-before-recompose.jpg)

![0のまま Re-Compose 後。進捗が継続](figures/sp-62-zero-recompose-settled.jpg)

![24へ戻した後。先頭付近から再生](figures/sp-69-valid-recovery.jpg)

## 設定変更と再生位置

進捗は画面のバーとアクセシビリティの生の値を照合した。下表の数値はツールで取得したコントロールの値。画面に表示される百分率や時刻ではない。変更直後のアクセシビリティ表示に前の状態が残ることがあったため、次の取得も確認した。 [各画像の時刻と進捗値](../../../work/original-gui/UI14-styleplayer/20261111T132150928505Z/run.json)

| 操作 | 変更前 → 変更後 | 観測 |
| --- | --- | --- |
| 再生中に Band を Default から Electric Buzz | 12,652 → 27,573 | リストを開いている間も進捗が進み、選択後も途中から継続 |
| 再生中に Brassy | 32,217 → 33,439 | 主演奏の進捗は前進 |
| 再生中に Shape を Song から Rising | 38,881 → 136 | 先頭付近に戻り、次の撮影では7,805 |
| 停止中に ChordMap を Boogie から Classical | 0 → 4,078 | Stop が有効になり演奏が始まる |
| Rumbling Point の再生中に Intro をオン | 24,490 → 1,129 | 先頭付近から再始動 |
| 続けて End をオン | 14,538 → 1,166 | 先頭付近から再始動 |
| 再生中に Re-Compose | 26,565 → 1,464 | 先頭付近から再始動 |
| 再生中に Play を押し直す | 20,002 → 801 | Play は再生中も有効で、押し直すと先頭付近に戻る |
| 進捗バー右端寄りをクリック | 4,126 → 4,955 | 通常の増加が続き、クリック位置へのジャンプは見えなかった |
| Stop | 値0、バーの色が消える | Stop は灰色に戻り、Play は操作可能 |

### Band、Motif、Shape

![Play で開始して Band を替える前の進捗](figures/sp-22-primary-before-band.jpg)

![再生中に開いた Band リスト](figures/sp-23-band-during-play.jpg)

![Electric Buzz に変更。途中位置を保持して前進](figures/sp-24-electric-buzz.jpg)

![Brassy を押す前の進捗](figures/sp-25-before-brassy.jpg)

![Brassy を押した後も進捗が前進](figures/sp-26-brassy-playing.jpg)

![再生中に Shape リストを開いた画面](figures/sp-27-shape-during-play.jpg)

![Rising を選んだ直後。進捗が先頭付近に戻る](figures/sp-28-rising-restarts.jpg)

![Rising の新しい演奏の進捗](figures/sp-29-rising-progress.jpg)

### ChordMap、Intro、End、進捗バー

![停止中に Classical を選んだ後。進捗が出る](figures/sp-36-before-intro.jpg)

![Intro をオフにした直後](figures/sp-37-intro-off.jpg)

![End もオフにした直後](figures/sp-39-end-off.jpg)

![進捗バーをクリックする前](figures/sp-40-before-bar-click.jpg)

![進捗バー右端付近をクリック。バーはクリック位置へ移らない](figures/sp-41-progress-click.jpg)

![試聴を停止。バーは空](figures/sp-42-stop-settings.jpg)

![Rumbling Point の再生中。Intro をオンにする前](figures/sp-70-before-intro-on.jpg)

![Intro オン後、先頭付近から進む](figures/sp-72-intro-on-settled.jpg)

![End をオンにする前](figures/sp-73-before-end-on.jpg)

![End オン後、先頭付近から進む](figures/sp-75-end-on-settled.jpg)

### Re-Compose と Play

![停止中に Re-Compose を押した後](figures/sp-31-recompose-stopped.jpg)

![続いて進捗が前進](figures/sp-32-recompose-progress.jpg)

![再構成した試聴を Stop した後](figures/sp-33-stop-after-recompose.jpg)

![再生中に Re-Compose を押す前](figures/sp-76-before-active-recompose.jpg)

![Re-Compose 後。進捗が先頭付近に戻る](figures/sp-78-active-recompose-settled.jpg)

![再生中に Play を押し直す前](figures/sp-84-before-play-again.jpg)

![Play を押し直した後。進捗が先頭付近に戻る](figures/sp-86-play-again-position.jpg)

## Motif 単独と録音

Rock I を停止して **Guitar Riff** を押した。進捗は0、Stop は灰色のままだったが、録音ではクリック後に音が出た。Play を押して始める主演奏と、停止状態から押せる Motif は画面上の進捗を共有しない。再生中の **Brassy** は主演奏を止めずに操作できた。混合から Brassy の音だけを同定する比較は残る。 [操作中の録音](../../../work/original-gui/UI14-styleplayer/20261111T132150928505Z/captures/audio-controls/output.wav) · [操作時刻と PCM の照合](../../../work/original-gui/UI14-styleplayer/20261111T132150928505Z/captures/audio-controls/analysis.json)

![主演奏が停止したまま Guitar Riff を押した状態](figures/sp-19-motif-while-stopped.jpg)

![Guitar Riff の操作後。Stop は灰色のまま](figures/sp-20-motif-only-later.jpg)

録音2本は Windows 既定出力の WASAPI ループバック、48,000 Hz、32 bit 浮動小数点、ステレオ。左右を平均した RMS は下表の通り。各録音の開始直後0.05秒未満に不連続が1件ずつあり、その後は時刻エラー0、フレーム位置差は最大2フレーム。物理スピーカーでの音圧、Band ごとの音色の同定、Shape の Groove Level の推移、Intro と End のパターン内容は今回の照合範囲外。

| 録音区間 | RMS |
| --- | --- |
| 1回目の起動前 | 0 |
| 起動後0.8〜5.8秒、Play 操作前 | 0.07351 |
| 主演奏停止中の Guitar Riff | 0.07442 |
| Band 変更前 / 変更後 | 0.09438 / 0.08730 |
| 停止中の Re-Compose 後 | 0.02999 |
| Stop 後1.8秒からの比較区間（2回） | 0.00001315 / 0.00001077 |

## About、移動、終了、素材なしの起動

**Alt+Space** のシステムメニューには **移動**、**閉じる**、**About StylePlayer...** がある。About には **StylePlayer Version 1.0** と **Copyright (C) 20002 Microsoft Corp.** が表示され、OK で閉じる。EXE の **New** ダイアログに相当する入口は今回の操作では確認できなかった。 [ダイアログリソースと確認範囲](ui-14-companion-apps.md#styleplayer)

![Alt+Space のシステムメニュー](figures/sp-49-system-menu.jpg)

![About StylePlayer](figures/sp-50-about.jpg)

タイトルバーのドラッグで左上が (517,157) から (580,199) へ移動した。247×437 pxの寸法は維持され、右端を内側へドラッグした試行でも変わらなかった。素材ありの1回目はタイトルの閉じる、2回目は Alt+F4 で終了した。次を起動する前に、既存の StylePlayer がないことを確認した。

![タイトルバーで移動した後](figures/sp-88-title-drag.jpg)

![右端のドラッグを試した後](figures/sp-89-right-edge-drag.jpg)

素材を置いていない別のディレクトリで同じ EXE を起動すると、操作ツールの一覧にウィンドウが現れず、プロセスだけが残った。確認時のメインウィンドウハンドルは0。該当する Application Error イベントは確認できなかった。画面から閉じられないため、パスと PID が一致する素材なし試験のプロセスだけを終了した。 [素材なしの起動](../../../work/original-gui/UI14-styleplayer/20261111T132150928505Z/captures/empty-launch.json) · [試験用プロセスの終了](../../../work/original-gui/UI14-styleplayer/20261111T132150928505Z/captures/empty-process-exit.json)

最後に StylePlayer と録音器が0件、Producer は調査前と同じ1ウィンドウ。配布版 EXE と素材のハッシュも一致した。次は Segment エディターに戻り、Script Track の Timing と複数選択を追試する。 [Script Track の既存調査](seg-14-script-track.md)

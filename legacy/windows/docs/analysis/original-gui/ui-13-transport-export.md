# UI13　Transport、Synth Status、Wave・MIDI 書き出し

2026-11-11。基本再生・停止は原版の [SEG-11](seg-11-transport-position.md) と [SEG-12](seg-12-loop-boundary.md) で観測済み。ここでは補助ツールバーの配置、操作の入口、書き出しの条件をまとめる。原版コピーで撮影済みの2枚と同梱 HTML Help の図9枚を掲載する。Secondary の起動、設定ダイアログ、書き出し、実音声は現行環境で追試する。 [実行記録](../../../work/original-gui/UI13-14-transport-aux/20261111T083323468666Z/run.json) · [ヘルプ図の出典と寸法](../../../work/original-gui/UI13-14-transport-aux/20261111T083323468666Z/published-help-figures.json)

## Synth Status と表示の切替

通常の4項目を表示した状態から **View > Synth Status** を選ぶ。Transport Controls の下に **CPU、Voices、Memory Use、Latency、Reset** の行が現れた。1280×752 px の親ウィンドウ画像では Timeline 上端が y=105 から y=135 に移動した。同じ項目で隠すと y=105 に戻った。 [View メニューと表示前後](ui-01-view-toolbars.md)

![原版コピーで Synth Status だけを追加表示した状態](figures/toolbar-21-show-synth.jpg)

全ツールバー表示の撮影では、CPU と Voices に現在値・ピーク・時刻が並び、Memory Use は 3007K、Latency は 112ms だった。Secondary の4枠はいずれも空で、開始ボタンは灰色。書き出しは Wave と MIDI のファイル名がともに空で、参照用の **…** は見え、開始・終了用ボタンは灰色だった。

![原版コピーの全ツールバー表示。停止中で Secondary の4枠と Wave・MIDI の出力名が空](figures/toolbar-05-all-segment.jpg)

追試では同じ Segment を使い、停止、単一音、重なる音、停止直後、Reset 直後を撮る。現在値とピーク値の変化、リセット対象、MIDI/Performance Options の設定値と実測 Latency の違いを記録する。 [Synthesizer Status](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/synthesizerstatuswindow.htm)

**View > Output Status** は PChannel のレベルとミュート用の別ツールバー。ヘルプでは演奏した PChannel ごとにボタンが増え、終了まで表示が残る。単クリックでミュートを切り替え、ダブルクリックでソロ、右クリックで PChannel 名の編集。2つ以上の PChannel がある素材でメーターの変化、ミュート、ソロ解除、改名の影響を確認する。 [Output Status Toolbar](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/outputstatustoolbar.htm)

![ヘルプの Output Status。PChannel 5 と 10 のレベルが黄色のメーターに表示される](figures/ui13-transport-help-output.gif)

## Transport Options

**View > Transport Options**。左から AudioPath リスト、メトロノーム、Play Cursor、MIDI の切替、MIDI/Performance Options。Play Cursor 切替後の横追従と再生状態は [SEG-11](seg-11-transport-position.md) にある。

![ヘルプの Transport Options。左端の AudioPath 候補が開いている](figures/ui13-transport-help-options.gif)

メトロノームのボタンを右クリックすると設定を開く。第1拍と他の拍に Velocity と Note value があり、下段に PChannel と Count-In Bars が並ぶ。図では PChannel 10、Count-In 0、Record Only。ヘルプでは All Playback を選ぶと試聴にもカウントインが入り、その間は演奏本体と Play Cursor が待機する。 [Metronome Options](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/metronomeoptionsdialogbox.htm)

![ヘルプの Metronome Options。拍の種類ごとに強さと音高を設定する](figures/ui13-transport-help-metronome.gif)

右端の MIDI/Performance Options には MIDI 入力、エコーと出力 PChannel 範囲、レイテンシー、同時発音数、サンプルレート、既定 Synthesizer、GM Set とカスタム DLS の Download Status がある。実機では入力なし、選べる場合は MIDI 入力あり、AudioPath の切替前後で、有効状態と変更の保持を撮る。 [MIDI/Performance Options](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/midiperformanceoptionsdialogbox.htm)

![ヘルプの MIDI/Performance Options。左列が入力・エコー・レイテンシー、右列が音声初期化と音源のダウンロード](figures/ui13-transport-help-midi-performance.gif)

## Primary と Secondary

**View > Secondary Segment Toolbar**。左端のスピンは表示する組数。番号付きの各組に文書のリストと開始ボタンが付く。ヘルプでは最大99組。文書をエディターで開いてから候補に入る場合があり、狭い場合はツールバーを浮かせてサイズを変えられる。Secondary の開始ボタンは再生中に境界停止へ変わり、Primary の停止で Secondary も停止する。 [Secondary Segment Toolbar](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/secondarysegmenttoolbar.htm)

![ヘルプの Secondary Segment Toolbar。スピンを3にし、3組を表示した例](figures/ui13-transport-help-secondary.gif)

対象の開始ボタンを右クリックして **Secondary Segment Audition Options** を開く。左側に Performance Boundary、Controlling Segment、Cut Off。右側に Align Options と Timing。開始境界の指定は選んだ枠に対する試聴用で、ヘルプ上は保存した Segment の境界を維持し、Bookmark に試聴設定を保存できる。 [Secondary Segment Audition Options](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/secondarysegmentauditionoptions.htm)

![ヘルプの Secondary Segment Audition Options。境界・制御・同期を指定する](figures/ui13-transport-help-secondary-options.gif)

追試は別名の2本の Segment と Motif を使う。各文書を開く前後の候補、1組から3組への増減、一覧の選択、開始要求直後と次の境界、Primary の停止後を撮る。右クリックの Cancel と OK、別枠への影響、ドッキング中と浮動時の配置も照合する。

## Wave/MIDI Export Toolbar

**View > Wave/MIDI Export Toolbar**。左が Wave、右が MIDI。各組にファイル名表示、参照用の **…**、開始・終了用ボタンがある。元の演奏と出力を照合できるよう、短い Sequence と Wave 項目を含む作業用 Segment を使い、出力先は専用セッションに指定する。 [Capturing a Performance](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/capturingaperformance.htm)

![ヘルプの書き出しツールバー。左右の各組に出力名と開始・停止ボタンがある](figures/ui13-export-help-toolbar.gif)

| 状態 | Wave | MIDI |
| --- | --- | --- |
| 出力先を設定 | Wave 側の **…** から名前を指定 | MIDI 側の **…** から名前を指定 |
| 開始操作の直後 | 対応する出力ポートと FileOutput を用意して開始。演奏前に押すと先頭に無音が入り得る | 待機し、演奏を開始した時点で収録が始まる |
| 対象 | Wave 出力をサポートするポート経由の Segment、Pattern、DLS の音 | Segment と Pattern の MIDI メッセージ。Wave 項目の音声は出力対象外 |
| 演奏を停止 | 書き出しボタンを押して収録を終える | 演奏の停止とともに収録が終了 |

ヘルプでは Wave 側は、アクティブな AudioPath に対応する出力ポートがある場合に有効。対応ポートの選定と FileOutput の経路は別々に確認する。右クリックの **Set MIDI Export Start Time** は **Save lead-in measure** と **Start at bar 1** の2択。前者では現在の Transport 対象の拍子で空小節を1つ挿入する。 [Wave Export](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/waveexport.htm) · [MIDI Export](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/midiexport.htm)

![ヘルプの Set MIDI Export Start Time。Start at bar 1 を選択した状態](figures/ui13-export-help-midi-start.gif)

FileOutput は AudioPath の Effects List に入れる DMO。図では PChannels 1–128 の Mix Group が Stereo (L+R) のユーザー定義バッファーを使い、FileOutput1 へ出力する。複数バッファーに FileOutput を入れる場合は、設定名に連番を付けた Wave が作られるとヘルプにある。 [AudioPath の File Output](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/fileoutputinanaudiopath.htm) · [AudioPath Designer](ui-08-audiopath-designer.md)

![ヘルプの FileOutput を含む AudioPath。Mix Group とバッファーから FileOutput1 への対応](figures/ui13-export-help-file-output.gif)

実機での撮影は、文書なし、出力ポートと FileOutput の組み合わせ別の有効状態、名前指定の取消と確定、待機、収録、停止、同名ファイルの確認、出力失敗時の順。MIDI は両方の開始設定で先頭イベントの時刻と拍子、Wave は PCM・長さ・先頭の無音・実音声を確認する。MIDI 入力を Sequence や Pattern に記録する操作は、接続した入力デバイスと記録後のイベントを確認する。撮影後は追加表示したツールバーを隠す。

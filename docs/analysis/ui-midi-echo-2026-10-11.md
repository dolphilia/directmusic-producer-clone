# MIDI Echo と出力 PChannel 範囲 — 2026-10-11

画像3・4（SSP01/UI13）の基本Echo設定と、再生中SegmentへのソフトウェアMIDI配送を実装し、本体でCancel・失敗時復元・保存・別プロセス復元を確認した。停止中の発音、実機入力、音声、Advanced、録音、原版の全操作・外観比較は未達。全40責務・全8受入は維持し、全体は6作業中/2障害/0合格、fullAcceptance=false。

## 一次記録

- [単位記録](../../work/analysis/ui-midi-echo/20261011T062616160Z/unit-record-v1.json) SHA256 7489cf3382d20eae15a5505ee591760216eac7226b7b43ac488372b76f91832c
- [開始契約](../../work/analysis/ui-midi-echo/20261011T062616160Z/start-v1.json)、[範囲の明確化](../../work/analysis/ui-midi-echo/20261011T062616160Z/contract-scope-v1.json)、[本体手順v2](../../work/analysis/ui-midi-echo/20261011T062616160Z/gui-protocol-v2.json)
- [保存ソース/生成物照合](../../work/analysis/ui-midi-echo/20261011T062616160Z/candidate-identity-v2.json)、[独立監査24条件](../../work/analysis/ui-midi-echo/20261011T062616160Z/artifact-audit-v2.json)、[判定器](../../work/analysis/ui-midi-echo/20261011T062616160Z/audit-midi-echo-v2.mjs)

## 変更と契約

Echo入力と通常MIDI InputのWinMM所有権を分け、同じ機器を選んだ場合は一つの入力キューを共有する。新しい機器を開いてから古い所有権を解放し、設定・開閉・保存失敗では復元を試み、復元不完全は明示する。物理機器の同一/別機器ケースは未検証。

Echo MIDI Inと機器選択、16 PChannel単位の範囲を有効化した。DWORDの4予約値を除く整列済み範囲を扱う。MECH v1と内部MINP v1を別設定ファイルへ原子的に保存し、通常MIDI Inputと音声設定の副作用を避ける。ツールバーのEchoチェックも同じ設定へ接続した。

短いchannel-voiceイベントを検査してチャンネル0〜15を選択範囲へ写像する。Conductorは所有する再生中SegmentのAudioPathにだけ配送する。バッチ全体の妥当性・PChannel変換を送信前に検査し、実際にSendPMsgが受理した音符/ペダルだけを追跡する。Stop・設定切替時に残音を清掃し、清掃失敗でも通常のSegment Stopを試みる。停止中のBand/DLS発音は未実装としてエラーと状態表示に残す。

原版CHMのMIDI/Performance Optionsを契約根拠にした。正式原版では上矢印で1–16から17–32へ動き、Cancelで閉じることだけを観測した。原版の設定を保存しておらず、検証入力3ファイルと正式EXEのhashを照合した。実機・音声・完全外観の同等性は主張しない。

## 候補と検証結果

採用する限定検証候補は **20261011T071630614Z**。configure/compile/installはすべてexit0、保存/作業ソース288件と出力4件が一致。Producer.exe SHA256 **7644183702650145c0c643a49421d6d5eeac2618b05e7559b3ff7a090252635d**。

| 検証 | 結果と限界 |
|---|---|
| 登録native100一巡 | 53合格2965検査、46既存OS凍結、既知匿名Band1検証待ち |
| 関連6 | Timeline117/Transport57/Meter65/Sequence audition30/Metronome12/Count-In19、計300成立 |
| 登録driver172一巡 | 20合格、28既存OS凍結、124準備/対話前提の未実行 |
| Echo専用 | 16条件、336ソフトfixture、実runtime10メッセージ、音符/ペダル清掃、不正バッチ拒否。正常exit0。物理入力/PCMではない |
| 既存専用回帰 | MIDI Input8条件・Performance Options成立、正常exit0、列挙機器0件 |
| Main | 利用不能機器の有効化失敗→設定/ツールバーチェック復元、上下矢印、None/on/17–32のCancel/保存・再表示、ツールバーoff/on保存 |
| 保存/別復元 | 作者3264・復元13832、ともに通常exit0。120 BPM・既存1ノート・None/on/17–32を復元 |
| 独立監査 | 24条件成立。Segment/DLS bytes不変、Projectの既存filh FILETIMEのみ更新。3ファイルすべてのbytes不変ではない |
| 実ロード | 専用Echo実行57モジュールをx86実ファイルへ照合、正式原版インストール先0件。点観測であり全40/Q2の証明ではない |

[Echo run](../../work/acceptance/midi-echo/20261011T072354465Z/run.json)、[MIDI Input run](../../work/acceptance/midi-input/20261011T072357561Z/run.json)、[音声Options run](../../work/acceptance/performance-options/20261011T072635120Z/run.json)、[native round](../../work/acceptance/regression/20261011T072400722Z/run.json)、[driver round](../../work/acceptance/registered-drivers/20261011T072638602Z/run.json)を保存した。専用3runの成功を原始driver一巡20合格へ加算していない。

## 失敗と修正の保全

063810候補のStop清掃順序、064503候補のツールバーチェック復元/受理済み送信の追跡を修正した。065322候補ではMainの範囲矢印が逆だった。原版上矢印の増加に合わせて符号を修正し、071630新候補で上下両方を確認した。各候補・診断・失敗画像・通常終了記録を保持し、旧成功を新候補へ転用していない。

依存照合v2は64bit側のSystem32ファイルを誤読した記録として保持。v3で検証済みx86プロセスのSystem32直下をWOW64のSysWOW64実ファイルへ対応し、57件のPE Machine/hashを照合した。原版依存解消の全体判定は変更していない。

## 残責務と次

- No physical MIDI input available: actual receive/latency/timbre/acoustic/original hardware comparison unverified
- Stopped-state Band/DLS MIDI Echo audition not implemented; requires running Segment AudioPath
- Advanced multi-device/>16 PChannels per input, SysEx, StepTime/Realtime recording remain
- Shared primary/Echo physical-device ownership/error rollback not exercised against hardware
- Full screenshot layout/icons/keyboard/mouse/original same-input equivalence remains
- Native 46 OS freezes/known anonymous Band1 pending and driver28 OS/124 prerequisites unexecuted retained
- Count-In031359 PCM failure, Sequence/Band initial onset failures, Style015244 fixed PCM, other SSP and all40/all8/Q2 remain

次はSSP04/SSP05の初回発音遅延を、保存済みPCM・操作時刻・製品traceから照合する。1秒閾値とSequence1.215秒/Band1.15秒の失敗を保持し、UI入力時間・製品dispatch・synth準備・PCM onsetを分けて原因を特定し、必要な製品修正→新候補/関連/Main/native保存復元/自動PCMへ進む。Echo停止中発音/実入力/Advanced/録音、Count-In静音失敗、その他SSP/Style015244/Q2/OS凍結をqueueに保持する。[優先順位レビュー](../../work/analysis/ui-midi-echo/20261011T062616160Z/priority-review-v1.json)を保存した。

再現はPowerShell7でBuild-ProductSnapshot.ps1を実行し、新build-summaryに対してTest-MidiEcho.ps1、Test-MidiInput.ps1、Test-RegressionManifest.ps1、Test-PerformanceOptions.ps1、Test-RegisteredNativeDrivers.ps1を順に実行する。本体はgui-protocol-v2とfixture/launchの引数に従い新しい検証用コピーを作る。旧runへ追記/上書きしない。Git対象外workの移送は単位記録と各hashを含めて保全する。

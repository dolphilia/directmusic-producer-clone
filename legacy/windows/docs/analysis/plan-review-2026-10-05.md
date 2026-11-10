# 2026-10-05 計画・進捗の再評価

結論：本体成立を先行させた前回の方針は有効で、実装を捨ててやり直す必要はない。ただし、現行のまま細部を連続追加する進め方は改善が必要。通常回帰の失敗を解消し、同一候補の統合受入と残責務の消化を明示的な作業単位にする。全体完成は未達で、完成率や残り日数を信頼できる形では算出できない。

調査時点のソース：`1370271d9570d7e29543282f2a02fd0b24a183f9`。調査開始時は前コミットと多数の未コミット変更だったが、調査中に上記コミットへ更新された。これは本レビューが作成したコミットではない。改訂前計画をバイト一致で [履歴](../analysis-plan-history-2026-10-05.md) に保存した。ソース・試験・既存成果の変更は行っていない。

## 確認方法と進捗

CMake、製品ソース、試験dispatcher、40行の機能台帳、状態JSON、時系列記録、最新build-summaryとunit-recordを照合した。製品の新規ビルド、GUI操作、再録音、原版の起動は実施していない。保存済み候補で本体smokeと通常コア試験を一度実行した。

| 項目 | 確認した事実 | 判定の限界 |
| --- | --- | --- |
| 前回からの前進 | 10月2日のレビューでは本体ターゲットなし。現在は `producer`、`producer_core`、`producer_core_tests`、Win32 preset、installが存在 | 全40モジュールの責務を実装したという意味ではない |
| 現行候補 | `20261004T150511775Z`、75保存ソース、構成・compile・install各exit 0 | ビルド成功と実行成功は別 |
| 追跡性 | 75ファイルのcheckoutと保存ソース、4生成物、最新unit-recordの180証拠をSHA-256で再照合し不一致0 | 証拠の保全確認。180件の試験再実行ではない |
| 文書・GUI | Framework/Timeline、Segment/Style/Band/DLS/AudioPath、保存・別プロセス復元の実装と版別記録あり | 限定入力・旧版を含む。全編集機能の同一版受入ではない |
| 再生 | 現行共通Transport defaultのMotifで、WASAPI録音、880Hz/120BPM、Stop・再開・無音区間の記録あり | 現行GUI、live切替、未接続PChannelのsilent互換は未完了 |
| 原版依存 | 現行再生の56ロードモジュールで原版40ファイルのhash一致0 | 点観測。隔離環境・全機能・別版の原版バイナリ不使用の証明ではない |
| 最新の実行再確認 | 本レビューのhost-smokeはexit 0、通常coreは6件目でexit 1 | GUI・音声・後続core・全体受入は未実行 |

最新の一次記録：

- [build-summary](../../work/build/product-snapshot/20261004T150511775Z/build-summary.json)、[unit-record](../../work/analysis/transport-default/20261004T151000Z/unit-record.json)、[Transport report](../../work/analysis/transport-default/20261004T151000Z/report.md)。
- 製品EXE SHA-256：`d0a7abbee86dcb391262efefde6ecb07170aea02595776830990d803214a411e`。
- コア試験EXE SHA-256：`fe16fdf97e520b4ba6ea3c1ea2a632dfb4fe4ed8ecd2b9e1d1d04079abbfa3a8`。
- 改訂前計画SHA-256：`fdf9faf5976994e35f3ca3f8a5f053d6d0d8e9b9935882e53f7db95376e599b5`（313,075 bytes、1,289行）。

## 最優先で直すべき試験の不整合

実行コマンド：

```powershell
./scripts/Test-ProductSnapshot.ps1 -BuildSummaryPath work/build/product-snapshot/20261004T150511775Z/build-summary.json
```

[今回のrun.json](../../work/acceptance/product/20261004T152608791Z/run.json) は `executionBlocked=false`、host-smoke exit 0、core exit 1、UI/audio未実行。実行ユーザーは `CodexSandboxOffline`。起動拒否ではない。

stderrは `after 6 checks: note boundaries retain document`。`tests/producer/core_tests.cpp:821` はPChannel 16の追加を拒否すると期待する。一方 `src/producer/sequence.cpp:56` はDWORD PChannelを許可し、予約broadcast値 `>= 0xfffffffc` を拒否する。[Sequence契約](sequence-crud-contract.md) の10月4日追記にも0〜15制限の訂正が記録されている。実装と旧期待値の不整合を確認した。これだけで実装側の全面的な正しさや、残りの試験の合格を主張しない。

次の作業では一次契約と既存AudioPathの根拠を照合して、許可値16以上・予約値・不正入力時の文書不変を別々に試験する。単にassertを消す、旧制限へ戻す、試験件数を減らす対応はしない。修正候補の通常coreを再実行し、その先で顕在化する失敗も診断する。

run.json SHA-256：`578980c0b9e40a4156dc112ae277265cf7c64a50b58520cba987185f3336ca59`。
core.stderr.txt SHA-256：`0648268cc232c5d60917e310e6745cea551f012792c2f33ac0a3b0302a147b0d`。

## 計画上の問題と改善

| 問題 | 根拠 | 改善 |
| --- | --- | --- |
| 通常回帰が追従していない | 上記6件目の失敗。通常dispatcherは新しい専用引数の全試験を呼ばない。CTest登録は `producer_core` 一件 | 通常coreと専用試験を列挙する回帰manifestを作り、節目で全登録群を実行。合計checksを網羅率にしない |
| 現在状態と履歴が混在 | 計画1,289行、product-host 2,188行。両方に古い「次」の記述。implementation-status冒頭は10月2日の候補を最新と記載 | 計画を短い運用本文に戻し、履歴を保存。現在状態と実行queueに一意の入口を設ける |
| 台帳が実装を正しく反映しない | feature-mapは40行中26行が文字列「未着手」。ADSREnvelopeは未着手だがarticulation/envelope実装記録が存在 | 未着手を完了扱いにもせず、責務単位でコード・仕様・受入へ対応。26/40を完成率に使わない |
| 詳細機能の追加で統合確認が先送りされる | Runtime Recovery、AudioPath、Motifへ拡張しつつ「全core/全40/全八未完了」が継続 | 不具合修正→固定候補の代表経路統合→未着手責務というqueueを設定。細部の追加は受入上の必要性を先に記録 |
| 原版との互換性より内部整合が先行 | 独立RIFF/PCM監査は充実するが、多数の機能で原版動的比較は未実施 | 原版の同一入力・操作・出力を責務ごとに比較。独立パーサの成功を原版互換と呼ばない |
| 原版なしの証拠が部分的 | ロードhash一致0は記録済み、物理隔離は未確認 | 原版実行物・登録・探索先のない試験環境でinstallから実行し、由来を確認。通常環境の登録を壊して作らない |
| 集計器が状態を後退させ得る | Summarize-ImplementationStatus.mjsは固定の10モジュール集合と古い比較文言でCSVを再生成 | 再生成前に正本・schema・全40行の保持を整える。無差別な一括上書きをしない |

## 残る範囲

Chord/ChordMap、Wave、MIDI、Script、Container、ToolGraph、Lyric/Marker/Mute/SignPost/Param/SegmentRefなどが台帳上で未着手。既存機能にも本体操作・原版比較・保存形式・通知・寿命などの残責務がある。MFC/MSVCRT/OCXコントロール、Uninst、Farm/StylePlayerは役割を分け、製品による代替・外部依存・補助アプリの対応を根拠付きで整理する。40ファイルを同数のDLLへ復元する必要はないが、責務を削ってよいという意味ではない。

今回の改訂は [現行計画](../analysis-plan.md) と [再開プロンプト](continuation-prompt-2026-10-05.md) に反映した。全体受入8項目は改訂前から本文をそのまま保持する。今回のレビューで発見した失敗は未修正のまま明示し、次の実装作業へ渡す。

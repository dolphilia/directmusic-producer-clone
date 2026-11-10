# DLS サンプル継承の本体・保存・再生確認

候補 **20261007T001647816Z**、199保存ソース。Win32 Release、reference tools OFF、configure/build/install 各 exit0。Producer.exe SHA256 `816662ff396acb05d14ca5bba1c5070e6d774149f80db99ebc3b7474a69fc31c`。対象は DLSDesigner と Conductor のサンプル設定継承である。[最新単位](../../work/analysis/q3-sample-inheritance/20261006T233800Z/unit-record.json)、[同候補の統合監査](../../work/analysis/q3-sample-inheritance/20261006T233800Z/runtime-fixed/unit-proof.json)、[契約と一次根拠](dls-sample-inheritance-contract.md)。全40責務・全8受入は未完、`fullAcceptance=false`。

Wave/Region の Unity Note 0–127 を採用・再生準備時に検証する。不正値の lossless load/save は維持し、拒否時に文書 bytes、dirty checkpoint、Undo/保留 Redo を変えない。原版 help と SDK の型・意味に基づく契約であり、原版の不正入力 GUI を動的観測した結果ではない。

候補234743632Zの本体録音では、Region WSMP がない継承状態と Undo 復元が約75msで途切れ、明示単発だけが元 PCM の約1秒鳴った。この失敗を保存し、OS 再生器へ渡す私有 DLS コピーで、欠けている Region WSMP にリンク先 Wave の設定全体を複製した。編集・ディスク・履歴の継承状態は変更しない。明示ゼロループは単発のまま保持する。正規化後に同じ bytes になっても、同一 GUID の異なる所有スナップショットは拒否する。修正前の独立診断はその衝突を検出できなかった証拠を保持している。

専用 `--dls-sample-policy` は28チェック合格。境界0/127、不正128以上、単発・継承・全設定除去、完全 bytes/履歴、私有コピー、GUID衝突、native三文書 carrier を検証した。固定候補の登録一巡は native78＝47合格・31障害。Script Reference.spp の新しい Windows error5 を凍結し、通常coreの旧atomic Chordmap Project error5と区別する。driver108の原始一巡は20合格・21障害・67未実行、今回の同候補音声判定器を補足して21合格・21障害・66未実行。障害・未実行は合格に含めない。

作者PID22048で明示単発から「Wave設定を使用」、Undo/Redo、DLS/native Project保存、通常exit0を確認。別PID20504が同じ保存三文書を読み、継承値320/11987と保存済み状態を復元した。同PIDで継承、未保存Region単発、Undo復元を再生してから通常exit0。独立RIFF監査で保存DLSが元のRegion WSMP全体除去だけであること、Band/Segment不変、native JAZP所有GUID/size整合、保存コピーと作業文書の全4ファイル一致を確認した。

| 同じ再読込PIDの校正済みWASAPI録音 | 結果 |
| --- | --- |
| Wave継承 | MIDI60を2発、10秒間隔。元1秒PCMを越える持続RMS約0.03265、発音中14秒Stop前RMS約0.03263、Stop後RMS0 |
| 未保存Region単発 | MIDI60を2発、10秒間隔。発音後1.5–2.5秒はRMS0 |
| Undoによる継承復元 | MIDI60を2発、10秒間隔。持続を復元し、発音中Stop前RMS約0.03263、Stop後RMS0 |

録音器は保存ソース版 `20261006T211007927Z`。240秒録音の前後無音RMS0、peak0.04661、最大packet gap3frames。操作UTC、endpoint/QPC、入力・製品・録音器・判定器hashを固定した。[音声証拠](../../work/acceptance/product-project-gui/20261007T002626038Z/audio-20261007T003127638Z/dls-sample-inheritance-proof.json)。正例再実行と、持続欠落・単発のループ化・Stop後残音・再開欠落・異音高・Undo欠落・同作者PIDという7反例を全て期待どおり判定した。[対照結果](../../work/analysis/q3-sample-inheritance/20261006T233800Z/runtime-fixed/audio-controls/negative-tests.json)。人の聴取待ちは用いていない。

最初の録音は操作期限超過/フェーズ不足、最新候補の初回もStopが自然終了後で不成立。古い録音器へ追加引数を渡した起動はexit2で不成立だった。各入力・操作・生ログを保持し、適合する既存録音器を明示した再録音だけを合格にした。SDK configure access拒否、修正前の専用試験失敗、GUID衝突診断、Script保存拒否も単位から参照する。

再監査コマンド（リポジトリroot、Node24/PowerShell7）：

```powershell
./scripts/Build-ProductSnapshot.ps1
./scripts/Test-RegressionManifest.ps1 -BuildSummaryPath work/build/product-snapshot/20261007T001647816Z/build-summary.json
./scripts/Test-RegisteredNativeDrivers.ps1 -BuildSummaryPath work/build/product-snapshot/20261007T001647816Z/build-summary.json
node scripts/Inspect-DlsSampleInheritanceAudio.mjs work/acceptance/product-project-gui/20261007T002626038Z/audio-20261007T003127638Z
node scripts/Test-DlsSampleInheritanceAudioAuditor.mjs work/acceptance/product-project-gui/20261007T002626038Z/audio-20261007T003127638Z NEW_CONTROLS_DIRECTORY
node scripts/Inspect-DlsSampleUnit.mjs work/analysis/q3-sample-inheritance/20261006T233800Z/runtime-fixed work/build/product-snapshot/20261007T001647816Z/build-summary.json work/acceptance/product-project-gui/20261007T002626038Z/audio-20261007T003127638Z work/analysis/q3-sample-inheritance/20261006T233800Z/runtime-fixed/audio-controls/negative-tests.json
```

Buildは新候補を生成する。新候補には今回の成功を転用しない。固定候補一巡の原始runは `work/acceptance/regression/20261007T002143381Z/run.json` と `work/acceptance/registered-drivers/20261007T002205196Z/run.json`。GUI再現は新しい作業コピーを `Test-ProductProjectGui.ps1` で開き、上記作者操作・保存・正常終了後に保存コピーを別起動する。録音は `Capture-ProductGuiAudio.ps1 -RecorderBuildSummaryPath work/build/audio-capture/20261006T211007927Z/build-summary.json -DurationSeconds 240 -SilentKeepAlive`、ready後に各Playと14秒StopをUTC記録する。判定器の正例は全入力と観測ログの同一性を要求する。

原版対象編集画面は New種別/Add-Ins不足で動的比較障害。Q2の原版なし独立Windowsも未用意。RIFF・PCMは原版動的比較を代替しない。全8は6作業中・2障害・0合格。旧222153066ZのQ1五形式統合は履歴であり、新候補では未実行。この三形式単位をQ1全体成立とは扱わない。

次はQ3FのFileOutput複数バッファ。既存writer/DMOを維持し、直接PChannelを持つ2mix-groupをRecord.wav/Record1.wavへ接続する。native AudioPath履歴・保存・別起動、両ファイルのPCMと本体WASAPIを終了条件にする。Send/未接続group/多重session/legacy ABI、WaveTrackループbit/終端、継承DLSのruntime export、GUID-only曖昧解決はqueueに残す。

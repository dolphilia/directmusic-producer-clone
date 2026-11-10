# Lyricを共通Timeline範囲へ接続

候補 `20261006T202431545Z` の限定単位。全体40責務・全8受入は未完了。単位は [unit-record](../../work/analysis/q3-lyric-range/20261006T202000Z/unit-record.json)、現在状態は [product-state](product-state.json) と [受入状態表](acceptance-status.md)。

対象はLyricStripMgr.dll、Timeline.dll、本体。前単位後に残っていた未昇格変更と中断buildを保持し、選択group/nth LyricをTempo/Sequenceと同じ範囲・履歴へ接続した。physicalの半開範囲で選択し、移動差分をlogical/physical両方へ適用する。Unicode・配送設定・拡張header・未知entry childを保持し、不正なclipboard、負のlogical結果、文書末尾超過では全stripの文書・dirty・Redoを変更しない。空範囲貼付けでtrackを製造しない。

保存195ソースのconfigure/build/install各exit0、保存ソース不変。[build-summary](../../work/build/product-snapshot/20261006T202431545Z/build-summary.json) SHA256 `33261e4504c2ef0e60b96b1eed121759d83d005cd9ebbc7c636e1ac104075736`。install本体SHA256 `829b4a3b08c4f160d50dae1af6f5c869577ad17c080f8093b1ccf16f38247415`、core試験EXE `1d9811b1fcd9def904ab683c01490b64bd475b233e3d8fd7749aa5b0b61b68ed`。

関連試験はTimeline60、Lyric文書48、Lyric実ランタイム31、Sequence26が合格。[native77一巡](../../work/acceptance/regression/20261006T202831767Z/run.json) は47合格・30障害。[driver107一巡](../../work/acceptance/registered-drivers/20261006T203036877Z/run.json) は20合格・21障害・66未実行。通常coreは旧候補051458521Zのatomic Chordmap Project Windows5条件を凍結して未実行、合格に含めない。gateに残っていた「same candidate」文言を履歴付きで訂正し、raw runは保持した。Q0のDWORD許可値0..0xfffffffbと予約4値・不正入力時不変の修正は維持され、Sequence26で現候補を再検証した。

本体PID9160でTempo/Sequence/Lyricの `[2304,3072)` を3072へ移動し、Undoでsaved、Redoでmodifiedを観測。Lyric Segmentを保存し、初期Segmentも保持した新規native `GuiLyric.pro` を作成した。[保存監査](../../work/analysis/q3-lyric-range/20261006T202000Z/gui-save-proof.json) は6個のclock DWORDだけの変更とその他全bytes一致を確認。別PID15248ではTempo3072、note3204、Lyric physical/logical=(1536,3072)/(3104,2404)/(3304,6304)、Unicode歌詞、配送設定を本体で復元した。再読込側は [通常終了exit0](../../work/acceptance/product-project-gui/20261006T203913557Z/launch.json)。作者側は保存後の通常閉鎖・process消失を観測したが、exit handle取得はOS拒否のためexit0未検証。迂回・同条件再試行なし。

原版既存Segment designerが開かず、今回のbulk動的比較は障害。内部RIFF監査はソース版の保存整合性だけの証拠で、原版比較の代用ではない。原版境界・duplicate/overlap・meter再anchor・snap・全strip・OLE/COM ABI、Q2独立Windows、全40/全8は残る。この候補のQ1音声は未実行。旧185539のStyle成功・Transport混入失敗は保存し、新候補へ転用しない。

再現入口（PowerShell、repo root、Win32 Release/reference tools OFF）：

```powershell
./scripts/Build-ProductSnapshot.ps1
./scripts/Test-RegressionManifest.ps1 -BuildSummaryPath work/build/product-snapshot/20261006T202431545Z/build-summary.json
./scripts/Test-RegisteredNativeDrivers.ps1 -BuildSummaryPath work/build/product-snapshot/20261006T202431545Z/build-summary.json
node scripts/Inspect-LyricRangeSave.mjs work/analysis/q3-lyric-range/20261006T202000Z/baseline.sgp work/analysis/q3-lyric-range/20261006T202000Z/GuiLyric/Lyrics.sgp work/analysis/q3-lyric-range/20261006T202000Z/gui-save-proof-reproduced.json
```

buildのSDK探索は通常host実行が必要で、restricted探索の失敗候補201836030Zと中断201904818Zも保持。新buildは新candidateになり、現候補結果の転用はできない。GUI操作は単位observationsを参照し、Computer Use支持APIで現在windowに実行する。

次は同候補Q1の新規native五形式・履歴・保存・両通常終了・別復元を実行し、native回帰と直列にWASAPIでStyle/Transport競合優先・発音中Stop・無音・全再開・テンポを結ぶ。Q2と原版障害を保持し、独立した次のQ3Cは既存Marker/Muteの共通範囲不足。

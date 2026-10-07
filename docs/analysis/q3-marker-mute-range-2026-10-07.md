# Marker/Mute を含む五strip共通範囲

2026-10-07（日本時間）。候補 `20261006T213433891Z`、最新単位 [unit-record](../../work/analysis/q3-marker-mute-range/20261006T213500Z/unit-record.json)。**限定成立、全体未完了**。40責務と全8受入を維持する。

Marker/Mute が共通範囲と単一 Undo/Redo に参加していなかった不足を閉じた。既存 CRUD・再生・Lyric 範囲の実装を保持し、Tempo/Sequence/Lyric/Marker/Mute の五stripへ接続した。[暫定契約](timeline-range-contract.md) は原版 bulk 操作の観測不足を明示する。

|工程|同候補の結果|
|---|---|
|構成/compile/install|195保存ソース、Win32 Release、reference tools OFF、各exit0。保存ソースと出力hashを再照合|
|関連native|Timeline93、Marker70、Mute55、Lyric48、Sequence26合格|
|登録native77一巡|47合格、30障害。通常coreは既知Windows5保存条件を凍結し、現候補未実行|
|登録driver107一巡|20合格、21障害、66未実行。前候補の音声判定器2補足成功を転用しない|
|本体編集|作者PID20544、五strip範囲 `[2304,3072)` を3072へ移動。一回Undoでsaved、一回Redoでmodified|
|本体保存・終了|Events.sgp とnative JAZP Projectを保存し、作者は通常exit0、強制終了なし|
|別プロセス|PID22036で同候補・保存Projectを読込み、五stripの移動結果を復元。文書/Project再保存後、通常exit0|
|保存監査|選択された時刻7 DWORDだけを768移動、それ以外のSegment全バイト一致。別プロセス再保存も完全一致|
|Project差分|300 bytes。`LIST:file/filh` の最終書込FILETIME bytes16..23だけが更新。Project/文書GUID、名称、size、runtime metadataほか全bytes一致|
|audio|新候補未実行。前候補202431545ZのQ1音声を履歴として保全|
|原版/Q2/全体|原版bulk動的比較未観測、独立Windows未用意。全8は6作業中/2障害、合格0|

拡張strideの Marker/Mute はコピー時も全レコードを保持し、空の貼付け先で採用する。既存データとstrideが異なる場合は拒否する。Markerの重なりと一致する重複MARKリストを保持し、Muteの同一PChannel/時刻衝突は拒否する。予約PChannel、範囲外・破損入力の拒否時は私有コピーを捨て、元文書、dirty、選択、Redoを維持する。半開区間の終端とSegment終端の復元は移動しない。継続Muteの合成境界イベントや原版と同じbulkポリシーはこの限定契約で保証しない。

本体の別復元で Tempo/Sequence 3204、Marker/Enter 3104、Lyric physical3254/logical3404 と `range`/Before Time Stamp、Mute3204/PChannel17（raw16）を確認した。範囲外Marker1536、Mute復元1536/PChannel17、Segment終端30720/PChannel2も保持した。

文書を再保存すると owned Project のfilh最終書込情報も更新されるため、終了確認で未保存Projectを検出した。破棄せず終了を取り消し、Projectも再保存して通常終了した。元入力、初回保存、再保存の三版と差分を保全する。これは新たな原版比較成功ではない。

一次証拠は [unit-proof](../../work/analysis/q3-marker-mute-range/20261006T213500Z/unit-proof.json)、[7時刻の監査](../../work/analysis/q3-marker-mute-range/20261006T213500Z/save-proof.json)、[関連run](../../work/acceptance/regression/20261006T214110161Z/run.json)、[native一巡](../../work/acceptance/regression/20261006T214127592Z/run.json)、[driver一巡](../../work/acceptance/registered-drivers/20261006T214123795Z/run.json)、[作者exit](../../work/analysis/q3-marker-mute-range/20261006T213500Z/author-exit.json)、[別復元exit](../../work/analysis/q3-marker-mute-range/20261006T213500Z/reload-exit.json)。UI観測は同単位の `observations/states.json`、操作は `actions.json`。ビルドは [build-summary](../../work/build/product-snapshot/20261006T213433891Z/build-summary.json)。

主要hash（SHA-256）：

- build-summary: `07f79736c329bc691be370af0c91af23ae771203d15a23d6bcfe22ba6eb3d35f`
- install Producer.exe: `2688c1641bf871ba1d6308e69d8af4414f8407993e64aa74175ea451cdcfdc44`
- producer_core_tests.exe: `17219a0d55d80e5554fc120d48da9a444725f12173e9b7c69c14589dadf327de`
- 編集前Events.sgp: `73ac7efe4d221a12fbcfa225a8b30bd8b88eadf073860e292031734a47c49b95`
- 保存/再保存Events.sgp: `e3ae7bd14b2eeede59216d055edca30cd040343d926b085038e62eb32b322a89`

回帰と保存済み証拠の再検証はリポジトリrootから実行する。GUIは同候補のinstallと生成native fixtureを使い、五checkboxとtrack1を選び、上記範囲/target→Move Selection→Undo→Redo→File Save Document/Save Project As→通常終了→別プロセス読込の順で行う。生成fixture原本を編集せず、同名フォルダーへ複製する。

```powershell
./scripts/Test-RegressionManifest.ps1 -BuildSummaryPath work/build/product-snapshot/20261006T213433891Z/build-summary.json
./scripts/Test-RegisteredNativeDrivers.ps1 -BuildSummaryPath work/build/product-snapshot/20261006T213433891Z/build-summary.json
node ./scripts/Inspect-MarkerMuteRangeSave.mjs work/analysis/q3-marker-mute-range/20261006T213500Z/before-events.sgp work/analysis/q3-marker-mute-range/20261006T213500Z/MarkerMuteRangeProject/Events.sgp work/analysis/q3-marker-mute-range/20261006T213500Z/save-proof.json
node ./scripts/Inspect-MarkerMuteRangeUnit.mjs work/analysis/q3-marker-mute-range/20261006T213500Z work/build/product-snapshot/20261006T213433891Z/build-summary.json work/acceptance/regression/20261006T214110161Z/run.json work/acceptance/regression/20261006T214127592Z/run.json work/acceptance/registered-drivers/20261006T214123795Z/run.json
```

録音/再生試験は直列に実施する。単位の状態反映scriptは一度だけ実行済みで、再実行しない。記録更新前のmanifest/state/受入/CSV/派生MDも同単位へ保全した。

Timeline関係の三単位で共通接続は2→3→5 stripへ増え、具体的な未接続責務は減った。一方、原版比較/Q2/全8は閉じていない。細部の追加を続けず、次はQ3Eの既存ToolGraph/Parameterを本体のsource Tool生成・能力発見へ接続する。原版観測障害、残strip/OLE/Snap/meter/ABI、他メディア/和声/参照/出力/配布、現候補Q1/音声をqueueに保持する。

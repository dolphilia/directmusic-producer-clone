# Q3E Source Tool供給・能力列挙の限定単位

候補 `20261006T222153066Z`、198保存ソース。configure/build/install各exit0。Producer install SHA256 `1229c2ba21b29c73223d929e03f785aaf5025d4a9f4a53ca8c4fabe6fb5a81b3`、build-summary SHA256 `9cdb5e73ba8b99afe7b4b833f3caa6f0c9829513c377704e336ed1259e5f21b5`。全40責務・全8受入は未完了。

[単位記録](../../work/analysis/q3-tool-factory/20261006T215905Z/unit-record.json)と[監査](../../work/analysis/q3-tool-factory/20261006T215905Z/unit-proof.json)が候補・入力・保存物・PID・録音を接続する。[契約](source-tool-factory-contract.md)は既存モデルと実行器を維持して、一つの製品固有GUIDのVelocity Toolを供給する範囲。外部Toolや原版の同名実装との同一性を主張しない。

本体palette、gainプロパティ、選択Segmentのembedded AudioPath Tool列挙、IMediaParamInfoによるparameter名/範囲/Jump・Linear能力を追加した。無効なfactory payloadや既知Source曲線は再生中Stopより先に拒否する。文書変更は単一履歴で行い、既存tolh/opaque/trackを保持する。

OS実行で初回の13件目が失敗した。neutral48のままになった原因はmusic-time format dataの誤りで、実OSのSetTimeFormatは768 PPQを要求した。768の宣言・検証、ALLPARAMS SetParamのenvelope保持を修正した。期待12/36を維持し、実際の8 Note PMSG velocityは12×4・36×4で合格。GUI pasteの境界判定はincoming曲線に適用し、既存signed pickupを誤拒否しない。旧失敗候補・診断ライブラリ・ログを単位内に保持する。

固定候補の関連試験はTimeline93、ToolGraph runtime27、Parameter typed53、OS runtime15合格。[専用一巡](../../work/acceptance/regression/20261006T222825975Z/run.json)は77=47合格/30障害、[driver一巡](../../work/acceptance/registered-drivers/20261006T223107211Z/run.json)は107=20合格/21障害/66未実行。通常coreは既知Chordmap Project atomic replace Windows5条件を凍結し、現候補未実行。失敗・障害・未実行を成功へ算入しない。

作者PID14124はnative Project内で3つ目のTool追加、gain0.8適用/UndoRedo、AudioPathへ埋込、Authoring Segmentへcopy、object/parameter追加、0..3072 gain0.25と3072..6144 gain0.75の2曲線追加/UndoRedo、文書・Project保存を実施し正常exit0。別PID7004は3Tool・2曲線を復元し、文書再保存後にProject catalogのdirtyを保存して正常exit0。所有5文書は作者保存と再保存がbyte一致。Projectの差分はfilhのdocument FILETIME bytes16..23のみで、名前/GUID/サイズ/その他catalog bytesは一致する。

同じ別復元PIDの[64秒WASAPI](../../work/acceptance/product-project-gui/20261006T223900829Z/audio-20261006T224042023Z/run.json)は対照/適用各8音、pitch60/62/64/65/67/69/71/72、120→180BPMの独立attack間隔と相対gain効果に合格。前半/後半の対照比RMS平均は0.00911/0.09041。絶対velocity対音圧の較正ではない。両Stopは自然終了後なので、発音中Stop/再開をこの単位で合格にしない。

初回判定器は対照のdecay rippleを11attackとして誤検出した。旧失敗判定を保持し、10ms RMSの相対上昇30%以上をattack条件にした。期待発音時刻はpeak選択に使わず、検出後に全pitch/intervalを判定する。同じ保存PCMを再解析し、無音・1音欠落・gain未適用・pitch入替・pitchを保持したtempo誤り・packet clock破損の[6反例](../../work/analysis/q3-tool-factory/20261006T215905Z/audio-audit-controls/proof.json)を各exit1で拒否した。実再生の同条件再試行はしていない。

原版は最新New dialogでProjectのみ利用可能で、ToolGraph/Parameterの相互操作を観測できなかった。[原版証拠](../../work/analysis/q3-tool-factory/20261006T215905Z/original-new-types.json)を障害として保持する。RIFF監査とPCMは原版動的比較の代わりではない。外部Producer ABI/Tool property pages、DMO/buffer、reference-time、残shape/overlap/flush、Timeline連携、全40、Q2原版なし独立環境は残る。

再現はWindows/MSVC17 2022/Win32/SDK10.0.26100.0/CMakeで `scripts/Build-ProductSnapshot.ps1`。保存済み候補への検証は `scripts/Test-RegressionManifest.ps1 -BuildSummaryPath work/build/product-snapshot/20261006T222153066Z/build-summary.json` と `scripts/Test-RegisteredNativeDrivers.ps1 -BuildSummaryPath ...`。これらは新runを生成し、同条件OS障害はmanifestのgateを維持する。GUI入力は `node scripts/Prepare-SourceToolGuiInput.mjs RELATED_DIRECTORY NEW_UNIT_DIRECTORY` で専用試験fixtureから作り、`scripts/Test-ProductProjectGui.ps1`でnative Projectを起動し、単位内のauthor-actionsとGUI観測順を再現する。生成helperは原版oracleではない。

保存証拠だけの再監査は次のコマンド。

```powershell
node scripts/Inspect-SourceToolUnit.mjs work/analysis/q3-tool-factory/20261006T215905Z work/build/product-snapshot/20261006T222153066Z/build-summary.json work/acceptance/regression/20261006T222646777Z/run.json work/acceptance/regression/20261006T222825975Z/run.json work/acceptance/registered-drivers/20261006T223107211Z/run.json work/acceptance/product-project-gui/20261006T222839155Z work/acceptance/product-project-gui/20261006T223900829Z work/acceptance/product-project-gui/20261006T223900829Z/audio-20261006T224042023Z
```

次は現候補Q1の新規native Projectから全代表形式・別復元・発音中Stop/再開/Transport優先まで接続する。その後は独立Q3DのWave loop inheritance等へ戻る。Source Toolの同群細分化を延々と追加しない。

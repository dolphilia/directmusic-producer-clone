# Q1 固定候補・新規native Projectの代表経路

候補 `20261006T202431545Z` の同じinstallで、新規Project、五形式の変更・Undo/Redo・保存、通常終了、別プロセス復元、Style音声とTransport優先・停止・再開・テンポ音声を接続した。代表経路限定合格。全40責務・原版互換・全8受入・Q2は未完了。

正本は [unit-record](../../work/analysis/q1-fixed-integration/20261006T205000Z/unit-record.json)、機械監査は [scenario-proof](../../work/analysis/q1-fixed-integration/20261006T205000Z/scenario-proof.json)。全8の現在表は [acceptance-status](acceptance-status.md)。195保存ソースのconfigure/build/install各exit0。Producer SHA256 `829b4a3b08c4f160d50dae1af6f5c869577ad17c080f8093b1ccf16f38247415`。

|段階|同候補の証拠と範囲|
|---|---|
|新規・編集・履歴|作者PID21436。File/New Projectで新規作成。Initial note追加→Undo0→Redo1、Sequence5→6→Undo5→Redo6→最終Undo5、Style108→109、Band100→101、DLS key low0→1、AudioPath名Conflict→Q1 Fixed Conflict。各変更はUndo/Redoを実観測。Style/Band/DLS/AudioPathは保存した最終値を保持。|
|保存と作者終了|Fresh/Fresh.proはnative JAZP、4 Segment＋Style/Band/DLS/2 AudioPathの9参照。作者通常終了exit0をread-only exit watcherで取得。独立起動したPID15472でScript所有不足を診断し、SourceHost.sppを本体で開いて10参照へ保存、通常終了exit0。|
|最終再読込|PID15740が最終Fresh.proを別起動。Initial1音、Sequence8音5/7.5BPM、Style109、Band101、DLS low1/Region loop320＋11987frames、AudioPath名とConflict1..15/Route0..15を復元。最終通常終了exit0。|
|入力の連続性|Script修正前の全12ファイルをinputs-before-script/Freshへ保全。修正で変わったのはProjectだけ。他11ファイル、既存9参照、Project identityは同じ。最終Project SHA256 `9033943c496b743507ffe6e6c45c1e08126c0ec2b18fa6ec10ade8fe8621367a`。|
|Style音声|最終Project/PID15740、Empty.sgpのStyleRef＋owned DLS/Band。60秒録音でC4の10発、109BPMのPattern契約に対応する2秒間隔、前後無音、peak0.09408、packet整合合格。無音/欠音/誤ピッチ/誤テンポ/基準区間混入を拒否。|
|Transport音声|同じProject/PID。Controlはembedded AudioPathなし＋Conflict defaultでRMS0。SourceSequenceはembedded Routeを優先して発音。発音中Stop直前RMS0.06524、停止後待機RMS0。再開8音60/62/64/65/67/69/71/72、12/12/12/12/8/8/8秒、5→7.5BPM、全音1.5〜2.5秒のDLS持続合格。基準・tail RMS0、peak0.09305、packet gap最大2frames、初回のみdiscontinuity、timestamp error0。|
|対照・回帰|Style判定器6対照、Transport11対照の正例のみ受理。native77一巡47合格/30障害。driver107原始一巡20合格/21障害/66未実行、同候補の判定器2件補足後22合格/21障害/64未実行。通常coreは既知Windows5保存拒否で未実行・障害。|

音声正例は [Style proof](../../work/acceptance/product-project-gui/20261006T211208776Z/audio-20261006T211637034Z/gui-project-style-dls-audio-proof.json) と [Transport proof](../../work/acceptance/product-project-gui/20261006T211208776Z/audio-20261006T211818105Z/gui-transport-dls-priority-proof.json)。各runは製品・入力・録音器・endpoint・操作UTC・packet/QPC・WAV・解析器を追跡する。今回の実ロード全module inventoryは取得していない。Windows DirectMusic/DirectSound等の宣言依存と、全機能の原版依存解消は別判定。

録音器に明示オプション `--silent-keepalive`、GUI driverに `-SilentKeepAlive` を追加した。共有render endpointへゼロのバッファを供給し、idleから最初の発音までの録音packet欠落を避ける。default挙動を保持し、packet flagsを消したりgap判定を緩めたりしていない。APIの無音バッファは [Microsoft ReleaseBuffer仕様](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudiorenderclient-releasebuffer) に基づく。

録音器は [保存ソースbuild](../../work/build/audio-capture/20261006T211007927Z/build-summary.json)、2ソースconfigure/compile0、EXE SHA256 `9e53dacd2b5793935b459fd04233330065d35edc7cbbbfff019fbeede0ec94d2`。6秒の無再生対照は600packets、peak0、timestamp error0、gap最大2framesで合格。製品195保存ソースには録音器とGUI capture driverが含まれず、製品候補を変更していない。

初期52秒録音の最初の発音時の第2packet discontinuity/468frame gap、Script未所有でPlayが拒否された録音、停止が収録終了後になった60秒録音は不合格のまま保全した。静かな収録を無音経路の合格へ転用していない。統合監査の対照JSON形式誤認と、台帳の古いStyle判定器hashも診断・履歴保持した。

再現はPowerShell/VS2022/MSVC19.44/SDK10.0.26100.0/CMake/Nodeと宣言OS DirectMusic依存を使う。`Build-ProductSnapshot.ps1` は保存sourceの新規configure/build/installを分離記録する。今回の固定候補そのものを再構築する場合はそのsummaryのsourceRootを `cmake -S` に指定し、同じWin32 Release・reference tools OFF構成を使用する。新生成物は別候補として回帰・GUI・音声を実行し直す。

```powershell
./scripts/Build-ProductSnapshot.ps1
./scripts/Build-AudioCapture.ps1
# 新生成物のsummaryを指定してTest-ProductProjectGui.ps1で本体を起動。
# File/New Project→上表の五形式操作→Fresh/Fresh.pro保存→通常終了。
# 必要なSourceHost.sppもowned catalogへ含め、別PIDで同じProjectを開く。
# 同じ最終入力hashをAdditionalInputsへ指定し、StyleとTransportを直列収録。
./scripts/Capture-ProductGuiAudio.ps1 -GuiRun $taskGuiRun `
  -RecorderBuildSummaryPath $taskRecorderSummary -SilentKeepAlive `
  -DurationSeconds 60 -AdditionalInputs $taskFinalInputs
# Control7秒→Stop、SourceSequence3.4秒→Stop→4.5秒待機→再開81.5秒→Stop。
# Transport収録は同引数・DurationSeconds180。操作はbefore/after UTCとwindowを保存。
node scripts/Inspect-ProductGuiProjectStyleDlsAudio.mjs $taskStyleCapture --style-tempo 109
node scripts/Inspect-ProductGuiTransportDlsPriorityAudio.mjs $taskTransportCapture --slow --require-lifecycle
node scripts/Test-ProductGuiProjectStyleDlsAuditor.mjs $taskStyleCapture $taskNewStyleControls --style-tempo 109
node scripts/Test-ProductGuiTransportDlsPriorityAuditor.mjs $taskTransportCapture $taskNewTransportControls
```

保存証拠の再監査:

```powershell
node scripts/Inspect-Q1FixedScenario.mjs `
  work/analysis/q1-fixed-integration/20261006T205000Z `
  work/build/product-snapshot/20261006T202431545Z/build-summary.json `
  work/acceptance/product-project-gui/20261006T211208776Z/audio-20261006T211637034Z `
  work/acceptance/product-project-gui/20261006T211208776Z/audio-20261006T211818105Z
```

次は既存Marker/Mute CRUDを共通Timeline範囲の選択・copy/delete/merge/overwrite/move・単一履歴へ接続する。原版bulk designer未取得、Q2独立Windows未用意、通常core保存拒否、他40責務の残差は維持する。このQ1の成立を全体完成にしない。

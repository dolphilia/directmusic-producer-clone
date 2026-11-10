# Timeline Range keyboard routing（限定完了）

候補20261007T074917222Z、保存ソース208、configure/build/install各exit0。Producer.exe SHA-256 `c16fc585fcfdfa0c63367c2deb7a0e5b1c62491b3abae11f9d31c80e536cf74e`。全40責務・全8受入は未完了、`fullAcceptance=false`。

[単位記録](../../work/analysis/q3-timeline-keyboard/20261007T074251899Z/unit-record.json)、[一次証拠を結ぶ監査](../../work/analysis/q3-timeline-keyboard/20261007T074251899Z/unit-proof.json)、[開始契約](../../work/analysis/q3-timeline-keyboard/20261007T074251899Z/unit-start.json)。原版helpのCtrl+C/V/A/Z/YとPaste=Mergeを根拠に、既存range/history commandsへkeyboard routingを追加した。Edit focusではdocument commandを送らず、canvas clickでfocusを得る。Q0 DWORD契約とSequence26を維持。

作者PID17596で[0,768)のnote/Tempoを3072へCtrl+V merge。UndoでTempo1/notes2/saved、RedoでTempo2/notes3/modified、Ctrl+Aで[0,30720)。EditのCtrl+ZはTarget text3072→0でdocument不変。Edit Ctrl+A/Redoによる文字選択/Redoそのものは本単位で証明していない。作者native保存後exit0、別PID7248でnative Projectを開き3notes/2Tempoを復元・再保存・exit0。

独立RIFF監査はseedから第一note20bytes・Tempo16bytesをclock3072へ複写した期待bytesを組み立て、Segment682bytes全体と一致を確認。Band/DLS全bytes一致。ProjectはSegment size+36とfilh last-write FILETIME以外を保持し、作者→復元再保存はFILETIME8bytes以外一致。未知データ保持だけを編集互換の完成としていない。

[録音証拠](../../work/analysis/q3-timeline-keyboard/20261007T074251899Z/runtime-audio-retry-session/audio-20261007T083147744Z/timeline-keyboard-audio-proof.json) は同じ7248、90秒、6attacks、10秒間隔、C4/loop sustain、Play24秒後の発音中Stop、停止2秒後から再開までRMS0、末尾RMS0、packet gap最大2frames。固定閾値の[19対照](../../work/analysis/q3-timeline-keyboard/20261007T074251899Z/audio-controls/negative-tests.json) はunchangedのみ受理し18異常を拒否。最初の録音は処理中断でStop時刻を過ぎたため[失敗](../../work/analysis/q3-timeline-keyboard/20261007T074251899Z/runtime-session/audio-20261007T082645605Z/timing-failure.json) とPCM/操作時刻を保持し合格に含めない。

同候補native82の原始一巡は46合格/3失敗/33障害。Container Project PID13764、Trigger Project PID13092のWindows5を別々のOS拒否へ分類後46合格/1失敗/35障害。Style GetDefaultChordMapはS_FALSE/NULLで未完。driver124原始21合格/22障害/81未実行を保持し、この単位の3補足後は登録125・24合格/22障害/79未実行。原始一巡を補足で書き換えない。

再検証（録音・native inputsを保持し、作業ソースが208-source保存候補と一致する場合。同候補のソース一致検査は後続のTempo/Farm変更を含む作業treeでは意図的に拒否する。旧一次実行と保存ソースは保持）:

```powershell
$unit='work/analysis/q3-timeline-keyboard/20261007T074251899Z'
$capture="$unit/runtime-audio-retry-session/audio-20261007T083147744Z"
node scripts/Inspect-TimelineKeyboardAudio.mjs $capture native
node scripts/Test-TimelineKeyboardAudioAuditor.mjs $capture work/analysis/new-timeline-controls native
node scripts/Inspect-TimelineKeyboardUnit.mjs $unit $capture
```

Build入口は`./scripts/Build-ProductSnapshot.ps1`。固定ソース/生成物/コマンド/宣言依存は[build-summary](../../work/build/product-snapshot/20261007T074917222Z/build-summary.json)、全回帰入口はmanifestのrunner/引数に保存。新規controls directoryを指定する。

残責務は他strip mapping、CtrlX/Delete、OLE/system MIDI clipboard、snap/meter/reanchor、外部ABI、原版動的比較、現候補Q1全代表経路、Q2独立Windows、全40/全8。原版StylePlayer approval timeoutと各Windows5の解除証拠はない。次は同操作で見つかったmain Tempo label文字重なりを修正し、固定候補Q1統合へ進む。

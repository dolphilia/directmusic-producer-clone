# Q1 最終候補の新規native Projectと音声統合

候補 `20261007T004920093Z` の同じinstallで、新規native Project、五形式の変更・Undo/Redo・保存、正常終了、別プロセス復元、StyleとTransportの停止・再開・テンポ音声を接続した。代表経路限定合格。全40責務、原版動的比較、Q2、全8受入は未完了。

正本は [unit-record](../../work/analysis/q1-final-candidate/20261007T014258Z/unit-record.json)、[scenario-proof](../../work/analysis/q1-final-candidate/20261007T014258Z/scenario-proof.json)。[8受入状態表](acceptance-status.md) は6作業中・2障害・0合格を維持する。200保存source、configure/build/install各exit0。Producer SHA256 `27b5a6eee6911c1d50a73c9a0f57a5c419e40ce17e70fb24b9dd7006c1fbd95e`。本単位では製品source・生成物・録音器を変更していない。

|段階|実行証拠と範囲|
|---|---|
|新規・履歴|作者PID15852が本体New Projectから作成。Initial note追加→Undo0→Redo1、Sequence5→6→Undo5→Redo6→最終Undo5、Style108→109、Band100→101、DLS key low0→1、AudioPath名変更。五形式の履歴を実観測。|
|保存|Fresh.proはnative JAZP、10所有文書・6形式（4 Segment、Style、Band、DLS、2 AudioPath、Script）。全12入力をauthor-savedへ凍結。Project SHA256 `615a2218c3442111e0990c63cf8b42fbad4c239b8f453aa5a6b5f0e733bb8179`。|
|正常終了・再読込|作者15852 exit0、別PID8424で全保存値とUnicode Scriptを復元しexit0。両終了ともread-only Watch-ProductGuiExitの実測値とEXE/hashを結合。強制終了なし。Sky起動区間の唯一の該当PID/windowからの対応付けは推論として記録。|
|Style|同じ最終Project/PID。Style設定109、Empty Segmentテンポ120の10発MIDI60、2秒間隔、前後無音、packet整合。Style6対照は正例のみ受理。|
|Transport|既定Conflictへ接続しないControlはRMS0。embedded Route優先のSequenceを二音目の発音中に停止、停止後RMS0。再開後8音60/62/64/65/67/69/71/72、間隔12/11.995/12/12/8/8/8秒、5→7.5 BPM。全音DLS持続・packet整合・前後無音合格。11対照は正例のみ受理。|
|回帰|同じ候補・生成物が不変のnative79一巡48合格/31障害、driver111原始21合格/21障害/69未実行。既存FileOutput2判定器と今回Style/Transport2判定器の同候補補足後25合格/21障害/65未実行。通常coreは既知Windows5保存拒否を凍結し未実行・障害。|

Control.sgpは旧fixtureのSequenceとroot GUIDが重複していた。immutable inputs-onlyでoffset68の16bytesだけを分離し、原入力と修正入力を両方保存した。イベント・参照・routeその他全byte不変、Project catalogと全所有GUIDの一意性を独立監査した。Initialは本体で新規作成した別identity。未知chunk保持やRIFF監査だけを原版互換合格にしていない。

初回Style操作582msは500ms操作時刻上限を超え、未合格のまま保存。次の録音は新sessionで時刻上限を満たした。初回Style対照は109指定漏れで拒否され、正しい明示引数の別folderで6対照を実行した。録音前metadataのpath区切り不一致と読み出しhelperの誤用、Transport監査コマンドのファイル名誤記も保持。OS拒否の迂回や判定緩和は行っていない。

再現はVS2022/Win32 Release/SDK10.0.26100.0、reference tools OFF、宣言したWindows DirectMusic/DirectSound/GM.DLS等を用いる。保存候補の再buildはbuild-summaryのsourceRootをcmake -Sへ渡し、同じgenerator/SDK/configurationでconfigure/build/installする。新生成物は新候補で再検証する。以下は保存証拠の再監査。

```powershell
node scripts/Inspect-Q1CurrentScenario.mjs work/analysis/q1-final-candidate/20261007T014258Z work/build/product-snapshot/20261007T004920093Z/build-summary.json work/analysis/q1-final-candidate/20261007T014258Z/style-session-v2/audio-20261007T021515742Z work/analysis/q1-final-candidate/20261007T014258Z/transport-session/audio-20261007T021743166Z
```

新規実行はBuild-ProductSnapshot.ps1→native/registered-driver一巡→Skyで同じ表の本体操作→Watch-ProductGuiExit.ps1で両通常終了を実測する。Capture-ProductGuiAudio.ps1に同じGUI/PID/EXE/buildと全12最終入力、互換録音器summary `work/build/audio-capture/20261006T211007927Z/build-summary.json`、SilentKeepAliveを指定する。Style90秒、Transport300秒を直列収録し、操作before/after UTCを保存する。TransportはControl10秒、Sequenceを二音目まで14秒でStop、約9秒無音を挟み、全80秒を再開する。判定器はStyleに `--style-tempo 109`、Transportに `--slow --require-lifecycle`。各Test-ProductGui…Auditorへ新対照folderを渡す。WAV/run/endpoint/packet/QPC/actions/解析器hashを結合し、人の聴取を待たない。

次はQ3G StylePlayer。既存original補助appと同梱txtを観測し、既存Style/ChordMap/Band/Conductorを保持して不足composition/audition/motif責務を本体へ接続する。FileOutput再生中録音Stop、Send/未接続group/多重session/ABI、WaveTrack loop/end、継承DLS runtime export、Timeline/OLE/clipboard、原版全形式比較とQ2はqueueに保持する。

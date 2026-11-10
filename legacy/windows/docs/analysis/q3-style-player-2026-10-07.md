# Q3G StylePlayer：Shape生成と本体試聴の限定検証

候補 `20261007T043410164Z`。206保存ソースからWin32 Release、reference tools OFFでconfigure/build/install各exit0。限定単位を終了した。StylePlayer全責務、原版比較、全40対象と全体8受入は未完了で、全体は6作業中・2障害・0合格、`fullAcceptance=false`。

[単位記録と証拠hash](../../work/analysis/q3-style-player/20261007T022658Z/unit-record.json)、[build](../../work/build/product-snapshot/20261007T043410164Z/build-summary.json)、[契約と未完部分](style-player-contract.md)を入口とする。Producer SHA256は`e7a41478123c090e6ceb0c37820e60930560e7c59b5337c07414a4b18a3e8239`、core testは`9bd85982953e49a40e41eec2d87a2b4fadf3bf53c00690f1bd2a2d704664569c`。保存ソース206件と現ソースのhash一致、全7最終入力の不変を終了時に再確認した。

原版StylePlayer起動は[approval timeout](../../work/analysis/q3-style-player/20261007T022658Z/original-launch-block.json)で未観測。同条件の再起動も別手段による許可ゲート迂回も実施していない。通常coreのChordmap Project Windows5、Script Reference.spp Windows5とは別の障害である。同梱StylePlayer.txtと公開SDKに基づく実装結果を原版動的互換とは呼ばない。

## 変更と検証範囲

`ComposeSegmentFromShape`の専用経路を、本体のStylePlayer session/windowと生成SegmentのProject所有・保存へ接続した。既存`compose_chord_track`、Style/Band/ChordMap編集、Motif、Conductor、参照解決の責務を保持する。SDK生成物の保存非対応をtyped GetParamで補い、Band/Tempo/Style/ChordMap/Command/Chordの6種をnative DMSGへ保存する。未知trackを無言で捨てず、複数Bandイベントは未対応として拒否する。

Shape/groove/intro/end、Re-Compose、Play/Stop、Motif重ね再生を接続した。設定変更とRe-Composeは停止中にも生成して再開始し、Band変更はprimaryを再開始せず現在snapshotと次の生成へ反映する。公開SDKのclass-only default Bandには所有Styleの明示的identityを使う。私有runtimeコピーでBand/tempo carrierを整え、所有入力を書き換えない。

| 判定 | 現候補の一次証拠 | 限界 |
| --- | --- | --- |
| native | StylePlayer64、Sequence26、Transport26、Motif27等。一巡原始80は45合格/3失敗/32障害、入力誤指定3件を同候補補足して48合格/32障害 | 通常coreと凍結専用モードは未実行・障害。原始結果を消していない |
| 本体GUI/native | 作者17556→通常exit0、別復元18940→通常exit0。停止中設定/再生成、live Band、Quiet、Layer Motif、Stop、生成Segment/native Project保存。6所有文書4形式、所有GUID一意、全7入力不変 | 五形式10文書6形式のQ1全体経路は新候補で未実行。旧0049の成功は履歴 |
| 専用WASAPI | PID13980/録音器7920ともexit0。Rising60→84、Quiet60、120BPM、Band音量比0.1596・primary連続、Motif67、発音中Stop、3無音区間、再開。9判定器対照合格 | 専用試聴経路の限定成立 |
| 保存Project本体WASAPI | 復元PID18940、180秒録音でPlay/Stop/再開/最終Stop。owned DLS音高、120BPM、無音、packet整合。5判定器対照合格 | 90秒の初回録音は最終Stop後の時間不足で失敗・保全。テンポ変更全経路は未実行 |
| driver | 原始117は20合格/21障害/76未実行。新登録1判定器を含む同候補7補足後、現118は27/21/70 | 原始一巡と補足を別記録。FileOutputMulti wrapperの`-Only`委譲検出を修正 |

[native原始](../../work/acceptance/regression/20261007T043916688Z/run.json)、[入力補足](../../work/acceptance/regression/20261007T044330912Z/run.json)、[driver原始](../../work/acceptance/registered-drivers/20261007T045801262Z/run.json)、[集計](../../work/analysis/q3-style-player/20261007T022658Z/driver-review.json)、[GUI監査](../../work/analysis/q3-style-player/20261007T022658Z/gui-v2/style-player-gui-proof.json)、[専用PCM](../../work/acceptance/style-player-audio/20261007T044902790Z/style-player-audio-proof.json)、[本体PCM](../../work/acceptance/product-project-gui/20261007T045023269Z/audio-20261007T045356272Z/style-player-gui-audio-proof.json)を保存。

旧0406候補、全開発失敗、誤指定したWVP/DLS入力、forward-slash Save As拒否、DLS identity判定器の誤指定、90秒録音を保持した。DLSは公開RIFFの`dlid`16bytesを監査し、Producer文書の`guid`と区別する。PCMやpacketの不合格閾値を緩めて合格にはしていない。

## 再現手順

保存ソースの`CMakeLists.txt`を使い、Visual Studio17 2022/Win32、Windows SDK10.0.26100.0、`PRODUCER_BUILD_REFERENCE_TOOLS=OFF`、`PRODUCER_BUILD_CORE_TESTS=ON`でconfigure、Release build、installする。引数・ツールhash・全206入力hashはbuild-summaryと隣接ログを参照する。新ビルドは新候補であり、この候補の結果を継承しない。

1. `Test-RegressionManifest.ps1 -BuildSummaryPath <build-summary>`でmanifest一巡。Windows拒否の凍結を維持し、`native-review.json`の3入力を明示した同候補補足を区別する。
2. `Create-StylePlayerFixture.mjs`の新出力を作り、`Test-StylePlayerAudio.ps1`の`FixtureDirectory`と`BuildSummaryPath`へ渡す。録音器build、DLS/Style/Map、endpoint、製品PID、qpc、packetsを連結した46秒録音を`Inspect-StylePlayerAudio.mjs`で判定し、`Test-StylePlayerAudioAuditor.mjs`を新controlsディレクトリへ実行する。正確な引数は保存run/driverのargumentsを参照する。
3. `Test-ProductProjectGui.ps1`でnative Projectを開く。本体Add-InsのStylePlayerで所有Style/Mapを選び、32小節、停止中設定変更→Re-Compose、Alternate Band、Quiet、Layer Motif、Stopを操作する。生成SegmentとProjectをnative保存して通常終了する。保存先はWindows backslashを使用する。
4. 同installの別PIDで最終Projectを開き、生成Segmentを選択する。WASAPI録音を180秒用意してPlay→Stop→再開→最終Stopを各64秒の自然終端前に行い、最終Stop後4秒以上を確保する。通常終了を観測し、`Inspect-StylePlayerGui.mjs`、`Inspect-StylePlayerGuiAudio.mjs`、`Test-StylePlayerGuiAudioAuditor.mjs`でnative/入力/PCMの連結と5対照を確認する。GUI操作観測・操作時刻は`gui-v2/states.jsonl`、captureのactions.jsonに保存する。
5. `Test-RegisteredNativeDrivers.ps1`で一巡し、入力準備が必要な未実行driverを専用生成物・判定器結果で補足する。古い候補の出力を現候補合格へ転用しない。

## 残責務と次の単位

StyleLibrary AASY、null Style/default ChordMapのSave(E_NOTIMPL)、任意の複数生成Bandイベント、原版動的・相互編集比較、外部ABIを保持する。Q2の独立Windowsは未用意で障害。宣言したWindows DirectMusic/DirectSound/GM.DLSとProducer固有原版実行物を区別する。

次は既存Q3F二バッファ実装を使い、長いnative Segmentが発音中にFileOutput録音をStopし、演奏sessionとWASAPI発音が継続すること、番号付きWAVがfinalize後不変であることを本体保存・別復元・二通常終了へ結ぶ。Send/未接続group/多重session/legacy ABI、WaveTrack loop/end、継承DLS export、Timeline/OLE/clipboard、Farm、全形式比較、配布・依存由来も現行queueに残す。

# FileOutput直接接続2バッファの限定成立

候補 `20261007T004920093Z`。全体未完。正本は [単位記録](../../work/analysis/q3-file-output-multi/20261007T003852Z/unit-record.json)、[結合判定](../../work/analysis/q3-file-output-multi/20261007T003852Z/unit-proof.json)、[全体8状態表](acceptance-status.md)。全40責務を維持し、6作業中・2障害・0合格、`fullAcceptance=false`。

Conductorが複数FileOutputを一律拒否していた不足を修正した。既存のsource writer/DMOを保持し、PChannel routeのmix-group順に独立controlを取得する。物理dbfl順を逆転した入力でも、route0をRecord.wav、route8をRecord1.wavへ出力する。全出力先の事前検証、部分失敗時の停止・解放、全controlのStop/shutdown確定、録音中の異なるAudioPath拒否を接続した。Sendなど未接続bufferを対応済みにしない。

根拠は [契約](file-output-multi-contract.md) と原版Help `fileoutputinanaudiopath.htm`。原版の対象designerを現在の起動構成で開けず、同入力動的比較は未観測。以下の独立RIFF/PCM監査を原版比較の代替にしない。

200保存ソース、reference tools OFF、VS17/Win32/Release、SDK10.0.26100.0の [build/install](../../work/build/product-snapshot/20261007T004920093Z/build-summary.json) は各exit0。Producer.exe SHA256は `27b5a6eee6911c1d50a73c9a0f57a5c419e40ce17e70fb24b9dd7006c1fbd95e`。ソース・生成物hashを判定器で再照合した。

- 関連5モード合格。複数FileOutput専用34checks合格。固定候補 [native一巡](../../work/acceptance/regression/20261007T005546448Z/run.json) は79件=48合格/31障害。通常coreは既知Windows5条件で凍結・現候補未実行。
- [driver一巡](../../work/acceptance/registered-drivers/20261007T013632971Z/run.json) は111件=21合格/21障害/69未実行。現候補のPCM・GUI音声判定器を補足した実効集計は23合格/21障害/67未実行。
- 作者15392でAudioPathを1→2 effectsへ追加、Undoで1、Redoで2、AudioPath/埋込Segment/native Projectを保存し正常exit0。別13892でProjectを復元、2 effectsを確認し正常exit0。保存5入力を保持し、Band/DLSは初期入力と一致、Path/Songは専用試験の実runtime入力と完全一致。
- [2本のPCM](../../work/analysis/q3-file-output-multi/20261007T003852Z/recordings/file-output-multi-pcm-proof.json) は22050Hz/stereo/16bit、各1,412,089frames、64.0403秒。Record.wavはMIDI69、Record1.wavはMIDI60のみが優勢。両方に2回の発音があり、各開始時刻と長さが一致する。正例と6反例合格。
- [WASAPI](../../work/analysis/q3-file-output-multi/20261007T003852Z/reload-session/audio-20261007T012735277Z/file-output-multi-gui-audio-proof.json) は48000Hz/stereo/float32、240秒、同再読込PIDの2回Playで両音高、後の無音RMS0、最大packet gap2frames。正例と7反例合格。FileOutputはrender前でUTC時計を持たないため、絶対時刻一致は主張せず、独立に測った2開始の間隔を比較し、観測offset約0.74秒を保存した。

GUI Stopと録音Stopは4秒音符の自然終了後だった。`activeGuiStopsPassed=false`、`recordingStopWhileAudibleEstablished=false`。Stop後も両ファイルが伸び、録音Stop後はsize/hashが固定されたことだけを成立とする。native試験の発音中Stop成功を本体GUIへ転用しない。テンポ変更、五形式Q1、全再生責務は未実行。UIとPIDの対応は、Sky起動区間に生じた唯一の同EXE新規PID・唯一の対象windowからの推論であり、read-only終了監視でEXE/hash/exit0を別照合した。

コンパイル失敗候補004418191Z、dispatcher inventory不整合、UIA geometry/index失敗、初期Project保存名・包含拒否、監視引数不足、音声判定器schema/時計比較不足、GUI非表示起動timeout18636、停止時刻不足を単位記録に保存した。失敗を合格に含めない。原版なし独立Windowsは未用意、原版対象designer不足と既知core/Script Reference.spp Windows5条件を凍結し、迂回・同条件再試行は行わない。

再現する場合、`pwsh -NoProfile -File scripts/Build-ProductSnapshot.ps1` で新しい候補を作り、そのsummaryを各試験へ渡す。新候補へ本候補の成功を転用しない。関連試験は `Test-FileOutputMulti.ps1 -BuildSummaryPath <summary>`、登録群は `Test-RegressionManifest.ps1` と `Test-RegisteredNativeDrivers.ps1` の同引数で実行する。専用試験が作るMultiCapture.proを別作業コピーで本体から開き、route0宛bufferへのFileOutput追加/Undo/Redo、Path保存、Segmentへコピー、Segment/Project保存、正常終了、別プロセス復元を実行する。

WASAPIは `Capture-ProductGuiAudio.ps1 -GuiRun <reload-session> -RecorderBuildSummaryPath work/build/audio-capture/20261006T211007927Z/build-summary.json -DurationSeconds 240 -SilentKeepAlive -AdditionalInputs <保存5ファイル>`。互換recorder SHA256は `9e53dacd2b5793935b459fd04233330065d35edc7cbbbfff019fbeede0ec94d2`。Start Buffer Recordingに新しいRecord.wavを指定し、Play/Stop/再Play/録音Stopを行う。`Inspect-FileOutputMultiPcm.mjs <recordings> <summary> <保存folder>`、`Inspect-FileOutputMultiGuiAudio.mjs <capture> <unit>`、両Test-*Auditorを新controls directoryで実行し、`Inspect-FileOutputMultiUnit.mjs <unit> <summary> <native-run> <capture>` で結合する。終了証拠にはread-only `Watch-ProductGuiExit.ps1` を使う。

次は同004920093Zを固定したQ1五形式統合。UI操作が自然終了前に入る長い/遅い入力を用い、能動Stop・無音・再開・テンポ変更まで同一シナリオへ結ぶ。その後はQ3G StylePlayerの未接続audition責務へ移る。Send/未接続group、多重session、live切替、legacy ABI、WaveTrack loop/end/override、DLS継承runtime export、共有GUID-only fixture解決、共通Timeline/OLE/clipboard、配布/補助app/全40・全8をqueueに保持する。

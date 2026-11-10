# Farm主再生と二次再生の定数修正

全体は未完了、`fullAcceptance=false`。Farmの実行中単位を継続する。

160候補の本体でNight再生中にCougar/Birdを追加すると、短い二次音の終了後に主再生も無音となった。元のBGNight.sgtは無限repeatであり、自然終了による無音ではない。診断用のScriptでは外側のVBScriptが`IsSecondary`を0として評価した。録音、操作時刻、native保存、通常終了と入力hashは[音声記録](../../work/analysis/farm-waves-runtime-repair/20261007T152621164Z/farm-play-context/main-round-20261007T160911000Z/audio-process/audio-stage-summary.json)に保持する。

173757732Zはcompile/installに成功したが、追加した型ライブラリ読取がScript/Farmの初期化を失敗させた。Script依存125のみ合格であり、この候補をruntime合格にはしない。[一次run](../../work/acceptance/regression/20261007T174455752Z/run.json)はScript/Farm各exit1と依存exit0を保持する。

同じ宣言済みWindows DirectMusicクラス、SDK、コンパイラと173候補のcore libraryを使った[公開API診断](../../work/analysis/farm-waves-runtime-repair/20261007T152621164Z/farm-play-context/constant-api-diagnostic/run.txt)では、OS Performance automationの型情報数が0で、GetTypeInfoはE_NOTIMPL（0x80004001）だった。OS内ScriptではIsSecondary=1、AtBeat=16、PlayIntro=4096、NoCutoff=131072を読み取れた。この値はDirectMusic scriptingの値であり、C++のDMUS_SEGF_*の値をそのまま代入してはならない。

修正はsource_script_host.cppとscript_runtime_tests.hに限定する。MicrosoftのローカルhelpのSegment.Play/Stop、PlayingSegment.Stopに記載された18定数名について、既存の所有OS Script内で評価し、実際の数値を読み取り専用のglobal automationとして公開する。 authored sourceとnative bytesは変更しない。未定義値をゼロへ変換する代替処理は入れない。主再生が実際に開始した後で二次再生を開始し、二次開始後/停止後も主再生が続くこと、JScriptの定数と未定義名、同名Constの拒否と作者の独立Constを回帰試験で確認する。

175519731Zでは依存125/Farm32とScript56まで合格し、AtImmediate=0という根拠のない試験期待値で停止した。独立したOS診断で18定数を確認し、AtImmediate=128へ訂正した。[一次runと期待値訂正](../../work/analysis/farm-waves-runtime-repair/20261007T152621164Z/farm-play-context/constant-namespace/175-partial-and-expectation-correction.json)を保存し、未実行だった共存試験を合格扱いにはしない。

180252214ZはScript57まで合格したが、PlayingSegment.IsPlayingの確認で失敗した。同じ保存coreとConductorソースによる診断ではPlayPrimaryは成功し、CheckPrimaryが0x88781215、詳細0x8000ffffと「An error occurred in a call to IsPlaying」を返した。これは開始していないという観測ではなく、外側のScriptからのinstanceメソッド実行の不整合である。[失敗記録](../../work/analysis/farm-waves-runtime-repair/20261007T152621164Z/farm-play-context/constant-namespace/176-playing-instance-failure.json)。

181339882Zでは、返されたPlayingSegmentのIsPlaying/Stopを所有OS Scriptで呼ぶ修正により、主/二次共存と各instance停止が成立した。JScriptのglobal初期化エラー通知を失敗へ反映する修正を追加した。182123503Zの同名Const優先という試験期待値は、実VBScriptのName redefined（0x800a0411）により訂正した。重複名を拒否して以前の有効なsessionを維持し、独立した作者Const=77は成立する。183144678Zはcompile/install、Script74、依存125、Farm32に合格した。原始native87一巡は50合格/Style default-map失敗1/既知OS障害36、driver136は一巡未実行である。元のautomation/interfaceを保持し、from-instance引数では元のOS objectを渡す。省略Stop引数、実HRESULT、参照寿命、native bytesを保持する。回帰成功後は新生成物の本体操作、native保存、別process復元、二つの通常終了、新WASAPI録音を確認する。160候補の成功・失敗・操作記録は保存し、新候補の実行済み結果へ転用しない。Farmの音楽の音源/音高/テンポ、混入、Loader残責務、原版比較と全40/全8受入は引き続き未達である。

診断用ビルドは深いTryCompileパスのFTK1011と未正規化includeパスのC1083で失敗した。失敗ログを保存し、同じSDK/コンパイラの新しい短いbuildパスとパス正規化だけを使用した。SDK探索拒否や個別Windows5の解除とは扱わない。[候補・診断・変更hashの記録](../../work/analysis/farm-waves-runtime-repair/20261007T152621164Z/farm-play-context/constant-namespace/173-failure-and-api-diagnosis.json)。

183候補の新しい[本体/native記録](../../work/analysis/farm-waves-runtime-repair/20261007T152621164Z/farm-play-context/main-183-20261007T183610413Z/main-native-proof.json)は、12操作、5334 bytesのScript保存、526 bytesのJAZP保存、別PIDの復元・初期化、二通常exit0を結ぶ。保存Scriptは原入力とbyte単位で一致し、復元と録音後も20原入力と3保存ファイルのhash/長さに変化なし。Producer SHA256は`e88b58a978db28268f18d47149c6d00a093f607a2c64611268f9ba71d8624f7c`、native証拠は`cd44db32730457a00b17bfca5e99a4323d54b93a5009b33eb30a5a2efc6b8fcd`。

[新音声記録](../../work/analysis/farm-waves-runtime-repair/20261007T152621164Z/farm-play-context/main-183-20261007T183610413Z/restore-process/audio-stage-summary.json)は同じ復元PID17204で300秒録音を2回実行した。六種単独SFXは固定0 cent/130 ms prefix相関0.98以上、負例5種をすべて拒否。NightのCougar終了後継続、Bird追加後の音声継続、停止無音、再開、Farmを閉じた後の無音が固定の8区間判定に合格。混合区間の相関は診断だけであり、音源同一性や音高/テンポの合格へ拡張しない。180秒で3操作だけだった録音と160候補の失敗も保持し、判定基準は変更しない。音声要約SHA256は`4c582b3a2c4f6251fe025f7977c011a9d9a8bed6ebce677f41e5d5d04964bfb9`。この修正範囲を閉じてWaves本体検証へ進むが、Farm全責務・原版比較・全40/全8は未完のまま。

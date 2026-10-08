# Farm Night / Bird 音声検証の進行状況

105655237Z の Farm Project 保存、別プロセスでの復元、二通常終了を確認した。Bird の先頭音は native と復元後 main で入力 DLS に一致した。native の単独6音/同時再生7音は入力譜面の音高・長さ・強さ・音楽クロックに一致した。全PCM音源、Night、100 BPM、混合、main の発音中背景停止は未達。単位は未達を保持して中断、`completed=false`、`fullAcceptance=false`。

保存済み製品は221ソースの [build-summary](../../work/build/product-snapshot/20261008T105655237Z/build-summary.json)、SHA256 `596725f5420f37c4e43965a19a1f2cd9642b7e60dd8d42d3a9cd837a69c7ab1e`。main EXE は `e70bc336b69cba16997286601a6f6319c5067fa06a3c1773c8f70cea9a8d0ad8`。製品 C++ は今回変更していない。0735 の Q1 や Farm の成功を1056へ転用していない。

author PID1220 は保存後 exit0、同じEXEの別 PID5636 は native Project と `FarmDescriptorSaved.spp` を復元し、初期化・Bird 再生後 exit0。[checkpoint](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/checkpoint-main-audio-v1.json) の SHA256 は `dab548c76321ef8c5b7c1dc9d758b7557778bc32d44c6c8847652fee387b21d6`。23入力は保存元と main コピーの双方で不変。保存した `main-inputs-v1.pro` は元 Project と同じ584 bytes、SHA256 `1ae7499b87a38dbe4554134f453a041129e13363b3df88ad99ebaca9c5d82da5`。

WASAPI は保存済み録音器 `9e53dacd2b5793935b459fd04233330065d35edc7cbbbfff019fbeede0ec94d2` を使用し、UTC/QPC、endpoint、packet、入力、操作時刻、PCM、解析器を結合した。判定は130 msの入力波形、相関0.98以上、RMS0.0003以上、静音RMS0.00005以下を維持する。

| 試行 | 結果と保持する不足 |
| --- | --- |
| main author 300秒 | 通常録音終了。Bird先頭相関0.961で不合格。背景停止の二操作はBird終了後で、発音中Stopの証拠にしない |
| native observer v1 / 32秒 | 53実ノート、二通常exit0、入力不変。背景StopがBirdの可聴開始前。単独Bird前の背景余韻RMS0.000962。失敗録音を保持 |
| main reload 90秒 | 先頭音は測定MIDI84、相関0.998、前後静音成立。独立したscore音程oracle、全テンポ・混合は未証明。6対照を保存 |
| native observer v2 / 32秒 | 背景停止後待機を1→3秒、Bird後背景Stopを0.4→1秒、Stop後観測を2.5→4.5秒に変更。53ノート、二通常exit0。先頭MIDI91との波形相関0.989、前後静音成立。8対照成立。後続音とStop後の相関は0.98未満 |

v2 の [run](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/native-note-observer-v2/audio-execution-v1/run.json)、[PCM監査](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/native-note-observer-v2/audio-execution-v1/bird-audio-proof-v1.json)、[対照](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/bird-native-first-prefix-controls-v2/negative-tests.json) を一次証拠とする。native note の収集時刻を実際の発音時刻と扱わない。PCMから推定した間隔は100 BPMに近いが、後続音の源照合が未達なので `strictSoloTempoPassed=false`、`postStopPrefixPassed=false`、`wholeFarmPassed=false`。

先頭音用3 driverに、`Inspect-FarmBirdScoreNotes.mjs` と `Test-FarmBirdScoreNotesAuditor.mjs` を追加した。native期待MIDIは独立入力の全バリエーション照合後に取得する。[追加実行](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/registered-driver-supplement-v1.json) は先頭音の限定契約での合格であり、Farm単位の終了ではない。1056の159原始driverは24合格/23障害/112未実行のまま。追加2と既存2の改訂実行は [補足v2](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/registered-driver-supplement-v2.json) に結合した。登録164、29限定合格/23障害/112未実行。Echo照合器/逆処理は受入driverに数えず診断欄へ登録した。0735の12補足と古い `primaryRoundResult` を履歴に分離した。

再現は v2 の保存 `build-observer.ps1` で計測器をビルドし、追加実行JSONの `command` を新しい出力ディレクトリに対して実行する。過去の出力は上書きしない。製品、core、Conductor、計測器、録音器の保存hashを照合する。既知SDK拒否の同条件再試行や原版の許可ゲート迂回は行っていない。

[入力DSP監査](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/independent-audio-path-echo-v1.json)（SHA256 `9423e19b66d42604407ce9ea0961bfa15ee4c6d3e103d61420d57363524553ad`）で、portの明示22,050 Hzと標準Echo `{EF3E932C-D40B-4F51-8CCF-3F98F1B29D5D}`、wet/dry30%、feedback0%、左右300/600 msを確認した。保存1056 coreで作った別OS AudioPathの `GetAllParameters` も一致、PID24204通常exit0、24入力不変。隠れた実Script PlayingSegmentのlive AudioPathを読んだ証拠とは分ける。計測器のlink/埋込選択/同一bytes複製のguard失敗とWOW64表記のprelaunch拒否は保存済み。製品の新しい失敗には数えない。

単音の局所線形余韻fitだけでは後続不一致を説明できなかった。独立Echo設定による診断逆処理でMIDI93/96/98は0.993/0.994/0.993に改善し、先頭91と合わせ4/6が0.98以上。短い第三音91は入力長112 ticks（100 BPMで87.5 ms）で130 ms窓内にnote-offが来る。最後100も未達。既定envelope、音の重なりとsample-rate経路のモデルは未確定、逆処理を正式合格にしない。

[独立score監査](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/score-auditor-execution-v2/score-proof.json) は26入力ノート、CM7 root0、zero humanizationから四variationを作り、solo6音はmask8、coexistence7音はmask4に音高/musicValue/music clocks/長さ/velocity全一致した。15対照は別通常exit0で拒否。PCMの可聴タイミングやmain全譜面は証明していない。改訂先頭音PCMはnative/mainの両方で通常exit0、8+6対照も成立、旧v1報告を保存した。

この時点での次作業として、入力譜面・Echo・同じ宣言OS synthによる独立referenceを作り、note-off/余韻/sample-rateを含む全PCM判定を検証する。相関閾値を下げず、Night音源、100 BPM、混合とmain発音中Stopの不足を保持する。原版比較、Q2、全40責務・全8受入は別に未完。

## 独立固定ノート参照（13:44〜14時台UTC）

製品コードをリンクしない公開OS Performance/Loaderによる参照を用意した。入力26ノートと230制御カーブから4variationのSequenceを独立生成し、物理RIFF監査で照合した。原入力24物理ファイル（本体最終23と補助記録）は不変。AudioPathは全bytes同一、接続は10/11、Echo30%/feedback0/300ms/600ms、実buffer22050Hzを読み取った。

初回[参照v1](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/fixed-reference-audio-v1/run.json)はDownload0x08781092でnormal exit1、録音器はnormal exit0。接続しない14Band楽器を参照コピーだけから除き、接続する10/11の楽器bytesをDLS ref valid-mask3の明示変更以外は一致監査した。v2と[時計追加v3](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/fixed-reference-audio-v3/run.json)は4自然終了と再生/録音二normal exit0。v3 PID20420/18020、入力不変。各PerformanceのGetPrepareTimeは1000ms、GetTimeとMusicToReferenceTime、FILETIME/QPCを結んだ。

[診断と監査の実行](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/reference-auditor-execution-v1/run.json)はNode PID20416/15232 normal exit0。130ms/.98/25msの比較条件は変更せず、native soloの六音は相関約.988/.991/.919/.936/.986/.942で全音成立せず。native混合とmainも未達。60msの乾音attack探索は診断専用で、製品音源合格には使っていない。main単独Birdの100BPM仮説より120BPMの後続4音に強い一致があり、入力のSSBird.Play(IsSecondary+AtBeat)と背景未開始条件を区別した。120BPMの確定・原版比較やmain100BPMを合格にしていない。

[固定参照診断v3](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/reference-auditor-execution-v1/diagnostic.json)はdiagnosticOnly=true、pcmAcceptance=false。hidden live Script pathの直接照合、独立ノートoff/重なり参照、Night音源、混合、main発音中背景Stop、全40/Q2/全8は残る。次は独立した単音参照から残音と重なりを調べる。

## 単音参照と100BPM準備（14時台UTC）

入力variation4/8から13音を独立生成し、元duration/velocity/MIDIと楽器10/11、AudioPathを保持した。初回buildのFileTracker FTK1011を保存し、repo内の短いbuildパスで成功。長いパスが原因という解釈は短いパスで同ソースがbuild0になったことに基づく。SDK探索拒否の解除やOS保存拒否の迂回には数えない。

初回13単音は二normal exit0だが、最後の音はDLS attack校正不成立。全13の参照成立と呼ばない。参照だけnote開始を384clockへ移し、Band設定後300msを置いたv2は13校正成立。ただし130msの重なりモデル相関約.966、六prefix中一音未達。係数やsample未満delayの探索は診断であり、音源受入ではない。

続いて無音の一次Tempo100.sgtで準備する[参照v3](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/isolated-reference-audio-v3/run.json)を実行。公開Performance GetTimeの二時点から100BPM±1を確認し、同一13入力を再生。PID14308と録音PID11564は二normal exit0、入力不変、13自然終了。[監査/診断](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/isolated-reference-auditor-execution-v1/run.json)はNode PID1056/10324 normal exit0。130ms/.98/25msは維持。診断モデル全相関.9603、六prefixの相関.9900/.9922/.9921/.9847/.9875/.9788で五音のみ基準超過。短い第三音の改善を、原因確定や全音合格にはしない。全PCM、Night、混合、main停止は未達。

[次の実Script経路診断契約](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/live-script-path-protocol-v1.json)では、一時的なsourceコピーでBirdの戻り値を一変数へ保持し、公開SegmentState8 GetObjectInPathを読む。原23入力は書き換えず、保存1056Conductor/coreを使用する。コピー/参照保持の条件差は明記し、原入力やmainの受入に転用しない。

## 実Script経路と本体停止の追加試行（14:58〜15:29 UTC）

[実Script経路のまとめ](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/live-script-path-summary-v1.json) は4試行とも計測器normal exit1。v1〜v3は予約状態からのAudioPath lookup、v4は1800ms後のPerformance lookupが0x88781161で停止した。v4はIsPlaying/GetTime/AudioPathの後段に到達していない。v1の録音正常exitは不明のまま、v2〜v4の録音はnormal exit0。原入力は不変。Bird戻り値を保持するsourceコピーの条件差を残し、原入力、本体、実live routeの受入にはしない。OS拒否や製品不具合の原因も確定していない。

[本体停止のまとめ](../../work/analysis/q3-farm-background-audio/20261008T110022122Z/main-stop-summary-v1.json) は1056 EXEのPID14316、開始2026-10-08T14:59:58.2620441Z、通常exit0、終了15:28:19.5545145Zを結ぶ。起動時Segmentはstartup-retained.sgpと一致名のnative ProjectへGUI保存して保持した。最初のProject名不一致の保存失敗も操作記録に残した。その後既存Project/Scriptを復元して初期化した。221保存/作業ソース、原23入力、各録音の25物理入力は一致。製品ソースは変更していない。

| 録音 | 通常終了と実測 | 未達 |
| --- | --- | --- |
| 90秒、15:19:52開始 | 録音exit0、背景RMS0.04291 | StopはBird操作後22.846秒で録音時間外。発音中停止/停止後無音は判定不可 |
| 300秒、15:24:16開始 | 録音exit0、背景RMS0.04490、停止後RMS0 | StopはBird操作後52.457秒。発音中Bird停止、Night/Bird音源分離、100BPMは未達 |

解析の初回はactionsの配列/ラッパー差でnormal exit1、修正版は二Node normal exit0。失敗ソース・stderr・runも保存した。相関.98/130ms/25ms等は変更していない。混合録音の第一音推定は診断のみで、score音高受入に転用していない。diagnosticOnlyとしてmanifestへ登録し、164driverの29限定合格/23障害/112未実行とnative91の53合格/37障害/1失敗は変えない。

固定参照、単音参照、live経路/本体停止の診断を続けても全PCM・テンポ・混合・発音中停止の不足は減らなかった。優先順位を見直し、記録済みSendのStandard Environmental Reverb宛先へ進む。Farmはcompleted=false、fullAcceptance=falseの未完queueに残す。同じ失敗条件の再実行や閾値緩和は行わない。原版比較/Q2/全40/全8は引き続き未達。

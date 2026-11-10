# 無人音声確認の契約と実測

更新：2026-10-07。全体未完了。ユーザーから音の確認を無人化する依頼があり、実測後に計画へ採用した。

Windowsの再生出力をWASAPI shared-mode loopbackで直接録音する。Stereo Mixの設定変更・マイク・追加インストールを要求しない。根拠はMicrosoftの[Loopback Recording](https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording)。全アプリの出力が混合されるため、静かな試験環境を使い、基準区間の雑音は不合格にする。PID隔離が必要な場合の正式な拡張は[Application loopback audio capture](https://learn.microsoft.com/en-us/samples/microsoft/windows-classic-samples/applicationloopbackaudio-sample/)（build20348以降）だが、今回は実装・試験していない。

## 実装

- `tests/audio/loopback.cpp`：既定render/eConsole endpointのIDを保存。実mix formatのWAV、QPC基準packet位置/flags/device position、固定録音時間を保存する。無packet/欠落区間はゼロで保持し、解析器がtimestamp error、初回以外のdiscontinuity、2msを超すpacket間gapを拒否する。初回discontinuityは初期化として許容。OS設定を変更しない。
- 明示オプション `--silent-keepalive` は同じshared render endpointへ無音バッファを供給してidle時のpacket欠落を防ぐ。defaultを保持し、packet/gap判定を変更しない。GUI captureの `-SilentKeepAlive` と録音器build/hashをrunへ記録する。[現候補Q1の無音対照・Style・Transport証拠](q1-fixed-integration-2026-10-07.md) を参照。旧録音の失敗を修復した扱いにはしない。
- `scripts/Build-AudioCapture.ps1`：2ソースを新規ディレクトリへ保存し、Win32/MSVCで構成とコンパイルを分離記録する。録音ツールは製品/原版比較DLLと独立。
- `scripts/Test-LoopbackAudio.ps1`：製品・録音器の保存ソース/EXE照合、ready後2秒の基準区間、現行本体`--note-observe`起動、16秒録音、解析、57ロードmodule由来監査を無人実行する。`-SilenceControl`は本体を起動しない。
- `scripts/Inspect-LoopbackAudio.mjs`：IEEE float32 WAVを解析。固定した6秒の120 BPM strings入力（C4×4、C5×4、C4×4）専用。前無音RMS<0.0001、終了後12〜15.5秒RMS<0.0001、再生区間RMS>0.001、発音開始2〜4秒、peak<0.99、12生成音の独立した固定期待値、録音のC4/C5成分と各区間を照合する。外側はC4/C5比>0.25、中間は<0.15、対象成分>0.00005。任意楽曲や別音源をこの閾値で受け入れない。
- `scripts/Test-LoopbackAuditor.mjs`：録音のコピーだけを無音/中間誤音へ変更し、API成功・12音の記録を保持しても録音判定が拒否することを確認する。派生物を実録音の結果と混同しない。

## 版と結果

製品：`work/build/product-snapshot/20261003T094600314Z/build-summary.json`、保存54ソース、EXE SHA256 `ac0f17480bb25ed0303ec42209712229c84858701290adb8592720171a7bf8a3`。製品ソースを変更していないため製品の再コンパイルは行わず、このEXEを新しいプロセスで試験した。

録音器：`work/build/audio-capture/20261003T102354462Z/build-summary.json`。保存2ソース、VS17.14.51/MSVC19.44.35228/SDK10.0.26100.0、構成0/compile0。EXE SHA256 `cf06449043e1b6cbea6e19b5ac8875e63f2f99d65e1614f167311cee6c855727`。録音器ビルド成功を音声成功とは別に判定した。

入力：`work/acceptance/pattern-properties/20261003T094717628Z/core/pattern-properties/fill.sgp`（SHA256 `18742471d53ea68c22509742cf1aa0d3ddc4f519d463ed06161e0066086d8783`）と同ディレクトリの`Heartlnd.stp`（`f0cacd7f52896d9c1f4d10ae03ef74e99e5dafb17c65f9605416ea6442b0f523`）。Band PChannel5/Program48のstrings。独立した生byte比較で確認済みの編集済みStyleを使う。Windows dmime/dmloader/dmstyle/dmband/dmusic/dmsynth/DirectSound/GM.DLSは宣言するOSランタイム依存として残る。

| 新規証拠 | 実測結果 |
| --- | --- |
| `work/acceptance/audio-loopback/20261003T102448148Z` | 本体exit0/録音exit0、48kHz/2ch/float32/16秒、前後RMS0、再生RMS0.0271139267、peak0.134527579、onset2.5秒。C4→C5→C4の12区間が合格。packet gap最大2frames/初回discontinuity1/時刻error0。現行57modules原版40hash一致0。GUI未実行 |
| `work/acceptance/audio-loopback/20261003T102549376Z` | 無再生対照、録音exit0、全区間RMS0。`negativeControlQuiet=true`かつ音声再生の`passed=false`。対照の成功と製品再生の合格を区別 |
| `work/acceptance/audio-auditor-controls/20261003T102700Z/negative-tests.json` | 派生した無音/中間誤音の2件をexit1で拒否。どちらも生成音のAPI記録は成功のまま |

各runの`run.json`、`output.wav`、`packets.csv`、`capture.json`、`audio-proof.json`、保存driver/auditor、notes、module-provenanceを保持。単位manifestは正例runの`unit-record.json`。WAVを現在の別runへ転用しない。

初期試作`101930830Z`正例/`102009614Z`無再生も保持する。最初のcompileは`<string>`不足で失敗して修正、以後新規保存ソースから構成/buildした。最初の解析器はfloat加算ループで末尾にNaN区間を追加し対照のquiet判定を失敗させた。整数区間へ修正し保存WAVの解析だけを再実行した。陰性派生コピーは正例ディレクトリの子に作ろうとしてNodeが拒否したため、元WAVを変更せず別ディレクトリで実行した。いずれもOS拒否の迂回ではない。

## 先行GUI Stop/再開の別記録

`scripts/Create-LongStylePlaybackFixture.mjs`で既存GUI保存入力からSegment length9216→49152（120 BPM/32秒）だけを変更し、全他byte/Style/projectを保持した。`work/analysis/style-stop-restart/20261003T100600Z/inputs.json`の作成UTCと実hashを基準とし、ディレクトリ名を実施時刻として推測しない。

現行EXEの`work/acceptance/product-project-gui/20261003T100413080Z/stop-restart-proof.json`はGUI Playing→17.137秒時点の明示Stopped→再Play→自然終了を独立監査、74modules原版40hash一致0/通常終了exit0。ユーザーの初回回答「聞こえませんでした」を失敗として保持する。ユーザー依頼による同版・同入力の新PID`100912895Z`の再試験では「ストリングスの音が聞こえます」。別の`audio-question.json`へ記録し初回失敗を修復しない。再試験は自然終了の実画像とUIAキャッシュのPlaying表示不一致を`visual-end-observation.json`に明示した。再試験も通常終了exit0、74modules原版40hash一致0。聴取は再試験の発音だけであり、Stop時消音・再開音の証明に流用しない。質問でpianoと呼んだのは誤りで、原始BandにはPChannel5/Program48が存在しstringsに整合する。初回無音の原因は未特定。

## 再現

PowerShell、cmake、既存VS2022/SDK、Nodeを使用。`$nodeExe`には利用環境のNodeパスを設定する。

```powershell
./scripts/Build-AudioCapture.ps1
# 出力された新規build-summary.jsonとEXEを次の2引数へ指定する。
./scripts/Test-LoopbackAudio.ps1 `
  -BuildSummaryPath work/build/product-snapshot/20261003T094600314Z/build-summary.json `
  -RecorderBuildSummaryPath work/build/audio-capture/20261003T102354462Z/build-summary.json `
  -Recorder work/build/audio-capture/20261003T102354462Z/build/Release/producer_loopback.exe `
  -Segment work/acceptance/pattern-properties/20261003T094717628Z/core/pattern-properties/fill.sgp `
  -Node $nodeExe
# 同じコマンドに -SilenceControl を付けて別runの陰性対照を取得する。
# 解析だけの再現（保存済みWAVを使用）:
& $nodeExe scripts/Inspect-LoopbackAudio.mjs work/acceptance/audio-loopback/20261003T102448148Z
& $nodeExe scripts/Test-LoopbackAuditor.mjs work/acceptance/audio-loopback/20261003T102448148Z work/acceptance/audio-auditor-controls/new-unique-directory
```

今回のAPI本体は実際の音源/文書経路を再生したが、GUIの操作試験とは別。録音で確認できるのはデジタル出力で、物理スピーカー・室内音圧・主観的音質を証明しない。無音がどの設定・音源・実装に起因するかの診断は別途必要。背景音混入、出力endpoint切替、非対応mix formatは成功にしない。

上記は094600314Zの結果。以下の新しい製品版では明示Stop/再開を追加確認した。版間で結果を移し替えない。

## 103503304Z 明示Stopと再開の実音声

本体`src/producer/main.cpp`へ`--audio-lifecycle <output> <long-segment>`を追加した。通常GUIと同じFramework文書/所有Style/Conductorを使用する。Segment長49152clocks以上を要求し、Playが返った後2秒再生→Stop→3秒待機→同じ文書でPlay→2秒後Stop→2秒待機/Shutdown。各操作のQPCを100ns単位でJSONへ記録する。再生前に本体の所有入力、実runtime Segment、Style snapshotを保存する。GUIはこの版で未試験。

製品`work/build/product-snapshot/20261003T103503304Z/build-summary.json`：保存54ソース/3targets、構成・compile・install各0、warning/error0。EXE SHA256 `4e6a6c074c152de215551e2a2b667f46ed23d0533bf0eae9e8164bc381e8daee`。core EXE `6032431e95dcddd5456489343165253b531fd5929197746bb93567474a294989`は生成したが未実行であり、旧版20件等の成功を流用しない。直前の103423429Zは存在しない`get32`を使ったcompile失敗として残し、`read32`へ修正して新規保存/構成/buildを行った。

録音器は102354462Zを再使用し、新本体/新PIDの録音を新規取得した。入力は`work/analysis/style-stop-restart/20261003T100600Z/fill.sgp`（SHA256 `f8886e599416cb97b2928bad35b94c1c50583daf2515768de1ac395aff813475`）、同じ場所の`Heartlnd.stp`（`e808287707e4954697870f693e26903bc16fc49b8b60d078136f69330c715e5d`）。32秒/120 BPM、開始PatternのC4 stringsを使う。

`scripts/Inspect-AudioLifecycle.mjs`はpacketのQPCとframe位置から録音開始時刻を推定し（最初100packetの中央値）、本体QPCを同じ時間軸へ写す。操作順序/2秒再生/3秒待機を照合し、前無音、両発音区間RMS>0.001/C4成分>0.00005、Stop返却後1.2秒の減衰猶予を除いた待機RMS<0.0001、最終Stop後無音、clip/packet整合を判定する。減衰猶予はこのstrings入力用で、Stop直後にゼロになるとは主張しない。

`work/acceptance/audio-loopback/20261003T103741753Z/lifecycle-audio-proof.json`：本体/録音exit0、録音16秒48kHz2ch。最初の発音RMS0.0254273422、再開後0.0252255585、前/Stop待機/最終Stop待機は各0。Stopは録音5.710886秒、再Play8.973807秒、最終Stop11.093059秒（各return時刻）。APIもfirstPlaying/firstStopped/restarted/finalStopped=true。first play後の58modules原版40hash一致0、point inventoryであり再Play後の別inventoryは未取得。`work/acceptance/audio-lifecycle-controls/20261003T103900Z/negative-tests.json`は派生した「停止中に音が残る」「再開後無音」の2件をexit1で拒否し、native API成功記録はそのまま残した。

同版hostは`work/acceptance/product-host/20261003T103853447Z/run.json`とmodule-provenanceでexit0/原版40hash一致0。Pattern/GUI/DLS等の既存大量成功試験は、今回その実装を変更していないため再実行せず、旧版結果として保持した。

再現は上の`Test-LoopbackAudio.ps1`コマンドの製品summaryを103503304Z、Segmentを上記32秒入力に変更し、`-Profile lifecycle`を追加する。録音/判定/module監査まで無人で行う。解析だけは`Inspect-AudioLifecycle.mjs <new-run>`、陰性派生試験は`Test-AudioLifecycleAuditor.mjs <new-run> <new-unique-control-directory>`を使用する。新runの`unit-record.json`へソース/生成物/入力/録音/判定器/依存証拠を結合した。

次は120→180 BPMの発音間隔（0.5→約0.333秒）を独立解析する。その後Pattern CRUD/variation/Motif、JAZPと残る本体機能を進める。現行GUI/全音源/全40責務と全八受入は未完了。今後の音声工程は録音・自動判定を標準にし、聴取質問への回答待ちを必須にしない。

## 105414596Z テンポの実発音間隔と先頭音欠落の修正

`Create-TempoAudioFixture.mjs`は既存012644800Zの`playback/playback.sgp`から、3072clocksのViolinへのBand切替だけを除いたコピーを作る。最初のGM piano Band、Sequence8音、Tempo0:120/3072:180は保持し、変更した祖先のRIFFサイズ以外は再帰的に元へ戻した全byte一致を確認した。`work/analysis/tempo-audio/20261003T105000Z/inputs.json`（実作成UTC10:45:21.642Z、ディレクトリ名で時刻を推測しない）に元SHA `afefe577cc749b77da522136d891f81824a7a7e0e25db8079007802abda2feff`、生成SHA `822af8867297dc95a6bd101cbb1fee8b3d456bcba0ad2ca5bedb765fdcdae7a8`を保持。原版依存の追加ではなくソース製品が以前生成した入力である。

103503304Zを新PIDで録音した`work/acceptance/audio-loopback/20261003T104730723Z`は不合格。生成APIはC4〜C5の8音を記録したが、録音の信号開始2.725秒・7発音のみで、実際の音高も2音目からに対応する。判定器の検出しきい値を緩めず、先頭音の実出力欠落として保持した。初期失敗proof/auditorと詳細候補を保存し、同条件で再起動を繰り返さなかった。

本体Conductorは`PlaySegmentEx`を即時flags0で呼んでいた。保存公開`dmusici.h`の`DMUS_SEGF_AFTERPREPARETIME=0x400`を`src/compat/playback_runtime.h`へ宣言し、`GetPrepareTime`を記録してこのフラグで実行するよう修正した。根拠はMicrosoftの[Time-Stamped Events](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/time-stamped-events)と[DMUS_TIME_RESOLVE_FLAGS](https://learn.microsoft.com/en-nz/previous-versions/ms808252(v=msdn.10))。準備時間を尊重する変更であり、音源/スケジューラ内部の細かな欠落原因を特定したとは主張しない。中間105103815Zの録音105228241Zは先頭を含む8音に復帰したが、最終版へ結果を転用しない。

この環境のGetPrepareTimeは1000ms。Stop/replay試験はPlay返却直後ではなく、positionの`playing && clocks>=start`を最大5秒待ち、`play-ready/restart-ready`QPCを記録してから2秒を計るように変更した。録音解析器はreadyを時間基準にする。旧9phase形式は旧保存録音の再解析用だけに対応し、新版は11phaseを出す。

最終製品：`work/build/product-snapshot/20261003T105414596Z/build-summary.json`、保存54sources/3targets、構成/compile/install各0、warning/error0。EXE SHA256 `4d7c0f21601d020adfa342d97d0c5b6e5b9416e72144712af2955417ae5b0ce8`。core EXE SHA `628409e9778a8d2e301d22baf350cadf28b977205789e784330ea2c2fd0eed03`は生成のみ、旧core/GUI成功を転用しない。

`Inspect-TempoAudio.mjs`は入力のRIFFからTempo/Sequenceを独立読取し、API8音の時刻差・音高・duration/velocityと比較する。音声onsetは録音だけの10ms RMSを5ms刻みで求め、10msで0.003を超える上昇の局所最大、200ms内の重複を除いたものを採用する。8発音・各間隔が入力120/180の期待0.5/1/3秒から30ms未満、各音の基本周波数成分が隣接半音の1.3倍超、前/後の無音、packet整合とclipも要求する。native時刻を音声onsetへ代入しない。このGM piano固定入力専用で、任意音源/楽曲の万能判定器ではない。

最終版の新規証拠：

- `work/acceptance/audio-loopback/20261003T105546720Z/tempo-audio-proof.json`：本体/録音exit0、実8音、間隔0.5/0.5/0.5/0.5/0.340/0.335/0.330秒、入力/実音高も合格、前後RMS0、peak0.1126063168。原版40hash一致0の同run module監査を保持。
- `work/acceptance/audio-loopback/20261003T105642085Z/lifecycle-audio-proof.json`：同版StyleのStop/3秒待機/再Play/最終Stopを実開始待機付きで再検証。両発音C4、前/停止中/最終停止後RMS0、API/録音exit0。再生スケジュール変更に関係する回帰だけを再実行した。
- `work/acceptance/product-host/20261003T105733464Z/run.json`とmodule-provenance：同版host exit0、原版40hash一致0。
- `work/acceptance/tempo-audio-controls/20261003T105800Z/negative-tests.json`：派生先頭音欠落を拒否。音高とAPI8音を保持した「後半も120 BPM=全間隔0.5秒」をintervalPassed=false/exit1で拒否。
- `work/acceptance/audio-lifecycle-controls/20261003T105800Z/negative-tests.json`：派生停止中発音・再開無音を両exit1で拒否。いずれも実録音そのものの失敗と混同せず、コピーだけを変更した。

再現はTest-LoopbackAudioの製品summaryを105414596Z、入力を上記tempo.sgp、`-Profile tempo`にする。同じsummaryで32秒のfill.sgpを指定し`-Profile lifecycle`で関係する回帰を行う。解析はInspect-TempoAudio、陰性派生はTest-TempoAudioAuditorの各mjsを使用。録音器は102354462Zの同じ保存生成物。正例tempo runのunit-record.jsonに最終/中間/失敗版・入力・録音・解析ソース・陰性派生・module証拠を結合した。

次は本体StyleのPattern新規/削除/クリップボード/variation/Motifを進め、Timeline/文書管理と保存復元へ接続する。今回の固定2経路の音声合格を全音源・現行GUI・全40責務・全八受入へ広げない。OS DirectMusic/DirectSound/GM.DLS依存は残り、原版比較/その他文書の未完を維持する。

## 無人運用の最新確認（111240720Z／112405950Z）

依頼された無人確認は実装・実録音済みであり、今後の標準工程として採用する。人の回答待ちは必須にしない。Stereo Mixの有効化や既定録音デバイスの変更は不要。録音器は既定の再生endpointを直接記録する。

現行製品111240720Z（EXE SHA256 f3c795ae016373a01d3195acd3a543c1d8dabbd603ed2e99f4b62dd6edc9966c）の新規Patternを保存・別Frameworkで復元した入力を、新PIDで録音した111436846Zは合格。48kHz/2ch/float32、12生成音と録音のC4成分、前後無音、packet整合を確認。再生RMS0.0204052619653924、前後RMS0、peak0.0962437093257904、onset4.5秒。crud profileの開始猶予は2〜5秒で、既存notes profileの2〜4秒とは別。再生58modulesで原版40hash一致0。詳細の入力・ソース・生成物対応はpattern-crud-contract.mdと当該runのunit-record.jsonに保持する。

`Test-LoopbackAuditor.mjs`をnotes/crudの両固定入力へ対応させ、未変更コピーの正例と背景音混入の陰性対照を追加した。`work/acceptance/audio-auditor-controls/20261003T112405950Z/negative-tests.json`では4件の期待結果が成立。未変更コピーexit0、無音/中間C5誤音/基準区間への背景音追加は各exit1。全ケースで生成API12音は成功のまま。背景音対照の基準RMSは0.021213814181415526。保存録音の派生コピーを解析した結果であり、本体の新しい録音成功とは数えない。元WAV・元判定記録は変更せず、対照と解析器を別ディレクトリのunit-record.jsonへハッシュで結合した。

現行入力を新しく無人録音するコマンド（Nodeは環境の実パスを指定）：

```powershell
./scripts/Test-LoopbackAudio.ps1 `
  -BuildSummaryPath work/build/product-snapshot/20261003T111240720Z/build-summary.json `
  -RecorderBuildSummaryPath work/build/audio-capture/20261003T102354462Z/build-summary.json `
  -Recorder work/build/audio-capture/20261003T102354462Z/build/Release/producer_loopback.exe `
  -Segment work/acceptance/pattern-crud/20261003T111400850Z/core/pattern-crud/selection.sgp `
  -Profile crud -Node $nodeExe
# 保存済み録音に対する解析器の正例/陰性対照。出力先は未作成の一意ディレクトリを指定する。
& $nodeExe scripts/Test-LoopbackAuditor.mjs `
  work/acceptance/audio-loopback/20261003T111436846Z `
  work/acceptance/audio-auditor-controls/new-unique-directory
```

製品や入力を変更した場合は新runとして録音する。音源を変更する場合はその期待音高・時刻・停止時の減衰条件を先に定め、対応する判定器を追加する。録音失敗/背景音混入は合格にせず原因を記録し、依存しない実装を続ける。物理スピーカーや主観的音色の確認は任意の補足工程として残る。GUI操作、全音源、全40責務/全八受入は未完了。Windows DirectMusic/DirectSound/GM.DLS依存は残る。


## 2026-10-03 現行Variation再生・無人録音とGUI別起動

製品は120854267Z（保存56sources、EXE 9a83ac2d6c1f2dbf62fa11c9e9457ddee1a51d53c086156eda714ef768044e46）を再利用。今回の製品ソース変更/新ビルドはない。構成・compile・installは当該保存build-summaryの各exit0、実行と本体受入は別判定。検証スクリプトと固定入力を追加し、前版の音声を転用せず同じ現行EXEで新規録音した。

Create-PartVariationFixture.mjsは所有Part/Patternを各1個持つ入力の4音をC4/C5へ複製し、membership mask1/0x80000000を指定。候補配列はVariation1だけまたは32だけFFFFFFFF、他を0にする。両Styleはその32DWORD以外の全bytesが同じで、Segment/Band/Pattern/GUIDも同一。独立Inspect-PartVariationPlayback.mjsはraw RIFFを再読取し、この差分・保存56sources・生成物・入力コピー・4実行を照合。work/analysis/part-variation-playback/20261003T122000Z/playback-proof.jsonが合格。API work/acceptance/product-notes/20261003T121912515Z と work/acceptance/product-notes/20261003T121936079Zは各12音、C4=60/C5=72、768clock間隔、duration384、PChannel5、velocity96、通常終了を確認。

新規録音work/acceptance/audio-loopback/20261003T122059286Z と work/acceptance/audio-loopback/20261003T122213065Zは48kHz/2ch/float32、各16秒、player/capture exit0。候補1 RMS0.022930106555072483/C4、候補32 RMS0.028394606261839195/C5、onsetはいずれも3.4秒、前後RMS0、最大packet gap2frames。各生成12音と録音12区間の音程を照合。音色入力はstrings、pianoとは記述しない。録音器102354462Zの保存生成物を使用した。初回録音後の説明scope修正は解析のみ再実行し、旧proofをinitial-proof-before-scope-correction.jsonへ保存、WAV/APIは変更していない。

work/acceptance/audio-auditor-controls/20261003T122500Z-first と work/acceptance/audio-auditor-controls/20261003T122500Z-lastは各4対照が合格。未変更コピーexit0、無音/中間の逆音程/基準区間への背景音混入はexit1。生成API成功を保持したまま録音だけの失敗を拒否する。派生コピーであり、新録音数には含めない。

GUI work/acceptance/product-project-gui/20261003T122419895Zは初回GUI PID4244の保存Styleを別PID5392で読込み、Variation1=127、Variation32=2435007847(0x91234567)を確認。Save Document後も全bytes一致、Style SHA c1f47f8805eaa9f3381e5870a0cf3ff48759766423267af6e46cd07480dd603d、通常終了exit0。45modules原版40hash一致0。variation-gui-reload-proof.jsonに画像/入力/前回proof/module/生成物を結合。GUI Playの音声試験とは別。

残る依存はWindows DirectMusic/DirectSound/GM.DLS。原版動的Variation/Clipboard比較、一般Variation組合せ、GUI Play、物理スピーカー、Motif、空StyleのBand生成、JAZP、全40責務/全八受入は未完了。原版の対象ウィンドウ未公開/registry警告という既存障害は保持し、起動を同条件で繰り返していない。次はMotifの所有文書/編集/保存復元/生成音と、空Style Bandの成立を順に進める。

再現：Test-PlaybackNotes.ps1へ現行summaryとfixtureのfirst/lastのselection.sgpおよびInputPaths Heartlnd.stpを指定。Test-LoopbackAudio.ps1へ同じsummary、RecorderBuildSummaryPath work/build/audio-capture/20261003T102354462Z/build-summary.json、Recorder work/build/audio-capture/20261003T102354462Z/build/Release/producer_loopback.exe、Segmentを各fixture、InputPathsを対応Style、Profile variation-first/variation-last、Nodeを環境のNode実パスへ指定。Test-LoopbackAuditor.mjs <audio run dir> <未作成の一意dir>で対照を作る。Inspect-PartVariationPlayback.mjs <fixture.json> <first API run.json> <last API run.json> <first audio run.json> <last audio run.json>で独立照合。GUI再起動はTest-ProductProjectGui.ps1とInspect-PartVariationGuiReload.mjs <reload dir> <first GUI dir>。work/acceptance/part-variation-playback/20261003T123500Z/unit-record.jsonは記録時刻・最終scripts/docsの保存コピーと入力/生成物/WAV/証拠を結合する。


## 2026-10-04 custom DLSの基準音に対応したMotif無人録音

全体未完了。前回155800Zは具体的な割当実装/対象28/録音失敗の証拠を残した進捗turn。その記録から再開し、単独/文脈Motifを観測する本体CLI note_observeへsource-collection-N.dls/runtime-collection-N.dlsの実所有snapshot出力を追加した。再生に用いたConductor snapshotをコピーするだけで音声データ/patchを書き換えない。集合数が一致しない場合は明示エラー。

Inspect-DlsSamplePitch.mjsを追加。独立raw RIFFで固定一Region/cue0/mono PCM16音源のWSMP Region優先・Wave継承、unityNote/fineTune、sample rate、PCM主成分を解析。前回音源はsample rate44601、81438frames、Region keys72..111、unityNote85、fineTune0、PCM主成分552.5Hz。MIDI72では552.5*2^((72-85)/12)=260.74527887831783Hzとなる。MIDI72という生成値だけから523.25Hzを期待するGM用判定はcustom DLSには適用できない。保存済み音源に基づく独立期待値であり録音の観測周波数を期待値として使わない。全音源の一般的な音程認識/articulation/非zero tuning/多Regionは今回の検査範囲外、未対応を合格へ丸めない。

Inspect-MotifDlsAudio.mjsとTest-LoopbackAudio.ps1 -DlsAudioを追加。Profile motif-standaloneのみ、無音controlとの併用不可。従来GM検査の結果はaudio-gm-assumption-proof.jsonへ保持し、音源対応の結果を別audio-dls-proof.jsonへ出す。source/runtime DLS全bytes一致、保存WSMPから算出した期待周波数の各6window成分と誤octave比、API、capture/player exit0、packet integrity/timestamp errors0、前後無音/RMS/peak/準備期限を検査する。GM検査のfalseをtrueへ書き換えない。

work/build/product-snapshot/20261003T160017103Z/build-summary.jsonは保存59sources、構成0/compile0/install0、EXE b725e52a0259c41abb45c28a29626c76566a90ed10392d950c639a18d81fe48a、core cf25d6d44b43a10e0a956e2114f30583a787cf660dcb6a2e991c8ca4eae9b167。work/acceptance/motif-dls/20261003T160256075Z/run.json対象28 exit0と独立raw割当監査passed。同版nativeは新GUIDでStyleを生成するため、今回録音した155334348Z入力と全bytes一致とは主張しない。録音は前回入力Heartlnd.stp hash352c78eae8217666560cee5443c0c00a4d0468f5e5c91530d4738ee91dc28d7a/owned.dls hash605021db6e944a38a17093624e51b92e86426973df6640978762b077df353011を現行EXEで新しく再生した。

work/acceptance/audio-loopback/20261003T160300614Z新規WASAPI録音は16秒/48kHz/2ch/float32、既存ソース製録音器102354462Z。capture/player exit0、MIDI72x6/768clock間隔/duration384/PChannel5/group1/vel96、DLS Register/Load/Get assigned instrument/Get owned Motif、自然終了成功。Segmentなし。入力Style/sourceStyle全bytes一致、sourceDLS/runtimeDLS/入力owned.dls全一致。onset4.4秒、active RMS0.011289944275575172、baseline/tail0、peak0.04311054199934006、最大gap2frames。音源対応判定passed、6window期待周波数成分約0.0058/誤octave比0.006..0.009。汎用GM C5判定は引き続きfalseの別結果。人の聴取/物理スピーカー確認は未実行、無人デジタル出力の合格。初回音源対応proofはcontrols用notes/onset/baseline metadata追加前としてaudio-dls-proof-before-controls-metadata.jsonへ保持。同WAV再解析でcapture/player/timestamp明示検査を追加、録音は再実行していない。

Test-MotifDlsAudioAuditor.mjsはwork/acceptance/audio-auditor-controls/20261003T160500Z-motif-dlsで未変更copy合格、全無音/中間window誤octave/開始前背景音をそれぞれ拒否。API6音はすべてpassedのまま。派生WAV対照で新製品録音ではない。work/acceptance/product-host/20261003T160313413Z/run.json host exit0、host/録音module provenance各passed、原版40hash一致0の点観測。Windows DirectMusic/DirectSound/GM.DLSは残る。GUI新割当/GUI音声/原版同等性/全40責務/全八受入は未完了。

再現：Build-ProductSnapshot.ps1、Test-MotifDls.ps1 -BuildSummaryPath <同summary> -Dls <既存source.dls>。固定155334348Z/core/Heartlnd.stpと同dir/owned.dlsを保持して、Test-LoopbackAudio.ps1へ同summary/recorder102354462Z+summary/-Profile motif-standalone/-MotifName 'Authored Motif'/-DlsAudio/-Node <実Nodepath>。Node scripts/Test-MotifDlsAudioAuditor.mjs <録音dir> <新control dir>。対象nativeのInspect-MotifDls.mjs、Test-ProductHost.ps1/Inspect-ProductModules.ps1。repo cwdの通常承認済みWindows環境で実行。前回失敗155410446Zを削除/再利用せず保持。

計画順序は音源による基準音の違いを仕様化してから比較するよう具体化。次は現行Motif DLS割当GUIで所有collection選択/楽器選択/UndoRedo/保存/別起動復元を実操作する。次に原版defaults/指定時刻/secondary/tempo、Clipboard/JAZP/全40責務/全八受入を継続。全体条件を縮小しない。凍結記録：work/analysis/motif-dls-audio/20261003T160600Z/unit-record.json。

# 2026-10-04 単独MotifのStyleテンポと無人録音

全体未完了。boundary-runtime193000Zまでの記録/計画と現行実装を確認し再開。既存変更を保持。全40責務と全八受入は維持。ユーザーが既知警告のOKを閉じた回答は受領済み。以前のOS起動拒否や古いプロセスの再試行はしない。音確認に人の在席は求めない。

新入力work/analysis/motif-tempo/fixture-20261003T192800Zは155334348Z有限MIDI72x6/PChannel5/duration384/768clock間隔・所有DLSからStyle.styhのテンポだけを120→180へ変更。元Style/所有DLSと新Styleのhashを保持。変更前work/build/product-snapshot/20261003T191634842Z EXE ed89f106cb2032831bf7d20eb17561df10dc7e25b82c6d9cdb75070fe1577b0dで新規16秒WASAPI録音work/acceptance/audio-loopback/20261003T192741196Z。native/capture exit0・6音API成功・packet integrity/前後無音成功なのに、独立raw Style値180/期待間隔1/3秒に対して実間隔は全5区間0.5秒。tempoPassed=false、driver exit1として失敗保存。Style.GetTempoの読取りだけでは演奏時計へ反映していなかった。ここで使用した録音と旧候補の成功を新候補へ転用しない。

Conductorのstandalone primary Motifで、生成SegmentのTempoParamを問い合わせ、DMUS_E_TRACK_NOT_FOUNDの時は公開OS TempoTrackのIPersistStreamを生成する。既存tempo::Trackから時刻0/sourceStyle.tempoのtetr bytesを生成してIStreamへ書込、Load/QI/Segment.InsertTrack(group1)。一時Persist/Stream/Track参照はRAII解放し、Segmentが挿入trackを保持する。既存TempoTrackがある場合はSetParamを使い重複追加しない（この分岐は今回入力では未実行）。SegmentのTempoParamが保存Styleと一致することをDownload前に確認する。standalone secondaryはこの挿入/上書きを行わず共通Performance時計を使う。context DMSG経路へ変更は加えていない。master tempoや原版COM fallbackは使わない。主/副で異なるテンポ、取消/置換時のテンポ移行、context Motif、原版Producer同等性は別途未確認。

main note_observeのplayback-request.jsonへ実defaultResolution/actualStart/standalonePrimaryTempoを出力。同単独経路をGUIのPlayも使用するが、今回GUI操作は未実行。保存文書/参照DLS bytesは再生による改変なし。音声監査Inspect-MotifTempoAudioは保存Styleを独立raw decodeし、期待間隔60/sourceTempoと録音10ms RMS窓の立上り間隔を±25msで照合する。今回固定有限6音だけの契約で、一般音源認識ではない。既存pitch監査と機能を混同しない。hashで保存sources/summary/EXE/driver/recorder/入力/API/WAV/packetsを接続し、API成功だけでは合格にしない。

最終work/build/product-snapshot/20261003T193013633Z/build-summary.json保存62sources、構成/compile/install各0、EXE 1cd7d918f4c4340609471ddbbd086fd4eeb4e80a3c7796ac2f13a88557713c88、core 654a5d7e83af0141deb4ad0affc2c570ed357545c6e333a628e1365cb4fd52bf。build.log warning/errorコードなし。work/acceptance/audio-loopback/20261003T193144680Zは別の新規録音、native/capture exit0、実Segment tempo180、実開始1612、6音自然終了、録音onsets 4.37/4.71/5.04/5.37/5.71/6.04秒、間隔 0.34/0.33/0.33/0.34/0.33秒で全五区間合格。baseline/tail0、packet gap最大2frames、timestampErrors0。音の高さ/音色認識や物理speakerは今回契約外。

work/analysis/motif-tempo/controls-20261003T193400Z/negative-tests.jsonは新WAVの派生5対照、unchanged合格、全無音/変更前120相当WAV/一音欠落/前背景音の四件をすべて拒否。native6音の成功はすべて保持。原録音と対照を区別し、対照copyの古いproofは消して必ず新しい判定を要求する。これらは別の製品再生録音ではない。

同最終版work/acceptance/boundary-runtime/20261003T193253933Zは実resolutionと主基準のImmediate/Grid/Beat/Measure/Stored五件・独立raw入力監査passed。work/acceptance/motif-concurrent/20261003T193300772Zは両Playing、invalid入力保持、副Stop後主継続/再開、主Stop後副継続、全解放と通知IDの独立監査passed（同時録音ではない）。work/acceptance/product-host/20261003T193300569Zは起動/正常終了exit0。各ロード由来監査passed、原版40hash一致0は取得した時点/経路だけの観測。Windows DirectMusic/DirectSound/GM.DLS依存は残る。全core suite/現行GUI/主副異テンポ録音/原版比較/Clipboard/JAZP/全40/全八は未完了。旧候補のcoreやGUI成功は現行へ転用しない。試験Producerプロセスは正常終了、残存なしを確認。

再現: Build-ProductSnapshot.ps1。Test-LoopbackAudio.ps1 -BuildSummaryPath <同summary> -RecorderBuildSummaryPath work/build/audio-capture/20261003T102354462Z/build-summary.json -Recorder <同summary.executable> -Segment work/analysis/motif-tempo/fixture-20261003T192800Z/Heartlnd.stp -Profile motif-standalone -MotifName 'Authored Motif' -MotifTempoAudio -Node <Nodepath>。Node Test-MotifTempoAudioAuditor.mjs <正例run> work/acceptance/audio-loopback/20261003T192741196Z <新control dir>。Test-BoundaryRuntime/Inspect-BoundaryRuntime/Inspect-ProductModulesとTest-MotifConcurrent/Inspect-MotifConcurrent、Test-ProductHostを同summaryへ実行。保存source/出力/inputを一致させ、音再生を並行しない。

順序の具体化: 無人音声は保存Styleのtempo・所有音源に基づく期待値を先に定義して検査する。次はGUIでStyleテンポ編集→保存→別起動復元→同版録音の一巡を進める。次に主180/副120等の異テンポで共有時計/個別停止、原版ProducerのMotifテンポ動作を比較する。長期診断/通知queue制約、Clipboard/JAZP、全40責務・全八受入は残作業として継続。

# 2026-10-04 GUIテンポ編集・別起動復元・直接無人録音

全体未完了。motif-tempo193600Zと現行計画/実装を確認して再開。既存変更と凍結証拠を保持。全40責務・全八受入の条件を縮小しない。Computer Use skillで現行本体を操作し、ソース製WASAPI録音器でGUI Playを直接録音。人の在席/聴取確認は不要。

製品ソースは今回変更なし。現行work/build/product-snapshot/20261003T193013633Z保存62sources/EXE 1cd7d918f4c4340609471ddbbd086fd4eeb4e80a3c7796ac2f13a88557713c88をそのまま使用し、現在workspace/snapshot両hash一致を確認。構成/compile/install0は同じ生成物の193600Z記録、今回再ビルドはしていない。新Capture-GuiMotifAudio.ps1はlive GUI PID/EXE/snapshot62sources/録音器2sources/入力を照合して32秒default-render endpoint loopbackを開始する。Inspect-MotifTempoGui/Inspect-GuiMotifTempoAudioと派生陰性対照driverを追加。

保存fixture work/analysis/motif-tempo-gui/fixture-20261003T193900Zは有限6音入力155334348Zから複製。initial.stpは120 BPM、expected.stpはstyhのdoubleだけ180へ変更し他全bytes一致。owned.dls/project.dmpj元hashは保持。first work/acceptance/product-project-gui/20261003T193916396Z PID18260で120→180 Change、Undo120/Redo180、Save Document。Heartlnd.stp全bytesがexpected180と一致し終了0。second work/acceptance/product-project-gui/20261003T194328473Z PID7068でproject.dmpjを再起動、180復元、Save Document As resaved.stp全bytes期待値一致。参照catalogが変更済のため終了時DiscardダイアログではNoを選び、Save Project As resaved-project.dmpjへ保存。raw project file参照がresaved.stp/owned.dlsであることを独立解析した。project.dmpjは元のまま保持。

GUIの即時UIA treeが旧値を返す場面は後続fresh状態で検査。changed180初回treeの120は成功証拠として採用せずsettled180を使用。保存filename elementが利用不可/検索focusと返る場面はfresh screenshotのfilename caretを確認して入力し、画面/保存bytesで確認。Pattern Play Selected Motifの実modalをlist_windowsから別windowとして取得しSaved boundary/準備後指定/secondary unchecked/delay0を観測。Scheduled後、短い有限再生のStopped(segment ended)を取得。Playing中の画面は今回採取できていない。

work/acceptance/product-project-gui/20261003T194328473Z/audio-20261003T195026704ZはPID7068のGUI Playだけを新規録音。capture exit0/32秒48kHz stereo float32。独立raw saved Style180→期待間隔1/3秒、6 onsets 10.46/10.79/11.12/11.46/11.79/12.12秒、五間隔 0.33/0.33/0.34/0.33/0.33秒（±25ms契約）合格。baseline/tail RMS0、peakRMS 0.030014843092200278、timestampErrors0、max packet gap2frames。ready→GUI action timestampとsource/EXE/driver/recorder/WAV/packet/UIA/screenshots hashを結合。GUIプロセスのCLI note traceは存在せずAPI音程属性の新成功を主張しない。endpoint録音はsystem-wideなので物理speakerや一般音色/音程認識、厳密な開始QPC同期を証明しない。

work/analysis/motif-tempo-gui/controls-20261003T200100Zは同じGUI録音から派生した5対照。unchangedのみ合格、silence/tempo120相当（6音の立上りを0.5秒間隔へ移動）/一音欠落/前背景音の4件を拒否。32秒PCM形式と元GUI記録は維持し、誤テンポは6音のままtempoPassed false。別の製品録音ではない。古いproofを消して今回解析のfresh proofを必須とした。

由来点監査first45/second127modules passed、原版40hash一致0。Windows DirectMusic/DirectSound/GM.DLS依存は残り、全40責務の置換完了は未主張。first exit0、second launcherは15分でtimedOut true/still-running/exit nullのまま終了。後に保存後GUI closeでwindow一覧から消え、read-only process checkもPID7068なしだが、OS exit codeは回収できない。shell driver自身のexit0をProducer exit0へ転用しない。second正常終了コードは未確認として保持。強制終了/OS拒否迂回なし。

再現: 同一summaryでTest-ProductProjectGui -Project fixture/project.dmpj -AdditionalInputs fixture/Heartlnd.stp,fixture/owned.dls。Computer Useで120→180 Change/Undo/Redo/Save、Capture-ProductGuiModules、通常終了。同じEXEの別起動で180復元/SaveAs。Capture-GuiMotifAudio -GuiRun <second> -Style <resaved.stp> -Dls <owned.dls>、ready後GUI Play、有限終了、module capture、project参照を保存、終了。Inspect-MotifTempoGui.mjs <first> <second> <fixture> <same-build host run>; Inspect-ProductGuiModules各dir; Inspect-GuiMotifTempoAudio.mjs <audio>; Test-GuiMotifTempoAudioAuditor.mjs <audio> <new-controls>。GUI起動driverの15分以内に終了すればexit codeを保持できる。今回secondのtimeoutは未確認として記録。

次の具体的な一手: 保存主180/副120の異テンポ入力で共有Performance時計/個別StopをAPI＋新録音で確認し、原版ProducerのMotifテンポ挙動と比較する。GUI second exit codeの別試験、context Motif/既存TempoTrack分岐、長期diagnostic/notification queue、Clipboard/JAZP、全40責務・全八受入は残る。今回有限GUI経路の合格を全体受入へ転用しない。

# 2026-10-04 主180/副120共有時計・新録音の不合格を保持

全体未完了。直前motif-tempo-gui200300ZはGUI/無人録音と198証拠凍結の進捗あり。最新計画・実装を確認して再開。AGENTS.mdはrg検索に該当なし。既存変更/凍結成果を保持。製品ソース/ビルドは193013633Zのまま、保存62sourcesと現在workspace両hashを照合。構成/compile/installは同生成物の既存記録、今回再ビルドなし。全40責務・全八受入を縮小しない。

新fixture work/analysis/mixed-tempo/fixture-20261003T200600Zは171900Zの所有DLS/2 Styleをコピーし、primary.stpのstyh double120だけ180へ変更。secondaryは120のまま、patch777/MIDI60 PChannel4/MIDI67 PChannel5/repeat15。元sourceFixture・両テンポ・各入力hashをmanifestへ保存。既存同時再生CLIを現行EXEで実行し、work/acceptance/audio-concurrent/20261003T200445724Zへ新24秒WASAPI endpoint録音。player PID10184/capture PID17100ともexit0。両Playing、副Stop後主継続、副再開、主Stop後副継続、全Stop/空所有状態、生成notes primary34/secondary26は実行記録にあり。音声合格は別判定。

既存Inspect-MotifConcurrentAudioは不合格。baseline/全Stop RMS0、timestampErrors0/maxGap2frames/packetIntegrity合格、both/both-restarted/secondary-after-primary-stopの成分検査は満たすが、primary-aloneの期待130.3726Hz最大0.0006054に対し二次期待195.3382Hz最大0.00008395（約13.9%）、primary-after-secondary-stopでも約14.9%となり既存5%不在成分閾値を超える。閾値を緩めて合格へ転用しない。両exit0/API成功から音合格を推測しない。

新Inspect-MixedMotifTempoAudioはraw styh 180/120、既存録音run/WAV hash、二音のfrequency envelopeを60ms窓/10ms間隔で独立解析。both/both-restartedの各音と主Stop後の副音は約0.33/0.34秒間隔。一方主単独区間は0.06/0.27秒、0.08/0.25秒等へ二重に検出され、tempoPassed false。既存pitchPassedも必須なので総合passed false。最低一周期だけの区間もあり、共有時計全体を合格としない。一般音源・物理speaker・原版同等性は未確認。

追加frequency sweep work/acceptance/audio-concurrent/20261003T200445724Z/spectrum-diagnostic.jsonは主単独の最大窓成分約141Hz、副単独約199.5Hzを観測（有限窓/短音の最大であり本来pitchの確定値ではない）。既存sourceDLS dominantからの期待値とスペクトル形状/立上りがずれる原因は未確定。高速短音の過渡成分、音源/ランタイム処理、解析窓の各要因を動的分離していないため、実装bug/解析bugのどちらかと断定しない。

ロード由来点監査passed、原版40hash一致0。Windows DirectMusic/DirectSound/GM.DLS依存は残る。GUIの前回180 BPM有限音成功は保持するが、今回主副異テンポの成功へ転用しない。同じ条件の録音再試行/OS迂回は行わず、同じ保存録音を診断した。

再現: work/create-mixed-tempo-fixture.cjsの新出力先で保存コピー/tempoだけ変更、Test-MotifConcurrentAudio.ps1 -BuildSummaryPath work/build/product-snapshot/20261003T193013633Z/build-summary.json -FixtureDirectory work/analysis/mixed-tempo/fixture-20261003T200600Z -Node <Node>。録音ready後CLI、24秒capture。既存音監査は今回exit1、run/capture/native/WAV保持。その後Inspect-ProductModules -RunPath <run.json> -CaseName audio-concurrent、Inspect-MixedMotifTempoAudio.mjs <run dir>（今回exit1）、work/diagnose-mixed-tempo-spectrum.mjs <run dir>。新auditorは主副180/120の固定fixture契約で、任意曲を判定するものではない。

次の具体的な一手: 同時再生CLIの各phaseでruntime position.tempo/tempoAvailableとmusic clock/QPCを記録し、主180→副120再生/副停止/主停止後もPerformance時計180が維持するかAPIで分離確認する。無人録音側は既知PCMの単音テンプレート/過渡形状を基準にして誤検出を検証する。今回不合格を未解決として保持し、共有時計合格条件を下げない。原版Motif比較、Clipboard/JAZP/全40/全八、前回second GUI exit code未確認も継続。

# 2026-10-04 本体の再生段階ごとの実テンポ・共有時計

全体未完了。直前mixed-tempo200800Zは異テンポ入力/新録音/不合格診断111証拠凍結の進捗あり。最新計画/実装/記録を確認し再開。AGENTS.md該当なし、既存変更を保持。全40責務・全八受入は維持。OS拒否/同条件の単純再試行なし。

製品main.cppのmotif_concurrent_audioでphase記録にpositionsを追加。各phaseのQPC取得後、所有する全PlaybackIdをConductor.position(id)で問い合わせ、playing/start/clocks/tempoAvailable/tempoをJSONへ保存する。既存ID個別Stop/共有Performanceとprimary TempoTrackの経路は変更しない。現在源Styleのtempo値を実ランタイム値の代わりに出力しない。GetParam成功の有無をtempoAvailableで区別する。GetTime/GetStartTime/IsPlayingの既存HRESULT処理を使用し、取得失敗は試験失敗となる。stampはshutdown前後の空所有時もpositions[]を記録する。

新work/build/product-snapshot/20261003T200959993Z/build-summary.json保存62sources、構成/compile/install各0、EXE 2de23fa4ee00a7470f2f5a5c7183b73a6f2b930aa1576c13dc0bf5bb1cf119b9、core 97afed6089cc2edbf9e3ce5edefc11e9885c684614be0852452d20c4ee1b3807、build.log warning/errorコードなし。保存sourcesと現在workspace全hash一致。全core suite/現行GUIは未実行。前版193013633ZのGUI/有限音成功は歴史証拠として保持し新版本体の成功へ転用しない。

同入力work/analysis/mixed-tempo/fixture-20261003T200600Zはprimary180/secondary120、元styh tempoのみ変更、patch777/MIDI60 PChannel4/MIDI67 PChannel5/repeat15の所有DLS。work/acceptance/audio-concurrent/20261003T201137367Zは新EXEでplayer/capture exit0、新24秒endpoint録音。API両Playing/副Stop後主継続/副再開/主Stop後副継続/全Stop/空所有成立。runtime-clock-proofはraw保存styh180/120と入力全bytes/native/保存sources/生成物/driverのhashを結合し、API TempoParamと別の時計傾きを判定する。

実TempoParamのavailableかつPlaying全14samplesが180。primary-ready→secondary-request、both-ready→secondary-stop-request、secondary-stop-return→secondary-restart-request、both-restarted→primary-stop-request、primary-stop-return→all-stop-requestの5約2秒区間で、music clock差とQPC秒×768×180/60を照合。誤差clocks -5.361/7.827/1.485/-19.251/-1.308、最大約19.3clockで事前60clock許容内。最後は主Stop後の副ID3だけで、実tempo180も取得成功し時計差4608が成立。主が止まっても副がsource120へ時計を変えた証拠は今回ない。endpoint差/各phaseだけの測定なので全時点の連続性/厳密同期は未証明。

work/analysis/mixed-tempo/clock-controls-20261003T201300Zはコピーnative JSONの派生3対照、unchanged合格、available tempoを120へ変更/時計差を120相当へ変更の2件を拒否。コピー旧proofを削除しfresh proofを必須とした。別製品実行/録音ではない。

音声は未解決。新WAVの既存音成分解析pitchPassed false、前後無音/packet integrityは合格。新mixed envelopeもtempoPassed false（主の0.06/0.27等二重立上り）。副は0.33/0.34秒が見えるがAPI合格から音合格へ転用しない。今回API evidenceにより共有時計の180維持は実測できたが、音源過渡/周波数計算/窓解析/実際の音のどれが録音不合格の直接原因かは未確定。前録音の不合格も凍結維持。GUI/物理speaker/原版比較未実行。

work/acceptance/product-host/20261003T201135665Zは新EXEのhost smoke exit0、host/audio point inventory原版40hash一致0、各module由来監査passed。Windows DirectMusic/DirectSound/GM.DLSは残る。全40責務の原版依存解消/全八受入は未完了。

再現: Build-ProductSnapshot.ps1 → Test-MotifConcurrentAudio.ps1同summary/fixture（音監査は今回exit1でも実録音/実行証拠保持）→ Inspect-MixedMotifRuntimeClock.mjs <audio dir>（今回exit0）→ Test-MixedMotifRuntimeClockAuditor.mjs <audio dir> <new control dir> → Inspect-MixedMotifTempoAudio（今回exit1）。Inspect-ProductModules -RunPath <audio run> -CaseName audio-concurrent。Test-ProductHost同summary → Inspect-ProductModules -CaseName host-smoke。構成/compile/実行/音声/全体受入を別判定する。

次の具体的な一手: 既知DLS waveの有限短音を基準に、主単独録音の二重立上り/不在周波数漏れの原因をsource PCMと再生PCMのテンプレート比較で検証する。合格に合わせて5%閾値を緩めない。主副異テンポの原版Producer動作比較と文書Clipboard/JAZP統合へ進む。現行GUI/全core suite、前版second GUI exit code、context Motif/既存TempoTrack分岐、長期通知/診断queue、全40/全八も残る。

# 2026-10-04 過渡PCMの波形比較とJAZP実入力観測

全体未完了。前回mixed-tempo201400Zはphase実tempo/music clockの本体実装/新ビルド/実測/214証拠凍結の進捗あり。最新計画/実装/記録を確認して再開。AGENTS.mdなし。既存変更を保持。現行製品work/build/product-snapshot/20261003T200959993Zは今回変更/再ビルド/再実行なし。保存62sourcesと現在workspace全hash一致を確認。構成/compile/install/実行の前回判定は同生成物の履歴として明示し、今回新録音は作っていない。全40責務/全八受入条件を維持。

work/acceptance/audio-concurrent/20261003T201137367Zの前回録音と実入力DLSを再解析。源PCMは44601Hz/81438frames、Region unity85/fineTune0/loop start29198 length23176。冒頭0..20ms RMS0.0035/zero-crossing200Hz、20..40ms RMS0.0164/850Hz、40..80ms RMS0.1079/575Hzに対し100..600msは約551Hz。zero-crossingはpitch精密推定ではなく複雑な過渡波形の存在を示す診断値。従来Inspect-DlsSamplePitchはsource100..600msのdominant552.5Hzを固定測定していた。しかし180 BPM/duration384clockは約166.7msで、MIDI60/unity85ではsource約39.3msしか進まず、loopにも定常測定区間にも達しない。副MIDI67もsource約58.9ms。したがって固定定常成分130.37/195.34Hzを短音全体の唯一の期待値にする前提はこの入力で成立しない。

新Inspect-MixedSourceTemplateは同sourcePCMの冒頭130ms出力相当をMIDI60/67のrate比でlinear interpolationし、録音PCMと比較する。一次だけ、副Stop後一次だけ、一次Stop後副だけの三単独区間を独立解析。gain>.05/かつ<1、RMS>.0003、絶対normalized correlation>=.98で一致候補を抽出し、異なるpitch templateが一致しないことと候補間隔1/3秒±25msを検査。初版0.5ms探索は副2番目の位相を取り逃がし単独副候補1で不合格。閾値は変えず録音sample単位の探索へ修正し、同じ録音を再解析した（再録音ではない）。

結果: 主単独4候補correlation0.987以上から最終0.9984..0.9988、間隔約0.3330/0.33319/0.33337秒。副Stop後主2候補correlation約0.9988、間隔約0.333375秒。主Stop後副2候補correlation約0.99988..0.99991、間隔約0.33327秒。対応する誤pitch templateは全て一致候補0（最大相関主0.438/副0.202でRMS/gain条件も不成立）。source/new PCM hashと保存build/EXE/native/入力を結合。旧5%不在周波数閾値を緩めて合格にしたものではない。源PCMの実際の過渡形状を期待値へ使う、新しい限定検査である。

work/analysis/mixed-tempo/template-controls-20261003T202300Zは同録音PCM派生4対照：unchangedのみpass、全無音/一次一音欠落/一次0.5秒間隔移動を拒否。0.5秒対照も正pitch template複数候補のまま間隔検査で拒否。native API成功を保ち、新fresh proofを要求。別録音ではない。

限界: 二音が重なるboth/both-restarted区間はこの単音template監査では検査していない。従来全音成分判定とenvelopeの不合格記録は凍結保持し、全同時音声受入をpassへ変更しない。一般音源/articulation/非linear処理/物理speaker/GUI/原版同等性は未証明。共有180 clockは前回実測である。今回単独波形一致は再生PCMとsource冒頭の一致を強く支持し、定常前提による判定失敗を示すが、同時二音すべての品質の証明ではない。

音声詳細だけに偏らず次の文書管理へ進むため、実原版QuickStart.proのJAZPをraw観測。work/analysis/product-inputs/20261002T132547062Z/0-QuickStart.proをwork/inspect-jazp.cjsでwalkし、work/analysis/jazp-observed-layout-20261003T202400Z.jsonlへ保存。LIST projにはpjct、UNFO/rdir、pjpn、rfld/fldr path+fltr等、LIST fileにはname、44byte filh、UNFO/rnam+nnam、node/name+edwp等がある。原版metadataはGUID/時刻/UI位置/未知fieldsを含み、単純にDMPJ file chunkをJAZPと名付けて出力できない。現在Frameworkはorig payloadを保持したDMPJ保存のみで.pro exportを拒否する。Pattern clipboardは現ソースにSPC1 copy/pasteとPart GUID再配置が既に存在し、台帳の古い「未実装」を最新未実装と同一視しない。原版clipboard互換/UI比較の残作業は維持。

再現: source work/acceptance/audio-concurrent/20261003T201137367Zを保存したままwork/inspect-mixed-source-pcm.mjs <owned.dls> <new output>、work/diagnose-mixed-template.mjs <run>、Inspect-MixedSourceTemplate.mjs <run>、Test-MixedSourceTemplateAuditor.mjs <run> <new controls>。追加sample探索は解析のみで実行/録音しない。JAZPはinspect-jazp.cjs <saved original pro>。Windows DirectMusic/DirectSound/GM.DLS依存は前回から残り、全40/全八は未完了。

計画の順序具体化: 共有時計の実測と単独過渡PCM検証をここで記録し、同時二音template/原版音声比較は未完了の独立作業として保持。次は本体FrameworkのJAZP保存経路を実原版metadata保存仕様から実装し、参照SaveAs/未知metadata/失敗時atomicityを検証する。既存Clipboardの台帳と実装差も整理する。全体対象/完成条件を縮小しない。


## 2026-10-04 通常StyleのGUIコピー・履歴・終了と音声不合格の分離

最新記録はwork/analysis/normal-style-gui/20261004T030000Z/report.mdとunit-record.json。現行024829130Z保存62sources変更なし。GUI PID10044でStyle root Bandコピー/Save/Undo Save/Redo Saveは現行core全bytes一致、Project/Style/DLS不変。GUI104と再生後128module/address由来passed/原版40hash一致0。File Exitはnative process handleでexit0、強制終了なし。別起動復元は未実行。

無人GUI録音は合格基準を維持して不合格を記録した。031121507Zは22秒曲のStopが自然終了に遅れtiming不合格。別128.6667秒入力（segh length4bytesのみ）031715101Zは両早期Stop/440Hz/120→180BPM/停止無音passedだがPlay前17〜18秒に音が混入しbaseline不合格。原因未特定、同条件再試行なし。CLI同版025034077Zの26秒passedは別範囲で保持。音声APIや保存成功でGUI音声合格を代用しない。

次は別プロセスの同版GUI Project復元/再保存、録音開始UTC/QPC対照とbaseline混入原因の切り分けを実装する。既存原版警告OK回答は確認済みで再要求しない。Windows DirectMusic/DirectSound/GM.DLS/WASAPI依存は保持。残る40責務/全八受入/原版動的比較/ndscを継続し、全体未完了。


## 2026-10-04 通常Style別起動復元と録音時刻対応

work/analysis/normal-style-reload/20261004T033300Z/report.md・unit-record.jsonを最新記録とする。現行製品024829130Zソース62点は変更なし。別PID8628/window32771656で前保存Projectの120→180 BPM/Band0/0/所有DLS bank2 program7/Region loop800を復元、Segment/DLS再保存全bytes一致、GUI104由来passed/原版40hash0。前10044 exit0と別起動を関連付けた。8628は保存済み停止中で保持、今回通常終了は未実行。

録音器032918787Z configure/compile0、開始終了UTC/QPC schema2を追加し、GUI操作を実録音起点へ対応させる。6秒033129067Zと現行GUI停止16秒034521901Zの時刻/packet由来passed、PCM測定RMS0/peak0、4時刻改変拒否。Date.parseのsub-ms欠落はBigInt小数復元で修正し同録音のみ再解析。GUI5phase音声は未実行、前不合格2録音を保持し原因解明/成功を推測しない。Microsoft API資料は本単位reportのリンクに記録。追加録音APIはWindows8以降、製品条件を独断で変えない。

次は保持中8628で別LongNormalGui Projectを開き、新録音器/時刻対応/事前無音確認によるGUI5phase音声を実施。その後articulation/native ndsc/原版比較、全40責務/全八受入を継続する。全体未完了。


## 2026-10-04 通常Style GUI無人音声受入

2026-10-04校正済み通常Style GUI音声：work/analysis/normal-style-gui-audio/20261004T035000Z。現行024829130Z/PID8628、録音器032918787Z、180秒5phase、440Hz・120→180BPM両再生合格（64/72音）、3無音RMS0、自然終了前Stop/再開/finalStop、128module原版40hash0。4派生反例拒否。製品62sources不変、configure/build/installは同版既存0。物理スピーカー/GUI MIDI callback/fullcore/full40/full8未確認。次はDLS articulation/native ndsc/原版比較。 詳細・再現手順は [work/analysis/normal-style-gui-audio/20261004T035000Z/report.md](../../work/analysis/normal-style-gui-audio/20261004T035000Z/report.md)。原版依存・全体対象と八受入条件は維持する。

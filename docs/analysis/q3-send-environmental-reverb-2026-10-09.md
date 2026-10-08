# Send の Standard Environmental Reverb 追加検証（2026-10-09）

全体未完了、fullAcceptance=false。0735限定採用は維持し、1616/1632および後続候補は専用検証として区別する。登録native91/driver164の旧一巡件数を新候補へ転用しない。

## 契約と実装範囲

Producer付属Helpの Insert a Send effect は、同じAudioPathに既定Standard Environmental Reverbが存在する場合にその宛先を初期選択する。既定バッファは一つ、直接PChannel/バスを持たず、既定設定の編集は不可。SDKのGUID_Buffer_EnvReverbとDefined/Mixinフラグをnativeへ保存する。[単位の開始・契約・終了条件](../../work/analysis/q3-send-environmental-reverb/20261008T154611123Z/unit-record.json)。

現在のWindowsで、SDKだけのLoader/CreateAudioPath検証はEnv既定バッファにE_INVALIDARG、明示I3DL2にはREGDB_E_CLASSNOTREGを返した。各条件の失敗を保存し、同一環境のStereo対照のみ成立。Dsound3d.dllが存在しないことを記録し、OSの登録やDLLを変更しない。旧DSPの音声や既定フォーマットは観測できていない。

Sourceのprivate runtime DMOを同一process内で登録し、宣言WindowsのXAudio2_9 reverbへ委譲する。SDK I3DL2 defaultから変換し、WetDryMix100でroom成分だけを出す。nativeのEnv GUID/flags10は変更せず、private runtimeのみ自前stereo混合バッファへ変換する。stereo形式はcloneの明示方針であり、原版から観測した形式ではない。shared寿命、custom parameter/quality、動的パラメーター、旧DSP音声等価、原版比較は残責務。

宣言DSPはXAudio2_9.dllのCreateAudioReverb（PE import ordinal2）。旧診断のWindows.Media.Audio.dll表記は参照資料からの推測であり、[実際のimport照合](../../work/analysis/q3-send-environmental-reverb/20261008T154611123Z/declared-dsp-binding-v1.json)で訂正した。V2_8 factoryとの混同は採用しない。

## 保存候補と失敗の切り分け

1616候補は224保存ソース、configure/build/install各0。関連10試験は9合格、実Send runtimeはPID6376がDSP初期化でc0000005。Initialize/Lock前にSetParametersしたSDK専用processでも同じアクセス違反を再現し、初期化後に処理threadから設定する条件は通常exit0。製品側も順序を訂正した。

1632候補は224保存/作業ソースと4生成物のhash一致。authoring56合格、runtime PID10236は通常exit1で、実DSP処理後のwet-only先頭試験が失敗した。初期化AVの修正とwet出力成立は別判定。失敗ソース・候補・runを保持する。[hash検証](../../work/analysis/q3-send-environmental-reverb/20261008T154611123Z/candidate-1632-hash-verification-v1.json)。

## 初回DSP遷移の修正

独立SDK-onlyの6条件で、native初期化データとin-place/out-of-placeの差にかかわらず、最初の0.5入力が0.499633789の原音として残った。Lock前SetParametersはAV、Lock後は通常終了。Process後に変換済み設定が反映される。これは[IXAPO Process](https://learn.microsoft.com/en-us/windows/win32/api/xapo/nf-xapo-ixapo-process)が要求する有効/through遷移の連続性と整合する観測であり、原版DSPの観測ではない。

最初の実入力の前に、固定最大4096 frameのゼロ事前履歴を処理すると、事前出力は完全ゼロ、後続の実入力先頭はゼロとなった。20k mono、22050 stereo、44100 mono、48k stereo、Reset後を固定条件で確認。実入力を捨てず、外部時刻/frame/latencyを変えず、初期化とReset後だけ処理threadで実施する。生音を混ぜて転送せず、事前出力がゼロでない場合はエラー。[修正契約](../../work/analysis/q3-send-environmental-reverb/20261008T154611123Z/wet-startup-repair-plan-v1.json)。

in-place呼出しは残響を[公開DMO契約](https://learn.microsoft.com/en-us/windows/win32/directshow/in-place-processing)のS_FALSEで知らせ、zero-inputで残響を取り出せるよう訂正する。typed Cloneは独立した処理資源を準備し、原本のtailと共有しない。元のwet-only先頭と有限/nonzero反射のassertは維持する。旧DSPの初回遷移・サンプルレート別時刻等価を合格扱いにしない。

## 未達終了条件

修正後の新候補で関連回帰、実Send/Env生成、wet-only/残響/Flush/clone、独立DSP PCM照合が必要。本体UIのAdd Env./Send既定宛先、native保存、二process通常終了と復元、同候補のWASAPI音声・Stop/再開は未実行。原版approval/Q2独立Windows/Windows5保存の障害は別件として保持する。

## 1659候補とFlush/Stop修正の現状

1659は224保存/作業ソースと4生成物hash一致、configure/build/install各0。関連10は9合格/1失敗、同候補追加Script依存/DLS3は3合格。実DSPのwet-only impulse、Clone、残響S_FALSEが成立したが、exact zero Flush後はcheck70で通常exit1。未完を保持する。製品Source DMOの32768-byte impulse PCMは独立SDK-onlyの同条件PCMとbyte完全一致（SHA a71f5dd5c83dbc3046ef9c325a5b007b44b26183b4a21fa5d74f2ea8fde97621）。全runtime/main/原版比較の合格ではない。

独立Reset検証は残響energyを0.0015395から1.46797e-35へ減らすが、floatのbit exact zeroを保証しない。same-object Unlock/Lock後のSetParametersはAVを保存し採用しない。Source側はReset後に実非ゼロ入力の履歴がない場合、しきい値によらずデジタルゼロを返す。適用済み既定値をFlushごとに再設定せず、入力音を初回遷移に混ぜない。また、保持FileOutput経路ではEnv DMOも取得し、最後の共有音楽sessionのStop時だけFlushする。Wavesの既存戻り値・照合と他sessionへの影響を維持する。

[停止修正契約](../../work/analysis/q3-send-environmental-reverb/20261008T154611123Z/flush-stop-repair-plan-v1.json)と固定13関連試験へ、実global DMO取得/共有Stop/再開を追加。1713保存候補のbuildを実行中。ユーザーの計画メンテナンス依頼に従い、そのbuild/関連回帰を保存したところで中断する。GUI author/reload/本体PCMは次の未達条件として残す。

## ユーザー依頼による中断点

171355889Zは224保存/作業ソースと4生成物hash一致、configure/build/install各0。[関連13試験](../../work/acceptance/regression/20261008T171912676Z/run.json)は13合格/失敗0、全process通常exit0。Send authoring57（PID9052）、runtime89（PID16276）で実global Env DMO取得、共有session保持、最後のStopで一度だけFlush、再開後のStop、FileOutputの継続を確認した。新候補のSource impulse32768 bytesも独立SDK PCMと完全一致。

[最新hash検証](../../work/analysis/q3-send-environmental-reverb/20261008T154611123Z/candidate-1713-hash-verification-v1.json)、[同候補DSP比較](../../work/analysis/q3-send-environmental-reverb/20261008T154611123Z/candidate-1713-dsp-independent-comparison-v1.json)、[再開用本体protocol](../../work/analysis/q3-send-environmental-reverb/20261008T154611123Z/main-preparation-v1/protocol.json)へ結合する。停止後無音は既存Sendの0.0001 RMS、Stop後0.7〜1.7秒を維持して実本体で検証する。初期draftのFarm基準0.0003はGUI/録音前に訂正し旧draftを保持した。

最新限定採用0735は維持。1713は専用/native検証候補で、本体GUI、native保存/別process復元/二通常終了、WASAPI、原版比較は未達。登録native91/driver164の全一巡は未実施で、全8は6作業中/2障害/0合格、fullAcceptance=false。全体・本単位を完了扱いにせず、計画メンテナンスのため中断する。今回のbuild/native/SDK CLIはすべて終了し、新GUI/recorderは起動していない。古い別processは操作していない。

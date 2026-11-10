# Waves Reverb 本体・実音声検証

2026-10-08 JST。候補 `20261007T183144678Z`、216保存ソース、生成物4。既定 Waves の限定経路を確認。原版比較/custom編集/Send全責務/全40/全8は未完、`fullAcceptance=false`。

本体 author PID13156 と restore PID20948 で default追加・Undo/Redo・native AudioPath/Segment/JAZP保存・別process復元・二通常exit0を確認。独立RIFF監査は既定fxhr56、標準CLSID、CTRLFX、保存/復元bytesを確認した。発音終了後の初回パラメーター取得失敗は保持し、PCM側の発音中取得を別に記録。

wet PID4548 と dry PID18296 は同じEXE/hashの別起動。各300秒の48000Hz stereo float WASAPI録音。同じ2route、60BPM、P0 pitch69/P8 pitch60、160秒ノート。wetだけP0 bufferにWavesを追加したread-only native入力を使用し、既知Windows5対象のProject/Band/DLSを保存し直していない。両processは通常exit0。

発音中の Query で Buffer2/Waves1 の Input gain0dB、Reverb mix0dB、time1000ms、HF ratio0.001を取得。両録音のpitch69/60と終盤持続、発音中Stop、停止後無音、再開、終了後無音を固定プロトコルで確認。wet/dryのP69/P60振幅比は0.17102964/0.99825467で、相対差82.8671%。P60の変化0.0201%。natural noteoff後RMSはwet0.01111665/dry0。閾値変更なし。endpoint hashは一致。

独立判定器は正例2・負例7（無音、C60欠落、誤半音、Stop後発音、早いnoteoff、効果差なし、非対象経路変化）を確認。実行wrapperでcontrols PID13800/comparison PID23752は双方exit0。Node・判定器の保存版・入力・生成物・recording packet/clock/PCM・CU操作UTC・終了記録をhashで結んだ。driver3本を追加し登録139。これは183補足実行であり、driver原始一巡は未実行。

一次証拠:

- [native proof](../../work/analysis/q3-waves-reverb/20261007T102349728Z/main-183-20261007T193902968Z/main-native-proof.json) SHA256 `0ea17ead5cd93b6c1221cb9b1a6c836f141ab9e30bbca6d9da7d21b1eaf78e4c`
- [PCM proof](../../work/analysis/q3-waves-reverb/20261007T102349728Z/main-183-20261007T193902968Z/audio-analysis-final/pcm-proof.json) SHA256 `5cbd9cd4cfa8fdc8f00850ea7e34a617367f13a43de153bab48e7648a8aee3b3`
- [actual analyzer run](../../work/analysis/q3-waves-reverb/20261007T102349728Z/main-183-20261007T193902968Z/audio-analysis-final/run.json) SHA256 `1ed319b22aded499a28f114b396fe71ed0f8d7484bd94a96df112a8ff46825fb`
- [fixed protocol](../../work/analysis/q3-waves-reverb/20261007T102349728Z/main-183-20261007T193902968Z/audio-protocol.json) SHA256 `9259bda633ac0106e2c8e869c4507fe6e5037e9c3ad060130f30233aaf7dedb9`
- [adoption receipt](../../work/analysis/q3-waves-reverb/20261007T102349728Z/main-183-20261007T193902968Z/adoption-after-pcm/adoption.json) SHA256 `9dade9186c08de86bfab5099a0ef526aefd71575560f2827d2159f3f40287ed2`

再現解析は `scripts/Test-WavesReverbGuiAudio.ps1 -DryCapture <retained dry audio directory> -WetCapture <retained wet audio directory> -Protocol <audio-protocol.json> -OutputDirectory <fresh directory>`。新録音はComputer Useによる実際の本体操作と同じ固定プロトコルが必要。既存GUI操作をCLI成功へ転用しない。

原版StylePlayer approval timeout・各Windows5・Q2未用意を維持。実施可能な既定Waves不足を限定達成として保存し、Style default-map S_FALSE/NULLの入力/公開契約を調査して製品修正へ進む。custom parameters・Send factory/attenuation/global mix-in/order/external destination・動的tempoの残責務はqueueに保持。

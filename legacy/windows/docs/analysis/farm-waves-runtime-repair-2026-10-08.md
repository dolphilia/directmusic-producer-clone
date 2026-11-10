# Farm/Waves runtime修正と新候補の検証

候補20261007T153229477Zは216保存ソース、configure/build/install各0。通常ホストの承認審査を通った実行でSDK探索が成功した。旧sandbox拒否、各Windows5、原版approval、Q2障害は別々に保持する。全40対象・全8受入は未完、fullAcceptance=false。

[ビルド証拠](../../work/analysis/farm-waves-runtime-repair/20261007T152621164Z/build-proof.json)と[一次回帰run](../../work/acceptance/regression/20261007T153745613Z/run.json)を保存した。選択22件はraw19合格/2失敗/1障害。[OS分類後](../../work/analysis/farm-waves-runtime-repair/20261007T152621164Z/classified-regression.json)は19合格/1製品失敗/2障害。原始runは変更していない。全native87/driver133の一巡は未実行。

埋込DMAPのnative bytesと既存Loader guardsを維持した修正によりScript依存125が合格。Farm runtimeは初期化成功、CougarのPlayで失敗。本体でも完全な19入力で初期化成功後、0x88781215と「An error occurred in a call to Play」を観測した。最初のGUI入力コピー12件にDLS/Wave7件が欠けていた試験上の誤りを保存し、元入力hashに一致する19件へ補完した。入力不足を製品不具合/OS拒否の根拠にはしない。

[GUI診断](../../work/analysis/farm-waves-runtime-repair/20261007T152621164Z/farm-gui-diagnosis)はProducer PID18868、EXE SHA2565fd7e54e4906d065cc570560adf5841f2071c52838b5d9b58f19f61d7cff3d2e。Farm専用runtimeを閉じ、Scriptを含むFarmDiagnosis.dmpjを保存して通常exit0を確認した。入力19件の全bytes不変。別process復元/二正常exit/PCM/原版比較は未達。

Wavesは文書19と実DSP/defaults/native replay/two-buffer/Stopを含むruntime29が合格。GetPrepareTime1000ms、実開始待ち907〜984msで、従来固定200ms確認が早過ぎることを実測した。本体のdefault作成/保存復元/二終了/乾湿PCM/原版比較は未達。Send文書54等も限定合格で、実Send factory/減衰/global mix-in/寿命/PCMは未達。

DLS runtime exportは40check後、LoopSource.dlsのatomic replaceで新Windows error5。PID2964/exit1、旧destinationを保存した。[別障害記録](../../work/analysis/farm-waves-runtime-repair/20261007T152621164Z/dls-runtime-new-os-refusal.json)をmanifestへ接続し、同条件再試行を凍結する。既知RouteBand拒否の解除にはしない。

次は同じ未完Farm単位の[Play/Stopコンテキスト修正範囲](../../work/analysis/farm-waves-runtime-repair/20261007T152621164Z/farm-play-context/scope.json)。所有OS Script経由という仮説を実12routine、PlayingSegment返却、省略引数、エラー伝達で検証する。修正ソースを新候補20261007T160307258Zへ保存してbuild中。完了した新生成物へ旧成功を転用しない。

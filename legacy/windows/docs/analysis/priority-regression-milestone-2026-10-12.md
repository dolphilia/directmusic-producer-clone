# 現在候補151135の登録検証一巡（2026-10-12）

Sequence Properties変更後の同候補でnative100とdriver172を新規一巡した。実装変更なし、289保存/作業ソース・4出力一致、30入力hash不変。全体合格は未達。

|一次run|結果|
|---|---|
|native100|52合格・2966検査、46既知OS凍結、motif1失敗、既知匿名Band1保留。凍結とBandはプロセスを起動していない|
|driver172|19合格、29凍結、124前提未充足。今回のMotif拒否と同じ処理を再実行しないためTest-Motifを追加凍結|
|Motif一次失敗|PID10832/exit1。24検査通過後、新規検証出力Heartlnd.stpの置換保存がWindows error5で失敗。根本原因と新Propertiesとの因果は未判定。入力2ファイル不変|
|登録変更|native実行時46/28凍結から、Motif観測後の将来47native/29driver凍結へ。native一次runの46凍結+1失敗を47凍結に書き換えない。nativeとdriverのmanifest hashが異なる理由は追加凍結|
|driver一次証拠|19 producing runのbuild hashを確認。StartupFailuresはbuildSha256という別名をv2監査で解決。失敗試験を再実行せず形式だけ補正|
|正式原版|保存6ファイルとEXE hash不変、既存ウィンドウ保持。原版保存・終了なし|

[単位](../../work/analysis/priority-regression-milestone/20261011T160018805Z/unit-record-v1.json)、[native原始run](../../work/acceptance/regression/20261011T160035028Z/run.json)と[集計監査](../../work/analysis/priority-regression-milestone/20261011T160018805Z/registered-native-round-v1.json)、[driver原始run](../../work/acceptance/registered-drivers/20261011T160459160Z/run.json)と[集計監査](../../work/analysis/priority-regression-milestone/20261011T160018805Z/registered-driver-round-v1.json)、[driver候補同一性v2](../../work/analysis/priority-regression-milestone/20261011T160018805Z/driver-producing-identity-v2.json)、[Motif保存拒否](../../work/analysis/priority-regression-milestone/20261011T160018805Z/motif-publication-refusal-v1.json)、[構成hash](../../work/analysis/priority-regression-milestone/20261011T160018805Z/closing-identity-v1.json)、[原版保護](../../work/analysis/priority-regression-milestone/20261011T160018805Z/preservation-v1.json)。

再現コマンドは各原始runのdriver.ps1とmanifest.jsonに保存。既知拒否を同条件で再試行しない。Motifは保存先・所有権・Windowsの制限などに根拠ある条件変化が得られたときに再開する。閾値やassertは変更していない。今回全音声・原版互換・外観・全8受入は別判定。限定採用0735、独立Style015244、6作業中/2障害/0合格、fullAcceptance=falseを保持。

次は[Transport Optionsアイコン/配置単位](../../work/analysis/ui-transport-options-icons/20261011T161113801Z/start-v1.json)。画像3と正式原版の順序・幅を基準に、既存4文字ボタンをコードで描いたアイコンに変更する。SSP04親ドラッグ/原版空選択/全SSP01/Q2等の残責務は保持する。

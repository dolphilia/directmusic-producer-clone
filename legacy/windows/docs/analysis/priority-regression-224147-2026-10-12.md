# 候補224147の登録試験一巡 — 2026-10-12

224147同候補一巡：native100＝51合格2990検査/48既知OS凍結/匿名Band1保留。driver172＝18合格/30凍結/124未実行。153入力（7ディレクトリを含む）・18 producing run・291保存/作業ソース・4出力を285検査で照合。新規失敗なし、全登録合格ではない。 全体fullAcceptance=false。

| 原始一巡 | 合格 | 既知障害で未起動 | その他 |
| --- | ---: | ---: | --- |
| native100 | 51（2990検査） | 48 | 匿名Band1検証待ち |
| driver172 | 18 | 30 | 124未実行 |

前回193237のPartVariation新規失敗を含む48native/30driverの凍結を維持し、同条件の再試行は行っていない。新規失敗はなかったが、未起動/前提未達を合格には含めない。native100とdriver172の登録配列は一巡前後で同一。291保存ソースと作業ソース、4生成物、各native実行のexit0/出力検査数、18driverのlogから辿ったproducing runのbuild/実行物hashを照合した。153入力は146ファイルと7ディレクトリで、ディレクトリは既存driverと同じPowerShell Sort-Object FullName順、相対名:hash/LF/UTF8で照合。過去の保護入力6件と正式原版EXEも不変。

DLS空コレクションの修正後候補を初めて一巡した記録であり、旧候補の合格件数は転用していない。音声比較はLA42の専用入力/操作での別証拠で、GUI/audio前提を満たさない124driverの実行済み判定には使わない。限定採用0735、独立Style015244、SSP04親残件、Q2、全40/全8と6作業中/2障害/0合格を維持する。boundedFinishedは一巡の集計と監査の終了だけを表す。

[単位](../../work/analysis/priority-regression-milestone/20261011T230711154Z/unit-record-v1.json)、[285検査](../../work/analysis/priority-regression-milestone/20261011T230711154Z/round-proof-v1.json)、[native原始run](../../work/acceptance/regression/20261011T231059391Z/run.json)、[driver原始run](../../work/acceptance/registered-drivers/20261011T231349806Z/run.json)。

次：SSP06/画像29〜37のPBend・CC11 Expressionの固定PCMで未達だった発音中Stop2条件を、224147・専用同入力コピー・前後時刻記録と絶対時刻待機で確認する。旧14/16失敗と固定閾値を保持し、既存4曲線のnativeを独立監査して保存/通常終了/別復元へ結ぶ。正式原版でも同じコピーの効果/Stopを比較し、差があれば修正する。空lane/PolyAT/厳密形状/全gesture、6640破棄回答待ち、独立Style015244/Q2、48native/30driver凍結、全40/全8を保持。

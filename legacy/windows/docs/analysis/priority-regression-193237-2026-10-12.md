# 候補193237の登録試験一巡 — 2026-10-12

193237同候補一巡：native100＝51合格2973検査/47既知OS凍結/PartVariation1新規Win5失敗/匿名Band1保留。driver172＝18合格/30凍結/124未実行。153入力（7ディレクトリを含む）・18 producing run・291ソース/4出力を監査。将来48native/30driver凍結。全試験合格ではない。 全体fullAcceptance=false。

| 原始一巡 | 合格 | 既知障害で未起動 | 新規失敗 | その他 |
| --- | ---: | ---: | ---: | --- |
| native100 | 51（2973検査） | 47 | PartVariation1 | 匿名Band1検証待ち |
| driver172 | 18 | 30 | 0 | 124未実行 |

PartVariationはPID22952・16検査後にHeartlnd.stpの原子置換でWindows error5、exit1。元の2入力はhash不変。根本原因とQuantize修正との因果は未確認。同条件再試行を止め、native原始記録の「失敗」を保持し、後続driverおよび将来nativeを凍結。既存Motif等と合算して将来48native/30driver凍結。安全設定やloaderを変えて迂回しない。解除には出版/所有権等の条件変更の証拠または根拠のある製品修正が必要。

18 driverのlogからproducing runを辿り、同じbuild-summaryと各実行物を照合した。StartupFailuresのbuildSha256別schemaとHostのcase別EXE hashも確認した。153入力は146ファイルと7ディレクトリで、ディレクトリは既存driverと同じ相対名:hash/改行/UTF8算法で監査した。初回監査はディレクトリをファイルとして読んでEISDIRとなったため保存し、入力の種類を正しく扱うv2で284検査が成立。試験自体を再実行したものではない。

native一巡には共通open53・timeline117・Sequence216・audition30の関連416検査を含む。291保存/作業ソースと4出力hash一致、過去保護入力6件と正式原版EXE不変。通常core等の障害、GUI/audio前提未達、Q2、Style独立単位、全40/全8を合格へ変更していない。boundedFinishedは一巡の会計と監査終了だけで、completed/allTestsPassed/fullAcceptance=false。

[単位](../../work/analysis/priority-regression-milestone/20261011T194000Z/unit-record-v1.json)、[一巡監査](../../work/analysis/priority-regression-milestone/20261011T194000Z/round-proof-v2.json)、[新規拒否](../../work/analysis/priority-regression-milestone/20261011T194000Z/part-variation-publication-refusal-v1.json)、[native原始run](../../work/acceptance/regression/20261011T193959557Z/run.json)、[driver原始run](../../work/acceptance/registered-drivers/20261011T194316342Z/run.json)。

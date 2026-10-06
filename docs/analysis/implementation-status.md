# 本体再構築の現在状態

候補 20261006T185539543Z。全体未完了。正本は [product-state.json](product-state.json) の current。旧schema1フィールドと従来CSVは版別履歴として保持する。

構成 {"status":"合格","candidate":"20261006T185539543Z","exitCode":0,"evidence":"work/build/product-snapshot/20261006T185539543Z/build-summary.json","scope":"Win32 Release reference tools OFF,195 saved sources"}、compile {"status":"合格","candidate":"20261006T185539543Z","exitCode":0,"evidence":"work/build/product-snapshot/20261006T185539543Z/build-summary.json"}、install {"status":"合格","candidate":"20261006T185539543Z","exitCode":0,"evidence":"work/build/product-snapshot/20261006T185539543Z/build-summary.json"}、通常core {"status":"障害あり","reason":"Known Windows5 publication condition frozen; not rerun or included as pass","candidate":"20261006T185539543Z","evidence":"work/acceptance/regression/20261006T190050749Z/run.json","scope":"Frozen dedicated ordinary core Windows5 publication rejection. DWORD PChannel contract/Sequence26 current pass; no rejected retry."}。現候補の専用native: 障害あり 30、合格 47。driver: 未実行 66、障害あり 21、合格 20。

最新単位: work/analysis/q3-timeline-move/20261006T185000Z/unit-record.json。build: work/build/product-snapshot/20261006T185539543Z/build-summary.json。機能責務の入口 [feature-map.csv](feature-map.csv) は全40行を維持し、古い「未着手」を現在の未実装判定へ転用しない。

| 全体受入 | 状態 | 残差 |
| --- | --- | --- |
| クリーンビルド | 作業中 | Shared Tempo/Sequence half-open selected range atomic move, button and actual drag; whole40 not accepted |
| 起動と終了 | 作業中 | Shared Tempo/Sequence half-open selected range atomic move, button and actual drag; whole40 not accepted |
| 原版データの読込 | 作業中 | Shared Tempo/Sequence half-open selected range atomic move, button and actual drag; whole40 not accepted |
| 編集と保存 | 作業中 | Shared Tempo/Sequence half-open selected range atomic move, button and actual drag; whole40 not accepted |
| 終了後の再読込 | 作業中 | Shared Tempo/Sequence half-open selected range atomic move, button and actual drag; whole40 not accepted |
| 再生と停止 | 作業中 | Current candidate audio未実行; old Wave/Fresh audio retained only as history |
| 繰り返しと異常入力 | 障害あり | Shared Tempo/Sequence half-open selected range atomic move, button and actual drag; whole40 not accepted |
| 原版依存の解消 | 障害あり | Shared Tempo/Sequence half-open selected range atomic move, button and actual drag; whole40 not accepted |

次の作業: Q1: fixed185539543Z new native Project five forms → edit Undo/Redo save exit0 → distinct reload → WASAPI active Stop/replay/tempo, Style and embedded Segment versus Transport priority; no old audio transfer

旧implementation-status.csvは現行の完成率やqueueへ使用しない。専用試験と全8受入の正本は [regression-manifest.json](regression-manifest.json)、[acceptance-status.json](acceptance-status.json)。

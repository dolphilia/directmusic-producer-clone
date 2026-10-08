# Style default-map の限定修正と本体検証

候補 `20261007T210136925Z`、216保存/作業ソースと4生成物一致。全体は未完、`fullAcceptance=false`。この記録は名前の欠けた既定ChordMap参照を所有mapのidentityで補う修正の終了条件を扱う。

既定mapから生成したQuiet32小節、120 BPMのSegmentを本体Author PID14772でnative保存し、通常exit0。別PID2944でnative Projectを復元し、Style Playerの公開「Style default ChordMap」選択による再生成と、保存済みComposed.sgpの選択を確認した。PID2944も通常exit0。独立GUI監査はNode PID2292、通常exit0。

Computer Useの過去の入力障害は保存した。観測した「いいえ」のAlt+nで確認を取消し、起動時のSegmentとProjectをDefault外の別名へ保存してから復元した。破棄の承認や未回答の人手操作は成功の根拠にしていない。

WASAPIを固定した300秒プロトコルで録音した。最初の録音は再開から最終停止まで163.588秒となり、規定の62秒未満を超えたため不合格として保持。Node PID17288の独立解析も正常exit1、timingPassed=falseだった。

再測定では最初の再生28.750秒、再開39.631秒で発音中に停止。Node PID20708の解析は通常exit0。55回/77回の発音、120 BPMの0.5秒間隔、所有DLSの440 Hz、baseline/停止保持/最終停止後の無音、packetと時計の整合が合格。閾値を変更していない。デジタルendpointの観測であり、物理スピーカーは検証していない。

判定器の正例1と反例4（無音、停止保持中の雑音、誤テンポ、時計の段差）はNode PID20068、通常exit0。派生録音の対照であり、追加の製品実行ではない。元の録音と6保存入力のhashを保持した。

再生中PID2944の129モジュールをbase address付きで観測し、WOW64の物理ファイルを照合。原版40hash一致は0、未解決由来は0。これは一点の観測で、全責務の依存解消やQ2の証明にはしない。

一次証拠は[終了条件の照合](../../work/analysis/q3-style-player-default/20261007T071213536Z/resume-20261007T205401175Z/adoption-after-style-pcm-20261008T0005/proof.json)、[GUI監査](../../work/analysis/q3-style-player-default/20261007T071213536Z/resume-20261007T205401175Z/gui-audit-20261008T0001/proof.json)、[PCM監査](../../work/analysis/q3-style-player-default/20261007T071213536Z/resume-20261007T205401175Z/main210-20261007T210836225Z/restore/audio-20261007T235751022Z/style-player-gui-audio-proof.json)、[判定器対照](../../work/analysis/q3-style-player-default/20261007T071213536Z/resume-20261007T205401175Z/second-audio-controls-20261008T0003/controls.json)。各実行run、失敗録音、通常終了、モジュール監査と更新前ファイルは終了条件の照合から辿れる。

登録native87の原始一巡51合格/36既知障害と、driver139の原始一巡25合格/22既知障害/92前提不足は変更しない。追加したStyle判定器5件は同候補補足としてすべて実行済み。全体8受入は6作業中/2障害/0合格のまま。

Legacy AASY/PER、複数Band、同入力の原版動的比較、Q2と全40/全8は未完。原版approval timeoutと個別Windows5は別々の障害として維持する。次は別保存218ソースのSend修正を採用し、新候補で登録回帰、本体native保存/別PID復元/二通常終了/WASAPIを進める。旧候補の実行結果を新生成物へ転用しない。

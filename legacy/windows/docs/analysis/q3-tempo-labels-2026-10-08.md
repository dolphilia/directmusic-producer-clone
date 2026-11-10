# Tempo label描画の限定検証

候補20261007T133337527Z、216保存ソース、Win32 Release/reference-tools OFF。configure/build/install各0。Producer SHA-256 27079fdef85ec5221b5f8869e84621e598b7fac36658152c8a663cbd6dec3ee0。

clocks0/3072の12BPMを本体で読み、二つのlabelが分離することを確認した。作者PID20616と別process復元PID24856はそれぞれnative保存後に通常終了し、実processの終了コード0を取得した。監視scriptの終了コードとは区別する。

独立RIFF監査でSegment/Band/DLSは入力・保存・復元後の全bytes一致。Projectの差はfilh内FILETIMEだけであり、正規化範囲以外の全bytesと3文書参照は一致。Timeline93/Sequence26は同候補で合格。描画だけの修正なので、この単位ではfresh PCMを非適用とし、Q1の現候補音声は別途未実行。

一次証拠：[全体proof](../../work/analysis/q3-tempo-labels/20261007T084216531Z/runtime-20261007T142500Z/tempo-round-proof.json)、[独立判定器](../../work/analysis/q3-tempo-labels/20261007T084216531Z/runtime-20261007T142500Z/Inspect-TempoRound.mjs)、[22関連run](../../work/acceptance/regression/20261007T135728070Z/run.json)、[単位記録](../../work/analysis/q3-tempo-labels/20261007T084216531Z/unit-record.json)。旧監視失敗・保存試行は保全し、終了コード証拠に含めない。全22件は18合格/3失敗/1既知障害で、全登録群の合格ではない。

次はFarm埋込AudioPath resolver失敗とWaves開始状態失敗を同じ未完単位で修正する。Send factory、原版比較、独立Q2、全40責務、全8受入は未完。fullAcceptance=false。

# SSP01 Metronome 本体統合と初回発音遅延の限定修正

最新試験候補は `20261011T022456348Z`。[単位記録v3](../../work/analysis/ui-transport-metronome/20261011T005908347Z/unit-record-v3.json)（SHA256 caf286e2279ffa8f994cab46bb70e49f151455e4fd1931ed88618a1b041d0597）へ、284保存・作業ソース/4出力一致、build/install、関連5モード275検査、同候補の登録native99/driver170一巡、本体保存・別復元・二通常終了、固定PCMを結び付けた。限定採用0735、SSP08候補220927、独立Style015244は別判定。単位未完、全体6作業中・2障害・0合格、`fullAcceptance=false`。

Transport OptionsのMetronomeは左クリックで切替、右クリックでOptionsを開く。強拍/他拍の音高・velocity、1-based PChannel、Count-In Bars、Record Only/All Playback、OK/Cancelを本体に接続済み。設定は文書外の専用sidecarに保存する。primary Segmentの所有AudioPathとPerformance時計にprivate Track/Toolを接続し、先読みしたクリックも発音時の最新設定を参照する。Stopで発音を止め、文書へクリックトラックを書き込まない。

旧012509の初回PCM onset1.245秒を、予定時刻・Tool到達・実PCMに分けて調査した。新たな任意のruntime traceはメモリー内4096件に制限し、通常終了時だけ指定先へ書く。最初の診断は定期pollも記録してoverflowしたため、その失敗を保持し、反復pollの記録を外した。原版への入力や設定変更はこの修正中には行っていない。

prepare-timeをInitAudio前に300msへ設定する最初の修正は、実Performanceの初回800ms条件で失敗した。[失敗記録](../../work/analysis/ui-transport-metronome/20261011T005908347Z/failed-prepare-round-v1.json)を保持する。再診断ではqueueが約94ms先だった一方、AFTERPREPARETIMEの解決開始が約983ms先になった。古い1秒queueが原因という仮説は支持されなかった。失敗した前倒し変更を外し、即時・responsive・private primary・delayなしの場合だけ、現在から300ms後かつqueueから50ms以上後の明示開始時計を渡す。boundary/delay/shared/secondary/複雑なScript・ToolGraph・Motif経路の既存条件は保持する。

[遅延修正証拠](../../work/analysis/ui-transport-metronome/20261011T005908347Z/onset-repair-proof-v2.json)では、native実時計の初回0.359秒/再開0.336秒、本体traceの初回0.354秒/再開0.333秒を確認した。本体の要求開始時計と解決開始時計は両方一致し、1067件のtraceはoverflowなし。固定PCMの初回0.582秒/再開0.585秒は事前の1秒以内に収まる。これは即時private primary経路での限定修正であり、Count-Inや全再生経路の成立ではない。

関連5モード275検査は同候補の一巡結果から抽出した。登録native99は52合格・2940検査/46既知OS凍結/既知Band1保留。登録driver170は20合格/28既知OS凍結/122前提未充足の未実行。凍結対象は再起動せず、既知Bandを合格扱いにしない。新しいmetronome-runtimeの12検査は、実Track/Toolの強拍、768clocks間隔、live toggle/設定・zero velocity、Stop/restart、文書bytes/履歴の保持を確認する。PCMと原版比較は別判定。

[本体保存・復元証拠](../../work/analysis/ui-transport-metronome/20261011T005908347Z/repair2-main/native-save-reload-proof-v2.json)では、Save Projectによる書込時刻更新、Project/Segment/DLSの3文書bytes一致、作者316/別復元15860の二通常exit0を確認した。別プロセスでvelocity118/100・pitch36/37・PChannel10・Bars0・Record Onlyが復元され、Notes0、Undo/Redo無効を確認した。PIDは履歴値であり再開対象に固定しない。検証用cloneはすべて通常終了済み。正式インストール原版は開いたまま保持する。

90秒固定PCMは[14/15・全体不合格](../../work/acceptance/product-project-gui/20261011T023918519Z/audio-20261011T024009775Z/metronome-audio-proof-v1.json)。120BPM・4/4・PChannel10のTR808で、発音、約0.5秒の拍間隔、toggle-off/Stop後の無音、toggle-on/再開の発音、capture時計/packet/無clipが成立した。off9.687秒とStop待機8.566秒が事前の4〜8秒範囲を超えたため、操作窓の1検査が不成立。直前の同候補録音も14/15で、Stop待機15.385秒が超過した。両録音と前候補の失敗を残し、閾値や操作窓を緩めない。同じ手順の再録音は打ち切り、条件を変えた操作方法の準備後に検証する。

Count-In All Playbackの実時計接続は未実装で、nonzero要求は既存再生を壊す前に明示拒否する。Record Onlyの録音自体も未実装。強拍/音高/velocityのPCM対照、可変Tempo/TimeSig、別PChannel/custom DLS、loop/range、Style/Motif、原版同入力比較、全値域のキーボード/マウス入力、実ロード元監査は未確認。Optionsは原版730×498に対し本体730×480で字体・余白にも差があり、厳密外観は未達。owner対象modal UIAの要素未取得は今回も発生し、Escapeでは閉じられた。これを製品focus不良や全キーボード操作成立と断定しない。

次はAll Playback Count-Inを事前契約から実時計へ実装し、新候補で関連回帰・本体native保存/別復元/二通常終了・固定PCMへ接続する。操作窓だけを満たすための無変更再録音は繰り返さない。SSP01〜08の他残件、独立Style015244、全40責務/全8条件、46native/28driver OS凍結、Q2別Windows環境の未用意を維持する。

前版本文は[改訂前保存](../../work/analysis/ui-transport-metronome/20261011T005908347Z/before-progress-v3/docs__analysis__ui-transport-metronome-2026-10-11.md)、旧v2の一次結果は[旧単位](../../work/analysis/ui-transport-metronome/20261011T005908347Z/unit-record-v2.json)に保持する。最新Producer SHA256 e70a13a3f71d9f889618f6aa2ef0100db6d923058314bdfd2f05543fe99ffdf3、PCM SHA256 c6a0fc8acc1279c65734f80073104054c718b9251e22a1521b3c135bbb29bc94。

# Wave Find Best Loop End From Selection — 1734限定検証

2026-10-10日本時間。候補20261009T173457914Z、Producer SHA-256 c53e794630563ab9a81dfcaf7e487d08c53f340f132df52b7235b9f364c9efc4。全体は未完、fullAcceptance=false。

原版Helpの選択範囲→右メニュー入口を実装した。既存ループ始点を保ち、範囲内の終端候補を各channelの隣接sample接続誤差で比較する。数学的探索法・境界外挿・同値時の最初の候補は実装上の推論であり、原版のアルゴリズム一致は未確認。

同候補の236保存/作業ソースと4出力は一致。関連8群が合格し、wave-pcmは既存80と新66の146検査。Main TreeでWaveを開き、88190..88210の範囲から長さ22050→44100、CtrlZ/Y、一回のUndoで戻るno-op、別Wave・空範囲・始点より前の無効表示を実操作した。GUI10・native5検査は限定合格。保存Waveは事前期待値と全bytes一致し、変化は選択WSMP loop lengthだけ。PCM・独立SMPL・不明chunk・別Wave・Segmentは全bytes一致。Projectは3filhのtimestamp/size journalだけ変化。

Wave CtrlSは既存実装で対象外で、二回の無操作を保存した。FileメニューでWaveを保存し、Projectを保存した。author PID23860とreload PID2788は実体・開始時刻・hashを照合し、両方とも通常exit0。別processでloop type0/start44100/length44100・SavedとMainの空UndoRedoを確認した。

WASAPIの固定96秒録音は不合格。初回・再開とも終端以降の441Hz期待に対し882Hzのsource tailが出た。現入力のwaih loop override0/0のままでは編集したWSMP loopが適用されていない。初回/再開Stopの操作間隔10.490/11.217秒は事前4..5秒を超え、5秒source発音後なのでactiveStopも不成立。録音・失敗・固定判定器を保持し、閾値を変更していない。初回onset0.990秒・再開0.249秒と無音はこの録音の限定結果で、1446の1.021秒失敗は消さない。次はWave項目のloop enable/end/継承・override契約を確認し、独立単位で実装する。

候補menu画像ではHelpの正確なlabel、Snap checked、選択時有効・3境界時無効を確認した。4K/OS200%は変更していない。原版approval timeoutは同条件で再試行しておらず、原版の字体・配置・色・メニュー順・操作・数値探索比較は未完。Wave editorは別modalでMainを無効化し、原版MDI再現は残る。必要音声と原版比較が未達なのでboundedFinished/completed=false。0735限定採用・Q1/latestUnit/latestReportと2137原始一巡は保持する。

一次記録: [unit-record-v1](../../work/analysis/ui-wave-best-loop/20261009T173239834Z/unit-record-v1.json)。関連回帰: [run](../../work/acceptance/regression/20261009T174033411Z/run.json)。本体: [GUI](../../work/analysis/ui-wave-best-loop/20261009T173239834Z/gui-proof-v1.json)、[native](../../work/analysis/ui-wave-best-loop/20261009T173239834Z/native-proof-v2.json)、[menu pixels](../../work/analysis/ui-wave-best-loop/20261009T173239834Z/visual-proof-v1.json)。音声失敗: [audio proof](../../work/analysis/ui-wave-best-loop/20261009T173239834Z/audio-proof-v1.json)、[protocol](../../work/analysis/ui-wave-best-loop/20261009T173239834Z/audio-protocol-v1.json)。開始時のfeature/UI IDs誤記は二つのclarificationで訂正し、原記録は保持。

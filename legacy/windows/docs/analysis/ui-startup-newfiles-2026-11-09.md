# 起動と最初のNew Files — 2026-11-09

084341は自動Segment追加を止め、原版同様に空起動/NewCancel/無確認終了を実現。関連4件238検査、空5500/作者8652/復元15300の三通常exit0。最初Segment1を1件だけ作成し、20小節/40秒・TimeSig4/4のみ、作成後の破棄確認とNo保持、native保存/別復元/再保存を確認。292ソース/4出力一致、Segment全bytesとProject FILETIME以外保持。音のない作成内容とframe変更のため新PCMは不要、過去PCMを転用しない。

原版3984は引数なし起動でeditor/primaryなし。NewはProject既定、既定名checked・名前disabled。Cancel、Segment選択後Escapeも空を保持し通常終了。旧081250候補は自動Untitled Segmentを作り、空操作でも破棄確認を出した。084341はUIだけで起動空状態を認識し、Frameworkのnew_project dirty/history契約を維持した。明示作成した文書には従来の破棄確認が出る。

AddTracksで初期BandをSpace/CtrlSpaceで解除できなかった画面も保全し、CancelでTimeSigだけを保持した。空選択OKの成功とはしない。segment-creation CLIの既知Mixer.sgp Windows5は凍結のまま未実行。

原版は以前の専用NameProof.proを起動/通常終了時に自動更新した。旧hashは保全したが、その時点の全bodyは固定されておらず再現不能。3つのSegmentは旧hash一致。現時点の4ファイルを入力に使わない固定フォルダーへ保存した。詳細はoriginal-prior-output-recheck-v1.json。原版Project全bytes互換へ昇格しない。

一次入口：`work/analysis/ui-startup-newfiles/20261109T083128897Z/unit-record-v2.json`、`gui-proof-v1.json`、`native-proof-v1.json`。1280×800/96DPIの点観測で厳密なfont/glyph/layout一致ではない。

SSP01→SSP02継続。空workspaceのlegacy編集フォームを隠し、原版のdisabled CurrentTempo120表示へ合わせる独立作業へ進む。NewCancel/View再配置/新規文書のdirty保護/保存/通常終了/別復元を確認する。AddTracksは実UIでBand初期選択をSpace/CtrlSpaceで解除できずCancelしたため空選択OK未達。厳密外観・全component/Recording/Transition/全40/全8/Q2/Style015244、既知49native/30driver凍結・匿名Bandを保持し同条件再試行しない。全体fullAcceptance=false。 goal active。

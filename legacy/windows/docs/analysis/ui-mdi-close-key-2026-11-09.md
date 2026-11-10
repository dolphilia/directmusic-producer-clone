# Ctrl+F4 とメニュー操作状態の診断 — 2026-11-09

131516同一候補でCtrlF4の条件差を診断。EditをクリックしてEscape一回では、canvas実focus・修飾キー全upでもGUI_INMENUMODE=4が残りCtrlF4一回は編集窓を残す。追加Escapeでflags0となり、同じfocusのCtrlF4がgrayへ正常Close。別processでも4→0と通常Closeを確認し、既知menu状態での無効キーは再試行しなかった。Properties/子Band開閉と独立に条件を確認した。ソース不具合の断定/修正なし、原版同条件比較は未達。作者3172/復元4536は二通常exit0、全Segment bytes・3Project FILETIME以外・292ソース/4出力不変。新build/PCM/関連実行なし、同候補4/285参照。

Windows SDK10.0.26100.0 WinUser.hのGUI_INMENUMODE=0x4を参照。GetGUIThreadInfoの実focus/メニューowner/flags、MDI child数、GetAsyncKeyStateの修飾キーを読み取りのみで記録した。最初のEscape後はドロップダウン表示が消えてもmenu modeが残っていた。追加Escapeという条件変更後に正常Closeとなることを2processで確認。raw message hook/入力helper/OS設定変更は行っていない。メニューloopや製品routingのどの箇所がキーを消費したかを直接traceした証拠ではなく、原版との同条件差があるとも断定しない。既存の一回無効観測は取り消さない。

native監査v1は全byte/build検査後、作者runtimeのファイル名を誤参照して停止した。宣言済launchのdiagnostic-runtime.jsonを読むv2で同じ監査条件を通し、未合格v1を保持。音楽/再生変更や新PCMなし。

一次入口：`work/analysis/ui-mdi-close-key/20261109T133909247Z/unit-record-v2.json`、`diagnostic-proof-v1.json`、`native-proof-v2.json`。

131516候補のnative100登録節目を実行する。50既知OS凍結と匿名Band1はPIDなし、残49実行の期待2785検査を別集計し、driver172の新実行はしない。入力/出力/manifest/292保存・作業ソース/4出力を照合し、新たな拒否があれば証拠保全・凍結して同条件再試行しない。次はSSP02の完全同一論理時刻Band重複、または独立したSequenceの未達へ進む。menu Escape同入力原版比較、Cycle同拍別physical tick/全外観、Recording/Transition/全40/全8/Q2/Style015244は残しfullAcceptance=false。 goal active。

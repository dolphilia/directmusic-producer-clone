# Tree NewとLengthのCancel — 2026-11-09

091437同一候補でTree右クリックNew→最初Segment1のLength未確定30→Cancel→AddTracks→Cancelを本体確認。作成済み20小節/TimeSig4/4のみ・空UndoRedoが残り、作者20272/別復元20904は二通常exit0。native全Segment bytes/Project FILETIME以外と292ソース/4出力一致。ソース変更/新build/新PCMなし。関連4/238は同じ候補の既存2run参照で新実行ではない。原版契約は110746時の正式原版観測を参照し、新しい同入力比較とはしない。

正式原版の保存観測original-created-document-v1/original-length-cancelを照合した。New Files自体のCancelは作成を取り消し、作成後LengthのCancelは既定20小節を残してAdd Tracksへ進む。今回は未確定30を確認してからCancelし、保存nativeを61440clocks/TimeSig1件として監査した。

最初のCtrlAは全選択にならず2030へ追記された。UIA set_valueはcached app stateの要素不在で失敗。再観測後Home/ShiftEndで選択して30を直接確認した画面だけを採用。native監査v1は期待するファイル名配列の順序誤りで停止し、v2で両辺をsortした。いずれも失敗記録を保全し、製品データ/判定閾値は変えていない。Length前のGUIDを未保存なのでその区間の同一GUIDを直接測定したとはしない。

一次入口：`work/analysis/ui-segment-length-cancel/20261109T093620584Z/unit-record-v2.json`、`gui-proof-v1.json`、`native-proof-v2.json`。AddTracks空選択OKや既知Mixer.sgpを再試行していない。

SSP01の原版見た目/操作を継続。保存済み正式原版のメニューバー先頭File/Edit/View/Add-Insを照合し、候補の先頭Transport AudioPathとFile mnemonic欠落を限定修正する。実機の原版15872は起動通知がSkyへ列挙されず手動OK待ちで同じPIDを維持。独立したメニュー修正/保存build/関連/本体/保存/通常終了/別復元へ進む。全40/全8/fullAppearance/fullAcceptance=false、49native/30driver凍結と匿名Bandの同条件再試行なし。 goal active。

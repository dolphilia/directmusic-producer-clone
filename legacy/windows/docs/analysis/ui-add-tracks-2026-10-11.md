# Add New Tracksの初期Band選択と保存復元

2026-10-11。画像7・8、SSP02、UI03.priority-07/08の限定単位。原版で観測した「Add New Tracksを開くとBand Trackが選択される」に合わせて初期選択を修正した。空選択OK、Cancel、Band追加、Undo/Redo、保存と別プロセス復元が成立した。全操作・厳密外観・全体受入は未完。

## 契約と変更

開始条件は[開始記録](../../work/analysis/ui-add-tracks/20261011T114547994Z/start-v1.json)と[事前判定基準](../../work/analysis/ui-add-tracks/20261011T114547994Z/gui-criteria-v1.json)。正式原版5.3.0.900の既存Segment2で初期Band選択を再確認し、Cancel後はTimeSigのみを保持してSegment1表示へ戻した。[原版観測v2](../../work/analysis/ui-add-tracks/20261011T114547994Z/original-observation-v2.json)が正確なEXE identityを含む。v1の誤った参照フィールドは履歴として保持する。

変更はsrc/producer/new_files_dialog.cppのみ。利用可能なBand種別を見つけ、LB_SETSELとLB_SETCARETINDEXで初期選択する。既存の拡張選択、空選択OK、Cancel、未対応種別の扱いは維持する。初期SetFocus(list)は既存処理。ソースhashは67ca8a74e46ac79fa5fa97c6bc0278c524ae126fed1689389c4d8ff6d8099fcaから3f1641458e1efb1b5b805c9c48d7822191400198811bf9458241f00174502a91へ。[変更記録](../../work/analysis/ui-add-tracks/20261011T114547994Z/source-changes-v1.json)。

## 候補と実行結果

候補20261011T114844643Z。build/install成功、保存ソース288と作業ソース、生成物4を終了時にhash照合済み。[build](../../work/build/product-snapshot/20261011T114844643Z/build-summary.json)、[終了時identity](../../work/analysis/ui-add-tracks/20261011T114547994Z/closing-identity-v1.json)。EXE SHA256 71a4a0487835c07c183167205872fbf36fbd0d2b8fd090b43fa5d0ccec40b4b3。

関連回帰はcommon-open53、timeline-range117、segment-creation200の3モード370検査が合格。[一次run](../../work/acceptance/regression/20261011T115615305Z/run.json)。全native100/driver172の今回候補一巡は未実行。100711の原始一巡・Band PCM19/19を今回候補へ転用しない。

[本体証拠](../../work/analysis/ui-add-tracks/20261011T114547994Z/gui-save-reload-proof-v1.json)は事前10条件中9条件を直接確認、1条件を部分直接確認とソース推論へ分離した。初期Bandの選択表示は成立したが、UIAのfocused_elementはダイアログを返したため、リストの初期focusを直接確認済みとしない。Ctrl+Spaceは原版・製品とも表示変化なし。

CancelはTimeSig-only・Undo無効を保持。製品では未対応Pattern行の既存選択解除処理から空選択にしてOKを押し、Undo無効・保存後の4入力hash一致を確認した。この空選択の作り方は製品固有の検証で、原版の同操作確認ではない。原版の空選択解除操作と動的比較は未確認として残す。

既定BandでOKするとTimeSigの下に空のBandが一つ増え、UndoでTimeSigのみへ戻り、保存bytesは元と一致。Redoで一つ復帰し、Standard Save FileとSave Projectで保存した。作者PID9800は通常exit0。別プロセスPID7700でTimeSig+Bandを確認し、通常exit0。再読込の前後で4ファイルhashが一致した。音声を生む操作を含まないため音声検証は非適用。

## ネイティブデータと保持

[独立RIFF監査v2](../../work/analysis/ui-add-tracks/20261011T114547994Z/native-audit-v2.json)の8条件が成立。Segment2はGUID49bfa35f73176247b3392d2ce8ef68d1、長さ61440clocks（4/4・20小節）、TimeSig payload080000000000000004040400を保持。追加DMTK一つを除くと元ファイルと完全一致。追加DMBTはbdth=1と空lbdlのみで、Bandイベントや楽器を作らない。Seed/Length24は不変。Projectは3参照と他の14葉を保持し、Segment2のfilhメタデータだけ更新した。

[DLL点観測](../../work/analysis/ui-add-tracks/20261011T114547994Z/module-proof-v1.json)は作者PID9800の80モジュール（x86 74、WOW64補助6）、原版40hash一致0・未解決0。x64 wow64cpuの4GiB未満配置はPE・正確なSystem32パス・既存Windows補助hashで分類した。連続観測やQ2、全40責務の依存解消を意味しない。

[保全記録](../../work/analysis/ui-add-tracks/20261011T114547994Z/preservation-v1.json)で正式原版EXE、既存3入力、前単位で作成したSegment2のhash保持を確認。正式原版PID25548は同じ開始時刻で応答中。未保存Project内のSegment2参照を残し、原版は終了・破棄していない。

## 残件と次

初期Band選択の差は解消。リスト初期focusの直接確認、原版空選択操作、全17種の編集互換、厳密外観、SSP02全体は未完。次はUI01のメニュー終了後Ctrl+S無反応について、元の操作列・focus・文書状態を確認し、正式原版と安全な検証コピーで比較して原因を絞る。原因未確定の段階でfocus不具合と断定しない。旧Band停止失敗、Count-In15/16、Echo・SSP04・SSP08比較、独立Style015244、Q2、OS凍結は保持。全体候補0735、6作業中/2障害/0合格、fullAcceptance=false。

# Current Tempo編集の実装と休止地点 — 2026-10-12

20261012T004853599Z Current Tempoの停止中Segment編集を実装。正式原版で132.25の保存/UndoRedo、spin上133・下131、bar2から既存time0 markerの編集を観測しnative7合格。新候補configure/build/install0、291保存/作業ソース・4出力一致、関連4件/439検査合格。segment-creationは既知Mixer.sgp Windows5のため未実行・障害維持。候補本体/native二process/音声/追加原版契約は未検証。ユーザー指示で休止。 全体fullAcceptance=false。

停止中SegmentのCurrent Tempo欄を編集可能にし、カーソル位置で有効な既存テンポを表示・変更する。1..1000、小数2桁、Enter/フォーカス離脱確定、Escape復元、spin、文書Undo/Redoへ接続した。文書identity/index/cursorを編集開始時に束縛し、文書切替後に別文書へ適用しない。不正値・同値は文書bytes/dirty/historyを変えない。Styleの既存経路も接続したが原版比較は未達。再生中の編集は契約未確立のため無効で、表示だけ更新する。

正式原版25548の専用コピーで120→132.25をEnter確定・Save、Undoで120・Save、Redoで132.25・Saveを確認。spin上は133.00、下は131.00。bar2からspinしてもtime0の既存markerを133へ変更し、新markerを追加しない。最後はUndoで132.25へ戻して保存した。独立RIFF7条件で17notes/seqt不変。原版Escape/フォーカス離脱/不正値、Style/Pattern/再生中の契約は未観測。原版と実装の推論を分離する。

ビルド20261012T004853599Zはconfigure/build/install exit0。Producer SHA256 e07895d9d8b28ce982d319401d03851e517d7553931a408a6e462fff4030c15d。timeline-range 117、sequence-crud 259、tempo-properties 33、sequence-audition-runtime 30が合格（計439）。segment-creationは既知のMixer.sgp atomic replace Windows5をmanifestに従って未実行・障害として記録した。全関連合格ではない。旧224147のnative100/driver172一巡や235833のGUIを本候補の証拠へ転用しない。

ユーザーの休止指示により、候補本体の起動・編集・native保存・通常終了・別process復元・PCMは行っていない。原版25548と未保存状態保持の旧候補6640は読み取り観測で応答中。終了・破棄せず開いたまま。step37の入力前に手動入力を検知し、再観測したところ原版はCodexに隠れていたため追加操作していない。新しい主画面/低解像度を使い、旧座標/4K200%は再利用しない。

[単位](../../work/analysis/ui-current-tempo/20261012T003122326Z/unit-record-v1.json)、[保存/作業/入力照合](../../work/analysis/ui-current-tempo/20261012T003122326Z/pause-identity-v1.json)、[関連回帰](../../work/acceptance/regression/20261012T005547643Z/run.json)、[原版native](../../work/analysis/ui-current-tempo/20261012T003122326Z/original-native-inspection-v1.json)。New Files空選択の観測は[未完記録](../../work/analysis/ui-new-files-empty-selection/20261012T004000Z/unit-record-v1.json)として保存。選択を解除できず、空OKの結果は未確認、ソース変更なし。

再開時：ユーザーが再開を指示するまで作業を進めない。再開後は004853の保存/作業291ソース・4出力・入力を照合し、現在の低解像度と最新windowを再観測。正式原版のEscape/focus-loss/不正値契約の未確認を保持し、004853本体を専用candidate Projectで初起動→132.25入力/Enter/UndoRedo/spin/文書所有権→native保存→通常終了→別process復元/通常終了→固定WASAPIテンポ/発音中Stop/無音/replay比較へ進む。保存済み235833空CC・224147一巡/CC15/16を新候補の実行証拠へ転用しない。49native/30driverの凍結、SSP04親/6640破棄回答待ち、独立Style015244/Q2/全40/全8を保持。

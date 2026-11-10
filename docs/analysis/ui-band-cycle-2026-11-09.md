# 同じ Start Time 拍の Band 表示と巡回 — 2026-11-09

131516は異なる論理時刻/同じStart Time拍のBandを斜体でまとめ、Cycle Bandsで選択を巡回するTimelineだけの修正。作者C→A→B→Cを2巡/復元1巡、選択を右クリックしても維持。Propertiesはphysical−1と各BelongsTo1/2/3拍、C/Aと復元Aの子editorは各16inactive行を表示。Solo/空白はCycle無効、dirtyなし/UndoRedo無効。関連4/285、作者3548/復元1156は二通常exit0。初期/作者/復元の全Segment bytes、4Band名前/GUID/時刻/無効flags・2control文書各17音符を保持し、3ProjectはFILETIME以外同一。292ソース/4出力一致、前候補と291ソース同一。新PCMなし。

正式CHM bandtrack.htmの同拍Bandをitalic/Cycle Bandsで選ぶ契約と、bandproperties/belongstobeatのStart TimeとBelongs Toの区別に従う。負の先行tickは既存Band Propertiesと同じ拍へ帰属させ、現在選択したBandを描画/hit/ダブルクリック/右メニュー/Propertiesで共有する。選択がない場合は既存と同じ最後のBandを表示し、単独/空白のCycleを無効にする。

入力は古い固定GUI出力の無効Band1から、名前/新GUID/論理0,768,1536,3072/物理−1,−1,−1,3071を独立合成した専用文書。入力の全16行flagsと時刻・参照を先に監査した。候補でのGUI作成や原版動的比較の証拠にはしない。fixture事前監査v1はPChannel offset20のassertで書込前停止、native offset24へ訂正したv2で通過。元コード/失敗記録を保全した。

完全に同じ論理時刻を持つBandの読込/移動はcoreの拒否が残る。これは未達のまま保持し、今回のdistinct-logical/同Start Time条件を全重複対応へ拡大しない。同一beat内で異なるphysical tick、全viewport、厳密な原版font/geometry/docking、clipboardも未比較。Aの子窓はcascadeで右端が一部画面外に出ており全geometry成立とはしない。

EditメニューEscape後のCtrlF4一回は本体窓が残り、fresh観測でも同じ。mouse MDI Closeでgrayとなり、再Open/保存を完了した。子BandのCtrlF4は成立。失敗観測を残し、次の独立診断で扱う。新候補への旧native/PCM受入転用、既知OS凍結の再実行/迂回は行っていない。

一次入口：`work/analysis/ui-band-cycle/20261109T131212131Z/unit-record-v2.json`、`gui-proof-v1.json`、`native-proof-v1.json`、`keyboard-close-observation-v1.json`。

同じ131516の専用文書で、Edit→Escape直後のCtrlF4が一度無効だった操作差を独立診断する。子Band CtrlF4とmouse Closeは成立しており、原因は未断定。最新window/PID/表示/実focusを再観測し、main system-key routingとTranslateMDISysAccelの順序を照合。再現した場合だけ修正し保存build/未凍結関連/本体/保存/通常終了/別復元を進める。その後の現行native100節目は50OS凍結/匿名Band1をPIDなしで保持し、49実行だけを集計する。完全同一論理時刻Band重複・同拍別physical tick・原版同入力比較/全外観/全40/全8/Q2/Style015244は未達。fullAcceptance=false、50native/30driver凍結同条件再試行なし。 goal active。

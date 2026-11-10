# SegmentのTransport Listとprimary選択 — 2026-11-09

055400の順序/glyph/View再配置/native保存復元/2Play PCMを保全した次のSSP01単位。全体fullAcceptance=false。

原版/CHMでは一覧選択は編集画面を切り替えず、停止中は選択対象のTempoへ、再生中は現在primaryのTempoを保ちStop後に次の選択へ変わる。最初の範囲を既存Segment editor hostに限定し、file:componentをソートした一覧をTransport Controls左端へ追加した。再生選択を編集対象と別のindex/ownerで保持し、停止中Tempo表示/変更と次のPlayは選択Segmentへ接続。編集ウィンドウ/編集欄の再アクティブ化は、旧Tempoのfocus-loss確定後にprimaryを編集対象へ戻す。Project交換時に選択を消し、所有者identityを再確認する。Style/他componentは既存経路を保ち、この一覧登録は未対応として残す。

x86 SDK26100の3翻訳単位構文検査exit0、保存候補062231をビルド中。ソース実装を本体実行/保存復元/音声合格とは扱わない。一次入口：`work/analysis/ui-transport-primary/20261109T062110913Z/unit-record-v1.json`。

次：同候補build/関連回帰→独立primary選択/Tempo所有権/focus移動/View→2文書native保存/通常終了/別起動復元→Seed132.25再生中にPeerを選択しStopまで現tempo保持、次のPlayはPeer120を固定PCMで確認→通常終了/機能群一巡。全40/全8・厳密外観・録音/Transition・全component・Q2/Style015244/既知凍結を維持する。

## 本体・保存復元・音声の限定結果

062231はconfigure/build/install0、関連5件496検査、7Seed+7Peer native状態と17notes保持、2process保存復元/二通常exit0。選択Peerと編集Seedの分離、Peer125 focus-loss確定→Seed再primary、PeerUndo120、View hide/showを確認。再生中Seed132.25維持→次PlayPeer120の固定PCM合格、中央値0.455/0.500秒、負例6件拒否。全native100節目を実行中、全体fullAcceptance=false。

NoProject引数なし起動は既存WM_CREATEがUntitled Segmentを作るため、空状態とは扱わない。専用EmptyTransport.proで新しい一覧/2Play/Tempo/spinの無効状態を確認し、この空Projectのbytesも保持した。初回Untitledのcomponent名が空表示になる細部は残件。

音声初回は一覧開閉/選択の個別観測を含む最初Stopが32秒となり、事前上限25秒に違反。テンポ/音程/無音/再生元は成立したが録音全体は不合格で保全。取り直しは最初40秒・35..45秒窓（旧32秒を含まない）、次14秒を録音前に固定し、両発音中Stopと無音が成立した。発音/音程/無音/入力時刻/時計の閾値は変更せず、判定器末尾の古いprotocolhash出力先だけを訂正した旧/修正版を保持する。

一次入口：`work/analysis/ui-transport-primary/20261109T062110913Z/unit-record-v2.json`、`gui-proof-v1.json`、`native-proof-v1.json`、`audio-summary-v1.json`。現在1280x800/96DPI、復元PID9436は通常exit0、全6control中心hitが自身に一致。

062231同候補native100一巡の終了/全入力と保存ソース・4出力を照合して状態/UI台帳へ記録し、Transport Listの名前fallback/他component登録・全外観等の独立次作業を継続。既知49native/30driver凍結や匿名Band失敗の同条件再試行、全40/全8の昇格はしない。

## 同候補の機能群一巡と現行再開点

062231のSegment限定Transport Listで、編集Seed132.25とprimaryPeer120の分離、選択先Peer125のfocus-loss確定/Undo120、編集再アクティブ化/View/空Project無効状態を確認。関連5/496、7Seed+7Peer native状態・17notes保持、作者8932/復元9436の二通常exit0と保存復元成立。固定PCMは再生中Seed132.25保持→次PlayPeer120、17発音/音程/activeStop/無音、中央値0.455/0.500秒、負例6件拒否。全native100は50合格2830検査/49既知OS凍結/匿名Band1保留。30入力（7directory）・291ソース・4出力一致、新実行失敗なし。driver172は旧一覧参照のみ。

原版の同じ一覧選択/Tempo表示の限定観測とCHMを契約根拠にした。全component登録、所有者別編集保存の同操作原版比較、録音/Transition、全boundary/Style/live/secondary、厳密glyph/font/配置/dockingは未達。native/PCMの限定成功を全Transportや全40/全8へ昇格しない。

一次入口：`work/analysis/ui-transport-primary/20261109T062110913Z/unit-record-v3.json`、`native-milestone-proof-v1.json`、`gui-proof-v1.json`、`audio-summary-v1.json`。

SSP01継続。実画面で確認したUntitled SegmentのTransport List名の空表示を、ツリー/編集対象と同じfallbackへ修正し、保存後ファイル名/文書ownerの保持を限定検証する。続いてTransport Listの他component/開閉lifecycle、録音/Transition、Style/live/secondary、全外観の独立未達へ進む。全40/全8/Q2/Style015244と既知OS凍結・匿名Band失敗を保持し、条件が変わらない再試行はしない。 全体fullAcceptance=false、goal active。

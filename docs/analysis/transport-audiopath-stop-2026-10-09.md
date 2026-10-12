# 通常AudioPathの停止と初回再生

2026-10-09。限定採用0735を維持。最新0239は通常Stop・再開と保存済みWaves設定の本体/音声経路が限定合格。原版UI比較・全40責務・全8受入・Q2は未完、単位completed=false/fullAcceptance=false。一次証拠は[作業単位](../../work/analysis/transport-audiopath-stop/20261009T020035331Z/unit-record.json)。

最後の利用者の通常AudioPathをStopで非アクティブにし、共有セッションと録音用経路を保護した。短い準備時間は独立した通常Segmentに限定し、Script/Style/Motif/ToolGraphは1000msを維持。本体起動後に音声runtimeを準備するprepareはセッションを作らず、重複呼出しと再生中呼出しも状態を維持する。初期化失敗時も編集を継続でき、Playで再初期化する。

0239はconfigure/build/install各0、231保存/作業ソース462照合、4生成物一致。[関連37](../../work/acceptance/regression/20261009T024613581Z/run.json)は33合格/4既存保存障害凍結/0失敗。Waves runtimeは49確認、通知・同時再生4製品driverも合格。原始93/164一巡の合格ではない。

[本体監査](../../work/analysis/transport-audiopath-stop/20261009T020035331Z/main-proof-v2.json)は6確認。標準値と変更値(-6,-12,750,0.5)を別PID8572/22008で復元し、取消し、8ノート、120/180Tempo、二通常exit0、入力6ファイル不変を確認した。0239で新しい編集/native保存は実行していない。0127の編集/保存成功はその候補の履歴として保持する。閉じたウィンドウの再取得エラーは保存し、親ウィンドウの観測と通常終了監視で実際の結果を確認した。

[固定WASAPI解析](../../work/analysis/transport-audiopath-stop/20261009T020035331Z/audio-proof-v4.json)は合格。同じendpointの各64秒/48kHz/stereo32bit、製品/録音器/入力/操作時刻/UTC-QPC/packet/PCM/判定器hashを結合した。Play完了からの初回開始は約0.989/0.943秒、再開0.334/0.240秒。4つの発音中Stop後、基準/終端窓はRMS0。69,71,73,74,76,78,80,81の8音と120/180の時刻窓が成立し、7音で設定差10%以上を検出。peakは標準0.148、変更0.0543。端末の実スピーカー、原版DSP等価、processのロードmodule由来はこの解析の合格対象外。宣言OS runtime/DSP/GM.DLSは終了後の読取hashとして別記する。

[判定器対照5件](../../work/analysis/transport-audiopath-stop/20261009T020035331Z/audio-controls-v4.json)は無音・過大peak・停止後混入・標準録音の取り違え・早期Stopを各意図したguardで拒否(exit1)。[stereo対照4件](../../work/analysis/transport-audiopath-stop/20261009T020035331Z/stereo-controls-v1.json)は正/逆位相正音高を受理し、無音/誤音高を拒否。符号付き平均の打消し訂正は周波数/窓/最小振幅/比/その他閾値を変えていない。

0200のglobal300msによるScript PREPARE失敗、0209の時間外Stop/初回遅延/音高平均の失敗、判定器操作名指定ミスを削除せず候補別に保持。旧結果を0239へ転用していない。[改訂前報告](../../work/analysis/transport-audiopath-stop/20261009T020035331Z/report-before-v4.md)と単位履歴から辿れる。

次は[原版Help基準](../../work/analysis/transport-audiopath-stop/20261009T020035331Z/next-effect-properties-reference.json)のEffects List単一選択とF11/右クリックPropertiesを本体へ接続する。InstanceName/Software flag/Palette/複数選択/clipboard/全graph/配置と原版動的比較は未達のまま。原版64100706のapproval timeout、個別Windows5、Clipboard/File Save、Q2は解除条件を別々に保持。4K/200%表示設定を変更していない。

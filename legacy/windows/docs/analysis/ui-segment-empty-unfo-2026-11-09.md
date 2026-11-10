# 空の任意UNFOを保持したSegmentの再生 — 2026-11-09

081250は保存済み空UNFOをnativeに残し、再生用コピーからだけ省く修正を検証。関連5件497検査、作者19008/復元7736の二通常exit0、12native状態・各17notes・全Segment bytesとProject FILETIME以外を保持。137 BPMの2Play/発音中14秒Stop/無音PCM合格、発音間隔中央値0.440/0.440秒、負例5拒否、時計4負例。全native100は50合格2855検査/49既知OS凍結/匿名Band1保留。30入力（7directory）・292ソース・4出力一致。

074348はFile SaveAsの新GUID/元文書保全/独立履歴/3文書保存復元は成立したが、最初PlayのLoad current DMSGが0x88781184で失敗した。SDKの意味はDMUS_E_LOADER_FAILEDCREATE。生の失敗入力を再試行せず、無音声の条件変更診断でSegment生成/UNAM追加/空UNFO除去の成功を確認した。081250ではNativeScriptTrackRuntime.loader_bytesのprivateコピーだけからzero-child LIST:UNFOを除外し、既存Script track処理を保持。保存ファイル/unknown非空metadata/他空LIST不変の4検査も追加した。

081250の入力は失敗候補と同じ保存bytes。SaveAsのmain/frameworkソースは074348と同一で、FileSaveAs操作そのものの本体証拠は候補を分けて保持する。原版のSaveAs後Tempo disabled/dirty表示・sgdl/segh正規化の模造・全外観は未達。

PCM判定初回は補助action JSONのwindow省略で測定前に停止した。元JSON/失敗ログを保全し、同時記録済みactions.jsonlと全4画面のwindowが入力時刻まで完全一致することを照合してv2を生成。時刻・録音・事前protocol・判定器・閾値を変えず合格した。

一次入口：`work/analysis/ui-segment-empty-unfo/20261109T081231083Z/unit-record-v2.json`、`native-proof-v1.json`、`audio-summary-v1.json`、`native-milestone-proof-v1.json`。driver172は旧一覧参照で新実行なし、別記30driver凍結も解除しない。

SSP01→SSP02の起動/New Files lifecycleへ継続。原版引数なし起動PID3984を同じPIDで初期化待ちし、NoProjectのactive editor/primaryとNew/Cancelを観測。候補の自動Untitled Segment追加と空状態dirty/close/新規1文書の操作を照合し、根拠に従って修正・保存/通常終了/別復元へ進む。原版のSaveAs後Tempo disabled/dirty/全layout、他component/Recording/Transition/全40/全8/Q2/Style015244は未達。既知49native/30driver凍結・匿名Bandの同条件再試行なし、全体fullAcceptance=false。 goal active。

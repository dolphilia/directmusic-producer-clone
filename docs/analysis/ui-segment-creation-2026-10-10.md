# Segment作成と実トラック追加の限定検証

2026-10-10。最新試験候補111522（20261010T111522433Z）で、画像5〜8に対応する作成経路と、Tempo120・空Sequenceの追加を実装した。SSP02全体、原版との同入力比較、外観の一致は未完。限定採用0735と独立Style015244は維持し、fullAcceptance=false。

File New・Standard New・ツリー右クリックNewとCtrl+Nを共通Create New File(s)へ接続した。11種の一覧、複数選択、既定/指定名、取消しを共用する。Segmentは20小節（120BPMで40秒、61440clocks）を初期値にし、最初は4/4・4gridsのTimeSigだけを持つ。作成直後にAdd New Tracksを開き、空選択でも進める。17種の一覧のうち15種は既存native契約のfactoryへ接続し、未確定のPattern/Waveは無効にしている。Extension bars/pickupもnative契約が未確認のため無効。

Timelineは固定の架空トラックを常時表示する方式を改め、文書内の実トラック・順序・グループを表示する。Band追加後はTimeSig/Bandの2行、Tempo追加後は120、Sequence追加後は4行になる。Sequenceのパート表記・開閉・ピアノロールは次段階に残す。トラック一括追加は一つの履歴とし、不正/未対応入力は文書を変更しない。

252保存ソースと作業ソース、4出力のhashが一致し、configure/build/installは各0。EXE SHA256はfa125aa4faa312d787f1e945575753750bbd14aab113038e20c773fdcba85275。関連7モード441確認（Timeline117、Mute99、Sequence26、ComponentName70、Tempo19、Meter65、SegmentCreation45）は全て合格。これは同候補の補足試験で、全登録97native/170driver一巡ではない。4回の失敗buildとRIFF payload/fixtureの修正前診断を保持した。診断Muteは必要入力を省いた呼出し誤りで通常coreへ落ちたもの。正式Mute99の結果と混同せず、新しいMute製品障害を作らない。

本体で3入口とCtrl+N、Length取消し、TimeSigだけの作成、Add Tracks空OK、Band/Tempo/Sequence追加、Undo/Redo、Segment/Project保存を確認した。作者PID1256と別復元PID11140はともに通常exit0。新ファイルは検証専用GuiCreation/s.sgp、内部名Segment2。復元後も20小節・4トラック・Tempo120で、Project/Seed/Segmentの3ファイルは復元前後でbyte一致した。独立RIFF監査でもTimeSig4/4、空Band、Tempo120@0、空Sequenceを確認した。

Save AsでComputer Useのtype_textが文字を入れない観測があったため、通常キーでsを入力し、nativeダイアログで検証用フォルダーへ移動して保存した。原因未確定であり、日本語パス非対応やOS拒否と断定しない。状態キャッシュ/座標エラーもfresh観測後に回復し、許可ゲートの迂回は行っていない。

正式原版は既存文書を維持したままNewの一覧を開いて取消し、CHMのnew components/new segment/add tracksも参照した。画像5〜8の機能入口を接続したが、同じ入力での原版編集/保存復元、正確な字体・アイコン・配置・200%での外観比較は未実施。空トラックにはノート/音源がなく、この単位では音声試験を非適用とした。旧候補のPCM・module点監査を111522の実行証拠へ転用していない。

次は画像9〜11のBand Insert・Properties・埋込Band子編集画面。画像44→46→47の長さ再編集、SSP03〜07のピアノロール/楽器/ノート/CC/複数パート/ドラム/DLS、SSP01残件、全40/全8を保持する。既定名のrename後衝突、New Projectのフォルダー入力、未対応2track種別も残件。

一次証拠: [単位記録](../../work/analysis/ui-segment-creation/20261010T104037241Z/unit-record-v1.json)、[本体保存復元](../../work/analysis/ui-segment-creation/20261010T104037241Z/main-create-save-reload-proof-v1.json)、[保存ソース照合](../../work/analysis/ui-segment-creation/20261010T104037241Z/closing-build-identity-v1.json)、[関連回帰](../../work/acceptance/regression/20261010T112217579Z/run.json)、[復元画面](../../work/analysis/ui-segment-creation/20261010T104037241Z/product-reload-four-tracks.png)。

# 共通PropertiesとSegment・Style名の本体統合（2137候補）

2026-10-09（日本時間）。ユーザーの再開指示に従って実装・検証を継続中。2137候補では固定した七つの終了条件のうち六つの本体統合条件が成立し、原版の外観・操作比較は既存のapproval障害として残る。単位completed=false、全体fullAcceptance=false、0735限定採用を維持する。

原版HelpはF11／右クリックProperties、単一の非modalウィンドウ、選択追従、Segment・StyleのName欄を示す。実装はこの入口を既存Frameworkの文書と履歴へ接続した。選択だけで別の編集文書を開かず、名前を変更するとProject Tree・dirty表示・UndoRedoへ反映する。Segmentは任意のUNFO/UNAM名を既存RIFFへ追加し、既存ID・未知データ・ノート・テンポを保つ。Styleは既存の名前編集APIを使用する。

2122では名前を入力した後、タブが兄弟の名前欄を覆っていた。2137でタブを兄弟コントロールの後ろへ置き、入力後と別起動後の実画面で欄が見えることを確認した。OSの200%表示を維持しており、表示倍率を変えて不具合を回避していない。2122の失敗・保存データ・通常exit0は[失敗記録](../../work/analysis/ui-properties/20261008T205518032Z/unit-2122-gui-failed.json)へ保存した。

| 条件 | 今回の実証 |
| --- | --- |
| 候補の同一性 | 229保存ソースと作業木、4生成物が一致。configure/build/install各0 |
| 関連回帰 | 10群合格。component-name33、common-open53、Timeline93、Sequence26、Pattern32、Send文書57/runtime89等 |
| 本体の入口・追従 | F11、右クリック、Shift+F10で同じmodeless Properties。非active文書の選択・名前編集でもactive編集内容を維持 |
| 名前・履歴・表示 | SegmentとStyleの日本語名、可視性、Tree・dirty・UndoRedo、Propertiesを開いたままの編集を確認 |
| 保存・別process復元 | 作者PID21168、復元PID21752、音声PID13356が別の実起動。三通常exit0。復元後の名前、Segment7/7.5 BPMと8ノート、Style109 BPM・Patternを確認 |
| native監査 | 17入力を結合し7条件合格。変更は二つの内部名と対応project metadataのみ。二FileOutput／実Send／既定Env／−600／二PChannelと音源は不変 |
| 必要音声 | 300秒のWASAPI＋二つのFileOutputを10条件で監査し合格。440Hzと261.625565Hz、二回のPlay・発音中Stop・停止後無音・再開・通常close後無音を確認 |
| 原版比較 | 実際の原版画面・操作の動的比較はapproval timeoutで障害あり。Help由来の期待と本体内部確認を区別 |

音声の固定Stop窓と保持無音、通常close後のRMSは0。UTC/QPC driftは約4.2μs、packet-origin誤差は最大約20.8μs。旧失敗録音の時計資格・時間外close・onset超過は保持し、今回も閾値を変更していない。mixed endpointからSend利得や旧DSP等価を主張せず、runtime Tempoの数値測定も今回の対象にしていない。

一次証拠は[単位記録](../../work/analysis/ui-properties/20261008T213743141Z/unit-record.json)、[本体GUI](../../work/analysis/ui-properties/20261008T213743141Z/gui-proof.json)、[native7](../../work/analysis/ui-properties/20261008T213743141Z/native-proof.json)、[音声10](../../work/analysis/ui-properties/20261008T213743141Z/audio-proof.json)。[build](../../work/build/product-snapshot/20261008T213748049Z/build-summary.json)のSHA256は5b6ae4df1d8563003ab60587bbe8df59e1f8519e448b5e5e02e973ec9adebfb2、installed Producer.exeは8f699a8b7a66c3cb2f83771f23128405cb8a04fda545c05341627f3b8917fb7b。[関連run](../../work/acceptance/regression/20261008T214233375Z/run.json)はc14595f66e84698945042d6569b653bfa094daae1b535130425ab10fea0d19e6。主要入力・判定器・依存・録音・実process・通常終了は各証拠内のhashで結合する。

節目の[原始native92一巡](../../work/acceptance/regression/20261008T220516753Z/run.json)は54合格／37既知障害／既存三Band失敗1。[原始driver164一巡](../../work/acceptance/registered-drivers/20261008T220746580Z/run.json)は24合格／23既知障害／117未実行。原始一巡と専用関連10・今回のGUI/native/音声監査を区別する。後続anonymous Bandのsource解決は49チェック後に失敗し、保存拒否とは別の未達として残る。

残るPropertiesページ、各component型、Toolbar／MDI／docking／font/icon/theme、原版のName境界値・UI復元方針、全40／全8／Q2は未完。次は原版File menu Helpで明確な、既存pathのSave Projectとactive file名を含むSave／Save Asメニューを実装する。新規pathの取消し・未保存確認は固定した次単位の範囲で扱い、既存のWindows5／原版approvalを同条件で再試行しない。

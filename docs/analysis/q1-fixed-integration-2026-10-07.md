# Q1 固定候補の新規 native Project と Sequence lifecycle

候補 `20261006T154607013Z`、Producer SHA256 `b232c90206c224b2edbb7582f032209977a22536a54fcfec07e689d85ec43ab4`。製品ソース194件の checkout/hash 不一致0。製品ソース・生成物は変更していない。全体完成ではない。

新規 Project の Initial Segment で note追加→Undo→Redo→保存を実行し、Sequence のテンポを20→30 BPMへ編集・保存した。Style、Band、DLS、AudioPathを本体で読み込み、native JAZP `Fresh.pro` を保存した。作者23136は通常終了exit0。別起動16916の最初のPlayは所有Script不足で失敗した。参照するSourceHost.sppを本体で読み込み、変更前入力を保持してProject保存、通常終了exit0、固定入力の別起動4040で再読込した。最終再読込プロセスも通常終了exit0。

既存WASAPI録音では8音の音程、20→30BPMの発音間隔3,3,3,3,2,2,2秒、発音中Stop、停止後無音、先頭から全8音の再開、packet連続性が合格。Stop前RMS0.065298、停止後RMS0、peak0.093043。持続PCMの位相揺れを追加発音に数えた解析器の不整合は、期待時刻によらない120ms無音先行条件で修正した。無変更の陽性対照は合格、欠音・旧テンポ・停止後発音・再開なし・誤音程・余分なattackの6対照は拒否。旧解析の不合格結果は保持した。

証拠は [`unit-record.json`](../../work/analysis/q1-fixed-integration/20261006T161100Z/unit-record.json)、[`audio proof`](../../work/acceptance/product-project-gui/20261006T164031160Z/audio-20261006T164215022Z/gui-sequence-audio-proof.json)、[`controls`](../../work/analysis/q1-fixed-integration/20261006T161100Z/lifecycle-controls/negative-tests.json) に結ぶ。全入力hash、ビルドhash、exehash、作者/再読込PID、操作UTC、録音hash、解析器hashを単位に保存した。初回失敗入力はfailed-missing-script/Freshとproject-before-script.proに保持。

75native群は同じ変更なし候補の45合格30障害を維持。107入口の新しい一巡はraw20合格21障害66未実行、今回の音声対照を結んだ分類は21合格21障害65未実行。通常coreのWindows5拒否は再試行していない。全8は6作業中2障害。

Style発音、Transport競合default/embedded優先、未接続PChannel無音、DLS長持続、原版動的比較、独立Q2、全40責務が残る。20→30BPM入力のgateは1～1.5秒なので、1.5～2.5秒の長持続測定へ転用しない。

次の一手: 同じ154607013候補・Fresh native ProjectへControl/Empty/Routeを本体読込して保存・通常終了・別起動。Style発音とTransport Conflict(PChannel1開始)対Control無音、embedded Route(PChannel0開始)対SourceSequence発音をWASAPIで比較。長持続DLS測定は5→7.5BPMのNoteOff前窓で実行し、20→30BPMの1～1.5秒gateに1.5～2.5秒窓を適用する解析器契約を明確化する。その後独立Q3へ優先を更新。

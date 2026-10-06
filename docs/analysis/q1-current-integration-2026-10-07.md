# Q1 同一候補の五形式統合

候補20261006T185539543Z、195保存ソース、Win32 Release / reference tools OFF。全体未完了。build SHA256 dd54cc967afe8a39480071c36e805fb1c98bde2b985b99cbd18b51478632383f、install EXE a89961826e90c01e4a163a6c0fd8d518d7f8ca35bcae9e847a0e34bc1062ef26。

新規native Fresh.proに10文書を登録。本体でSegment/Style/Band/DLS/AudioPathの読込・変更・Undo/Redo・保存を行い、作者22360の通常exit0後、別PID13748で五形式の最終値を復元して通常exit0を取得。Initialの1音符、Sequence5→7.5BPM、Style109BPM、Band volume101、DLS Region lowkey1/loop320+11987、AudioPath名Q1 Conflictを保存。195ソース・12入力・EXEのhash同一性を監査した。

Style WASAPIは10発・2秒間隔・MIDI60・無音・packet integrity合格、6異常対照も期待通り。Transport録音は対照無音、発音中Stop、停止後無音を確認したが、全曲再開後半に余分な音が混入しpitch/tempo/sustainは不合格。録音中にnative runtime回帰を並行実行したため混入を疑う。合格へ緩和せずraw録音・失敗を保持。再収録用PID22768は起動したがComputer Use一覧に現れず、kernel再初期化後も対象未取得。ハンドル推測・迂回・強制終了はしていない。

判定器はStyle期待tempoを明示可能にし、Sequenceの無音対照を完奏から除外し、停止後音の異常対照をStop interruptedへ配置した。旧154607録音の別コピーで7対照を検証したが、これは判定器の回帰素材であり現製品音声の合格へ転用しない。最初の対照生成失敗も保持。

一巡はnative77=47合格/30障害、driver107=20合格/21障害/66未実行。通常core既知Windows5は凍結、Sequence26のDWORD契約は現候補合格を維持。全8受入は6作業中/2障害。原版動的比較・Q2独立Windows・全40責務を未完として保持。

証拠入口: [最新単位](../../work/analysis/q1-current-integration/20261006T192000Z/unit-record.json)。各観測、入力hash、終了監視、WASAPI PCM、判定器版、失敗記録をそこから参照する。再現はTest-ProductProjectGui.ps1で同buildのFresh.proを開き、Capture-ProductGuiAudio.ps1へ12入力と300秒を渡す。GUIでConflict default/Control無音/SourceSequence中断/無音保持/全曲再開を記録。録音中はnative/audio回帰を実行しない。Inspect-ProductGuiTransportDlsPriorityAudio.mjs --slow --require-lifecycleと異常対照を実行する。Styleは --style-tempo 109。

次の具体策は、支持APIで22768の対象ウィンドウを取得して混入なしTransportを再収録すること。GUI障害継続時はQ3Cの既存Lyric/Marker/Muteを照合し、共通Timelineへの不足を実装する。既存CRUDやMessage Windowを作り直さない。

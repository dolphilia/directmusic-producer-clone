# Transport Optionsと音声初期化設定

2026-10-10（日本時間）。画像3/4のうち、Transport OptionsからMIDI/Performance Optionsを開き、voices・sample rate・default synth・latencyを実音声エンジンへ適用する経路を限定実装した。[一次単位](../../work/analysis/ui-performance-options/20261010T091656219Z/unit-record-v1.json)が証拠の入口。SSP01全体、全MIDI設定、原版互換、全体受入は未達。限定採用0735と独立Style015244を維持する。

## 実装と仕様根拠

3本目のTransport Optionsを追加し、Viewから独立して表示・非表示できるようにした。AudioPath選択は既存の共通コマンドへ接続した。Optionsは原版の2列構成を参考に、初期化設定、遅延とPersist、Default Synthesizerを実装した。MIDI入力/Echo/Advanced/PChannels、GM/Custom DLSのDownload/Unload、メトロノーム等は無効表示とし、未実装の残件として保持する。

正式原版5.3.0.900のOptionsを開き、64 voices・22050 Hz・55 ms・MIDI入力None・Echo None・Default Synth未指定を読み取り、Cancelで閉じた。設定適用やユーザー文書の編集は行っていない。抽出CHMのTransport Options、MIDI/Performance Options、Microsoft Synthesizerの話題とSDKの宣言を併読した。55 msへの既定復帰とPersistの意味はCHMを根拠とする。原版での設定変更・再起動・同入力再生比較は未実施。

InitAudioへ40バイトのAudioParamsを渡し、返却voices/rateとポートの実フォーマットを取得する。遅延はAudioPathのポートからIKsControlでGET/SETする。初期化項目の変更では新しいruntimeを準備してから交換し、無効な値や利用できないsynthの拒否で旧設定を保持する。設定は文書と独立したPREFファイルに保存し、検証では専用の設定パスを指定した。

## 同候補で確認した範囲

候補093938（20261010T093938365Z）はconfigure/build/install各exit0。246保存/作業ソースと4出力のhashが一致した。Producer.exeは`5e313d3b8da91c9c341e8fa3ebbf8dae8aa16888256c246f61eeaefe5644a508`。関連timeline-range/transport-audiopath/sequence-crud/playback-monitor/tempo-propertiesの5件が合格した。

[専用runtime](../../work/acceptance/performance-options/20261010T094614165Z/native/performance-options.json)では64/22050/55→遅延70→96/44100/70→明示synth→不正synth拒否→既定復帰を確認した。実ポートも22050/44100 Hzを返した。voices=0、rate=0、latency=10000の拒否、PREF往復と破損拒否、再生/Stop、文書bytes不変も成立した。多ポートや実機器を含む全設定の試験ではない。

本体では初期値、66 voicesへ変更してCancel→64に復帰、96 voices/44.1 kHz/70 ms/PersistをOK→再表示、Optionsバーだけの非表示/再表示を確認した。[保存・復元記録](../../work/analysis/ui-performance-options/20261010T091656219Z/gui-native-preferences-exit-proof-v1.json)では専用Project/Segment/Styleの3ファイルがbyte一致し、通常終了後の別PIDで170ノート/120 BPMと設定96/44100/70/Persistを復元した。設定だけを96→97へ変える別の短いセッションでもUndo/Redoを増やさず、保存操作や未保存確認なしで通常exit0となった。

[本体150秒WASAPI](../../work/acceptance/product-project-gui/20261010T100900730Z/audio-20261010T101126833Z/transport-position-gui-audio-proof.json)は、復元した設定でカーソル6336 clocksのG4と先頭C4、境界停止後と発音中即時停止後の無音を合格とした。境界要求後の最後の可聴区間は約1.04秒、QPC/UTC driftは約5.2μs、最大packet gapは2 frames。これは設定後に本体が再生できる証拠であり、物理スピーカー出力や原版音声との一致を証明しない。

最初の本体の73ロードモジュールは、保存候補・Windows構成・署名等へ照合し、原版40モジュールとのhash一致0で限定合格した。連続全ライフサイクルや原版のないQ2環境を証明するものではない。

## 失敗・比較差分・残件

最初の093018はポート検索にGUID_NULLを使いNOTFOUNDとなった。公開APIの限定診断でGUID_All_Objectsの必要性を確認して修正し、失敗候補・診断・新候補を別々に保存した。最初の180秒本体録音は、検証者がSpaceとAlt+>/Alt+<を取り違えたため不合格。正しい操作列を固定して150秒収録し直し、閾値を変更せず合格した。

長い最初のセッションには、Zoom連打後に意図しないProperties表示と終了時の未保存確認があった。入力3ファイルは不変で、検証専用状態を破棄して通常終了した。dirtyの由来は未解明で、起動時のUntitledやOptionsの副作用と断定しない。別の設定変更だけのセッションと再読込セッションは未保存確認なしで終了した。

UIAが直前値や非表示コントロールを列挙したり、モーダルのindex/キー操作が対象欄へ届かない場合があった。再取得した画像とスピン等のマウス操作で確認し、ツールの失敗を製品の成功へ転記していない。

原版Optionsは1132×700、製品は1144×664で、字体・高さ・間隔・有効状態に差がある。ツールバーも文字ラベルで、原版アイコン・配置・ドッキングは未達。原版の初期表示とOpen/Cancelだけの部分比較であり、originalComparison=false、boundedFinished/completed/fullAcceptance=falseを維持する。

MIDI入力/録音/Echo/PChannels、Download/Unload、メトロノーム等、Transport List/Transition/編集BPM、全AudioPath設定、複数ポートへの遅延適用と失敗復帰、全sample rate/voices範囲を残す。現在の遅延適用は各AudioPathの先頭ポートであり、全ポート対応を主張しない。同093938の全native96/driver170一巡は未実行。manifest再生成が95/170へ欠落した記録を保存し、既存96件と履歴を戻して新driverだけ追加した。

## 次の一手

SSP01の上記残件を保持し、独立したSSP02の画像5/6/7、3入口のNew→Create New File(s)→Segment Length→TimeSig4/4→Add New Tracksへ進む。Segment作成からSequence編集へつながる経路を先に統合し、未比較を完了へ昇格させない。全40/全8の6作業中・2障害・0合格、独立Style015244固定300秒PCM/96-164一巡、凍結条件とQ2未用意を保持する。

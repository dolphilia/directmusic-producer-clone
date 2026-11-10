# Transport の共通キー操作

候補2048は237保存/作業ソース・4出力一致、configure/build/install各0、関連11試験が合格。主画面/Segment Timeline canvasのSpaceとAlt+<を既存Play/Stopへ接続した。実本体でSpaceのPlay/再生中即時Stop、Alt+<の先頭再生/即時Stop、文字欄のSpaceと69への復元、履歴/native入力不変、元入力の別process復元、三通常exit0を限定確認した。固定音声・原版比較・全体は未達。completed=false/fullAcceptance=false。

短いSegmentは次の独立した入力セルより先に自然終了したため、停止ラベルだけでは能動Stopの成立にしない。末尾lengthのみ12288→196608 clocksへ変えたGUI専用コピーを事前登録し、元入力と全操作を保持。その実画面でPlaying→Stopped（自然終了表示ではない）をSpace/Alt+<各経路で確認した。新ウィンドウ取得のエラーは古いlexical変数が終了済みのwindow17439116を参照した検証コードの誤りだった。返却済みの新window158470588を別変数で使用して回復した。[訂正](../../work/analysis/ui-transport-keys/20261009T204823638Z/window-binding-correction-v1.json)と初回失敗を保持し、OS拒否/製品不具合にはしない。

別process11936の元短入力で64秒WASAPI録音を実行した。通常sandboxはGet-Processで検証PIDを不可視にしたが、読み取り専用CIM対照で同じPID/EXE/開始時刻を確認し、実行条件を変えて録音。実Stop時刻は初回約9.85秒、再開約8.39秒となり、固定4.2〜6.3秒を超過した。[音声判定](../../work/analysis/ui-transport-keys/20261009T204823638Z/audio-proof-v2.json)はexit1。録音・packet/QPC・実時刻・六文書観測を保存。発音中Stopや音声全体を合格にせず、閾値も変更しない。

同候補native95原始一巡は48合格/37既知障害/10失敗。読み取り専用native realpath対照で通常sandboxはEPERM、許可された実行環境は同じパスを解決した。該当9試験の別条件補足は全合格（Send89、Waves persistence42を含む）。原始10失敗を保持し、37凍結拒否と後続匿名Band失敗は再試行しない。driver164原始一巡は24合格/23障害/117未実行。[一巡結合監査](../../work/analysis/ui-transport-keys/20261009T204823638Z/milestone-proof-v2.json)は現在/保存ソース・出力・全登録・入力hashと原始/補足を確認した。旧2137一巡と0735限定採用は履歴/採用として維持。

原版HelpはSpaceを停止中Play/currentcursor・再生中StopImmediate、Alt+<をPlayFromStart/StopImmediateと記述する。現在cursorが未実装のため停止中Spaceはfromstartに限定。Alt+>境界停止/Ctrl primary-only/全Transport List/Recording/Transition/TempoModifier/stateful toolbar/fonts/iconsも残る。200%は変更せず、原版approval timeout、Q2と個別Windows5/Clipboard5の凍結を保持。

[開始](../../work/analysis/ui-transport-keys/20261009T204823638Z/start-v1.json)、[固定protocol](../../work/analysis/ui-transport-keys/20261009T204823638Z/protocol-v1.json)、[GUI/native監査](../../work/analysis/ui-transport-keys/20261009T204823638Z/gui-native-proof-v1.json)、[現行未完単位](../../work/analysis/ui-transport-keys/20261009T204823638Z/unit-record-v2.json)。次は[Farm/Script ANSI metadata開始](../../work/analysis/q3-script-ansi-name/20261009T211806287Z/start-v1.json)。原版/必要音声の未達がある間は全体完成にしない。

# Transport Controlsの順序とglyph — 2026-11-09

Current Tempo050149の停止中Segment限定機能/native保存復元/二通常exit0/実PCMを保存した後の独立SSP01作業。全体fullAcceptance=false。

CHMはTransport List→From Start→Play→Recording→Transition→Current Tempo→Tempo Modifierの順。現行製品のCurrent TempoとModifierが逆順で、From Start/Play・ImmediateStop/BoundaryStopが同形だった。埋込欄の重なり修正を維持し、幅64のTempo/spin用slotをModifierの前へ置く。先頭markerと境界markerをソース描画glyphへ追加し、状態切替後も同じ位置に保つ。録音/Transition/Listを描画だけで成立にせず、未実装として保持する。

初回054537は混在auto宣言のC3538でcompile失敗、install未実行。保存スナップショットを変更せず、HBRUSH/HPEN宣言を分けた055400を保存build中。旧050149の実行結果を新候補へ転用しない。

正式原版PID1984は起動待機の後に同じプロセスで画面を取得し、一覧選択で編集画面を変更しないこと、再生中の選択変更ではTempo132.25を保ちStop後Peer120へ切り替わることを観測した。復元workspaceには同名Project/コピーGUIDが複数あるため、初回の所有Projectの厳密同定は未達。固有Project/Segment GUIDを持つTransportProofも開き、Seed132.25の停止/再生中glyphを保存。2Segment全bytes不変、Projectmetadata不変は主張しない。境界停止クリック後も再生を観測し、最後にImmediateStop、通常exit0。境界完了や原版PCM成功にはしない。

一次入口：`work/analysis/ui-transport-list/20261109T054141550Z/unit-record-v2.json`、`original-observations-v1.json`。次：055400 build→関連回帰→本体glyph/順序/View再配置/最大幅1000.00/Tempo編集とUndoRedo→native保存/通常終了/別process復元/通常終了→2つのPlay入口とImmediateStopの固定PCM。Transport Listの独立primary選択/編集owner、録音/Transition、厳密外観、全40/全8/Q2/Style015244/既知OS凍結を残す。

## 055400の結果

055400でCurrent Tempo→Modifierの順序、FromStart/PlayとImmediateStop/BoundaryStopのglyph識別、Standard非表示/再表示後の入力・CtrlS、最大1000.00、UndoRedo/spinを確認。関連5/496、8Seed+8Peer native状態、291保存/作業ソース・4出力一致。作者6156/別2868の二通常exit0、別起動復元/再保存成立。2つのPlay入口の固定132.25 PCMは17発音/音程/Stop/無音/replay合格。

録音時計の負例4件拒否、最初/再再生とも発音間隔中央値0.455/0.455秒。判定器は050149で負例5件検証したv4と同一bytesを使い、新候補の録音自体を独立判定した。音声後のSeed/Peerも作者保存と全bytes一致。Project再保存差はfilh FILETIME16..23のみを許容し、他bytes一致。中心hit診断は各control自身に一致。

候補BoundaryStopボタンの要求後にGUI停止へ戻ったが、原版は観測内で境界停止を完了しなかった。境界の全条件・タイミング同等や境界PCMは主張しない。新候補の全native100/driver172を再実行したとは扱わず、直前050149の機能群一巡50合格2830検査/49凍結/匿名Band1保留は候補を区別して保持する。

一次入口：`work/analysis/ui-transport-list/20261109T054141550Z/unit-record-v3.json`、`gui-proof-v1.json`、`native-proof-v1.json`。Transport全体と厳密外観は未完。

SSP01 Transport Listを実装する。編集対象とprimary再生選択を分け、停止中の選択はTempo表示へ反映し、再生中は現在primaryのTempoを維持する。編集窓の再アクティブ化でprimary選択を更新。Segmentの限定登録/選択/文書owner/FromStartとPlay/保存二process/必要PCMを閉じ、録音/Transition/他component/厳密外観は残す。既知OS凍結は条件変更なしで再試行しない。 全体fullAcceptance=false、goal active。

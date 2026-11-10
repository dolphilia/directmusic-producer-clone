# Sequenceノートの数値Properties・限定実装

2026-10-12（日本時間）。151135：SequenceノートのF11/右クリック/ボタンPropertiesと15数値欄を実装。原版の同入力15値一致、pitch/velocity/Start/End/Duration編集、並び替え後Undo/Redoの選択追従と不正値拒否を確認。289ソース/4出力一致、関連4/391、本体11条件・全バイトnative監査7回とProject filh限定更新、作者20000/別8584の二通常exit0。74 modules=68x86+6WOW64、原版40hash一致0は点観測。旧compile失敗/旧GUI Undo誤選択を保存。複数ノート/Belongs to Beat/変拍子の原版編集/厳密外観/親ドラッグ比較は未完。 全体fullAcceptance=false。

対象は画像14・18〜20に関連する補足操作。画像20のEdit Velocity全体受入とは分ける。契約は正式原版5.3.0.900の同一SnapSeed入力とCHM sequencepropertieswindow.htm / editingnotesbyusingthepropertieswindow.htm。原版ではC#5、velocity80、Start1:4:4:-96、End2:1:3:-96、Duration0:0:3:0を観測し、候補でも15値が一致した。200%表示は維持。

F11、ノート右クリックProperties、下部Propertiesボタンから同じページを開ける。単一ノートの音階・octave・velocity・Start/End/Durationを既存文書の編集/保存/historyへ接続した。最近傍gridと符号付きtickで表示し、開始時刻の変更は長さ、終了時刻の変更は開始時刻を保持する。曖昧な履歴選択では別ノートを推測せず選択を解除する。

検証ではvelocity80→72、C#5→D6、End Grid3→4、Duration Tick0→1、Start Bar1→2を操作。最終ノートはtime5856、duration769、pitch74、velocity72となり、並び順index1→4に追従。Undoでtime2784/index1、Redoで5856/index4へ戻ってもD6/72を保持した。velocity0は72へ戻り、保存データ不変。別process8584で同じ15値を復元し、作者20000と共に通常exit0。

![再読込後のSequence Properties](../../work/analysis/ui-note-properties/20261011T143657802Z/v3-reload-07-settled-1.png)

独立auditorは製品parserを使わず、基準RIFFの1イベント20byteと安定整列だけを期待値として変更し、SGPファイル全体を比較した。7回すべて合格、他7ノート・offset/status/拡張bytes・他chunkを保持。Projectは既存filhのFILETIMEと実サイズのみ更新。関連common-open53/timeline-range117/sequence-crud191/sequence-audition-runtime30、合計391合格。core dispatcherや登録modeの追加なし。音声の合格はこの単位では主張しない。

失敗履歴を保持：144200はtest headerのnamespace内includeによりcompile失敗しinstall未実行。144905はbuild/関連384が通ったが、GUI Undo時に別のE5/96を表示したため不合格。151135で選択の再同定とUI更新を修正し、同じ本体手順で再確認した。旧失敗を成功に書き換えていない。

一次証拠は [work/analysis/ui-note-properties/20261011T143657802Z](../../work/analysis/ui-note-properties/20261011T143657802Z/unit-record-v1.json)。build/保存ソース/出力/回帰/各画面/native/通常exitのhash参照をunit-recordにまとめた。74モジュールの照合は観測した作者processだけであり、独立Windows Q2ではない。

未達：Multiple selected note Properties and Belongs to Beat logical position are not implemented in this bounded core。Original editing semantics for all 15 fields, variable meter, logical preparation offsets and exact appearance remain comparison-unverified。Parent SSP04 left/right/body original gesture attempt did not edit notes; parent remains open; do not repeat unchanged attempts。Current151135 full native100/registered-driver172 rounds unexecuted; previous candidate rounds not transferred。Current numeric UI scope has no PCM/audition/audio acceptance; prior audio stays candidate-specific。Old Band Stop/audio failures, Count-In15/16, Echo idle/Advanced/physical MIDI, SSP08 remain。Independent Style015244, Q2 and frozen Windows failure conditions remain; all40/all8 incomplete。

次：Add New Tracks（画像7）の初期focusと空選択OKを正式原版で観測し、記録済み候補経路と比較する。初期選択の差があれば修正し、新候補で関連回帰・本体・native保存/別process復元/二通常終了を確認。Sequence複数選択/Belongs to Beat/変拍子と原版端/中央ドラッグ比較、旧Band停止失敗・Count-In15/16・Echo/SSP08・独立Style015244/Q2/OS凍結を保持。

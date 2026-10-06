# Q1 Fresh native Project Style / Transport（2026-10-07）

候補 `20261006T154607013Z`、194保存ソース一致。Producer SHA256 `b232c90206c224b2edbb7582f032209977a22536a54fcfec07e689d85ec43ab4`。構成・compile・installは同じ不変候補の既存exit0。全40責務・全8受入は未完了。

新規Fresh Projectの前単位を継承し、Control/Empty/Routeを本体で所有してnative保存、Sequenceを5→7.5 BPMへ編集保存。EmptyへのCommandGroove50とBand埋込、Style固定MIDI音符0→60を本体で修正した。失敗2録音と入力は保全し、失敗を合格へ書換えていない。作者23044、修正作者21816、Style修正作者の終了記録と最終別起動17680 exit0を結合した。Style保存済み終了時の汎用破棄確認も記録し、そのPIDでProject新保存を実施したとは扱わない。

Styleは別起動で10音のMIDI60、Segment120 BPMによる2秒間隔、停止後無音をWASAPIで確認。無変更を受理し、無音・欠落音・誤音高・誤テンポ・baseline混入の5異常を拒否。判定器の固定gui.pro名は、起動Projectと録音入力の一致およびnative JAZP検査へ置換した。一般の和声musicValue互換は未完了。

同じ最終PID/ProjectでConflict既定経路の非接続ControlはRMS0、埋込RouteのSequenceは全8音を発音。間隔12,12,12,12,8,8,8秒、NoteOff前1.5～2.5秒のDLS持続、発音中Stop直前RMS0.065322、停止後RMS0、全曲再開を確認。無変更を受理し、10異常を拒否。長持続判定器2本は短いgateを測定前に拒否する契約を追加した。

専用75件は同一不変候補の45合格/30障害を保持。新しい107driver一巡はraw20合格/21障害/66未実行、同候補の明示PCM対照3件を接続した分類は23合格/21障害/63未実行。全8状態は6作業中/2障害。原版動的比較、Q2独立原版なしWindows、通常core保存Windows5、全40責務と最終同一構成受入は残る。

次はQ3A native Project per-file Runtime folder/nameの保持・保存復元。既存実装と原版の利用可能なRuntime Propertiesを照合し、一契約をnative回帰→本体保存→通常終了→別起動まで完結させる。追加Style/AudioPath細部はqueueに残す。

単位・全入力hash・終了記録・失敗対応表・PCM/対照・再現コマンドは [unit-record](../../work/analysis/q1-style-transport/20261006T165221Z/unit-record.json)、全体正本は [product-state](product-state.json) / [acceptance-status](acceptance-status.json)。

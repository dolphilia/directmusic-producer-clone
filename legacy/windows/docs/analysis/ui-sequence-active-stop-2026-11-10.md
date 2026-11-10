# Sequence長音の演奏中停止 — 2026-11-10

131516同一候補の独立Sequence長音単位で既存1note終端を第16小節(duration46272、30.125s)へ変更。Undoは元1638bytes全一致/Redoは長音全一致、作者3352/復元16176は二通常exit0、別復元再保存/空UndoRedo。240s PCMは18/18、cold試聴0.825/warm0.520/通常Play0.485/replay0.465s、停止25.001/25.015s、音高72/発音/停止後無音/有限PCM・clipを確認。負例6とclock4全拒否。APIはprivate2/primary2の各Play/Stop/explicitstart、音響判定は別証拠。native9段階でdurationLONG1値だけ変更、二control17notes全bytes・3Project参照/GUID/entryID/FILETIME以外・音声前後/終了後4filesを保持。292saved/working/4outputs一致、新buildなし、同候補native49/2785参照。

旧Sequence最小単位の二180s/6s Stopは時刻不合格のまま。今回はtool/model境界17.172sを先に観測し、新単位で30.125s持続音と25s Stopを録音前固定した。旧6s受入へ転用しない。1s開始/音高/RMS/静音/clip/clock閾値不変。準備状態書込のcp932エラーは失敗bytes保存→直前backup exact復元→UTF8更新済み。runtime監査v1のstage-only既定0fieldsを72/96と仮定したassertは、source call record(stage)に従うv2へ訂正し、count/HRESULT/PCM/native閾値を変更していない。

一次入口：`work/analysis/ui-sequence-active-stop/20261109T144604000Z/unit-record-v2.json`、native/runtime/PCM/control証拠は同directory。

正式原版の同入力Sequence比較を独立継続する。今回保存済み4track/Organ1/MIDI72長音の専用別コピーを原版で開き、正式EXE/登録先/PID/1280x800・96DPIを再照合。既知cold startup6〜7分は同一PIDで待ち重複起動/forcekillなし。Tree/Timeline/part/keyboard/楽器メニュー/Properties/endBar/Close・再open・保存native・通常終了を比較し、必要な実装修正だけ保存build/関連回帰/本体/保存別復元へ進む。全Sequence gestures/CC/複数part/全外観、Band完全同一論理時刻、全40/全8/Q2/Style015244は未達。50native30driver凍結と匿名Band同条件再試行なし、fullAcceptance=false。 goal active。

# Sequence最小操作と保存復元 — 2026-11-09

131516同一候補でSequence追加/part開閉/Piano1→GS Cancel→Organ1/Band子editor/Insert/Properties MIDI72・endBar6/UndoRedoを本体で確認。初期402→Sequence486→Band1618→1note1638bytes。14保存段階でduration192↔15552、1note/velocity96/PCh0、Band1同GUID・他15inactive、既存TimeSig/Tempo/外側GUID/opaqueを保持。二control各17notes全bytes、ProjectはSegment1sizeとFILETIMEだけ。作者11720/復元7596/再検証2528は三通常exit0、別復元再保存/空UndoRedo/音声前後4files全bytes一致、292saved/working/4outputs一致。新buildなし、同候補native49/2785参照。

二180s PCMの音声開始/音高/静音は成立したが、固定6秒[5.5,6.5]のStop操作時刻が不合格。初回は時刻変数bindingの入力前エラー後に63.687s cold、Stop7.699/6.001s・最後129.697sでcoverageも不合格。再検証はbinding準備を先行したが、Play画像確認から次入力まで17.172sかかり初回音符が自然終了、再開Stopも7.122s。失敗証拠・監査コード/閾値を保全、6秒受入/負例clockは合格へ昇格しない。

一次入口：`work/analysis/ui-sequence-minimum/20261109T140607429Z/unit-record-v2.json`。初回/再検証PCM proofとactual actionsは別保全。正式CHM NoteEditingWindows/SequenceTrack/NoteEntryと固定画像13〜16・21〜24を契約として照合したが、正式原版同入力動的比較・全パレット/GS/gesture/CC/複数part/font/geometryは未達。全40/全8を合格へ変更していない。

短い6秒操作条件の反復を止め、同じ131516の独立したSequence activeStop単位へ進む。専用コピーでEndBar16(duration46272、約30.125s)を本体編集/UndoRedo/保存/通常終了/別復元し、録音前に固定する240s PCMでcold/warm試聴・25s演奏中Stop・再開/quiet・負例6/clock4を検証する。1s開始/音高/静音/clip閾値を維持し、操作時刻条件の変更を新単位に明示、旧6秒失敗は未達で残す。原版同入力/全外観/全40/全8/Q2/Style015244、完全同一論理時刻Band等は未達。50native/30driver凍結と匿名Band同条件再試行なし、fullAcceptance=false。 goal active。

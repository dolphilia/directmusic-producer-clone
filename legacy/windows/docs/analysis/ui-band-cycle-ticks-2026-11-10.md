# 同じ拍の異なる物理tickを持つBandの巡回 — 2026-11-10

131516同一候補の追加Band入力で、同じ表示拍・異なるphysical0/96/191tickの3Bandを斜体1表示にまとめ、作者/復元それぞれC→A→B→Cへ一巡（計6calls/2wraps）。右clickで現在選択を保持、Properties A/B/CはStart1:1のtick0/96/191とBelongsTo1/2/3拍を照合。Solo/空白はCycle無効、dirtyなし/UndoRedo無効。作者13564/復元16048は二通常exit0、全3Segment bytesと4Band名前/GUID/時刻/16inactive・二control各17notesを保持。3ProjectはFILETIME以外同一、終了前後4filesも全一致、292saved/working/4outputs一致。source/build/PCM変更なし、現候補native49/2785参照。

今回の入力は旧Band Cycle固定出力の4physical LONGだけを独立変更したfixtureで、原版同入力動的比較・GUI作成の証拠ではない。旧単位の子editor所有先は別証拠、今回子editorは未実行。完全同一logical時刻の読込/移動/更新の拒否は残し、全重複対応とはしない。

一次入口：`work/analysis/ui-band-cycle-ticks/20261109T151300000Z/unit-record-v2.json`、`gui-proof-v1.json`、`native-proof-v1.json`。

正式原版のSequence比較はPID1164のunexposed registry更新通知でuserのOK操作回答待ち。同一PIDを保持し重複起動/forcekill/登録再変更/架空targetなし。独立してBand完全同一logical時刻のcore/API/選択・owner・履歴/opaque保護を一次契約と照合し、根拠ある範囲を実装・保存build・未凍結関連回帰・本体操作・保存通常終了・別復元まで進める。原版回答後はSequence同入力比較を再開。全Sequence gesture/CC/複数part/全外観・全40/全8/Q2/Style015244は未達、50native30driver凍結/匿名Band同条件再試行なし、fullAcceptance=false。 goal active。

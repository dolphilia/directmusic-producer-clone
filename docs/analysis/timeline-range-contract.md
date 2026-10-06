# 共通Timeline範囲のソース版契約

対象はTempoと選択された一つのSequence。責務全体は未完了。原版helpの複数stripと時間領域の選択、Copy/Cut/Delete、Paste Merge/Overwriteを入口にする。原版の動的clipboard観測はTempoの過去単位に限定され、今回の原版Main比較は既存起動障害のため未実行。

ソース版ではclock範囲を[begin,end)として表示し、先頭と末尾の空白をspanとして保持する。これは今回の明示的な内部仕様で、原版のclock境界・anchor互換を確認済みとはしない。内部RIFF:TRNG、version1、strip mask、span、Tempo copy stream、Sequence timed recordsを所有し、原版COM/OLE format互換は主張しない。

SequenceはSDK DMUS_IO_SEQ_ITEM/DMUS_IO_CURVE_ITEMのtime+signed offsetで所属を判定し、raw record全体と拡張bytesを保持する。controller/curveも同じ選択に含める。未知subchunkはコピー元・貼付け先に保持し、時間意味が不明なためclipboardへ勝手に複製しない。空destinationはsource strideを受け入れる。既存の異なるstrideは情報を捨てず拒否する。開始時刻overflow、ノート/curveの文書末尾超過、破損・重複envelope、strip不一致は文書と履歴を変更しない。

TempoとSequenceの編集は一時文書で検証し、成功時に文書全体の一つの履歴として確定する。選択だけでは保存内容やdirtyを変更しない。MergeはTempoの既存equal-time置換契約とSequenceのrecord追加、Overwriteは同じclock区間の削除後に挿入する。重複Tempoの原版互換は未解決として残す。

本体Timeline Range画面は選択strip、数値/drag範囲、Copy/Cut/Delete/Merge/Overwrite/Undo/Redoを提供し、既存Segment保存とnative Projectへ接続する。全strip、複数同種stripの縦順対応、外部MIDI clipboard、OLE drag/ABI、Ctrl+V共通routing、原版境界比較はqueueに保持する。

現候補20261005T205038896Z（174保存ソース）は専用30/通常core0、Main各履歴/native Project/別PID復元を確認。2回の自然終了Playの音程/120BPM間隔だけPCM合格で、activeStopとテンポ変化は未確認。単位と主要hash・再現コマンド：[report](../../work/analysis/q3-timeline-range/20261005T210000Z/report.md)。原版比較と全40/全8は未完了。

## Selected-range relocation (source provisional, 2026-10-07)

`move_range(at)` uses an absolute destination clock and the selected half-open span. It copies the selected Tempo/Sequence events, deletes them on a private document, and merges at the destination. Overlapping moves are allowed; the existing Tempo equal-clock replacement policy and Sequence append policy still apply. Outside events are retained except a destination Tempo collision. It commits exactly one document history edit, selects the destination range, and retains exact document bytes, dirty checkpoint, selection and Redo for unchanged/invalid/empty/overflowing or incompatible moves. Note duration and curve endpoints must fit inside the Segment. Extended event records and opaque destination chunks use the existing copy/paste preservation rules.

The main Timeline dialog provides a Move Selection button using Target at, plus a Move by drag checkbox (or Alt-drag) inside the selected region. With Move by drag off, ordinary dragging continues to select a range. Clock resolution is one tick; original Snap To increments, all strip types, drag cursor/preview, OLE and exact original range-move overlap semantics remain unverified responsibilities. Local original help `selectingregionsinsegmentsandpatterns.htm` describes selected tracks plus timeline regions and cut/copy/delete; `toselectanitemanddragit.htm` describes dragging a selected item to an accepting location. These help pages do not establish this source overlap policy. Dynamic original designer comparison remains blocked by the already recorded missing designer; this limited source contract is not parity acceptance.

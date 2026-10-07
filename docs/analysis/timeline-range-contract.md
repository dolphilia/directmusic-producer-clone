# 共通Timeline範囲のソース版契約

対象はTempo、選択された一つのSequence、選択group内の明示nth Lyric/Marker/Mute。責務全体は未完了。原版helpの複数stripと時間領域の選択、Copy/Cut/Delete、Paste Merge/Overwriteを入口にする。Lyric単独properties/保存の原版観測はあるが、今回の原版designerが開かず、動的bulk比較は未実行。

ソース版ではclock範囲を[begin,end)として表示し、先頭と末尾の空白をspanとして保持する。これは明示的な内部仕様で、原版のclock境界・anchor互換を確認済みとはしない。内部RIFF:TRNG、version1、strip mask、span、Tempo copy stream、Sequence timed records、任意Lyric lyrcを所有し、原版COM/OLE format互換は主張しない。

SequenceはSDK DMUS_IO_SEQ_ITEM/DMUS_IO_CURVE_ITEMのtime+signed offsetで所属を判定し、raw record全体と拡張bytesを保持する。controller/curveも同じ選択に含める。未知subchunkはコピー元・貼付け先に保持し、時間意味が不明なためclipboardへ勝手に複製しない。空destinationはsource strideを受け入れる。既存の異なるstrideは情報を捨てず拒否する。開始時刻overflow、ノート/curveの文書末尾超過、破損・重複envelope、strip不一致は文書と履歴を変更しない。

各stripの編集は一時文書で検証し、成功時に文書全体の一つの履歴として確定する。選択だけでは保存内容やdirtyを変更しない。MergeはTempoの既存equal-time置換契約とSequence/Lyricのrecord追加、Overwriteは同じclock区間の削除後に挿入する。重複Tempo/Lyricの原版bulk互換は未解決として残す。Lyricの範囲所属はphysical、移動差分はphysical/logicalの両方へ適用する。両結果がSegment内に収まらなければ全stripのbytes・dirty・Redo・selectionを保持して拒否する。Unicode、配送値、拡張header、未知entry child、text末尾bytesを保持する。空範囲はspanを保持し、空貼付けで欠けたtrackを生成しない。

本体Timeline Range画面は選択strip、数値/drag範囲、Copy/Cut/Delete/Merge/Overwrite/Undo/Redoを提供し、既存Segment保存とnative Projectへ接続する。全strip、複数同種stripの縦順対応、外部MIDI clipboard、OLE drag/ABI、Ctrl+V共通routing、原版境界比較はqueueに保持する。

現候補20261006T213433891Zは195保存ソース、Timeline93/Marker70/Mute55/Lyric文書48/Sequence26合格。本体五strip移動・一回Undo/Redo・native Project保存・作者20544→別復元22036を確認し、両processは通常exit0。移動対象7 DWORD以外のSegment bytesは一致し、別復元後の再保存も完全一致。Projectはfilh FILETIMEのみ更新。このシナリオのPCMは未実行。単位とhash・再現コマンド：[report](q3-marker-mute-range-2026-10-07.md)。旧三stripの作者exit取得拒否と旧PCMは版別履歴に保持し、現候補へ転用しない。原版比較と全40/全8は未完了。

## Selected-range relocation (source provisional, 2026-10-07)

`move_range(at)` uses an absolute destination clock and the selected half-open span. It copies the selected Tempo/Sequence events, deletes them on a private document, and merges at the destination. Overlapping moves are allowed; the existing Tempo equal-clock replacement policy and Sequence append policy still apply. Outside events are retained except a destination Tempo collision. It commits exactly one document history edit, selects the destination range, and retains exact document bytes, dirty checkpoint, selection and Redo for unchanged/invalid/empty/overflowing or incompatible moves. Note duration and curve endpoints must fit inside the Segment. Extended event records and opaque destination chunks use the existing copy/paste preservation rules.

The main Timeline dialog provides a Move Selection button using Target at, plus a Move by drag checkbox (or Alt-drag) inside the selected region. With Move by drag off, ordinary dragging continues to select a range. Clock resolution is one tick; original Snap To increments, all strip types, drag cursor/preview, OLE and exact original range-move overlap semantics remain unverified responsibilities. Local original help `selectingregionsinsegmentsandpatterns.htm` describes selected tracks plus timeline regions and cut/copy/delete; `toselectanitemanddragit.htm` describes dragging a selected item to an accepting location. These help pages do not establish this source overlap policy. Dynamic original designer comparison remains blocked by the already recorded missing designer; this limited source contract is not parity acceptance.

## Marker/Mute共通範囲（ソース暫定契約）

Markerはplay/enterの両種を時刻[begin,end)で選択し、raw recordの先頭clockのみを同じ差分で移す。原版で見た同内容二重MARKは全copyを一致させ、未知兄弟・record拡張・paddingを保持する。MuteはDWORD PChannel/mapと同じhalf-open時刻選択を使い、Segment末端restoreを対象外として保持する。MergeのMarker重複は既存契約どおり保持する。Muteの同PChannel/同時刻衝突、予約PChannel/map、範囲外・破損・重複clipboard・populated stride不一致は、全strip bytes/dirty/selection/Redo不変で拒否する。空destinationはsource strideを採用し、空clipboardは欠けたtrackを生成しない。Overwriteは対象区間の削除後挿入、一括moveはprivate削除/mergeを一つの履歴へ確定する。

内部TRNG v1へ任意mark/mutcとmask8/16を追加する。既存mask1/2/4のbytesと読み込み契約を維持する。nth Marker/Muteはselected groups内の明示選択。本体は各stripのcheckbox/nth入力、描画、共通clipboardとmove/historyを提供する。Muteの区間編集で自動的な境界restoreを捏造せず、イベント単位移動として定義する。原版bulkの同clock衝突・overlap・anchor・range clipboard動的比較は未観測であり、原版互換合格とはしない。既存原版単一Marker観測とMute相互編集/Sequence音声比較を再実装する指示として扱わない。

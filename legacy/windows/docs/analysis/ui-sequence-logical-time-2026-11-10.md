# Sequence の所属拍 — 2026-11-10

180733499はSequence論理位置読出/物理onset不変のlogicalLONG・signedSHORT変更/Document原子的history/Properties所属小節・拍/論理位置を含むUndo選択を実装。9source変更283同一/292savedworking4outputs、関連4モード566checks（新35）合格。作者19988で所属1:1→1:2でもC6・96/Start1:1:1:0/End16:1:2:0/Duration15:0:1:0を保持、Undo保存は初期全bytes、Redoは期待全bytes。復元7080で所属1:2/物理値/再保存、二通常exit0。native差は単一record logical0→768/offset0→-768（1972bytes中2bytes）のみ、他全native/control/track/opaque/GUIDとProjectFILETIME以外を保持。

新固定240s録音は18/18、負例6/clock4を全拒否し元WAV保持。初回/再試聴onset0.585/0.585s、先頭再生/再開0.475/0.815s、activeStop25.006/25.007s、C6/発音/停止後静音合格。作者試聴0/Play0、復元private2/primary2/explicitStarts4のAPI結果0を別監査。旧6秒停止不合格は保持し今回結果を転用しない。

原版019の初期所属1:1とCHM/SDK契約を根拠にし、原版offset編集正規化/variablemeter/loop包含/原版別process復元の全同等性は未達。ツールtype_textは空欄になり通常数字キーで確定、paste/IME/spin全操作合格とはしない。全geometry/globalTempo/全40全8/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-logical-time/20261109T180300000Z/unit-record-v3.json`。

Tempoが全グループに適用される原版/CHM契約を次に監査する。正式原版はgroup1入力を1-32表示/0xffffffff保存へ正規化する一方、製品はselected_groupsでTempoを選別。native保持と意味上のglobal範囲、複数Tempo track/index/Timeline/CurrentTempo/runtimeの既存依存・未凍結回帰を照合して独立単位を設計・実装する。Sequence loop包含/原版offset動的比較も未達として保持し、同条件既知OS障害の再試行はしない。fullAcceptance=false、goal active。

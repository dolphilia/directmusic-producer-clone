# 正式原版 Sequence 同入力比較 — 2026-11-10

正式原版PID1164はuserの通知OK後に専用Project絶対path/43登録/正式EXE/1280x800・96DPIを照合し、Sequence C6・96・終端16/所属拍1:1/楽器GS Cancel/Band子owner/保存・close再open・通常exit0まで比較した。原版保存は1638→1972bytes、seqt/tetr/tims全bytes・外側/BandGUID/名前/二control17notesを保持する一方、Tempo group1→全32、track順、Band priority DWORD36、設計chunks/Project author・rfld/open/nodeを原版側で正規化。全runtime/opaque不変とはしない。

- Tempo, Time Sig, Sequence, Bands; current candidate TimeSig,Band,Tempo,Sequence。Scoped view ordering; preserve native order/opaque bytes; original save also normalizes serialized order.
- Original 1-32: Tempo and plural 1: Bands; candidate group1 Tempo and singular Band。Separate display/native semantics: original saves Tempo group0xffffffff; do not silently mutate existing group1 native input for label parity.
- One keyboard arrow step changes one semitone; Part header has vertical zoom pair; candidate arrow changes12semitones and only horizontal footer zoom。Scoped Part pitch viewport/vertical zoom implementation and native non-edit verification.
- Original Note Properties has Belongs to Beat 1:1 in addition to start/end/duration; candidate omits this。Reconcile native offset contract before implementing editable logical beat.
- Original child title Band: Segment1 - Band1; default sixteen inactive/active equal stored pan64/volume100 overlap graph (16 visible), candidate spreads inactive rows over grid and omits Segment owner in title。Scoped owner-title and graph position parity; preserve active/selection identity and defaults.
- Original narrow bold labels/short rows and Part note area begins in Part header; Sequence context includes A/Z zoom and F11 Properties。Remaining geometry/font/menu scope; no whole appearance acceptance.

行順Tempo→TimeSig→Sequence→Bands、Tempo1-32/Bands表記、Part縦zoom/鍵盤1半音scroll、Sequence所属拍、Band子ownerタイトル/同pan-volume重なり、行高・ラベル幅・fontが製品との差。新候補153816119のBand同一logical本体/別復元・native監査を完了後、根拠ある外観操作修正を独立単位で進める。原版新PCM/別process原版復元/全外観/全40全8は未達。旧6s音声失敗を維持、50native30driver凍結/同条件再試行なし/fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-original/20261109T150614000Z/unit-record-v3.json`。比較は38captures/native監査、再生なし。goal active。

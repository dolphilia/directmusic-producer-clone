# Sequence Part の音域と縦ズーム — 2026-11-10

165309981はPartごとのpitch viewport、縦zoom虫眼鏡2個、native鍵盤scrollbarを実装し、paint/hit/pitch・velocity drag/CC境界を共通化。3source変更289同一/292savedworking4outputs、関連3モード494checks（新16）合格。作者10256/復元17884で矢印1半音（pos57→56）、zoom12→13→12（page18→17→18）とCC/Bands位置保持、作者page55→37→55/つまみ55→45、zoom後既存C6・96・終端16の右clickProperties、折畳3部品hide/再展開音域zoom保持、空UndoRedo/保存通常終了/別復元再保存・二exit0を確認。三Segment全bytes・opaque設計情報/全Track順/GUID/二control17notes保持、ProjectはFILETIME以外一致。

MIDI両端/全zoom境界/Part独立/drag scale/native pitchUndoは既存未凍結sequence-crud内の新契約、実GUI全drag/keyboard端点/複数Part/全zoomは未実行。UIA scrollbar百分率をrawpositionとせずGetScrollInfo別証拠。default12px/固定216pxは原版全geometry/defaultscale合格ではない。表示のみで新PCMなし・旧音声合格転用なし・旧6s失敗保持。全40全8/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-pitch-view/20261109T164346000Z/unit-record-v3.json`。

正式原版029で確定したBand子editor title「Band: Segment1 - Band1」に所有Segment名を含める修正を次の独立単位で進める。Band payload不変の早期returnでもowner名変更を反映し、本体/保存通常終了/別復元/native不変まで検証する。Band graphのinactive rack対storedpan-volume配置とactive flagsは原版/CHM/SDKを別照合し、今回に混ぜない。Sequence所属拍・globalTempo・全外観は残す。既知OS凍結の同条件再試行なし/fullAcceptance=false。 goal active。

# TimeSig-only Segment の追加通知 — 2026-11-09

122735はTimeSig-only Segmentの編集窓open/reopen時にAdd New Tracksを表示し、既存窓の再活性化では出さない。NewFiles初回はLength Cancel後の通知が1回だけで、Cancel一度で編集へ戻る。関連3件168検査、空11332/作者11348/復元20460は三通常exit0。3文書の保存/別process復元/再保存を確認し、全Segment bytes・20小節TimeSig-only・Seed132.25/Copy120と各17音符・3distinctGUIDを保持。3ProjectはFILETIME以外同一、292ソース/4出力一致。原版15872固定証拠のopen/reopen/Cancelと一致。音楽変更なし、新PCMなし・過去音声受入転用なし。

変更はmain.cppとSegment timelineの戻り値。編集窓を実際に作成した場合だけ、全native LIST:trklがTimeSig1件かを判定する。選択groupによる他Trackの見落としを避ける。NewFilesの既存初回通知は専用notificationで二重化を防ぎ、Cancelは文書を変更しない。4トラックを持つSeed/Copyでは追加通知を出さず、それぞれ132.25/120を表示した。

正式原版5.3.0.900/PID15872の固定観測を比較根拠とする。原版の保存時は234→406bytesへmetadata正規化したため、原版全bytes一致は主張しない。候補の初期/作者/復元では3Segment全bytes一致。原版の全空Track変種・全17種追加・空選択OK・字体/全geometry/docking比較は未達。

今回の関連対象3件は全合格。前114031のcomponent-nameは67検査後Names.proのatomic replaceをWindows5で拒否され、50件目の凍結として保持する。この単位の3文書GUI保存はそのCLIの代替受入ではない。全4件238合格とはしない。

一次入口：`work/analysis/ui-segment-reopen-tracks/20261109T122451800Z/unit-record-v2.json`、`gui-proof-v1.json`、`native-proof-v1.json`。

SSP02のAdd New Tracks→Band/Tempo追加・UndoRedo・120既定値・保存復元を独立に継続する。現候補の保存ソースを保ち、既存実装/画像7〜12と原版の限定比較証拠を先に照合し、未検証の操作から進める。空選択OKは既知CtrlSpace/Space不成立の同条件再試行をせず未比較として保持。前114031のcomponent-name関連終了条件はWindows5で未達のまま、今回の専用GUIで代替合格にしない。全40/全8/fullAppearance/fullAcceptance=false、native凍結50/driver30・匿名Band同条件再試行なし。 goal active。

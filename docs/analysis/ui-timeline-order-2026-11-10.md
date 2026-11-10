# Timeline の行順とラベル — 2026-11-10

162522992はTimelineの既知4種類を表示slot内でTempo→Time Sig→Sequence→Bandsへ安定配置し、Time Sig/Bands表記と実group maskの連続範囲表記を修正。1source変更/291同一/292savedworking4outputs、関連3モード478checks合格。原版保存由来1972bytesの独立入力はnative順TimeSig/Band/Tempo/Sequenceのまま、本体はTempo1-32先頭/各行Properties所有先/Organ1/C6・96・終端16を確認。作者17072/復元16084で二通常exit0、空UndoRedo/再保存/三Segment全bytes（全track順/headers/設計chunks/GUID/二control17notes）一致、ProjectはFILETIME以外全一致。

原版と同入力の比較結果は別単位で保持し、今回入力は原版保存1972bytesのtrack containersだけ並べ替えたfixture。原版がTempo group1を全32へ正規化するglobal semanticsは今回未修正、実mask表記のみ。初回Tempo文字クリックは既存hit範囲外で選択なし、時刻0位置選択で確認。未知kind/同kindの表示保持は実装方針であり全組合せの原版動的比較は未実行。全geometry/font/docking/Partzoom/所属拍/Bandgraph/全40全8は未達。新PCMなし/旧6s失敗保持/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-timeline-order/20261109T162156000Z/unit-record-v3.json`。

原版015/016とCHM tupiano/zoom仕様に基づき、Part鍵盤矢印1半音とPartごとの縦zoom・paint/hit/drag/CC境界を次単位で実装し、保存build/関連/本体/保存通常終了/別復元/native不変を進める。Sequence所属拍はmtTime+nOffsetの明示API/原版比較/音声を要する別単位、Band子ownerタイトル/graphとglobalTempoも残す。既知OS凍結の同条件再試行なし。fullAcceptance=false。 goal active。

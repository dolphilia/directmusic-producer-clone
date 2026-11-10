# Chord/Scale鍵盤とroot転調の本体接続

2026-10-10（日本時間）。試験候補004938、240保存/作業ソース・4出力一致。限定採用0735は維持。completed=false、fullAcceptance=false。

原版helpを根拠に4組のChord/Scale鍵盤、root上下spin、Sharps/Flatsを共有Propertiesへ接続した。levelごとの選択を共有SubChordから分離し、44-byte Chord/24-byte SubChord拡張・reserved・未知chunkを保持する。初回003350の上spinが逆向きになる本体不具合を保存し、通知deltaの方向を修正した。

修正版ではlevel1のChord B追加、Scale B除去、青い欠落音表示、他level不変、root12→13→12→13、文書UndoRedo、Tempo/拍子へのProperties追従を確認した。CtrlS/Save Project後の作者2424と再読込16544は通常exit0。Sharps/Flats変更はnative bytes不変。再起動時はSharps表示に戻り、原版の保持方針は未確認。

[独立native監査](../../work/analysis/ui-chord-scale/20261010T002919Z/unit-proof-v2.json)は714照合。関連回帰17は原始11+不足6の別runを結合したもの。公開API13は音高変換の証拠でありPCMとは区別する。[節目監査](../../work/analysis/ui-chord-scale/20261010T002919Z/milestone-proof-v3.json)は1480照合で候補・保存入力・画像・六通常exit0・録音・一巡結果を結合した。native原始96は58合格/37既知凍結/既存匿名Band失敗1、driver原始164は24合格/23障害/117未実行。旧2309の原始失敗と補足は履歴として保持した。

音声入力は本体でroot12から13へ保存。最初の録音は操作が区間外となり失敗、次はBand trackのない検証入力で完全無音となった。実生成ノート比較ではCommand有無ともMIDI61が生成され、Command欠落仮説は否定した。本体でStyleのGM Strings Bandを時刻0へコピーし、作者5420/再読込7724を通常exit0で閉じた。独立監査でBand追加以外のSGP/Style bytes不変を確認した。

[Band接続後PCM](../../work/acceptance/product-project-gui/20261010T012525957Z/audio-20261010T012616088Z/chord-audio-proof-v1.json)はMIDI61、二回の能動Stop（約0.718/0.276秒）、停止後無音、再開音が成立した。しかしonset約1.093/1.121秒は固定上限1秒を超過。Strings包絡の複数riseを拾うテンポ判定も不成立であり、120BPMのPCM合格とはしない。時刻/packet整合性は成立、閾値は変更していない。必要音声全体は不合格のまま。

4K/200%を変更せず、[保存画像の評価](../../work/analysis/ui-chord-scale/20261010T002919Z/gui-review-v2.json)に4組鍵盤/黄赤青表示と残差を記録した。原版動的比較は既知approval障害で未確認。Inversion tab、MIDI、試聴、黄領域root gesture、CRUD/drag/clipboard、Track Group/Flags/Key Propertiesを残す。保護対象原版にはUI入力していない。

[単位記録](../../work/analysis/ui-chord-scale/20261010T002919Z/unit-record-v1.json)から失敗候補、240/4 build、関連17、公開API13、本体保存復元、六終了、原始一巡、録音へ辿れる。次は共通TransportのStyle開始準備1000ms分類を見直し、Script/Tool/共有sessionの先読みを維持して修正・検証する。

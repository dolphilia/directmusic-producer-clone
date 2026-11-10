# 通常速度の発音中Stop検証

2026-10-12（日本時間）。候補175125037Zの限定単位SSP08-normal-speed-held-stopは終了条件を満たした。SSP08全体および全40責務・全体8受入は未完、fullAcceptance=false。製品ソース変更なし。

## 確認結果

- 291保存/作業ソースと4生成物のhashが一致。関連sequence-audition-runtimeは1モード30検査合格。全登録一巡の代用にはしない。
- 固定4ノート・120 BPM・通常速度・120秒WASAPI録音で16/16合格。発音遅延は初回0.357秒、再開0.339秒。GMの2音、440 HzのDLS、GMドラム、停止後無音と再開を確認。
- 再開後10.213〜10.317秒の入力区間でStopを実行。DLSの自然終了予定12.020833秒より前に停止。Stop前RMS 0.0652968、直後RMS 0.0000534699・peak 0.0000935103。発音RMS>0.001、無音RMS<0.0001・peak<0.0005等の既存閾値を変更していない。
- 明示Save File/Save Project後、作者PID21000を通常exit0。別process PID9240で復元して録音後通常exit0。SGP/DLPは元入力と全バイト一致。PROはSaveに伴うfilh LastWrite 8バイト欄内の5バイトだけ変化。復元/再生後は3入力全バイト不変。
- 73ロードmodule（67 x86 + 6 WOW64）に原版40hash一致0。停止中の点観測であり、全機能依存解消やQ2の証明にはしない。正式原版EXEと保護対象6ファイルのhash不変。

## 中断と表示条件

手動操作待ちと物理Esc中断を記録してから再開した。ユーザーがメインディスプレイと解像度を変更。再開時にウィンドウを取得し直し、旧座標を使わずUIAで操作した。再開後の製品画像は1206×792。変更後OS倍率は未計測で、元の4K/200%画像との厳密外観比較は行っていない。短い発音区間では画像転送を挟まず、各入力の開始/終了UTCと直後のUIAを保存した。

## 範囲と次の作業

旧候補の通常速度2録音の期限失敗と50%成功を保持し、今回の新候補の結果と区別する。既存native assetsの再保存/別process復元/音声追補であり、旧候補の一連のGUI作成操作が新候補で全実行済みという意味ではない。全PB/Expression/試聴、原版同条件比較、厳密外観、全登録一巡、Count-In、DLS Enter/原版空OK、Quantize原版動的比較/複数Part、SSP04親ドラッグ、Add Tracks原版空OK、独立Style015244、Q2、47 native/29 driver凍結を残す。次はSSP04親単位のノート左右端・中央moveとGrid/Noneの比較へ戻る。

一次証拠：[単位記録](../../work/analysis/ui-priority-normal-stop/20261011T181742Z/unit-record-v1.json)、[最終監査](../../work/analysis/ui-priority-normal-stop/20261011T181742Z/final-audit-v1.json)、[音声判定](../../work/analysis/ui-priority-normal-stop/20261011T181742Z/main/audio-proof-attempt-1-v1.json)。

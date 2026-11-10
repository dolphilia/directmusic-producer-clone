# Count-Inの期限制御と録音再検証 — 2026-10-12

204251 Count-In追補：固定300秒PCM16/16、初回0.639秒/再開0.915秒。途中停止34.198秒、停止後73.248秒で元の開始予定を越えて無音、再開128クリック/500msと約64秒後C6/発音中Stop/末尾無音が成立。関連6/300・閉鎖25、291保存/作業ソース・4出力一致。13468/25328二通常exit0と32小節All Playback復元、4入力全bytes不変。73 modules=67x86+6WOW64/原版40hash一致0は点観測。原版同入力PCMと厳密外観は未完。 全体fullAcceptance=false。

前回031359の固定PCMは操作待機80.906秒が上限80秒を超え、15/16で不合格だった。そのPCMと閾値を保持し、今回はPlay/Stopの前後時刻、単調時計による絶対目標時刻、短い分割待機を使用した。目標時刻そのものと実際の入力時刻を区別し、実測34.198/73.248/77.504秒が固定範囲内にあることを判定した。再開時のUIAキャッシュエラーも履歴に残し、新しい観測の要素158で一度復旧した。

300秒の新規WASAPIループバックを既存の校正時計・測定アルゴリズム・16条件で解析した。初回0.639秒/再開0.915秒で発音、32小節の128クリックと500ms間隔・4拍ごとの強拍、約64秒後C6、途中Stopによる元の予約演奏取消し、発音中Stopと末尾無音が成立。RMSは基準0.000012998、取消し区間0.000012998、発音中0.047951、末尾0.000012998。最大gapとpacket/timestampも既存閾値内。元の80秒や1秒等の閾値を緩めていない。

製品ソースは変更していない。今回の5関連モード270と同じ204251のaudition30を入力/依存/実行物条件を照合して合わせ、6/300とした。全登録一巡ではない。作者13468と別25328は通常exit0、別processでも32小節・All Playbackを表示。新規編集がないためnative Saveを再実行せず、既存保存済みPRO/SGP/DLSと設定4件の全bytes不変を確認。291保存/作業ソースと4出力一致。

再生中の73 modulesは67x86と6WOW64。原版40hash一致0はこの時点だけの観測で、Q2ではない。初版監査は低位アドレスのx64 wow64cpu.dllを未分類にした。ValidなMicrosoft署名・正確なSystem32のファイル・x64を照合した限定分類をv2へ追加し、初版失敗を保持した。

原版同入力再生、厳密外観、Record Only等は未確認なのでcompleted=false。boundedFinished=trueは候補の固定PCMと保存済み入力/設定復元経路を指す。

[単位](../../work/analysis/ui-countin-timing-resume/20261011T205700Z/unit-record-v1.json)、[固定PCM](../../work/analysis/ui-countin-timing-resume/20261011T205700Z/countin-audio-proof-v1.json)、[閉鎖25検査](../../work/analysis/ui-countin-timing-resume/20261011T205700Z/final-audit-v1.json)、[実操作時刻](../../work/analysis/ui-countin-timing-resume/20261011T205700Z/audio-actions-v1.json)、[モジュール点観測](../../work/analysis/ui-countin-timing-resume/20261011T205700Z/module-provenance-v2.json)。

次：SSP01 Count-Inを正式原版と同入力・32小節All Playback・120BPM4/4・PCh10/36/37/118/100で比較する。204251の合格PCMを固定し、正式原版の既存processを再照合して専用コピーだけを開く。設定の前後/実再生/途中Stop/元の予定時刻を越えた無音/再開/handoffを別録音し、原版と候補の差を先に確定する。原版install/既存6入力は保護し、既存アプリを閉じない。DLS flags/再表示/空collection音声、Quantize親残件、6640破棄回答待ち、Style015244/Q2、48native/30driver凍結、全40/全8を保持。

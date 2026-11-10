# Band停止診断と固定音声試験（画像25〜28、2026-10-11）

候補100711（20261011T100711958Z）では、固定300秒・19条件が **19/19成立**した。初回発音0.485秒、再開0.505秒、音量・パンのUndo/Redo、発音中Stopと停止後静音を確認した。変更は停止時刻のログとログ保持だけで、停止動作を変更していない。前候補080641の残音約1.78秒は今回再現せず、原因解決とは判定しない。

[一次単位](../../work/analysis/ui-band-stop/20261011T100520982Z/unit-record-v1.json)、[PCM判定](../../work/analysis/ui-band-stop/20261011T100520982Z/fixed-repeat/audio-proof-v1.json)、[固定条件](../../work/analysis/ui-band-stop/20261011T100520982Z/fixed-repeat/audio-protocol-v1.json)、[証拠監査](../../work/analysis/ui-band-stop/20261011T100520982Z/artifact-audit-v2.json)を参照。

## 確認した範囲

191秒のC4・Organ1ノート/192秒Segmentを先頭から再生した。1秒発音・settle1秒・静音RMS0.0001/peak0.0005等、前回の数値条件と12操作を変更していない。音量低下比0.15886、回復比6.29493、左右パンとUndo/Redoが成立。停止後の静音窓20.003秒と84.880秒はいずれもRMS/peak約0.0000610。背景レベルがあり、完全なゼロとはしない。

[停止ログ](../../work/analysis/ui-band-stop/20261011T100520982Z/fixed-repeat/stop-critical-events-v1.json)は837イベント、overflow=false。2回のStopは約0.10670秒/0.11564秒、IsPlaying待機約0.10657秒/0.11534秒で終了した。[PCM短窓診断](../../work/analysis/ui-band-stop/20261011T100520982Z/fixed-repeat/pcm-stop-diagnostic-v1.json)の最後の基準超過は入力開始から約0.365秒/0.300秒。この診断は19条件の判定を変更しない。前回失敗時のStopログは上限で欠落しており、原因の比較はできない。短い別録音の停止診断も独立して保持する。

作者PID14868と別復元PID15624は通常終了exit0。保存後はPCh1/Organ1/Pan R64/Volume100とpitch60/velocity96/duration293376を復元した。nativeは1 binsの意図したpan/volume/flagsだけ変更し、他30leafは不変。Projectは既存filhのFILETIMEとファイルサイズ2894→1622だけが更新され、byte不変とはしない。

288保存/作業ソース・4生成物を照合した。実ロード80モジュールは74x86/6WoW64ホストで、正式原版インストール先からのロード0件。正式原版EXE、保護した原版3入力、ユーザー画像47枚はhash不変。証拠整合24/24は受入全体の合格を意味しない。

同一候補で関連4モード118検査が成立。原始native100は53合格2965検査/46OS凍結/既知Band1保留。driver172は20合格/28OS凍結/124未実行。旧候補の一巡結果を転用していない。

## 残る不足と次の作業

最初の300秒診断録音は誤ったドラッグ開始でvolume30となり、手順も録音枠を超えたため無効。[手順失敗](../../work/analysis/ui-band-stop/20261011T100520982Z/procedure-deviation-v1.json)と録音を保持した。成功した再試験の12操作も189.791秒を要し、150秒の作業目安は未達。これは19条件を変更する追加基準ではなく、別に記録する。

旧初回1.15秒失敗と080641停止後残音失敗を保持する。ドラッグ途中preview/取消し、継承値、画像27のL2/102の今回再確認、原版の厳密外観/全操作は未達。Band全体はcompleted=false。Sequence12/12、Count-In15/16（80.906秒>80秒）、Echo停止中/実機/Advanced/録音、SSP08の通常速度/原版比較は別条件のまま残す。

画像5のCreate New File(s)で初期選択、空選択OK、有効状態、Use Default Names、Enter/ダブルクリック、取消しを正式原版の検証画面と比較。CHM/画像/動的観測を分け、実在する操作差だけ修正して本体/native保存復元へ進める。Band途中preview/取消し・継承値、旧停止失敗、Count-In15/16、Echo/SSP04/SSP08原版比較、Style015244/Q2/OS凍結を保持。

限定採用0735・独立Style015244・全体8受入6作業中/2障害/0合格・fullAcceptance=false・goal activeを維持する。

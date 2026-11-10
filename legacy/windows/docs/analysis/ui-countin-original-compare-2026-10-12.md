# Count-Inの正式原版比較 — 2026-10-12

204251 Count-In原版比較：同じ3入力と32小節All Playback/120BPM4/4/PCh10/36・37/118・100で、候補と正式原版が固定PCM16/16を満たした。原版初回0.666秒/再開0.275秒、128クリック/500ms、64.000秒後C6、途中Stop取消し/発音中Stop/無音。候補handoff63.885秒との差0.115秒は既定0.12秒内だが完全一致ではない。閉鎖22、291ソース/4出力・旧6入力/原版EXE不変、原版設定復旧・既存process保持。endpoint IDと原版内部route/GMロード元は未記録、厳密音色/外観/全SSP01は未完。 全体fullAcceptance=false。

検証用PRO/SGP/DLSは候補と全bytes一致。原版には同名の旧Projectが既に開いていたため、専用パスを明示して新しいツリー1224を開き、旧ツリー203のSegment2がないこと、新しいSegment1子1227とTransportの選択を確認した。インストール版PID25548/開始時刻/EXE hashを前後照合し、既存アプリは閉じていない。

原版の出力は初期Standard Stereo & Reverb。候補のCreateStandardAudioPath(8,16,TRUE)とSDKのDMUS_APATH_DYNAMIC_STEREO=8を照合し、原版でStandard Stereo (Dry)を選んだ。原版内部の型値は未計測で、同じ種類という推論と厳密な内部経路同一性は分ける。録音器はdefault-render-loopback/48kHz/2ch/float32を双方で記録するがendpoint IDを保存していない。音色や波形の完全一致は主張しない。

|測定|候補204251|正式原版|
|---|---:|---:|
|初回発音遅延|0.639秒|0.666秒|
|再開発音遅延|0.915秒|0.275秒|
|再開Count-Inクリック数|128|128|
|隣接クリック間隔の実測範囲|0.480〜0.520秒|0.495〜0.505秒|
|最初のクリックから本編C6|63.885秒|64.000秒|
|本編発音中RMS|0.047951|0.048119|
|停止後RMS|約0.000013|0|

既存の音声測定関数と16条件・全閾値は変更していない。原版の操作実測は途中Stop30.142秒、停止後62.349秒、再開後Stop90.214秒。元の本編開始予定を越える無音と、再開時の新しいカウント、本編発音中Stopと末尾無音が成立。差0.115秒は固定handoff許容0.12秒の内側であり、原版と時刻が完全に一致した意味ではない。

初版録音driverはPowerShell7のConvertFrom-JsonがISO日時をDateTimeへ変換したため、同じ開始時刻の文字列比較で停止した。録音前の失敗とdriverを保持し、v2でUTC ticksを厳密比較した。録音器exit0、原版と入力は前後不変。元のメトロノーム117/100・36/37・PCh10・0小節・Record Onlyを復旧し、再表示して確認。toggleを逆操作し、Stereo & Reverbへ戻した。製品ソース変更・新build・新回帰はない。候補の関連6/300・二process終了はLA40の同一実行物証拠を参照する。

変更後画面は原版961×752で操作・撮影可能。OS倍率は未測定で、旧4K/200%を現行条件として使っていない。全SSP01、Record Only/録音等、endpoint同一性、原版内部経路、厳密外観は未完。

[単位](../../work/analysis/ui-countin-original-compare/20261011T211100Z/unit-record-v1.json)、[原版固定PCM](../../work/analysis/ui-countin-original-compare/20261011T211100Z/countin-audio-proof-v1.json)、[比較と閉鎖22検査](../../work/analysis/ui-countin-original-compare/20261011T211100Z/final-audit-v1.json)、[実操作](../../work/analysis/ui-countin-original-compare/20261011T211100Z/audio-actions-v1.json)、[復旧](../../work/analysis/ui-countin-original-compare/20261011T211100Z/original-settings-restored-v1.json)。

次：SSP07 DLS空コレクションのflags0x10e3対0x63を分離する。LA39原版保存は未編集Saveの対照がないため、専用コピーの原版Saveのみと空collection確定を比較し、共通serializer正規化と選択操作の差を分ける。公開SDKのUSE_DEFAULT_GM_SET/TRANSPOSE契約と既存非空DLS assertを保護し、確認した機能差は修正後に新候補関連回帰/native二終了/復元/必要PCMまで実施する。Count-In16比較は保持し、endpoint/原版内部経路・厳密外観等の残差、Quantize親残件、6640破棄回答待ち、Style015244/Q2、48native/30driver凍結、全40/全8を保持。

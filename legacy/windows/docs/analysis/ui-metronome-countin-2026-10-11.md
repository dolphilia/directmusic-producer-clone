# Metronome All Playback Count-In の実装と限定検証

2026-10-11。最新試験候補 `20261011T031359218Z`。All Playback の Count-In を実時計へ接続した。284保存/作業ソースと4出力が一致し、関連6モード300検査が合格。本体で32小節を設定し、native保存・別process復元・二通常exit0を確認した。固定300秒PCMは測定訂正後15/16で、操作窓超過により総合不合格。原版同入力の再生比較と厳密外観も未達のため、単位は未完、`fullAcceptance=false`。

一次入口は [単位記録](../../work/analysis/ui-metronome-countin/20261011T030344139Z/unit-record-v1.json)、[開始契約](../../work/analysis/ui-metronome-countin/20261011T030344139Z/start-v1.json)、[build](../../work/build/product-snapshot/20261011T031359218Z/build-summary.json)。限定採用0735、前022456の初回発音修正、独立Style015244を区別する。

## 実装した範囲

原版CHMの Metronome Options は、Count-In中にはPattern/Sequenceとplay cursorが始まらず、指定小節後に再生することを定める。All Playbackを今回の対象とし、Record OnlyのMIDI録音は別の未実装責務として保持した。

入力文書を時間移動せず、現在位置の拍子・Tempoと有効なBand履歴だけを持つ私有carrierを作る。Sequence/CC/Script/Trigger/Wave等の著作イベントをcarrierへ含めない。同じPerformance/AudioPath上のcarrier開始時刻から小節分のmusic clocksを加え、元のSegmentを予約する。Stopはcarrierと予約済み元Segmentの両方を取消し、所有するdownload/state/segmentを解放する。Count-In中の本体表示を維持し、将来開始待ちを再生終了と誤判定しない。

disabled/Record Onlyの通常再生は従来経路を維持する。時計範囲のoverflowは既存再生を変える前に拒否する。前022456の即時private primary開始方針を保持した。実変更10ファイルと保存ソースの対応は[終了時ソース記録](../../work/analysis/ui-metronome-countin/20261011T030344139Z/source-changes-closing-v1.json)にある。

## 同候補の回帰

| 判定 | 結果 |
| --- | --- |
| configure/build/install | 各exit0、284ソース・4出力一致 |
| 関連6 | timeline117、transport57、meter65、sequence audition30、metronome12、count-in19、計300 |
| 原始native100一巡 | 53合格2965検査、46既知OS凍結、既知匿名Band1保留 |
| 原始driver170一巡 | 20合格、28既知OS凍結、前提未充足122未実行 |

[native原始run](../../work/acceptance/regression/20261011T031947885Z/run.json)と[driver原始run](../../work/acceptance/registered-drivers/20261011T032240304Z/run.json)を専用GUI/PCM補足と混ぜない。Count-In19は4/4の4クリックと正確なhandoff、途中Stop後の予約取消し、現在位置3/8・90BPM・50%の6クリック、Record Only/disabled bypass、overflow時owner不変、元文書不変を確認した。OS凍結は同条件で再試行していない。

## 本体・保存・復元

最初の8小節fixtureは、元入力にSequenceがない準備失敗を保存した後、別の検証用コピーへSequence/GM Organ 1を追加した。8小節/All PlaybackのMain設定、保存・別process復元・二通常終了が成立した。120秒録音はStopが57.592秒後となり事前窓4〜14秒を外れ、再開操作も未実施。失敗録音を保存し、成功扱いにしない。

操作条件を変えた32小節fixtureは、Segment240秒、pitch72/PChannel0/velocity96の120秒ノート、120BPM 4/4、Count-In64秒とした。Mainで32小節を入力し、Projectを保存。作者process7660と別復元5524はともに通常exit0。保存Project/Segment/DLSの3入力と設定bytesが復元後も一致した。[保存復元証拠](../../work/analysis/ui-metronome-countin/20261011T030344139Z/long-main/native-save-reload-proof-v2.json)。4processはすべて終了済みで、次回は古いPIDを使わない。

## 固定PCMと測定訂正

[事前protocol](../../work/analysis/ui-metronome-countin/20261011T030344139Z/long-main/audio-protocol-v2.json)、[操作時刻](../../work/analysis/ui-metronome-countin/20261011T030344139Z/long-main/audio-actions-v1.json)、[録音run](../../work/acceptance/product-project-gui/20261011T033056365Z/audio-20261011T033412857Z/run.json)を結合した。最初のCount-Inを25.373秒でStopし、再開後75.466秒で発音中Stopした。再開前の待機80.906秒が事前上限80秒を超えたため、録音全体は不合格。

元判定器v2は13/16。その拍間隔判定は5msずつ加算した浮動小数点時刻の境界誤差、handoff判定は120ms spectral窓の先読みを含む時刻だった。失敗結果・判定器・PCMは保持した。訂正版v3は整数5ms格子で包含境界を比較し、持続音高を確認した最初のspectral窓内の独立RMS attackで開始を局在化する。期待Count-In時刻を音の開始探索に使わない。±30ms拍間隔、±120ms handoff、1秒onset、80秒操作窓等の閾値は変更していない。

合成対照は境界内外、既知4開始時刻、誤音高、clickのみ、無音の12/12合格。初版対照の9/10失敗は近接clickとノートを200ms単位でまとめていたためで、初版コードと失敗も保存した。[対照v2](../../work/analysis/ui-metronome-countin/20261011T030344139Z/audio-measurement-controls-v2.json)。

同じPCMの[訂正後結果](../../work/acceptance/product-project-gui/20261011T033056365Z/audio-20261011T033412857Z/countin-audio-proof-v2.json)は15/16、総合false。初回onset0.649275秒、再開0.990275秒、再開後128クリック、強拍、500ms間隔、Count-In中の著作ノート不在、予約時刻を過ぎた取消し後無音、著作音の発音中Stopとtail無音が成立した。著作音attack188.475秒と先頭click124.590秒の差63.885秒は固定64±0.12秒内。操作窓失敗を測定訂正で置き換えず、同じ手順の再録音はしない。

## 原版の観測と未達

正式インストール原版の実パス/hash/process/開始時刻/windowを照合し、Transport OptionsのToggle Metronomeを右クリックしてOptionsを撮影し、Escapeで変更せず閉じた。[部分比較](../../work/analysis/ui-metronome-countin/20261011T030344139Z/original/comparison-v1.json)。原版は0小節/Record Only、製品は検証設定32小節/All Playbackなので、同条件動的比較とはしない。原版498pxに対し製品480pxの高さ、字体・余白・toolbar文字とアイコンに差がある。正式原版の文書・設定・再生は変更していない。

必要PCMの操作条件成立、原版同入力Count-In/Stop/再開、厳密外観、全入力値域・変更中操作、可変Tempo/TimeSig・別route/DLSのPCM、Record Only録音、Style/Motif/secondaryとの適用範囲、実ロード元監査は残る。他SSP段階、全40責務、全8受入、独立Style015244、46native/28driver凍結、Q2独立Windowsを維持する。

次は同じCount-In録音を繰り返さず、SSP01で無効のPlay Cursorを原版CHM・現在のTimeline更新経路から調べる。表示toggleの責務と既存位置更新を分け、文書/historyを変えない共通コマンドへ接続する。Count-Inの未達は未完として保持し、必要再録音は操作条件の成立を事前に確認した別protocolとして準備する。

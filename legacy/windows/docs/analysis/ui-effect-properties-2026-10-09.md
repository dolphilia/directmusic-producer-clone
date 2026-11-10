# AudioPath Effects Listと選択した効果のProperties（2026-10-09）

候補0311（20261009T031141001Z）で、選択したWaves ReverbからF11／右クリックProperties、個別設定、取消し、Undo/Redo、native保存、別process復元、二通常終了まで限定確認した。原版Helpにある操作入口の不足を減らした。音声は3回とも固定Stop時刻条件で不合格。原版比較・外観・残操作・全40／全8は未達で、completed=false、fullAcceptance=false。

一次入口は[開始記録](../../work/analysis/ui-effect-properties/20261009T030855824Z/unit-start.json)、[限定記録](../../work/analysis/ui-effect-properties/20261009T030855824Z/unit-0311-limited-v2.json)。原版の動的観測障害window64100706を再試行していない。4K・200%を維持。

## 変更と原版基準

対象責務はAudioPathDesigner.ocx／DMUSProd.exe、UI08.effect-selection-properties／UI08.waves-effect-properties。変更はsrc/producer/audio_path_editor.cpp。原版HelpのManaging Effects、Effect Propertiesと2画像を基準に、選択bufferの順序付きEffects List、F11、右クリックProperties、固定Effect Type、Edit Effect Propertiesを追加した。Waves／Sendの既存設定エディターへ選択したinstanceを渡す。公開dsound.hのDSFX_LOCSOFTWARE（0x2）を読み取り専用で表示する。

今回のGUI検証はWaves effect2に限定。Send等の別分岐の本体検証はこの記録で成立させない。Instance Name保存先はProducerの未確定契約であり、DSOUND_IO_DXDMO_HEADERの予約GUID／DWORDを転用していない。

## 候補と関連回帰

[build](../../work/build/product-snapshot/20261009T031141001Z/build-summary.json)はconfigure/build/install各0。Producer SHA256 f02d627bf4e10ca86edfd92fb6dbe4e70c4b25607162da240f4fa2b3fbb49232。保存231ソースと作業231ソース、4生成物の一致を確認した。[関連37](../../work/acceptance/regression/20261009T031708401Z/run.json)は33合格／4既知凍結／0失敗。通知・短時間監視・Motif同時再生の4製品driverも同じ0311で合格。2137原始一巡を置換せず、旧0735限定採用を維持する。

## 本体と保存

[GUI監査](../../work/analysis/ui-effect-properties/20261009T030855824Z/gui-proof-v1.json)6項目、[独立RIFF監査](../../work/analysis/ui-effect-properties/20261009T030855824Z/native-proof-v1.json)7項目合格。author PID17628、reload PID24116は同じEXE、異なる開始時刻、二通常exit0／forcedTermination=false。

Effects ListでWavesを選びF11からPropertiesを開く。右クリックPropertiesも同じ効果を示した。固定Type、既定値0／0／1000／0.001、四項目−6／−12／750／0.5、取消し後の一覧focus、一回のUndoで既定値とclean復帰、一回のRedoでcustomとdirty復帰を確認。設定の確認後に選択を保持した。Undo/Redoボタンを押した直後のfocusは当該ボタンであり、Propertiesを閉じたときに一覧へ戻る。

既存Effect.aup保存、Song.sgpへの埋め込みと保存、Waves.pro保存を実施。AUPでは選択Wavesのdataだけが、現候補の独立OS DMO保存blobに一致。Segmentは埋込AUP以外のSequence／Tempo／Band／名前／未知bytesが完全一致。Projectは対象2ファイルのfilh writeTime／sizeだけが変わった。復元用3ファイルは終了後も不変。ディスク参照・入力binding・before／afterは限定記録から辿れる。

Computer Useの閉じたwindowへのrefreshエラーとメニュー座標エラーをactionsに保存。原版approvalや製品保存拒否の証拠として扱わず、返された現行windowを再取得した。Fileメニューは現在画像から開き、保存済み入力への通常saveを実行した。凍結中の非空literal filename／Recording filename条件を再試行していない。

## 音声の未達

64秒WASAPI（48kHz stereo float32）の対照1本とcustom3本を保存した。各録音器はexit0。編集前author入力と不変controlコピーを録音直前hashで結び、customは別processが読み込んだ保存後の不変3ファイルを宣言した。run.inputsだけをロード済みauthorファイルと同一pathと呼ばない。

[時刻診断](../../work/analysis/ui-effect-properties/20261009T030855824Z/audio-timing-diagnostics-v1.json)と3失敗proofを保持。判定器は同じ1秒onset／発音中Stop 4.2〜6.3秒／停止後quiet／8音高・120/180tempo窓／peak／効果差の契約を使用し、閾値を緩めていない。

| 録音試行 | control上限秒（2回） | custom上限秒（2回） | 判定 |
| --- | --- | --- | --- |
| 1 | 5.891／5.761 | 5.843／9.849 | Stop時刻条件で不合格 |
| 2 | 5.891／5.761 | 6.869／7.194 | Stop時刻条件で不合格 |
| 3 | 5.891／5.761 | 5.095／6.911 | Stop時刻条件で不合格 |

時刻検査で止まったcustomの音高・無音・効果差を合格と推測しない。正の実録音が資格成立しないため、故障対照5生成器は準備のみ・未実行。0239の音声成功は旧候補の履歴として保持し、0311へ転用しない。Computer Use入力間隔による逸脱と製品DSPの不具合を区別する。

## 原版差分と次の単位

原版の横並びMixGroup／Bus／Buffer／EffectsListグリッドとPalette、Instance Name、Software flag編集、複数選択・clipboard・drag・Insert/Delete、共通modeless Properties方針、字体・色・アイコン・配置の比較は残る。今回のtab表示はEffectであり、原版HelpのEffect Propertiesとも差がある。原版同条件の動的比較は未確認。

音声時刻の同一診断をさらに細分化せず、独立したUI02のTempo mark Properties／Insert／右クリック操作へ優先順位を移す。既存Tempo core／native保存と共通Propertiesを活用し、原版Helpにある入口・手順の不足を閉じる。0311音声・効果UI残差をunfinishedとして保持する。全8は6作業中／2障害／0合格、全40未完。

# 共有Timeline・時刻付きstripの本体統合

2026-10-09。候補0743では共有Timelineの6行表示・イベント選択・Properties連携・zoom/scroll・代表Tick編集/UndoRedo/native保存/別process復元を限定確認した。GUI7/native6、二通常exit0。固定WASAPIは2回ともStop時刻条件を満たさず未達。原版動的比較も障害あり。boundedFinished=true、completed=false、fullAcceptance=false。0735限定採用を維持する。

## 対象と変更

- UI02.timeline-timed-strips、Timeline.dll/TempoStripMgr/TimeSigStripMgr/SequenceStripMgrの共有表示、UI09のLyric/Marker選択入口。
- 原版Help timeline.htm、markertrack.htm、semarkertrack.gif、editingdatainasegmenttrack.htmと保存済みMarker境界契約を参照した。重い小節線・青い拍線、上段の青Marker/下段の緑Enterを実装した。Help期待と現在候補の動的観測を区別し、原版実行の代用にはしていない。
- Segmentのmodeless TimelineにTempo/Time Signature/Sequence/Lyric/Marker/Muteの6行を表示。現在文書の時刻付きイベントを選び、既存の共通Propertiesへ接続した。zoom/水平scroll/Fitは文書履歴を作らない。
- 0720実GUIで見つかった空名の「Segment:」captionは、文書pathのstemへfallbackする修正で0743の「Segment: Song」になった。失敗候補・入力・通常終了は保持した。
- 音楽的編集は選択EnterのTick 13→29のみ。10189→10205 clocks、4:2:2:13→4:2:2:29。Main EditメニューのUndoで元へ、Redoで変更後へ戻る。Timeline footerはgridを分けず拍内Tickで4:2:205/221を表示する。

## 候補と関連回帰

- build: `work/build/product-snapshot/20261009T074306988Z/build-summary.json` sha256 90f43666583d04b99acdb282e16ab58121efd237c13c8a9693db1e5f80663b77。234保存/現在ソース、4出力、472照合一致。Producer sha256 7fcfa583eb814bbb53ab7a72b7e1fba751487dae853c22fea494f5a82f7ea24c。
- 0743関連native40：36合格/4既知凍結/0失敗。timeline-range 117。一次run `work/acceptance/regression/20261009T074903907Z/run.json` sha256 a053d69c2c41a60ade0f045abebdcadc6213c0144db17b2ff883f2970885f49e。
- ShortPlaybackMonitor、NotificationIdentity、MotifConcurrent、PlaybackNotificationsの4製品driverは同候補・入力・driverコピーhashへ結合して正常完了。GUI/実PCMとは別判定。
- 回帰資格v2のpassed字段が数値36で上書きされたメタデータ誤りは保持し、v3でbooleanとcountsを分離した。一次runの結果は変えていない。
- manifestへ40+4補足を登録。原始一巡2137とnative94/driver164登録数を保持した。登録証拠 `work/analysis/ui-timeline-strips/20261009T070455002Z/manifest-registration-v1.json`。

## 実GUIとnative保存

| 確認 | 結果 |
| --- | --- |
| ruler/6行、正しいSong caption、modeless Main有効 | 限定成立 |
| Tempo1536選択→137.75、1:3:0 Properties | 限定成立 |
| Lyric3864選択→UI Ω、2:2:24、Belongs4:1 | 限定成立 |
| Marker3072とEnter10189を別laneで選択 | Typeと時刻が一致 |
| zoom/scroll/Fit後の選択・履歴不変 | 限定成立 |
| Tick変更・1回Undo/Redo・Timeline追従 | Main Editメニューで成立 |
| native Song/Project保存・別process復元 | Tick29/10205、clean/履歴空 |

- GUI判定：`work/analysis/ui-timeline-strips/20261009T070455002Z/gui-proof-v2.json` sha256 8c8714f91842165083e228d180eab259fcadca04db5660a9a5a7a10b8693211c、7確認。観測・PNGはgui-v3/gui、集合はobservations-v3.json/actions-v3.json。
- native判定：`work/analysis/ui-timeline-strips/20261009T070455002Z/native-proof-v3.json` sha256 0185ad18db85c07295170313f4a7936c986541358a6fc2ad662ba488ea47d65a、6確認。選択Enter clock以外のSong RIFF bytesと末尾6bytesを厳密保持。Project差分はSong size/writetimeのfilhだけ。8notes/3Tempo/他Marker/Lyrics/meter/Effect.aupを保持。
- author PID14704 start07:53:35Z、reload PID20912 start08:07:52Z。異なる起動時刻・EXE hashを結び、両方通常exit0/forcedTermination=false。保存/再読込コピー/before合計12hashを終了後にも照合。
- closure `work/analysis/ui-timeline-strips/20261009T070455002Z/closure-0743-v1.json` sha256 e9efc9c60a8ebe37f5593fe8f71489ac48c16f75da074361200173756355c4a7。原版Window64100706には操作していない。

## 固定WASAPIの未達

64秒/48kHz stereo float、既存録音器・UTC/QPC/packetと操作時刻を結ぶ。criteriaはonset≤1秒、Stop [4.2,6.3]秒、quiet≤1e-7、active≥5e-6、8音の最低tone/neighbor、peak<0.99を維持した。

| 録音 | Stop1 after上限側 | Stop2 after上限側 | 判定 |
| --- | --- | --- | --- |
| audio-20261009T081023279Z | 7.022秒 | 13.116秒 | 固定時刻条件不合格 |
| audio-20261009T081232880Z | 9.788秒 | 9.465秒 | 固定時刻条件不合格 |

両録音器はexit0。audit-audio-v1が時刻assertでexit1、audio-proof-v1/v2と各run/PCM/actionsを保持した。入力・候補の失敗とは断定していない。Computer Useの「一操作→fresh snapshot→確認」の呼出し間に遅延があり、後続Stop helperの5秒目標に間に合わなかった。閾値を緩めず、現在音声全体は不合格。正の固定条件baselineがないため5故障対照は未実行。旧Marker0615音声成功を0743へ転用しない。

## 外観・操作の残差と次

4K/OS200%を維持。Timelineは論理850×360に対し今回外枠1672×706px、Windows DEFAULT_GUI_FONT/system colorsを使用した。原版MDI/docking、全toolbar、clock切替、font/icon assets、全40必要画面/操作一覧、同条件の動的外観比較は未達。選択描画と行の観測は画像を保存したが画像一致の合格ではない。

Timeline自身のF11/context Propertiesはソース実装があり今回動的delivery未検証。Ctrl+Z/Ctrl+YはMain及びそのchildに限定され、owned Timeline/Propertiesからの履歴キー入口が不足する。Main EditメニューでのUndoRedo成功をowned keyboard成功にしない。次の独立単位で履歴キーとfocusを接続する。multi-select、overlap cycling、drag、OLE/clipboard、full cue/stop/Enter playback policyもqueueに残す。

音声時刻条件の未達をunfinishedとして保持し、原版approval/Q2/個別Windows5/Clipboard5/literal filename、全40/全8は維持する。一つの単位の終了で全体完了にはせず、独立して減らせる操作差分を進める。

一次封印：`work/analysis/ui-timeline-strips/20261009T070455002Z/unit-0743-partial-v1.json` sha256 6170015d7c3e2a42d387fdc59c39ea3da252476c21d5841680c9672052f8ec63。

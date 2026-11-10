# Segment PropertiesからLengthへの経路の原版比較

2026-10-11（日本時間）。ユーザー画像44→46→47に対応する限定単位。45はTimeSigの誤撮影として除外を維持する。

正式原版と同じ20小節・TimeSigのみのSegment2.sgpをコピーして比較した。原版では、ツリーの親「Segment2.sgp」はDesign／RuntimeのSegment File Propertiesを開き、子「Segment2」はSegment Propertiesを開く。ユーザー画像44も子ノードを選択している。ファイルと本体を混同しない条件を計画へ追加する。

候補114844のName／Length行には配置・高さ・ラベル・「20 Bars...」の差があったため、Segmentページだけの行配置、ページ寸法、Name／Lengthラベルと小節表示を修正した。新候補は130456757。properties_window.cppだけを変更し、Lengthの設定処理や他ページの配置値は維持した。修正後のPropertiesは200%表示で782×400、原版と同じ「20 Bars」を表示した。フォント、全タブ、全内容の厳密一致を意味しない。

| 確認した経路 | 結果と範囲 |
| --- | --- |
| 原版の子Properties→Length | Segment2／20 Bars、20 Measures／0:40:000を確認 |
| 原版の変更なしOK／入力後Cancel | 20 Barsを維持。原版の旧EditではCtrl+Aが全選択にならず、変更入力が202に見える操作記録を保持し、Cancelした |
| 候補の変更なしOK／24入力後Cancel | 20 Barsを維持 |
| 候補で24を確定 | 24 Barsへ更新しdirty表示。保存したseghの長さ73728 clocksを確認 |
| 候補でUndo／Redoと保存 | Undo保存は20小節の入力とbyte一致、Redo保存は最初の24小節保存とbyte一致 |
| 別プロセスで再表示 | Segment2／24 Bars、Lengthの24 Measures／0:48:000を復元。4ファイルは復元前後で不変 |

GUIの直接観測10条件とnative監査8条件が成立した。Undo／Redoのキー入力とdirty遷移はGUIで観測し、20／24の値は保存ファイルで確認した。原版で24の確定やUndo／Redoを試した扱いにはしない。Nameの編集は今回の対象外。長さ変更前後のnative差分はseghの1葉だけで、GUID、名前、TimeSig、segd等は不変。Projectは3参照を保持し、既存Segment1とScenarioToneは不変。

288ソースと4出力のsnapshot／作業木／生成物の同一性を確認した。関連するcommon-open 53、timeline-range 117、segment-creation 200、計3モード370検査は今回候補で合格。比較用114844のPID6152、修正候補の作者24300、別復元4980はすべて通常exit0。作者の80ロードモジュールを74 x86＋6 WOW64ホスト支援に分けPEとhashを照合し、原版40hashとの一致0・未解決0だった。点観測であり、独立Windowsや連続監視の証明ではない。音声は非適用。全native100／driver172の一巡は今回実行しておらず、100711等の履歴を新候補へ移さない。

原版EXEと検証入力4ファイルは今回の前後でhash不変。正式原版PID25548・開始時刻2026-10-10T12:11:40.4555881+09:00・応答を再確認した。原版の未保存Project参照を保持し、終了・破棄・Project保存を行っていない。Segment2の過去SK26時点のsegd変更は別履歴として残る。

Start Time／Loop／Info／Boundary、親ファイル専用Properties、Extension bars、Display pick-up bar、Clocktime要約、全入力・keyboard・初期focus・厳密外観は未完。原版と候補で、小節編集中に無効なClocktime側の表示を更新するタイミングにも差が見られるため、全動作一致にしない。原版にない仮のタブは追加しない。

一次証拠は[単位記録](../../work/analysis/ui-segment-properties/20261011T125300443Z/unit-record-v1.json)、[原版比較](../../work/analysis/ui-segment-properties/20261011T125300443Z/comparison-v1.json)、[GUI](../../work/analysis/ui-segment-properties/20261011T125300443Z/gui-save-reload-proof-v1.json)、[native監査](../../work/analysis/ui-segment-properties/20261011T125300443Z/native-audit-v1.json)、[候補同一性](../../work/analysis/ui-segment-properties/20261011T125300443Z/closing-identity-v1.json)、[関連回帰](../../work/acceptance/regression/20261011T131208552Z/run.json)、[原版保全](../../work/analysis/ui-segment-properties/20261011T125300443Z/preservation-v1.json)。

次は画像18・19のSequenceノート左右端・中央移動とGrid／Noneの原版同入力比較を独立単位として進める。操作差を確認してから必要な修正と保存・復元・Undo／Redoを検証する。既存PCM合格を今回候補の音声再証明に転用しない。Add Tracks初期focus／原版空選択、旧Band停止失敗、Count-In15/16、Echo、SSP04全gesture、SSP08、Style015244、Q2とOS凍結を保持する。限定採用0735、全体6作業中／2障害／0合格、completed=false／fullAcceptance=falseを維持する。

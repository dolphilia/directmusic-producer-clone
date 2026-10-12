# Wave Snap to Zero・Clone Loopの本体統合

2026-10-09。候補1208で、既存PCM編集へSnap to Zero、Set Loop From Selection、Clone Loopを接続した。限定範囲の機能・本体操作・保存復元・音声は成立。原版動的比較、全Wave責務、全40/全8/Q2は未完。`completed=false`、`fullAcceptance=false`を維持する。

## 契約と変更

原版[Wave編集Help](../../work/analysis/help/htm/cuttingcopyingandpastingportionsofawave.htm)はSnapの既定ON・開いたWave間の共有、ループ末尾への複製、連続Cloneでも最初の区間だけを複製する操作を示す。[Loop設定Help](../../work/analysis/help/htm/settinglooppoints.htm)は範囲選択からの右メニュー、緑の始点・青の終点を示す。原版Help画像の黒波形・赤選択に沿って描画色も修正した。

PCM8/16/24/32の全channelがゼロまたは符号反転する境界を探し、同距離なら前側、該当境界なしなら元の位置を保持する。量子化/stereoの具体的判定は実装の推論として記録した。Cloneは選択WSMPループを対象とし、最初の区間を履歴と一緒に保持する。Undo/Redoでその状態も戻す。別の実編集で系列をリセットし、同processのSaveでは維持、新規loadでは新系列とする。これらの系列保存方針は原版動的確認前の推論である。

CloneではPCMと選択loopを一つのtransactionで更新し、既存cue/label/WSMP/SMPL/factを挿入位置に対応させる。拡張header/record/tail・未知chunk・奇数padを保持する。圧縮/playlist等の未対応入力は変更前に拒否する。SMPL authoringやWave Trackの再生loop継承を新しく実装したとは扱わない。

## 実行結果

正本は[封印した単位](../../work/analysis/ui-wave-loop-tools/20261009T115211263Z/unit-1208-limited-v1.json)。[build](../../work/build/product-snapshot/20261009T120806108Z/build-summary.json)の234保存/作業ソース・4出力を照合した。Producer SHA256は`079a79db9b5a60e45111d340f9cff9a1d80c76fa3c988bddd18a12c0d7ea8f3d`。1158の実装buildと関連7試験は、色/影変数修正前の履歴として保持する。

| 判定 | 同1208候補の証拠と限定結果 |
| --- | --- |
| 関連回帰 | [native7](../../work/acceptance/regression/20261009T121356695Z/run.json)：timeline-range、dls-sample-policy、sequence-crud、wave-pcm、wave-sample-loop、wave-track、component-name。全7合格。専用試験EXEと本体EXEは別hashで同buildへ結ぶ |
| 本体操作 | [GUI8](../../work/analysis/ui-wave-loop-tools/20261009T115211263Z/gui-proof-v1.json)：右メニューのSnap共有、実drag、範囲設定、2回Clone、ShiftF10、canvas CtrlZ/Y、SaveAs、別processの復元と初期historyなし |
| native保存 | [独立監査4](../../work/analysis/ui-wave-loop-tools/20261009T115211263Z/native-proof-v1.json)：5→7→9秒、WSMP始点44100/長さ264600。保存・終了後・reloadのCloned全bytesが事前固定期待値と一致。Other/元Loop/Segmentは不変。Project差分は2番目のファイル名とfilh時刻/実サイズだけ |
| 通常終了 | author PID6444、reload PID16324のowning launcherが両方exit0。終了直後のwindow取得エラーは終了結果と区別して保存 |
| 実音声 | [64秒WASAPI](../../work/analysis/ui-wave-loop-tools/20261009T115211263Z/audio-proof-v1.json)：440/660/880/660/880 Hzを2回確認。複製660/880を元1100/1320から区別。発音中Stopは4.710/4.711秒、停止後RMS0、再開成立。loop終点7秒以降のwrap/override/tempoは主張しない |
| 判定器 | [5対照](../../work/analysis/ui-wave-loop-tools/20261009T115211263Z/audio-control-qualification-v1.json)：無音、過大peak、停止後信号、入力hash不正、早期Stopをそれぞれ意図したguardで拒否 |
| 外観 | [限定QA](../../work/analysis/ui-wave-loop-tools/20261009T115211263Z/visual-layout-proof-v1.json)：黒/赤/緑/青と右メニューchecked/shared・enablementを確認。4K OS200%は維持。原版の配置/font/icon/動的操作の一致判定ではない |

最初の64秒記録は時間外Play、2回目は停止時刻が規定より遅く不合格。両記録・操作時刻・判定を保存し、3回目で同じ64秒・同じ閾値を満たした。原版DMUSProdのapproval timeoutは条件変更なしのため再試行していない。

## 残差と次の作業

原版Waveの全画面/properties3タブ・modeless方針、zoom/scroll、字体・icon・配置、既定whole-wave loop、Find Best Loop End、圧縮/streaming/playlist、Wave Trackのloop/end/override/継承と実wrapは未完。数値選択の描画/メニュー状態更新、長いselectionラベルの欠け、空historyでも有効に見えるUndo/Redoも操作/外観差分として残す。機能欠落を200%の環境差へ移さない。

0735限定採用、2137原始native/driver一巡、全8の6作業中/2障害/0合格を維持する。次は原版HelpからWave Editorのzoom/scroll操作を固定し、波形を細かく編集できるviewportを本体へ接続する。原版比較障害を未完に残し、独立作業を継続する。

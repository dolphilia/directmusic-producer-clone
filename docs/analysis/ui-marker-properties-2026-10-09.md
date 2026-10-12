# Marker Properties・modeless編集の本体接続

2026-10-09。UI09 / MarkerStripMgr.dll / Timeline.dll / 共通Properties。0735限定採用を保持。0615は限定した機能経路が成立し、原版比較・全40・全8は未完、fullAcceptance=false。

原版の保存済み動的観測とHelpが示す読み取り専用Type、Measure・Beat・Grid・Tickを既存Marker文書へ接続した。Marker編集をmodelessにし、選択を変えたときはフォーカス中の欄も追従する。既存CRUDとnativeレコードを利用し、拍子とgridを共通Timelineで解決する。整数clockで表せないgridは未対応として残す。

候補20261009T061520744Zは232保存/作業ソースと4生成物一致、configure/build/install各0。関連40件は36合格・4個別凍結、4製品driverも同候補で実行合格。manifestへ補足として登録し、2137原始一巡と94native/164driverの登録範囲を維持した。

実GUIではフォーカスを残した両方向の選択追従、読み取り専用Typeへの入力拒否、Tree F11とMarker右メニューからのPropertiesを確認した。Enter SwitchPointを3:1:1:0から4:2:2:13（6144→10189clocks）へ4編集し、Undo4回でcleanな元値、Redo4回で編集値を復元。不正なGrid5は値・履歴不変で拒否した。共有Tempo/Lyricページの値を保護し、Markerへ戻っても編集値を保持した。Marker編集画面自身のF11完遂は未確認であり、Tree F11の成功で代用しない。

SongとProjectをnative保存し、author PID13320を通常exit0。同じ保存コピーを独立PID23868で開き、Typeと4:2:2:13、Saved・UndoRedoなしを復元した。reloadも通常exit0。保存前保全・author保存入力・保存コピー・reloadコピーの12hashを終了後に照合した。独立native監査6件は、選択Enterのclock4バイトのみの変更、peer Marker3072・全その他RIFF bytes・8notes・Tempo3・AudioPath保持、ProjectのSong size/writetime以外保持、異なるPID/開始時刻/同一EXE/二通常終了を確認した。

同reloadプロセスの64秒WASAPIで8音・Tempo120/137.75/191.5と二回の発音中Stop/無音/replayを固定条件で確認した。停止操作範囲は4.993〜5.274秒と4.791〜5.047秒、停止後RMSは両方0。無音・過大ピーク・停止後信号・入力hash不一致・早いStopの5対照は意図したguardで拒否。最初の試行は操作間遅延でStopが21秒台となり不合格を保持し、条件を変えず別録音で再検証した。この音声は保存後の代表再生を証明する範囲で、原版Marker cue/stop/Enter境界再生の全仕様を証明しない。

GUI監査9件は限定条件で成立。4K/200%を維持。メインWindowの撮影範囲では左上のowned Windowが一部欠けたが、別のbounded screenshotではディスプレイ内の全体を確認した。入力API引数不備、遅れたUIA状態、未到達の操作と回復観測を残し、製品不具合やOS拒否に転用しない。

次はTempo/Lyric/Markerの3 Properties単位の成果を保護し、共通Timelineのmeasure ruler・時刻付きstrip表示/選択・同じProperties入口を接続する。全体Timelineの不足を減らし、個別ページ追加のみを続けない。原版動的比較は既存approval障害のまま。上下strip/cycling/drag/OLE/clipboard/字体/icon/全境界再生、Lyric入口、Q2、個別Windows5/Clipboard5/literal filenameと全40/全8を未完として保持する。

一次入口：

- [固定限定記録](../../work/analysis/ui-marker-properties/20261009T061049207Z/unit-0615-limited-v1.json) SHA-256 42cd76f255386da09ee125b0a1a0f07df07d0aa3a720b64afd4fc69dcc3374dc。
- [固定GUI手順](../../work/analysis/ui-marker-properties/20261009T061049207Z/gui-protocol-v1.json)、[GUI9](../../work/analysis/ui-marker-properties/20261009T061049207Z/gui-proof-v1.json)、[native6](../../work/analysis/ui-marker-properties/20261009T061049207Z/native-proof-v1.json)。
- [固定音声](../../work/analysis/ui-marker-properties/20261009T061049207Z/audio-proof-v1.json)、[5故障対照](../../work/analysis/ui-marker-properties/20261009T061049207Z/audio-controls-proof-v1.json)、[時間外Stopの失敗](../../work/analysis/ui-marker-properties/20261009T061049207Z/audio-proof-failed-v1.json)。
- [終了後入力](../../work/analysis/ui-marker-properties/20261009T061049207Z/post-exit-input-integrity-v1.json)、[回帰補足の登録](../../work/analysis/ui-marker-properties/20261009T061049207Z/manifest-registration-v1.json)、[関連回帰](../../work/acceptance/regression/20261009T062417582Z/run.json)。
- [原版Marker境界契約](marker-boundary-contract.md)、[Lyricの成功と未確認](ui-lyric-properties-2026-10-09.md)。

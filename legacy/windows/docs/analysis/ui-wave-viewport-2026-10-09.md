# Wave Editor の拡大・スクロール統合（2026-10-09）

候補1306（20261009T130614294Z）で、拡大／縮小、水平スクロール、文書ごとの表示範囲、拡大時のドラッグ選択を実装・検証した。保存と別起動での復元、発音中Stop・停止後無音・再開まで限定成立。原版動的比較は障害あり、全40／全8と fullAcceptance=false を維持する。

[固定単位](../../work/analysis/ui-wave-viewport/20261009T125539491Z/unit-1306-limited-v1.json)、[GUI監査](../../work/analysis/ui-wave-viewport/20261009T125539491Z/gui-proof-v1.json)、[native監査](../../work/analysis/ui-wave-viewport/20261009T125539491Z/native-proof-v1.json)、[録音監査](../../work/analysis/ui-wave-viewport/20261009T125539491Z/audio-proof-v2.json)を一次入口とする。単位SHA256: 1ff1cae91ad2e3996ab0847d890c4a091e0477fd56362e8761343271cbd3fa75。

## 変更と根拠

原版 Help waveeditor.htm と dlfltwav.gif が拡大／縮小ボタンと水平スクロールを示す。表示spanの半減／倍増、insertion中心、per-open-document保持、別起動時fitは実装上の推論。原版が同じ保持方針であると判定していない。WaveViewportを分離し、空／1frame／奇数／UINT64_MAX境界を含む絶対frame↔pixel写像を保護した。黒い符号付きチャンネル波形、赤い選択、緑の開始／青の終了をviewportへclipする。個別サンプル描画は実装済みだが、本体の極端な拡大での確認は残る。

## 同候補の証拠

235保存source／現在source470照合・生成物4が一致し、configure/build/install各0。Producer SHA256 9807a0e696fb06e62a968734cbd4e2d343859b8f12a5163aca012e00ef6380ad。関連7群すべて実行・合格（wave-pcm80、wave-sample-loop22、wave-track68、timeline-range117、DLS sample-policy28、sequence-crud26、component-name44）。全登録native94／driver164の新一巡にはしていない。2137の原始一巡と0735限定採用は保持する。

OS200%の実GUIでは0..220500→0..110250→22050..132300へ拡大／スクロール。Other Waveはfit、元文書へ戻ると表示範囲保持。view-only Undo/Redoは文書を変更しない。固定したドラッグ位置は44167..88251（理想44100..88200、事前許容300frame）で、縮小／再拡大でも選択保持。宣言済みの数値欄で44100..88200へ正規化後、Set Loop／Clone一回／UndoRedo／Save As／Save Projectを実行した。

独立RIFF監査で264600frame（6秒）、WSMP開始44100／長さ88200、PCM全byte、cue／ltxt／SMPL／fact／拡張header／tail／opaque／padが事前期待と一致。Segment／元Wave／Other Waveは不変。Project差分は第二fileの名称とfilh時刻／サイズだけ。別processは6秒とloopを復元し、viewはfit・選択0・historyなし。author／reload／音声側の3processは各通常exit0。GUI7／native4監査が合格した。

最初の録音は、許容された発音遅延によって5音目の採取窓がStop後となり不合格。失敗録音・判定・診断を保持した。[録音前の改訂](../../work/analysis/ui-wave-viewport/20261009T125539491Z/audio-protocol-amendment-v2.json)で停止予定を4.7→5.9秒へ変更し、onset／Stop許容4.2..6.3秒／無音RMS／音高／peakの閾値を維持して新録音した。二回の再生で440/660/660/880/1100 Hz、発音中Stop、停止後と末尾RMS0、再開が合格。ゼロ音声／overpeak／停止後信号／誤入力hash／早期Stopの5対照を意図した箇所で拒否した。

## 比較限界と次の作業

静的Helpと現在の200%画面で、波形下のscrollbar・右端magnifierボタン・無効表示・選択／loop色を照合した。字体・アイコン・寸法・原版動的操作・原版のUI状態保持方針は未確認。FileメニューのUIA座標エラー2回と閉じたwindowのrefreshエラーを保存し、OS拒否／製品不具合と断定していない。

全WaveEditor、modeless Properties3tabs、FindBestLoopEnd、既定wholeWave loop、圧縮／streaming／playlist、WaveTrack loop/end/override、原版比較／Q2／全40／全8は残る。既存modeless Properties/F11へWaveのSample属性を接続する。原版HelpのGeneral/Playback/Compressionの責務を固定し、実装済みname/root/loop編集とnative保存を活用する。複数Bandはflags2/Save E_NOTIMPLの保存済みAPI限界が現在も障害のため、同条件プローブは再試行せず、後続匿名Bandの対応を残す。

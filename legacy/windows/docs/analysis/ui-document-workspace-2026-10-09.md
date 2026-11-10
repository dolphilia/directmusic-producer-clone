# Segment内部編集ワークスペースと実キー検証

2026-10-09。試験候補0927、限定採用0735は維持。全体受入8は6作業中/2障害/0合格、fullAcceptance=false。

原版Helpの内部編集ウィンドウとWindowメニューを根拠に、SegmentをMain内の複数MDI子へ接続した。文書別の選択・zoom・scroll・履歴を保持し、Ctrl+Tab、Window一覧/Cascade/Tile/Close All、Treeのdoubleclick、TimelineのCtrl+Z/Y・F11・Shift+F10/右clickを実GUIで確認した。旧0831のfloating editorキー配送未確認を残し、新しい製品構造の実キー証拠として記録する。

[封存単位](../../work/analysis/ui-document-workspace/20261009T085146502Z/unit-0927-limited-v3.json) SHA256 5fa31dfd1d2c687349c46e5386bb0b68ab9eff5fa120de87d2fb5dd268add63c。234保存/作業ソース・4生成物の472照合、build/install成功、[関連native40](../../work/acceptance/regression/20261009T093225234Z/run.json)は36合格/4既知凍結。候補0854/0908も個別のnative40・4driverを登録したが、Mainの描画重なり失敗を保存し0927へ転用しない。[登録記録](../../work/analysis/ui-document-workspace/20261009T085146502Z/manifest-registration-v3.json)はnative94/driver164、2137原始一巡を保持。

[GUI監査](../../work/analysis/ui-document-workspace/20261009T085146502Z/gui-proof-v3.json)11項目と[独立RIFF監査](../../work/analysis/ui-document-workspace/20261009T085146502Z/native-proof-v3.json)6項目が成立。Song Enter Tick13→41（10189→10217 clocks）だけを変更し、Otherの一時123 BPMを120へUndoした。Other/Effectは完全一致、ProjectはSongのsize/writetimeのみ変化。作者・別process復元の二通常exit0、[終了後12入力照合](../../work/analysis/ui-document-workspace/20261009T085146502Z/postexit-input-integrity-v3.json)を確認。

[固定WASAPI](../../work/acceptance/product-project-gui/20261009T095741797Z/audio-20261009T100555603Z/run.json)と[独立音声監査](../../work/analysis/ui-document-workspace/20261009T085146502Z/audio-proof-v4.json)は8音/3Tempo、発音中Stop、停止後無音、再開を成立。Stop境界は初回[5.006,5.232]秒、再開[5.642,5.870]秒で固定[4.2,6.3]秒内。peak0.150259、停止後RMS0。[5故障対照](../../work/analysis/ui-document-workspace/20261009T085146502Z/audio-controls-qualification-v4.json)はzero/over-peak/停止後信号/input hash/early Stopの各意図したguardで拒否。旧v3はUIA set_valueでselection通知が起きずOtherを再生し、replayは録音時間外でもあったため失敗として保持。閾値は変更していない。

Tile Horizontallyでは固定6行が高さを使い切りfooter/buttonsが画面外になる。Tile Verticallyでは選択時刻footerの折り返しが欠ける。4K/200%設定を維持し、次は子編集の縦viewportとfooter配置を修正する。CtrlF11全focuscycle/ShiftF11、全moduleの内部編集、font/icon/toolbar/OLE/clipboard/全40画面一覧と原版動的比較は未達。原版app approval障害・Q2・個別Windows5を保持し、単位completed=false。

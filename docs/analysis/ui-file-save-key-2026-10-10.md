# 共通File Ctrl+Sの限定統合

2026-10-10（日本時間）。候補20261009T160423422Z。原版Helpの「編集中のfileをCtrl+Sで保存」に沿って、MainのSegment/Style/Bandへ既存Saveコマンドを接続した。変更はmain.cppのキー配送とメニュー表示。全体完成・原版比較の合格ではなく、completed=false/fullAcceptance=false。0735限定採用を維持する。

236保存/作業ソースと4出力が一致し、関連native8試験が合格した。実GUIはSegment8→9、Style113→117、Band volume101→96を保存し、各文書のUndo/Redoを維持した。BPM欄の未確定99は保存へ入らず、TreeでControl.sgpを単一選択しても保存先は開いているSourceSequence.sgpのままだった。Ctrl+Shift+Sは保存/Save Asへ誤配送されず、Save As内のCtrl+SとEscapeも全ファイルを変えなかった。空ProjectのCtrl+Sはダイアログを出さず、Save Fileは無効だった。

独立GUI監査13項目とnative監査5項目が成立。3編集文書はGUI開始前に公開RIFF構造から生成した期待bytesと完全一致した。Projectは3編集文書のfilh更新時刻/sizeだけが変化し、残る入力は不変。作成PID22464、復元PID17412、空ProjectPID20016はすべて通常exit0。別processで9/117/96を復元し、Undo/Redoは空だった。

音声は今回のキー配送変更の判定対象外。fixtureのtempo/volumeは音に影響する既存編集であり、native保存対象だけを確認した。cold Wave初回onset1.021秒の失敗は保持し、今回の非適用で合格へ変えない。

FileメニューのCtrl+S文字列はUIAで確認したが、メニューの画素・字体・配置比較は未確認。共有modeless PropertiesからのCtrl+Sは実装済み・実操作未確認。Wave/DLS/AudioPath等の他editor、原版の動的操作/外観、Q2独立Windows、全40/全8、凍結Windows5/Clipboard5、最終登録94/164一巡は残る。

メニューUIA clickの範囲外エラーは保存し、現在window/screenshotを再選択して観測済み座標で復旧した。終了後のstate取得エラーはlauncherの通常exit0へ結び付けた。BPM欄のCtrl+Aが全文選択にならない既存差分も保存し、次の独立共通UI単位へ分けた。

- [単位記録](../../work/analysis/ui-file-save-key/20261009T160334488Z/unit-record-v1.json) SHA256 eba7b3bf1f4a9fe3185fa98d5e4380a9a240b22a452964d3a1f30334702a2462
- [GUI監査](../../work/analysis/ui-file-save-key/20261009T160334488Z/gui-proof-v1.json) SHA256 cdb047826e948a38dfa3f4751af1f51db89e720800286652f4cbceadd99ab1bf
- [native監査](../../work/analysis/ui-file-save-key/20261009T160334488Z/native-proof-v1.json) SHA256 6676f5edbe1e1f9e1eea8e800bf231ed59862cb04246f29f6ad9309f8c6692e6
- [関連回帰](../../work/analysis/ui-file-save-key/20261009T160334488Z/related-regression-v1.json)

再現: 固定SDKでBuild-ProductSnapshot.ps1を実行し、Test-RegressionManifest.ps1で記録の8 modeを実行。Test-ProductProjectGui.ps1でGUI protocolのauthor/reload/empty入力を別processで操作し、同フォルダのaudit-native-v1.mjsとaudit-gui-v1.mjsをNode24で実行する。全一次入力・操作時刻・観測画像・期待値・判定器・生成物・通常終了は単位記録から辿れる。

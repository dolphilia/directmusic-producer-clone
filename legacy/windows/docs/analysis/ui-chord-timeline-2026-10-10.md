# Chord Timelineと共有Propertiesの接続

2026-10-10（日本時間）。試験候補`20261010T000048822Z`。限定採用0735と全40責務・全8受入を保持。`completed=false`、`fullAcceptance=false`。

選択groupに交差するChord trackをnative順でTimelineへ表示し、空の先行trackも行として保持する。イベントを選択し、F11、右メニュー、Propertiesボタンから共有modeless Propertiesを開ける。今回の編集範囲はChord type名だけ。時刻・root・pattern・scale・level・flagsは変更しない。原版helpの鍵盤・転調・Inversion・MIDI・試聴等は残る。

[build](../../work/build/product-snapshot/20261010T000048822Z/build-summary.json)は238保存/作業ソース・4出力一致、configure/build/install0。関連13 modeは合格。既存Chord modeに空track・重複group・nth選択・拡張record/未知chunk保持・no-op後Redo保持の7試験を追加した。dispatcherや登録対象は変更していない。

本体では空Chord 1の次のChord 2を選び、MoldをM9 (#11)へ変更した。Timelineにフォーカスを移した文書Undo/Redo、別イベントG7へのProperties追従、Tempo/Time Signatureとの切替、Ctrl+S、Save Projectを実行。Chordのない別Segmentでも行は空のまま、PChannel 17のMuteクリックとUndoを確認した。作者22236と別起動22640は通常exit0。再起動後もM9 (#11)/G7と未編集状態を確認した。

[独立監査](../../work/analysis/ui-chord-timeline/20261009T235710971Z/unit-proof-v1.json)は744照合。保存SGPは最初のChord名のWCHAR16領域だけが変更され、他の全bytes、44-byte Chord/24-byte SubChord拡張、未知chunk、二つ目のChord、空trackを保持した。HighMute.sgpは全bytes不変。Projectは同じ二参照を保ち、変更はfilhの時刻/sizeだけ。3保存入力は再起動後もhash一致。

入力欄内のCtrl+Z/YはEdit control内の文字履歴であり、文書Undo/Redoの証拠には使用しない。所有Propertiesはlist_windowsに列挙されず、未定義windowの観測が一度拒否されたが入力は送っていない。最新の本体画像で対象を確認して継続した。終了直後の画面取得は閉じたwindowとして失敗し、launcherのexit0を別に確認した。[画像・操作の評価](../../work/analysis/ui-chord-timeline/20261009T235710971Z/gui-review-v1.json)に区別を残す。

音声は名前だけの変更に限り非適用。音程・再生への効果は今回の合格範囲に含めない。Mute拡張録音、Transport/Band/Sendの既存音声失敗は未達のまま保持する。原版動的比較、Chord/Scale鍵盤とroot転調、Inversion、CRUD/drag/clipboard、Track Propertiesを残す。4K/200%を変更せず、保護対象原版21252には入力していない。

次はChord/Scale鍵盤と転調操作の契約を確認し、既存native編集へ接続する。[単位記録](../../work/analysis/ui-chord-timeline/20261009T235710971Z/unit-record-v1.json)からソース・入力・生成物・13回帰・画像・保存監査・二終了を辿れる。

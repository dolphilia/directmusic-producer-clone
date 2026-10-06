# Q3 Timeline選択範囲の移動（限定単位）

候補 `20261006T185539543Z`。全体未完了。8受入は6作業中・2障害、`fullAcceptance=false`。

Tempo/Sequence共通Timelineに、選択範囲を指定位置へ移動する操作と実ドラッグを追加した。半開範囲をprivate copy上で削除・merge pasteし、一回のUndo/Redoとして確定する。不正位置、空範囲、同位置、音符終端超過は文書・dirty・Redo・選択を保持する。重複時のTempo置換とSequence追加はソース側の暫定契約であり、原版動的互換の合格ではない。

195保存ソースからconfigure/build/install各exit0。EXE SHA256 `a89961826e90c01e4a163a6c0fd8d518d7f8ca35bcae9e847a0e34bc1062ef26`、core EXE `0a5d8332265a91724c323da23903ddb28a95515a88ebe37b4a6fc608b01aa352`。build-summary SHA256 `dd54cc967afe8a39480071c36e805fb1c98bde2b985b99cbd18b51478632383f`。[build](../../work/build/product-snapshot/20261006T185539543Z/build-summary.json)、[単位記録](../../work/analysis/q3-timeline-move/20261006T185000Z/unit-record.json)にソース・生成物・入力・環境を接続した。準備候補2件も保存し、実行結果を最終候補に混同しない。

固定候補のnative77件は47合格・30障害、driver107件は20合格・21障害・66未実行。Timeline44、Sequence26合格。通常core等の既知Windows5保存拒否は凍結し、合格に含めない。[native一巡](../../work/acceptance/regression/20261006T190050749Z/run.json)、[driver一巡](../../work/acceptance/registered-drivers/20261006T190051438Z/run.json)。

本体PID19396で[768,1536)を1536へ移動し、一回のUndo/Redoを確認した。Move by dragを有効にした実ドラッグで[3072,3840)へ移動し、Timeline Undo→本体Redo→文書/native Project保存→正常終了exit0まで確認した。同一EXEの別PID22468で音符3204/4740 clocks、Tempo0/2304/3072 clocksを復元し、exit0で終了した。[GUI結果](../../work/analysis/q3-timeline-move/20261006T185000Z/gui/gui-result.json)。

保存Range.sgp SHA256 `ce9233c34fb104acddb53e8360bfbfaeca32226be6b3ce93807e1e5ac2bcc116`。独立RIFF監査で対象外イベント、拡張レコード、DWORD PChannel、未知データの保持を確認した。無変更入力・移動clock改変・拡張byte破損の負対照はすべて拒否した。監査器SHA256 `b41d19f96433676454ea3affdd2c6e2705a62ba15394cf98a85a9829a44f4664`。[監査結果](../../work/analysis/q3-timeline-move/20261006T185000Z/gui/gui-file-proof-cli.json)。これは原版比較の代替ではない。

再現はPowerShell7で `& ./scripts/Build-ProductSnapshot.ps1`、生成build-summaryを `Test-RegressionManifest.ps1` と `Test-RegisteredNativeDrivers.ps1` の `-BuildSummaryPath` へ渡す。Timeline入力はnative一巡の timeline-range/core/TimelineProject に生成する。監査はNodeで `Inspect-TimelineMoveSave.mjs BASE.sgp SAVED.sgp OUTPUT.json` を実行する。GUI操作の入力hash・座標・UIA観測は単位gui/observationsに保持した。

この候補の音声は未実行。旧Wave/Fresh音声成功は履歴のみ。原版designer不在、Q2独立Windows未用意、全strip Timeline/ABI/OLE/Snap/meter等、全40責務は未完了。次は同じ固定候補でQ1新規native五形式→編集/UndoRedo/保存/終了→別プロセス復元→WASAPI発音中Stop・再開・テンポ・Style/Transport優先を結ぶ。Marker/Lyric/Mute等の残責務はqueueに維持する。

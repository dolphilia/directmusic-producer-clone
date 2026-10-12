# Sequenceのスナップ・キー操作の限定検証（2026-10-11）

画像14・19の入力位置とSnap Toに対応する補足。候補は `20261011T135522063Z`。原版の同じnative入力で確認できたキー操作の差を修正し、本体操作、保存データ不変、別プロセス再読込、通常終了まで確認した。画像18の左右端・中央ドラッグ比較を含む親単位は未完であり、SSP04全体を合格にしない。

## 原版で確認した動作

正式原版 `C:\Program Files (x86)\Microsoft DirectMusic Producer\DMUSProd.exe`、PID25548を参照した。EXE SHA256は `fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011`。専用コピーのProject/Segmentは候補と同じバイトで開始した。

- Snap ToはNone / Grid / Beat / Barline。Ctrl+E / G / T / Bが表示され、Grid・Beat・Barlineのチェックとショートカットの反応を確認した。
- Noneでは空白クリックでグリッド外の入力位置を設定でき、Rightでは変化せず、Upで音高位置だけが変わる。
- スナップ設定をキーで切り替えても、現在の入力時刻を変更しない。
- Gridの左右キーはグリッド外のずれを保ったまま1グリッドずつ移動する。原版は赤線の15物理ピクセル移動、ルーラーとの対応、Offset表示で確認した。元の時刻を厳密な数値として取得した証拠ではない。

原版のノートドラッグは選択矩形になり、有効な変更結果を得られなかった。試行を保存し、操作手段や原因を未確定のまま残した。F11のSequence Propertiesで、対象ノートがC#5、Velocity80、開始2784、長さ576 clocksのままであることを確認した。候補のノートProperties入口が未実装であることもqueueに追加する。

原版と候補のクライアント寸法は異なる。200%表示を維持した機能比較であり、全画面の厳密な外観一致の証拠にはしない。

## 修正

| ソース | 内容 | SHA256 |
| --- | --- | --- |
| `src/producer/segment_timeline_window.cpp` | 原版と同じSnap To表記とショートカット。Sequenceノート文脈でキーを処理し、切替時に入力位置を丸め直さない | `e9e3caac8f0eb533eda63b85080e9d45cf6d4a3631bf9d0b42af77f8d16f4cdc` |
| `src/producer/sequence_strip_model.cpp` | Gridの左右キーで時刻のずれを保ち、1グリッド分を相対移動 | `a41c47a986ebbba648f0f0bf176f6c5b9e0a28a80d21dd3d7e9a5e2407feb1f5` |
| `tests/producer/sequence_strip_tests.h` | グリッド外4327→4519→4327の回帰条件を追加 | `5e16cceb8b7c94a7403087d1b976b1c909bf77247b88b8f0f835d7c3108326d1` |

Beat・Barlineのグリッド外左右移動の意味は原版未確認であり、変更していない。ノート端・中央のドラッグ処理も今回変更していない。

## 候補での結果

configure/build/installは全て0。保存ソース288件と作業木、生成物4件のhashが一致。EXE SHA256は `929dc43520f876092661332d1ea919d2bec4b409a4c11086df61d3bbbac7ef03`。

関連4モード364検査（common-open53、timeline-range117、sequence-crud164、sequence-audition-runtime30）が合格した。本体の11条件、native照合7条件も成立した。

- Noneで空白クリックした位置4335 clocksを、Right、Up、Ctrl+G/T/Bで維持した。Upでは半透明ノートの音高位置だけ変わった。
- GridのRightで4527、Leftで4335 clocksへ戻り、192 clocksの相対移動とグリッド外のずれの保持を確認した。
- Snap Toのショートカット表記とBarlineのチェックを確認した。
- 表示操作後にSegmentの全バイトと8ノートが不変。Project保存後は`filh`のFILETIME・サイズに当たる16〜27バイトだけが変化し、参照数1と他の全leafを保持した。
- 作者PID1988と別読込PID20596は同じEXEで実行し、再読込で8ノート・Organ1・TimeSig・Tempo・Bandを復元。双方通常exit0。
- 作者の終了時に未保存確認が出た。破棄は選ばず、Noで戻ってSave Projectを実行し、終了した。最初のCtrl+SだけでProject保存完了とは判定しない。
- 読込モジュール80件はx86アプリ側74件と既知WOW64ホスト6件。原版40モジュールのhash一致0。実行中の一点での証拠であり、Q2独立Windowsの証明ではない。

音声はこの表示設定・キー移動の補足では非適用。発音・ドラッグ中previewのPCM合格を主張しない。全native100/driver172一巡は新候補で未実行。100711等の旧候補の結果を転用しない。

## 証拠・再現

一次記録は `work/analysis/ui-note-snap/20261011T133724182Z/`。Git対象外のworkとscreenshotは別途保全し、hashを照合する。

- [開始記録](../../work/analysis/ui-note-snap/20261011T133724182Z/start-v3.json)、[原版キー観測](../../work/analysis/ui-note-snap/20261011T133724182Z/original-keyboard-observation-v2.json)、[原版ノート数値](../../work/analysis/ui-note-snap/20261011T133724182Z/original-note-numeric-proof-v1.json)
- [原版ドラッグ未成立](../../work/analysis/ui-note-snap/20261011T133724182Z/original-drag-attempt-status-v2.json)、[ソース変更](../../work/analysis/ui-note-snap/20261011T133724182Z/source-change-v1.json)
- [本体・保存・再読込](../../work/analysis/ui-note-snap/20261011T133724182Z/gui-keyboard-save-reload-proof-v1.json)、[native照合](../../work/analysis/ui-note-snap/20261011T133724182Z/native-keyboard-supplement-audit-v1.json)、[最終同一性](../../work/analysis/ui-note-snap/20261011T133724182Z/closing-identity-v1.json)
- [ビルド](../../work/build/product-snapshot/20261011T135522063Z/build-summary.json)：SHA256 `f5b402121d27c7e8333a632df1352fa0cca873a5625c825166974cd891a8385a`
- [関連回帰](../../work/acceptance/regression/20261011T140242784Z/run.json)：SHA256 `2b68e0469a11c14dca448fcdabfef282543a1deaac105ec857c84aaf2d79698e`

PS7で`work/analysis/ui-note-snap/20261011T133724182Z/run-related-regression.ps1`を実行すると関連4モード、`launch-candidate.ps1`でこの保存済みProjectの候補GUIを起動できる。操作列と前後PNGは同ディレクトリのoriginal/candidate/reload各JSONに保存した。v1/v2のfixture生成失敗も残し、原版の製品障害とは扱わない。

## 残る作業

親単位の画像18・19の同入力ドラッグ比較、候補の同候補での編集・UndoRedo・保存/再読込、原版の数値変更確認は未完。Noneでの右クリック位置保持、Beat/Barlineのグリッド外矢印、キー押下中のdrag/cancel、ノートSequence Propertiesも残る。原版のドラッグが成立しない同じ試行を繰り返さず、入口と数値編集など独立して進められる責務から継続する。

限定採用0735、独立Style015244、6作業中/2障害/0合格、`fullAcceptance=false`を保持する。旧Band停止失敗、Count-In15/16、Echo、SSP04/08全体、Properties残欄、Add Tracks初期focus/原版空選択、Q2、OS凍結の残責務を引き継ぐ。

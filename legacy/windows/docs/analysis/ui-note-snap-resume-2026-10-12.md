# ノート端・移動とスナップの再検証 — 2026-10-12

175125・画像18/19の親単位を再開。候補の左右端/中央move×Grid/Noneの6操作、UndoRedo、独立native54+閉鎖17検査、作者10876/別2788の二通常exit0と最終3420/760/C#5/80復元が成立。7他ノート/許可外bytesを保持。原版は低解像度後も選択矩形となり値不変で、ドラッグ比較は未完。製品ソース変更なし、291保存/作業ソースと4出力一致。 親単位はcompleted=false、fullAcceptance=false。

| 保存した操作 | 開始 | 長さ | 音高 |
| --- | ---: | ---: | ---: |
| 右端 Grid | 2784 | 803 | 61 |
| 左端 Grid | 2880 | 707 | 61 |
| 中央移動 Grid | 3072 | 707 | 62 |
| 左端 None | 3246 | 533 | 62 |
| 右端 None | 3246 | 760 | 62 |
| 中央移動 None | 3420 | 760 | 61 |

ベロシティ80とPChannel1は不変。Gridの開始位置は192 clocksのグリッド上、Noneはグリッド外。左端操作は終端を、中央移動は長さを保持。右端Gridの終端3587はグリッド外。Undo後は基準SGPと全バイト一致し、Redo後も編集済みSGPと全バイト一致した。別processのPropertiesは開始3420・長さ760・C#5・80で一致。物理trackは0始まり3（Sequence trackは1番目）で、旧抽出のSequenceのみ0始まりとは区別する。

RIFF監査は他7ノートと未知データを含む全許可外bytesを保護した。PRO差分は既存filhのLastWriteと、古いfileSize2894を実サイズ2902へ補正した箇所だけ。timestamp-onlyとは呼ばない。関連4モード413検査は同じ175125候補の3つのrunから適用条件を確認した補足で、全登録一巡ではない。291ソースと4出力のhash不一致0。新しいmodules/音声の観測は行っていない。

原版は右端の左右矢印と中央の十字矢印を観測できたが、dragは選択領域となり、Propertiesの2784/576/C#5/80は前後同じだった。OS拒否や原版の不具合とは断定しない。低解像度への変更だけでは解決せず、同条件の再試行は止める。旧入力2ファイル・過去の保護入力4ファイルとインストールEXEのhashは不変。原版の既存sessionは開いたままにした。

最初の候補ドラッグ2回も右端ではなく移動として成立したため右端成功に数えていない。Undoで復旧し、拡大表示と2pixel以内の右端を取り直してから長さ変更を確認した。操作セッション再選択とローカルmodule importのEPERMは観測/監査手段の履歴で、製品失敗へ転記しない。監査は既存独立RIFF parserをNode CLIから実行した。

[開始](../../work/analysis/ui-note-snap-resume/20261011T183449Z/start-v1.json)、[単位記録](../../work/analysis/ui-note-snap-resume/20261011T183449Z/unit-record-v1.json)、[native54検査](../../work/analysis/ui-note-snap-resume/20261011T183449Z/native-proof-v1.json)、[閉鎖17検査](../../work/analysis/ui-note-snap-resume/20261011T183449Z/final-audit-v1.json)、[原版未成立](../../work/analysis/ui-note-snap-resume/20261011T183449Z/original-observation-v1.json)。

次：SSP04 Quantize親単位の未観測算術を専用同入力コピーで比較する。原版でSelected Start/Part Startの同距離・Duration同距離・Strengthを実行しnativeを保存、観測値を固定してから候補へ同操作。差があれば修正/関連回帰/保存復元/二通常終了。ノート端の原版ドラッグは手動成功の数値証拠またはComputer Use drag条件変更まで同条件再試行しない。複数物理Part/境界、DL34 Enter/空OK、Add Tracks空OK、Count-In、Style015244、Q2、47native/29driver凍結、全40/全8を保持。

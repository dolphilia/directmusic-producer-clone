# Quantizeの原版算術比較と端数修正 — 2026-10-12

193237：正式原版の同入力5条件（Selected/Part開始、Part長さ、Strength33/1）を先に固定し、符号付き端数の0方向切捨てへ修正。原版2784→2753と1728→1729/3264→3265に候補が一致。8ノート比較/native27検査、閉鎖20検査、UndoRedo全バイト復旧、291保存/作業ソース・4出力一致。修正済みProjectで4696/7692二通常exit0と1729復元が成立。旧作者6640の未保存検証Project破棄は回答待ち。 全体fullAcceptance=false。

| 同じdivision2の操作 | 原版と候補の保存値 |
| --- | --- |
| Selected Start 100% | 対象2784→2688、他7ノート不変 |
| Part Start 100% | 1728→1920、3264→3456、対象2784→2688 |
| Part Duration 100% | 192→384、対象576→768 |
| Selected Start 33% | 2784→2753 |
| Part Start 1% | 1728→1729、3264→3265、2784不変 |

旧候補はStrengthでllroundを使い2752や+2/−1となった。符号付き整数除算で移動差を0方向へ切り捨てるよう修正し、原版観測3例をassertへ追加した。既存assertとPChannel DWORDの保護は維持。原版の結果を先に固定し、候補へ同じ5操作を適用。ノート以外のnative bytes、他ノート、opaque/tail/controller/padding、Undoの基準全バイト、Redoの編集後全バイトを独立監査した。

最初のProject fixtureはfilh+28のruntime object GUIDが古いままで、製品の保存guardがidentity changedを拒否した。fixture作成誤りとして履歴を保持し、原版/候補の保存済みSegmentを一切変更せずProjectの参照だけv2へ訂正した。filh+0は別のauthoring file GUIDなので変更していない。5算術操作はv1、正常Projectの保存復元はv2で、同じSGP hash031d34148541c02146384bb2c83a15199c4db605985b4e2dbdc4fbea2a4c9501が結ぶ。算術をv2で再実行したとは扱わない。

最初のv2ペア22484/3596は通常exit0だったが、Segmentを保存せずProjectだけ保存したためfilhサイズ2894が残り、監査は失敗した。この失敗を保存し、4696でSegment→Projectの順に保存した後、7692で復元/1729を観測して通常exit0。Project差分はLastWriteとfileSize2894→2902のみ。旧作者6640は未保存の検証Project状態の破棄回答を待って開いたまま。単位全体completed/boundedFinished=false。

旧報告の「原版初期division2一致」は音符glyphの誤読だった。今回は表示4を確認し、明示的に2へ変更した。セッション記憶の可能性があるためfactory defaultは未確認。原版はdivision/Strengthを保持し、Undo後Start/Durationのチェック状態が変わる観測も保存。候補の記憶・音符glyph・無選択入口・複数物理Part・Segment境界・厳密外観は別queue。今回の端数3例から全除算/全境界の仕様へ一般化しない。新PCMは未実行、旧175125の音声16/16を転用しない。

[単位](../../work/analysis/ui-quantize-original-compare/20261011T190600Z/unit-record-v1.json)、[原版固定](../../work/analysis/ui-quantize-original-compare/20261011T190600Z/original-proof-v1.json)、[native27検査](../../work/analysis/ui-quantize-original-compare/20261011T190600Z/native-proof-v2.json)、[閉鎖20検査](../../work/analysis/ui-quantize-original-compare/20261011T190600Z/final-audit-v4.json)、[fixture訂正](../../work/analysis/ui-quantize-original-compare/20261011T190600Z/project-fixture-correction-v2.json)。

次：SSP07/画像43のChoose DLS Instrument空一覧で、未確認のEnter/OKを正式原版→候補の順に同入力で観測する。空一覧でも原版OK有効・候補無効の差を、確定後の文書/履歴効果まで調べ、差があれば修正して関連回帰・保存/別復元/二通常終了を行う。6640の検証Project破棄は明示回答まで保留。Quantize複数物理Part/境界/音符glyph/設定保持、原版drag未成立、AddTracks空OK、Count-In、Style015244、Q2、48native/30driver凍結、全40/全8を保持。

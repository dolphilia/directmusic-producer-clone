# CC効果と発音中Stopの原版比較 — 2026-10-12

224147 CC原版比較：同じ4曲線/1ノートのPBend・Expression、初回発音、発音中Stop/無音/再開を両側で確認。原版・候補とも固定15/16で、再開Stop14.795/14.567秒が4〜8秒条件を超過。閾値不変、全体音声は不合格。native19/閉鎖25、291保存/作業ソース・4出力一致、18216/17936二通常exit0・別復元成立。原版の設定を復旧。厳密波形/外観・短時間Stop・全CCは未完。 全体fullAcceptance=false。

同じSGP（SHA256 e06729234faaad43e78b8db7080e140d8d2ee4e08dafa72d58ac2440bed36bd2）、120BPM/4拍子、Organ1/MIDI60/velocity96、63秒ノートを使用。PBendは8〜24秒で8192→12288、24〜32秒で復帰。Expressionは32〜40秒で127→40、40〜48秒で復帰する。原版はStandard Stereo、候補は既存default route8、録音endpoint.txtは一致。原版内部の実GMロード元や厳密波形/外観の一致は主張しない。

| 測定 | 原版（第2区間） | 候補224147 |
| --- | ---: | ---: |
| 発音遅延 | 0.091秒 | 0.317秒 |
| PB変化 | 0.919半音 | 0.904半音 |
| Expression低音量/基準 | 0.131 | 0.139 |
| 回復/低音量 | 7.659 | 7.185 |
| 最初のStop | 56.077秒 | 56.118秒 |
| 再開Stop（要求4〜8秒） | 14.795秒・失敗 | 14.567秒・失敗 |

両側のStop直前0.7〜0.2秒のRMSは約0.048で、最初のStopが自然終了前の発音中だったことを補足監査した。停止後と最終区間のRMS/peakは0。既存300秒/16条件の閾値・音高/音量測定窓は変更していない。原版の初回15.503秒超過を保存後、画像処理を省く手順へ変えたが14.795秒で再び超過した。候補も14.567秒で、短時間条件は未達。同じ操作方式での反復は止め、実装のStop不具合とは断定しない。

原版第1区間は14/16で、再開停止の超過に加え、後続の明示的な第2区間の音が最終無音窓へ入る。第1区間を成功へ差し替えていない。第2区間の15/16と候補15/16を比較するが、原版は既に再生済みのhot条件なので、初回発音遅延の完全等価とはしない。新しい画面操作は開始前の961×752/1207×794から取得。OS倍率は未測定で旧4K/200%を現行値にはしない。

候補は保存→作者18216通常exit0→別17936で復元/録音→通常exit0。SGP全bytesは不変。Project差分は明示SaveによるfilhのFILETIME/size12bytesのみで、その他bytes不変。最初のnative監査器はopaque seqt内部を探索せず曲線3条件が失敗したため、失敗を保存してpayloadの独立解析へ訂正し19/19。製品データ/期待値は変更していない。閉鎖25検査は証拠/生成物/保護入力の照合であり、音声16条件の合格を意味しない。製品ソース変更はなし。最新一巡224147 native51/2990、driver18は別の原始記録として維持する。

[単位](../../work/analysis/ui-cc-stop-original-compare/20261011T231700Z/unit-record-v1.json)、[閉鎖25](../../work/analysis/ui-cc-stop-original-compare/20261011T231700Z/final-audit-v1.json)、[native19](../../work/analysis/ui-cc-stop-original-compare/20261011T231700Z/native-current-proof-v2.json)、[原版15/16](../../work/analysis/ui-cc-stop-original-compare/20261011T231700Z/original-v2/audio-proof-v1.json)、[候補15/16](../../work/analysis/ui-cc-stop-original-compare/20261011T231700Z/candidate/audio-proof-v1.json)。

次：SSP06/画像29〜33の空CCレーン保存を、専用同入力コピーで正式原版の追加→明示Save→別文書/再openから観測する。曲線を作らずPBend/Expressionのレーンを保存できるか、design chunk/dirty/history/開閉を分離し、候補のsession-only状態との差が実在すれば修正する。DWORD PChannel、未知design/curve/note bytesを保護し、新候補の関連回帰・本体保存/別復元/二通常終了へ進む。CC短時間Stop未達、SSP04親/6640破棄回答待ち、独立Style015244/Q2、48native/30driver凍結、全40/全8を保持。

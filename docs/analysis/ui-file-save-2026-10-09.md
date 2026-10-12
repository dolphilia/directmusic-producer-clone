# File保存対象と既存Project保存の修正

2026-10-09（日本時間）。候補2217の限定成立と未達を記録する。単位は未完、`fullAcceptance=false`。限定採用0735を維持する。

原版Helpは、編集中componentのファイル名をSave/Save Asへ表示し、editorがなければ無効化する。[開始時の契約](../../work/analysis/ui-file-save/20261008T221224390Z/unit-start.json)に従い、main Segment/Style/Bandの対象判定とラベル、既存pathでのSave Projectを修正した。明示Save Project Asは別command3302を維持する。原版の動的観測は既存approval timeoutで未確認。

候補は[2217 build](../../work/build/product-snapshot/20261008T221704772Z/build-summary.json)。229保存/作業ソースと4生成物が一致し、configure/build/installは各0。Producer.exe SHA256は`d0478aed91263360b96baad4d6e7109f4f1d31c1f6f98e648dfdc2da22743ea4`。変更したmain.cppは`4f6360392bc0c1527331f449455499252f63ce1c3fc6c871fe93e0ec92a6ca8e`。表示設定は200%を維持した。

本体で、Tree caretが別文書にあってもactive editorを保存対象にする表示、空ProjectでのSave/As無効化、既存Project保存がdialogを開かないことを確認した。Segmentは7→8 BPM、Styleは109→113 BPMのUndo/Redo・保存を実行し、別processで値・内部名・8ノート・Patternを復元した。明示Document/Project Asと新規未命名Project/Documentの取消しで、native bytes・dirty・履歴・保存対象が変わらないことを確認した。

[独立native監査](../../work/analysis/ui-file-save/20261008T221224390Z/native-proof.json)は7項目合格。親の17入力を保全し、変更はSourceSequence.sgp・ToneStyle.stp・Fresh.proのみ。Segment/Styleは対象Tempo以外の全bytesが一致し、Projectは二つのfilhの更新情報以外を保持する。Empty・author・reload・unnamed・録音準備の5processは異なる開始時刻/PIDで、強制終了なしの通常exit0。新規Untitledの終了には人が承認した検証用文書の破棄を用いた。新規保存成功の証拠ではない。

[関連13の原始run](../../work/acceptance/regression/20261008T222216593Z/run.json)は9合格/3失敗/1既知障害。新たな3失敗は物理path解決エラー5。読み取り専用診断で同一ファイル/同一API・フラグと二つの同一フォルダー/std::filesystem::canonicalについて、通常sandboxの拒否と承認された実行環境の成功を確認した。OS/COM/セキュリティ設定は変更していない。[Send解除根拠](../../work/analysis/ui-file-save/20261008T221224390Z/send-path-context-release-v1.json)、[canonical解除根拠](../../work/analysis/ui-file-save/20261008T221224390Z/canonical-context-release-v1.json)に基づく[Send補足](../../work/acceptance/regression/20261008T225210113Z/run.json)89、[Runtime補足](../../work/acceptance/regression/20261008T225402510Z/run.json)25/15は合格した。実効結果は12合格/既知jazp障害1。原始失敗を保持し、全登録92/164の新しい一巡として扱わない。通常sandboxでは同条件拒否が残り、他の凍結対象も解除していない。

未達は、新規native実保存・固定音声・原版の外観/操作比較。Save dialogの観測された編集要素へのset_valueが`element 608 is not available in cached app state for Producer.exe`となり、座標focus後の入力も画面へ反映されなかった。録音準備の別processでも168への同じcacheエラーが残り、text-only再取得後も成功しなかった。[録音準備記録](../../work/analysis/ui-file-save/20261008T221224390Z/playback/preparation-blocked.json)を保持し、録音/Play/PCM判定は未実行とする。原因をOS拒否や製品不具合に断定しない。取消して通常終了し、別UI APIや判定条件変更による迂回をしていない。

詳細は[単位記録](../../work/analysis/ui-file-save/20261008T221224390Z/unit-record.json)、[操作列](../../work/analysis/ui-file-save/20261008T221224390Z/actions.json)、[画面・状態](../../work/analysis/ui-file-save/20261008T221224390Z/observations.json)。同じ未達入力の反復を止め、独立して不足を減らせる共通Editメニューとdocument Undo/Redoの入口へ進む。Toolbar/MDI、子modal、複数Project、未保存close、dialog初期値は別queueとして保持する。

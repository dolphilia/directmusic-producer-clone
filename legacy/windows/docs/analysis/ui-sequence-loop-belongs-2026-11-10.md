# Sequence所属拍とloop包含の不合格記録

210902079のSequence所属拍loop検証で、baseline新240sは19/19・7負例4clock合格、所属2:1の別復元新240sは18/19不合格。両再生11発音は確認したが、期待1.5+2.5n秒より約0.31〜0.39秒遅れる。閾値不変・不合格PCM保全。関連5/698、二編集/二UndoRedo/native2bytesのみ/別復元/作者7524・復元16908通常exit0は検証済み。runtime実測prepare300msが負offset500msより短いことを原因候補として、lookahead方針を次単位で修正・新build/関連/新PCM再検証する。同条件再録音はしない。全40全8・50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-loop-belongs/20261109T210700000Z/unit-record-v3.json`。論理所属による発音数の変化は観測したが、時刻条件が未達なので単位完了・受入合格へ昇格しない。Loop tab・原版同入力動的比較・全geometryも未達。

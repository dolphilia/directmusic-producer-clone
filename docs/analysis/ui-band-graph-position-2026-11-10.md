# Band グラフの保存値に基づく配置 — 2026-11-10

174532050はBand graphのinactive8列rackを除き、storedpan-volumeから全ボタン位置を計算。2source変更290同一/292savedworking4outputs、関連3モード501checks（新7）合格。作者/別復元PID[5788, 13916]で16buttonがstored64/100同座標に重なり16が上に表示、一覧row1/16選択とR1/100、resize後の再計算/再保存・二通常exit0を確認。三Segment全bytes（flags/値/Band/owner/GUID/全track/opaque/二control17notes）保持、ProjectはFILETIME以外一致。

inactive色判定/再生有効性を変更せず全同等性未達。drag開始位置も修正座標を使うため新7契約で端点/群移動/縮小量子化を確認したが、実GUI musical drag/livePCMは別未達。表示選択resizeのみで新PCMなし・旧音声合格転用なし・旧6s停止失敗保持。全40全8/50native30driver凍結/fullAcceptance=false。 作者の最初のresizeはclient内を掴み変化なし、境界再取得後の枠線dragで成功し別復元でも再現。次契約audit-v1の原版capture番号023は実在019へv2で訂正、一次結果変更なし。

一次入口：`work/analysis/ui-band-graph-position/20261109T174400000Z/unit-record-v3.json`。

Sequence Propertiesに未実装のBelongs to Beatを次単位で進める。CHMは物理発音位置と論理所属を区別、現parserはmtTime+nOffsetのみを公開。明示的な論理位置読出/変更API、物理onset・duration/PCh/status/pitch/velocity/opaque保持、signedSHORT境界、発生位置/Undo選択を先に設計し、保存build/関連/本体編集保存通常終了/別復元/必要PCMまで進める。原版の初期1:1比較は既存証拠、offset編集/variablemeter/loop全同等性は別照合。globalTempo/全外観も未達。既知OS凍結の同条件再試行なし/fullAcceptance=false。 goal active。

# 空DLS一覧のOK／Enter確定と表示修正 — 2026-10-12

204251：画像43の空DLS一覧で原版が数値bank0/0・patch16とcollection参照を確定することを先に固定し、候補のOK/Enterを有効化。空collection確定後もOrgan 1表示を維持。関連5/375、native32・閉鎖22検査、Undo/Redo全bytes、291保存/作業ソース・4出力一致、作者11260/別7240の二通常exit0・表示復元が成立。原版flags0x10e3と候補0x63、既定再表示Enter、空collection音声/厳密外観は未解決。 全体fullAcceptance=false。

正式原版を先に専用コピーで観測した。Empty Accept LA39はInstrument 0件だが、Bank Hi/Lo 0/0・Patch16とOKが有効。明示選択してOKするとPCh0にcollection GUIDと数値localeが保存され、音色名はOrgan 1のまま。再度コレクションを明示選択してEnterを押した結果も同じ2814 bytes。既定選択のまま再表示Enterは参照を外して2608 bytesとなったため、これをキーだけの差とは断定していない。元の3ノートと音楽データは保持。原版のserializerによるPCh1無効参照の除去やpriority等の差は、事前Save基準がないので全てをOKの作用とは扱わない。

候補は従来Instrument indexが必須だったため、空一覧でOKが無効だった。collectionと検証済みbank/programを受け取るFramework経路へ接続し、無効数値・予約PChannel・不正collectionでbytes/dirty/historyを保持する5検査を追加した。既存assertは削除・緩和していない。最初の203145で確定・保存・関連375は成立したがDLS17表示が原版と異なり、空collectionだけGM名へfallbackする修正を加えた204251でOrgan 1を確認した。非空collectionの表示方針を一般化して変更していない。

204251ではCancelが基準2086 bytesと完全一致。OKとEnter、Redo、保存・別復元は2150 bytes/hash b9ba5edcc01efa413ac3624b7a4c075ef3abb6c02f508418c917c54873c5cb2cで一致し、Undoが基準全bytesへ戻る。許可されたPCh0参照/flags以外、他チャンネル・全3ノート・track/opaque bytesを保持した。Project差はSegmentのLastWriteとfileSize2086→2150のみ。DLSは全bytes不変。作者11260と別7240が通常exit0、別プロセスでOrgan 1復元。関連common-open53/DLS policy28/articulation43/Sequence221/audition30が合格。291保存/作業ソースと4出力一致。

限定成立は確定入口・表示・候補の保存履歴経路。原版flags0x10e3に対し候補の既存DLS policyは0x63であり、完全native一致や音声fallbackは未確認。初期契約の「空一覧は割当てない」という仮定は原版観測で覆り、音声比較を残責務へ追加した。既定再表示Enter/collectionなし/非空一覧の未一致locale/ドラム/厳密外観も未完。単位completed=false、boundedFinished=trueは上記限定経路だけを指す。旧候補のPCMや193237全登録一巡を204251実行済み結果へ転用しない。

[単位](../../work/analysis/ui-dls-empty-accept/20261011T201500Z/unit-record-v1.json)、[原版固定](../../work/analysis/ui-dls-empty-accept/20261011T201500Z/original-proof-v1.json)、[native32検査](../../work/analysis/ui-dls-empty-accept/20261011T201500Z/native-proof-v1.json)、[閉鎖22検査](../../work/analysis/ui-dls-empty-accept/20261011T201500Z/final-audit-v1.json)、[ソース変更](../../work/analysis/ui-dls-empty-accept/20261011T201500Z/source-change-v2.json)。

次：SSP01 Count-Inの固定PCM15/16失敗へ戻り、観測済み操作遅延80.906秒を、Play/Stopの前後時刻記録と目標絶対時刻への短い分割待機で改善する。新候補204251・既存32小節/64秒fixtureコピーに同じ閾値を適用し、Count-In中Stop/元の開始予定を越えた無音/再開128クリック/64秒後C6/発音中Stopを録音監査する。旧PCM・原版未比較・DLS flags/再表示/音声差・Quantize親残件・6640破棄回答待ち・Style015244/Q2・将来48native/30driver凍結・全40/全8を保持。

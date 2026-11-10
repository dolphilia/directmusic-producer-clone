# 新規Tempoの全グループ作成 — 2026-11-10

194909234はGUI新規Tempoを全32groupで作成する明示APIを追加、四つのGUI経路へ接続し既存add_tracks/importedheader保持。5source変更287同一/292savedworking4outputs、関連5モード648checks（新21）合格。作者19800でNoTempo空欄/無効→AddTracks Cancel→Tempo追加120/1-32→CurrentTempo132.25を確認。二段階UndoRedo保存は期待全bytes、復元12236で132.25/再保存/二通常exit0。native差はTempo80bytes position4/groupsALL32のappendだけ、既存3track/17音/Band/opaque/GUID/control二17音保持。ProjectはFILETIMEとSegment1size+80以外一致。

新固定240s録音19/19。二回の主再生で17音/16間隔が60/132.25秒の許容±0.03に一致。C6/発音開始1秒内/25秒activeStop/停止後静音を確認。PCM負例7/clock4全拒否、元WAV保持。作者試聴0/Play0、復元private2/primary2/explicitStarts4/API結果0。旧6秒停止不合格保持。

CHM全group契約と既存正式原版のgroup1→ALL32保存・1-32表示を根拠に新規作成を実装。原版新規作成同入力動的比較/別process復元、GUI四経路の個別操作、importedheader正規化/global表示/Length/loop/fullgeometryは別未達。全40全8/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-tempo-global-creation/20261109T191400000Z/unit-record-v3.json`。

Segment Lengthがgroup1のTempoを使う依存を次に修正する。拍子のgroup1軸を保ち、既存CurrentTempoと同じ制御Tempoの読み取りmapを用いる。既存group別編集・全nativeheaderを保全し、未凍結tempo-propertiesで別group/後続Tempo/可変map/invalid/history契約、保存build/関連/本体Length/保存通常終了/別復元/必要新PCMを進める。一次設計はnext-contract-audit-v1.json。既知OS凍結の同条件再試行なし、全40全8/fullAcceptance=false、goal active。

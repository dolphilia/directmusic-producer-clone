# Segment Lengthと制御Tempo — 2026-11-10

201457752はLengthのTempo積分をgroup1編集Tempoから全体制御Tempoへ分離し、拍子軸group1/全importedheader/cache/selectionを保持。4source変更288同一/292savedworking4outputs、関連5モード671checks（新23）合格。作者18928で主132.25/後続180入力の20Bars36.295sプレビューCancel、24Bars43.554s確定/Undo20/Redo24保存を確認。復元20560で24Bars/43.554s/132.25・再保存/二通常exit0。native差は2356bytes中segh lengthDWORD61440→73728の2bytesだけ、全5tracks/Tempo masks2・1/17音/Band/opaque/GUID/control二17音保持、ProjectはFILETIME以外一致。

初回240s録音はStop25.782sが許容25.5sを超え18/19で不合格のまま保全。別process/newprefsへ通常再起動し、Play直後に相対Stop待機を即提出する手順へ変更、閾値不変の新固定240s録音19/19。二回の主再生で17音/16間隔が60/132.25秒の許容±0.03に一致。C6/発音開始1秒内/25秒activeStop/停止後静音合格。PCM負例7/clock4全拒否し元WAV保持。作者試聴0/Play0、復元private2/primary2/explicitStarts4/API結果0。旧6秒停止不合格保持。

最低dwPosition/同値native順は既存sourcepolicyで、原版同入力Length動的比較/別process復元/輸入並替tieの全同等性は未達。TimeSiggroup設定/trim/loop/globalimported表示正規化/fullgeometryも別未達。全40全8/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-length-primary-tempo/20261109T201400000Z/unit-record-v3.json`。

Sequenceの所属拍とloop包含、または残るglobal Tempo editor表示を一次資料・既存回帰・実機依存から監査し、独立して安全に進められる次単位を実装・保存build/関連/本体保存通常終了/別復元/必要新PCMまで継続する。既知OS凍結の同条件再試行なし。全40全8/fullAcceptance=false、goal active。

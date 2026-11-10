# Sequence論理時刻の準備時間

20261109T214458343ZはSequence負offsetに必要なnative準備窓を全Sequence tracksから判定し1000msを確保、共有時は増加のみ・単独時短縮可、即時開始300ms/serialized sourceは維持。3source変更289同一/292savedworking4outputs、新lookahead12契約、関連7/870checks合格。作者16800で所属1:4→2:4→2:1/各保存/二UndoRedo、別復元3032で2:1・物理1:4〜2:2/C6/96/Tempo120と再保存、二通常exit0。nativeは1976bytes中論理LONG/offsetSHORTの2bytesだけ、Loop/header/全他tracks/Band/opaque/GUID/control二17音とProjectのFILETIME以外を保持。

本候補の初回baselineもStop42.846sによる18/19不合格を保全、作者8000通常exit0後に新作者16800/newprefsへ変更して採り直した。新候補でbaseline/memberを各新固定240s・二25sStop採取し、各19/19・7PCM負例4clock全拒否。baselineは二発音[0,1.5]、memberは11発音[0,1.5,4,6.5,9,11.5,14,16.5,19,21.5,24]の事前±0.03s条件に一致。private2/primary2を両processで確認、実測prepareはbaseline300/member1000/private300、API結果0・cold/warm/onset1s内・停止後無音も維持。旧210902079のmember18/19遅延録音は不合格のまま保全し、閾値変更・旧PCM昇格はなし。

即時UndoのUIA値遅延を検出し、既存の後続保存キャプチャで値照合、全即時証拠と補正理由を保全。1000msは既存full prepare方針でありCHMの約1秒を超えるoffsetや任意Tempo/modifierの全域保証ではない。Loop tab/StartTime UI/clock-time loop/原版同入力動的・別復元/全geometry/全40全8は未達。旧6s停止・Length初回Stop失敗と50native30driver凍結も保持しfullAcceptance=false。

一次入口：`work/analysis/ui-sequence-logical-lookahead/20261109T214400000Z/unit-record-v3.json`。

SegmentのLoopタブを一次CHM画像・既存native segh保全・既存Length/Sequenceとの依存から監査し、music-time Repeat/Infinite/開始終了の次単位へ進む。独立実装・保存build/関連/本体保存通常終了/別復元/必要新PCMまで続行し、既知OS障害の同条件再試行はしない。全40全8/fullAcceptance=false、goal active。

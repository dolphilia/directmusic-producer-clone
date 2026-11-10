# Segment Loop playback cursor

005605175はmusic-time Loop中の再生位置を曲末clampから反復区間の折り返しへ修復。再生時snapshotのlength/loop/明示seekとelapsedを使用し、Repeat0 linear、有限intro→反復→outro、Infinite wrap、end0/巨大時刻/refusalを34model契約で確認。Band live判定も同じ位置を使う。5source変更287同一/292saved-working4outputs、関連7/988合格。実nativeではRepeat0/2の177/281samples、折り返し0/2と有限outro・自然終了、Infinite235samples/4wrap/elapsed3847と6発音を確認、GetStartTime基準は全て固定。

作者18116の本体で元8s曲長後もUIA4494→3432→6391 clocksと区間3072..6911内を移動。Play Cursor off二観測6742固定、on再開、通常停止3702。保存/通常exit0、別設定復元4812でInfiniteと2:1:1:0〜3:2:1:0を復元再保存、別起動も通常exit0。3SGP全byte、ProjectFILETIME以外を保持。新固定240sは19/19、7PCM/fourclock全拒否、二episode11発音[0,1.5,4,6.5,9,11.5,14,16.5,19,21.5,24]完全一致。private onset.630/.565、main.505/.455、Stop25.008/.007、停止後無音。WAV 762d0429aa9a4c3ddb9ff1f04bf9e7bf8ada55dcb60b3789883f486345521d2c。

動く間のUIAと画像は同時取得ではなく値差を保全。作者sample-threeでTree/toolbar一部欠落、別起動初期Tree白も観測、持続時間と原因は未確定。描画修復/原版同入力動的/clock-time cursor/任意seek全域/StartTime UI/InfoBoundary/全geometryは未受入。Clock/reference又は反復の不正bounds・seek>=loopEndは既存linearへfallbackしmusic-time合格範囲外。既知OS50native30driverと旧録音・Loop四候補失敗を保全。全40/全8は6working/2blocked/0passのままfullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-cursor/20261110T005500000Z/unit-record-v3.json`。

カーソル限定範囲の保存を終え、再生中Tree/toolbar/timelineの描画欠落をsource・観測時刻から監査し、根拠を得た変更だけ新候補として実装/関連/GUI保存通常終了/別復元/必要新PCMへ続行。StartTime UIとPlayFromStartのnative既定開始点も次依存。既知OS障害の同条件再試行は避け、goal active。

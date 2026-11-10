# Segment Loop music-time操作

20261109T235737154ZはSegment PropertiesにLoopタブを追加し、Repeat/Infiniteとmusic-time開始終了BarBeatGridTick/Resetを操作可能にした。CHM原版画像の配置・ラベルに合わせ、先行候補で観測した旧Length背景残像を子非表示→背景消去→新ページ表示で修復し、CtrlTabのMDI先取りは同一root制限、上下スピンはiDelta符号で修復。8source変更284同一/292savedworking4outputs、新model42/有限runtime20契約、関連7/932checks合格。作者18312でマウス/CtrlTab/開始2:1/終了3:1→3:2/Repeat0→1/Infinite/Reset/二UndoRedoと各保存、別復元14116でInfiniteと区間2:1:1:0〜3:2:1:0を復元し再保存、二通常exit0。

nativeは1976bytesのsegh Repeat/loopStart/loopEndの3DWORDだけ編集可能で、実際の最終値はInfinite/3072..6912の2bytes差分。既定end0とReset0/0を保持し、length/start/flags/tail/全tracks/負offset論理物理時刻/Band/opaque/GUID/control二17音、ProjectのFILETIME以外を保持。新baseline/member各固定240sは各19/19、7PCM負例4clockを全拒否。whole0/0は二episode各7発音[0,1.5,8,9.5,16,17.5,24]、customは各11発音[0,1.5,4,6.5,9,11.5,14,16.5,19,21.5,24]、事前±0.03sに一致。private2/primary2、prepare main1000/private300/API結果0、二25±.5sStop・停止後無音も確認。有限Repeat0/2は別native runtimeで各2/4発音と[0,576]/[0,576,1344,2112]clocks、自然終了を確認。

別復元のcold Play直後にTree/timeline描画欠落、次Stopで復帰を観測し全rawを保全。途中frame未観測のため持続時間は不明。loop cursorもlinear elapsedのlength-1 clampのままで未受入。Clock Time controlsはread-only/disabledで、clock/reference-length入力のmusic-time編集は原子拒否。Info/Boundaryタブ、clock-time loop、StartTime UI、loop中caret追随、原版同入力動的・別復元、全geometry/全40全8は未達。Finite fallbackは同一session内だけ保持し、Infiniteの別復元で有限数値の永続化を主張しない。先行四候補の表示・操作不合格・旧録音不合格・50native30driver凍結を保全しfullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T235737154Z/unit-record-v3.json`。

Loop設定の終了条件を保存した後、cold Play描画欠落とloop中再生位置表示、StartTimeの一次資料・native/runtime依存を監査し、独立する次単位へ進む。実装/保存build/関連/本体保存通常終了/別復元/必要新PCMまで続行し、既知OS障害の同条件再試行はしない。全40全8/fullAcceptance=false、goal active。

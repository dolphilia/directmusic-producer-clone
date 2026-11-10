# Segment buffered paint

020605949はSegment描画をcompatible memory DC/bitmap内で完成後、BeginPaintのclipを保った画面DCへBitBltする限定修正。WM_ERASEBKGNDの単独消去を抑え、RAIIで元bitmapを復元してGDI資源を解放し、割当失敗は従来direct描画へfallbackする。timeline1source変更291同一/292saved-working4outputs、音源/native/toolbar/Loop意味論は変更なし。関連7/988合格。

作者10292でProperties close、cold Playと14.505/28.689/45.972秒後の三frame、Play Cursor off二観測6167固定/on再開/Stopを確認。観測したtimeline内ではruler/grid/二C6notes/Band1/cursorが保持された。同一control構成のGDIは90→91（事前上限+8内）。作者通常exit0、別新設定8120でInfiniteと2:1:1:0〜3:2:1:0/Tempo120を復元再保存し通常exit0。3SGP全byteとProjectFILETIME以外、292source/4outputsを保持。新固定240sは19/19、7PCM/fourclock全拒否、二episode各11発音[0,1.5,4,6.5,9,11.5,14,16.5,19,21.5,24]が事前±.03s内一致。private onset.575/.560・main.475/.475、Stop25.014/.004、停止後無音。WAV f20840f17a7059b65ee279bfe7940bb16e462727b2569dd1c2f6a9ca2394c65b。

先の録音二件は0actionsとfirst Stop34.501秒/4actionsで不合格のまま保全し、両起動10348/9312も通常exit0/入力完全保持。第三では短いUIA確認へ変更し、各Play直後三応答のStop欠落とhelper errorも保全。別read-only観測で実Stop173/181を確認後に別入力で停止し、基準は変更していない。作者Properties close直後にはtimeline clientより上の親frame灰色域へ白矩形が残り、次stable frameで復帰。初期Tree白も保全。全時間描画/親frame/全geometry/原版同入力動的/StartTime/clock-time/InfoBoundary/全40全8は未受入。GDIは限定増加試験であり普遍的なleakなしとはしない。既知OS50native30driverと旧録音・四Loop候補失敗を保全。6working/2blocked/0passのままfullAcceptance=false。

一次入口：`work/analysis/ui-segment-buffered-paint/20261110T020500000Z/unit-record-v3.json`。

限定timeline描画単位の記録後、Properties非表示時の親frame白矩形とrefresh/layoutの再描画順を照合し、根拠を得た次候補の実装/関連/本体保存通常終了/別復元/必要新PCMへ続行。StartTime UIとPlayFromStart既定開始点のnative/runtime依存もqueueに保つ。既知OS障害の同条件再試行はしない。全40全8/fullAcceptance=false、goal active。

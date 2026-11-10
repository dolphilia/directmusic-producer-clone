# Toolbar steady refresh

012618577は100ms Transport更新で同値のnative button state/text/selection書込を抑制し、tempo/spin/primary sibling配置をgeometry変更時と明示layoutへ制限した。frame_toolbar.cppの1source変更291同一/292saved-working4outputs、音源/native/loop cursor/runtimeとテスト契約は変更なし。新関連7/988合格。

作者21056で未確定132.25を保持し、Escape後も保持する既存原版仕様を確認。計画の「Escapeで120復元」という誤前提はprotocol-correction-v1に全旧資料/rawとともに保全し、product/source/audio基準は変えず、120再入力Returnの同値確定で復元。AlternateTempo50→75→50表示、三定常Play画像（11.716/25.211/41.141s）でTree/toolbar/timeline、disabled120/Stop/cursor区間内、PlayCursoroff二観測5332固定/on再開、Stop後enabled120とPlay復帰を確認。UIA modal80 set_value拒否はinputなしで記録し再試行せず、実画面の数値選択と通常キーで完了。作者通常exit0/別新設定14660復元再保存/通常exit0。3SGP全byte・ProjectFILETIME以外を保持。新240sは19/19、7PCM/fourclock全拒否、二episode11発音列[0,1.5,4,6.5,9,11.5,14,16.5,19,21.5,24]が事前±.03s内一致。private onset.660/.565、main.485/.465、Stop25.005/.015、停止後無音。WAV 8f9cb6fb1638d8b9476adc9a8b9b8dcd88cb1eb190c461292ab0a904ea8e19e1。

初期Tree白は次idle画像で復帰。三定常Play画像は描画できたが、cursor-on-movingではノート/Band label欠落、close-properties直後にはtimelineほぼ白を観測、全raw保持。開始/終了/原因の全時間計測はなく、全描画修復を主張しない。UIAと画像の動く値の取得時刻差も保持。原版同入力動的/全geometry/StartTime/clock-time/InfoBoundary/全40全8は未達。既知OS50native30driverと旧録音・四Loop候補失敗を保全。6working/2blocked/0passのままfullAcceptance=false。

一次入口：`work/analysis/ui-toolbar-steady-refresh/20261110T012600000Z/unit-record-v3.json`。

定常toolbar更新単位の記録後、Segment paintが白地消去→部分描画を画面DCへ直接行う経路と欠落画像を一次API資料で照合し、描画をmemory DCで完了後に反映する限定修正へ進む。必要な関連/実画面複数frame/保存通常終了/別復元/新PCMまで続ける。StartTimeのnative/runtime既定開始点とUI依存も次queueに保つ。既知OSの同条件再試行はしない。全40全8/fullAcceptance=false、goal active。

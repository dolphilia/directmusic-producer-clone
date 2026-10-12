# Lyric Properties・modeless編集の本体接続

2026-10-09。0735限定採用を保持。UI09 / LyricStripMgr.dll / Timeline.dll / 共通Propertiesを対象とする。原版Help由来のページ・F11/context入口・Start TimeとBelongs To・Timingを既存Lyric編集へ接続する。全40・全8と原版比較は未完、fullAcceptance=false。

0503候補は232保存/作業ソース・4生成物、configure/build/install各0。関連39件は35合格・4個別凍結、製品driver4件は実行合格。本体でmodeless Lyric編集と共通Propertiesを開いたが、フォーカスした歌詞欄を残して選択を切り替えると文字列だけが前の歌詞を保持した。Propertiesを閉じると古い文字列が新しい選択へ書き込まれることも確認した。検証用入力でUndoしてcleanへ戻し、PID1064は通常exit0。保存済みProject/AudioPath/Segmentの3ファイルは終了後hash不変。

この不具合に対し、follow_lyricで文書/track/event/groupsが変わった場合はフォーカス欄も更新する。Timing groupをtab内に収め、ラベルと数値欄の重なりも修正した。0525候補20261009T052548846Zは232保存/作業ソースと4生成物を照合し、configure/build/install各0。関連39件は同候補で35合格・4個別凍結、製品driver4件も実行合格。0503の証拠を転用していない。

同じ2歌詞fixtureの実GUIで、フォーカスした歌詞欄を残した両方向の選択切替、全6欄とTimingの追従、dirty/history不変を確認した。ボタンから共通Propertiesを開き、文字列「Lyric UI Ω」、Start Time 2:2:24、Belongs To 4:1、Quick responseへ編集。5回のUndoでcleanな元値へ戻り、5回のRedoで全編集を復元し、不正な拍9は値と履歴を変えず拒否した。TreeのF11でSegment Propertiesを開けることから本体が操作可能なmodeless状態を確認した。

SongとProjectをnative保存し、PID6756を通常exit0。保存後のコピーを別PID2016で開き、同じ6欄・Timing・Saved表示を復元した。Message Windowに両歌詞を確認し、同候補の64秒WASAPI録音で8音/Tempo120・137.75・191.5、2回の発音中Stop/無音/replayが固定条件を満たした。停止操作範囲は4.705〜4.950秒と4.706〜4.961秒、停止後RMSは両方0。無音・過大ピーク・停止後信号・入力hash不一致・早すぎるStopの5対照をそれぞれ拒否した。PID2016も通常exit0、保存/保全/再読込の3入力は終了後hash一致。

独立native監査6件は成立。旧v1判定器は原版文字列の終端後6バイトを期待値から欠落させて失敗した。既存[Lyric契約](lyric-contract.md)が明記する末尾保持へv2を訂正し、入力由来の6バイトとそれ以外の全RIFFを厳密照合した。旧結果・訂正理由・両判定器を保全した。

GUI監査は7件成立・1件未確認。Lyric右クリックメニューの表示は確認できたが、本体Windowへ入力を送る際にownedメニューが閉じ、選択先のPropertiesを開く操作は届かなかった。LyricのF11/右クリック入口の完遂を合格にしない。通知画面を含む最初の録音でもmain再生ボタンの参照が使えずPlay未実行となり、idle録音を保全して回復後に新しい録音を実行した。GUI観測エラーを製品障害やOS拒否と断定していない。原版動的比較は既存app approval障害のまま、単位completed=false/fullAcceptance=false。

次は独立したUI09 Markerの共通/modeless Propertiesへ進む。Lyric入口未確認・原版比較・Timeline/OLE等の残差を保持する。

一次入口：

- [0503 GUI失敗・Undo・正常終了](../../work/analysis/ui-lyric-properties/20261009T045848834Z/unit-0503-gui-failed-v1.json) SHA-256 067f27defb71f0d5c4f7e8dd7cb421d956833f4727817645c41937589a305332。
- [0503関連回帰](../../work/acceptance/regression/20261009T050901944Z/run.json) SHA-256 95ddc12dc31d68898133f8827777a57c25dc4f6e3b14981194feee167b4b0c76。
- [0503保存入力不変](../../work/analysis/ui-lyric-properties/20261009T045848834Z/post-exit-input-integrity-v1.json)。
- [0525修正単位](../../work/analysis/ui-lyric-properties/20261009T052533197Z/unit-record.json)、[固定GUI手順](../../work/analysis/ui-lyric-properties/20261009T052533197Z/gui-protocol-v1.json)。
- [0525限定成果の固定記録](../../work/analysis/ui-lyric-properties/20261009T052533197Z/unit-0525-limited-v1.json) SHA-256 c038433a16607af9167484f786dc97e1604dc473d884079e693ef5cb5d597cb0。
- [GUI7成立/1未確認](../../work/analysis/ui-lyric-properties/20261009T052533197Z/gui-proof-v2.json)、[native6](../../work/analysis/ui-lyric-properties/20261009T052533197Z/native-proof-v2.json)、[固定音声](../../work/analysis/ui-lyric-properties/20261009T052533197Z/audio-proof-v1.json)、[5不正対照](../../work/analysis/ui-lyric-properties/20261009T052533197Z/audio-controls-proof-v1.json)。
- [native判定器訂正](../../work/analysis/ui-lyric-properties/20261009T052533197Z/native-auditor-correction-v2.json)、[GUIの画面種別・未確認判定](../../work/analysis/ui-lyric-properties/20261009T052533197Z/gui-auditor-qualification-v2.json)、[終了後入力](../../work/analysis/ui-lyric-properties/20261009T052533197Z/post-exit-input-integrity-v1.json)。
- [改訂前報告](../../work/analysis/ui-lyric-properties/20261009T052533197Z/report-before-repair.md) SHA-256 206945a83c70af80e0ebe42f34c40fbbb1760c7e01b373ca0bb53760c6bae5f8。

未達：原版のtrack一覧/共通Timeline挿入・drag・clipboard・全操作、外観/font/icon/配置の動的比較、同最終構成の全登録一巡/Q2/全40/全8。専用試験件数やページ表示だけを完成にしない。

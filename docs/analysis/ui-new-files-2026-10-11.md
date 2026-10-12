# Create New File(s)とSegment作成経路の限定修正

2026-10-11。画像5〜7、UI01.priority-05 / UI03.priority-06 / UI03.priority-07。候補20261011T110746657Z、全体受入false。

110746：画像5〜7のNew Filesダブルクリック確定・任意名focus・空名警告・Length取消し後の作成継続を原版観測に合わせて修正。288ソース/4出力一致、関連3/370。GUI15条件直接確認+1条件ソース推論、20/24小節のTimeSig-only保存・3参照Project・既存Seed不変・作者14288/別17724二通常exit0。厳密外観/全操作未達。

## 修正と根拠

正式原版5.3.0.900の手動起動済みPID25548と検証用Scenarioで観測した。Default Namesを外すとNameへfocusが移る。空名の警告は「You must type a name to replace the default names.」、タイトルDirectMusic Producer、警告アイコン。Segmentのダブルクリックは確定する。原版はLengthの前に既定20小節のSegmentを作り、LengthのCancelでもAdd New Tracksへ進む。そのCancel後はTimeSig4/4だけが残る。計画の「取消しでは文書を作らない」はNew Files自体のCancelに限定して訂正した。

new_files_dialog.cppでfocus/警告/ダブルクリックを修正。main.cppではFrameworkで既定Segmentを作成してからLengthを尋ね、指定時は同じdocument参照に長さを適用し、初期Undo履歴を作らずAdd New Tracksへ進める。原版の自動ディスク保存までは再現していない。

## 検証

保存ソース288と作業木、生成物4のhash一致。Producer.exe SHA256 98a9618a9b0ac9cab8b9e6bdc2365d557e6a1d59412269c9d0c19c9b84afb230。関連common-open53、timeline-range117、segment-creation200、計370検査で合格。今回の候補で全native100/driver172の原始一巡は未実行。100711の一巡・Band PCM19/19を新候補へ転用しない。音声は作成UIだけの変更のため非適用。

固定したGUI16条件のうち15条件を直接確認し、作成前後のGUID同一性1条件はソース推論に留める。未保存のLength前GUID bytesは取得しておらず16/16直接観測合格とはしない。既定名Segment2は61440clocks=20小節、任意名Length24は73728clocks=24小節。両方TimeSig4/4だけでGUIDはSeedを含め3つとも異なる。Projectには3参照を保存。SeedはSHA256不変。別PID17724でも20Bars/24BarsとTimeSig表示を確認し、4入力は再読込前後で不変。作者14288と復元17724はAlt+F4でexit0。

作者の点観測128modulesはx86 122/WOW64支援6、保護40原版hash一致0。wow64cpu.dllが4GiB未満に配置されたため住所だけで分類したv1は未分類1でfalse。System32の正確なpath/x64PE/既知Windows支援hashを照合したv2へ訂正し、v1も保持。点観測は全体の原版非依存/Q2成立ではない。

## 残件と保存

メニューを閉じた直後のCtrl+Sは無反応で、Standard Save Fileから保存した。このfocus経路は別queue。New Project保存先、空選択OKの原版観測、Add New Tracksの初期Band選択、厳密な字体/寸法、全入力入口・全Propertiesは未完。原版側に検証で追加したSegment2は保持し、既存3ディスク入力と正式EXEは不変。原版は閉じず、未保存のProject参照追加を保持する。

開始記録の誤ったUI02.priority-05は上書きせずscope-correction-v1で正しい3caseへ訂正した。旧100711 baseline/失敗/操作列/録画状態、変更前2ソースを保存。

一次証拠：[単位記録](../../work/analysis/ui-new-files/20261011T105253852Z/unit-record-v1.json)、[GUI保存復元](../../work/analysis/ui-new-files/20261011T105253852Z/gui-save-reload-proof-v1.json)、[独立native監査](../../work/analysis/ui-new-files/20261011T105253852Z/native-save-audit-v2.json)、[関連回帰](../../work/acceptance/regression/20261011T111507991Z/run.json)。

次：画像7のAdd New Tracksについて、正式原版で観測済みの初期Band Track選択を製品へ反映し、空選択OK/Cancel/Band追加・TimeSig保持/UndoRedo/保存/別process復元を固定する。原版の空選択解除操作は実観測してから契約化する。Ctrl+Sのメニュー終了後focus経路を別queueへ残す。旧Band停止失敗・Count-In15/16・Echo/SSP04/SSP08比較・Style015244/Q2/OS凍結を保持。

限定採用0735、独立Style015244、全40責務/8受入、6作業中/2障害/0合格、fullAcceptance=falseを保持。

# Add New Tracks 初期フォーカス比較（2026-10-12）

画像7・8に対応。正式原版と151135候補の既存SnapSeedを同じ入力で比較し、Band初期選択、DownでChord、HomeでBand、Cancelで無変更が一致した。空選択OKの原版比較は未達のため、この単位全体は未完。

|確認|結果|
|---|---|
|正式原版|PID25548、EXE SHA256 fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011。保存6ファイル・EXE不変、原版を保存・終了していない|
|初期選択とfocus|両方Band選択。追加クリックなしでDown→Chord、Home→Band。一覧のkeyboard focusを操作で確認|
|原版の空選択|Ctrl+Space/Ctrl+backslash/空白クリックではBandが残った。空選択OKは実行していない。原因は未判定。未実行のCtrl-clickは証拠にしない|
|候補単体の空選択|未対応Pattern行のクリックで選択解除→OK。原版と異なる手順であり原版一致とは数えない|
|候補無変更|4トラック・8ノート、Undo/Redo無効。Project/Segmentの全バイトhash不変。編集がないのでSave操作なし|
|別process|作者11124と別10296がそれぞれ通常exit0。再読込でも4トラック・8ノート|
|構成|151135、289保存/作業ソース・4出力hash一致。実装変更なし。関連4/391は同候補のNP29既存結果として参照し、再実行していない|

[開始と終了条件](../../work/analysis/ui-add-tracks-comparison/20261011T154449952Z/start-v1.json)、[未完単位](../../work/analysis/ui-add-tracks-comparison/20261011T154449952Z/unit-record-v1.json)、[原版観測](../../work/analysis/ui-add-tracks-comparison/20261011T154449952Z/original-observation-v1.json)、[候補/再読込証明](../../work/analysis/ui-add-tracks-comparison/20261011T154449952Z/candidate-proof-v1.json)、[独立native解析](../../work/analysis/ui-add-tracks-comparison/20261011T154449952Z/native-independent-v1.json)、[構成hash](../../work/analysis/ui-add-tracks-comparison/20261011T154449952Z/closing-identity-v1.json)、[原版保護](../../work/analysis/ui-add-tracks-comparison/20261011T154449952Z/preservation-v1.json)。

![候補の初期Band選択](../../work/analysis/ui-add-tracks-comparison/20261011T154449952Z/candidate-03-dialog-1.png)

原版で空選択を確立できる別の根拠・手順が得られたときに未確認条件を再開する。同条件の選択解除を繰り返さず、現在候補151135の登録native100/driver172一巡へ進む。46native/28driver OS凍結と既知匿名Band失敗は保持し、候補が変わったことだけを理由に再実行しない。全40対象・全8条件、独立Style015244/Q2、SSP04親ドラッグ、全トラック種別・厳密外観は未完。全体fullAcceptance=false、限定採用0735は変更しない。

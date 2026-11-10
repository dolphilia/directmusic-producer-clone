# Sequenceノート編集と数値ベロシティの統合

2026-10-11（日本時間）。ユーザー指定画像17〜20のSSP04。単位は `work/analysis/ui-sequence-edit/20261010T165953820Z`、開始は `start-v1.json`。SSP03候補162022の8ノート保存復元と発音1.15秒失敗は前単位として保全し、初回発音だけの反復を区切って未対応編集へ進んだ。

## 変更範囲と契約

ノート左端は開始時刻を変更して終了時刻を保ち、右端は開始時刻を保って長さを変更する。中央/下部は時刻と音高を移動し、上辺はベロシティを変更する。Grid/Beat/Barは左端と移動へ適用し、Noneは自由時刻とする。右端はユーザー観測に従って全Snap設定で自由伸縮。これは原版CHMの右端grid記述との差を明示して採用したもの。

ドラッグ中は別のNote値を描画・試聴し、元文書を変更しない。確定時に一回のedit_noteを行い、取消し・捕捉喪失・外部文書変更時は適用しない。保持中の端の左右キーは1tick、velocity上下キーは1、通常velocityドラッグはCHMに従い5単位。キー操作・Capture・Cancelの本体実行はまだ未確認。

画像20に対応してEdit VelocityのSelected Notes/Part/All Parts in Sequence、Offset Start/End、%/linear、Limit Range Min/Max、OK/Cancelを接続した。一括変更は単一のSegment履歴。Min=Maxならその数値へ設定する。%を元velocityへの相対増減、Start/Endを対象最初/最後の時刻の補間としているが、この細部は実装上の暫定解釈であり、原版比較で確定した仕様とは呼ばない。

## 現行状態

170536 (`20261010T170536990Z`) はconfigure/build/installすべてexit0。266保存ソースと作業ソース、4出力のhashが一致した。`source-changes-v3.json` が実行済みソースの記録。関連13モード822検査が合格し、うちSequence90検査・試聴所有権16検査を含む。この候補自身の全98native/170driver一巡は未実行で、162022の一巡を転用しない。

本体では右端の自由伸縮192→1088、Undo/Redo、Grid左端1920/duration704、中央移動2496/pitch62、上辺velocity96→56、None左端2624/duration576、中央移動2784/pitch61を確認した。最初のNone中央操作はhitせず未変更であり、成功へ数えない。Selected NotesのLimit Min80、Undo56/Redo80、PartのLimit Max40で全8ノート変更、一回のUndoで他7ノート96/編集ノート80に戻ること、数値ダイアログCancelを確認した。

編集済みSegmentは2784/pitch61/duration576/velocity80を含む8ノートとOrgan 1を保存した。作者20768は通常exit0。別process15016で同じ保存ファイルを復元し、通常exit0。独立RIFF監査 `gui-native-audit-v1.json` は合格。編集前後、Part40スナップショットともにSequenceの1leafのみ変わり、他49leaf payloadは全bytes不変。Part40は8レコードすべてvelocity40で、時刻・長さ・音高・PChannelなど他のbytesは不変だった。

固定300秒WASAPI録音は `work/acceptance/product-project-gui/20261010T173244655Z/audio-20261010T173541920Z/velocity-pcm-proof-v1.json`。録音前に保存したprotocolの1秒基準を変更せず、初回velocity80の発音は1.215秒で不合格。後続96/上辺drag/40は0.415/0.520/0.480秒、全phaseのpitch61を検出した。96/80 RMS比1.436、40/80比0.248で事前の音量差基準は成立。drag中の発音は検出したが、その初期窓はmousedown時の80も含むため、各中間velocityごとの音量一致までは証明しない。録音後Undoで80・cleanへ戻し、入力ファイルhash不変を確認した。音量差の限定成立を初回発音/音声全体合格へ置き換えない。

UIA要素クリック4回の失敗、`type_text`で数値が空欄になった操作を保存している。物理数字キーで80/40を入力できた。空欄の原因を本体の数値編集不具合と断定していない。preferences.binは存在せず、64voices/22050Hz/55msはOptionsで実見した既定値。保存設定の引継ぎ成功とは呼ばない。

## 検証と残件

nativeの境界/Undo/複数part/不正入力/Q0保護、試聴の独立所有権、関連回帰と上記の限定本体経路・native保存/別復元は実行済み。Beat/Barline各組合せ、本体の保持中キー/Escape/Capture取消し、All Parts/percent/ramp、不正数値、複数選択、各中間velocity試聴、原版同入力/厳密外観比較は未確認。初回発音1.215秒は未解決。無効Editに付属するspinnerが有効なままというUI残差も保持する。

原版CHMの複数ノート選択ドラッグ、Snap nearestと既存floorの差、数値編集のpercent/ramp細部、原版の端別cursor形状・厳密配置・全操作比較を残す。SSP03初回発音、旧46native/28driver OS凍結、独立Style015244、SSP01/02/05〜08の責務を維持。限定採用0735、全40・全8（6作業中/2障害/0合格）とfullAcceptance=falseは変えない。

次の独立実装はSSP05（画像25〜28）のBand選択・pan/volumeビュー。SSP04を完了扱いにせず、初回発音と比較/操作残件を版付き単位に保持して進む。

# 共通 Timeline の Time Signature 選択と Properties

候補2013（20261009T201346735Z）は237保存/作業ソース・4出力一致、configure/build/install各0、関連9 mode、GUI15/native6、二通常exit0・別process復元が限定成立した。原版比較は障害あり、completed=false/fullAcceptance=false。限定採用0735と原始一巡2137は保持する。

## 実装と契約

描画のみだったTime Signature行を選択・強調・位置表示・F11/右クリックPropertiesへ接続した。共有ページのTime Signature button/MeasureとBeats/Beat/Divide Beat dialogを追加し、既存の文書更新・Tempo再配置・Style/group guardへ接続した。

原版保存Helpと静的dialog画像が基準。Styleが対応groupの拍子を決める場合は編集禁止、一小節一拍子。先頭measure0固定と暗黙4/4 readonlyは自作モデルの制約で、原版一致とは扱わない。DivideBeatの音符表示/正確なlist、first-marker/implicit方針、track Flags/group、geometry/font/menuの差は未解決。

## 検証

明示TimeSig4/4 grids4 at0/3072を持つ固定Songで、二つ目のMeasure2→3（clock3072→6144）とgrid4→8を操作した。実画面でroot F11・右クリックProperties、Beats spin4→5とCancel、Measure/GridのUndoRedo、先頭の実数字キー2の拒否、Tempo/Lyric/Marker/TimeSigへの共有Properties追従を確認した。直接childへのF11送達やProperties内CtrlZの文書履歴は合格扱いにしていない。

Song全文は事前の期待bytesに一致（SHA256 d57e988e4a8a578ddfb7f13ca264492d3e9e8477219433d71232b8cca5a10785）。Effect不変、ProjectはSongのfile journalだけ変更。author PID18568とreload PID19096は同じexe hashで通常exit0。別processでMeasure3/4/4/grid8、clean文書とUndoRedo初期状態を確認した。4K/200%は変更していない。

関連9には新規meter-properties65check、既存batch Tempo再配置/group guardsを含む。native登録95/driver164で、旧原始一巡は歴史的候補の結果のまま。固定4/4の位置/grid変更は演奏clock/BPM/notesを変えないため本限定操作のaudioは非適用。非4/4の本体再生、旧Wave onset失敗、原版比較/Q2/全40/全8は未達。

## 失敗・訂正の保持

2001はinvalid indexの既存原子的例外契約を試験が誤認し16check後停止。2007はStyle fixtureがLIST:strfだけで正しいRIFF:DMTK/LIST:sttrでなく20check後停止。試験入力/契約を訂正し、readonly/bytes/history assertionを保持した。Beats spinの表示同期UDS_SETBUDDYINTも2013で修正・実画面確認した。fixtureのimplicit4/4誤認とNode module import EPERMも保存済み。

Computer Use type_textでは数値欄の数字が入らず、通常のpress_key数字で実証した。UIA index23はcached stateで使用不可となり、再観測後のEditメニューでUndoRedoを実証。GUI判定器v1は即時UIAのTimeline表示遅れで失敗し、後続の独立観測を使うv2で同じ存在条件が成立。正常close後のwindow unusableはlaunch記録exit0と区別した。いずれも原版/OS保存拒否や製品合格の代用にはしていない。

一次記録は[単位記録](../../work/analysis/ui-timeline-meter/20261009T195746728Z/unit-record-v1.json)、[GUI15](../../work/analysis/ui-timeline-meter/20261009T195746728Z/gui-proof-v2.json)、[native6](../../work/analysis/ui-timeline-meter/20261009T195746728Z/native-proof-v1.json)、[実画面レビュー](../../work/analysis/ui-timeline-meter/20261009T195746728Z/visual-review-v1.json)。全未解決条件と失敗候補・判定器は単位ディレクトリに保持する。

# Band / Tempo 追加と履歴 — 2026-11-09

122735同一保存候補でBand/Tempo追加の本体履歴を検証。Band追加322bytes、Undo234bytesは元入力と完全一致、Redo同一。Tempoはcursor1920でも時刻0に120で追加し402bytes、UndoはBandのみ322bytes・disabled空欄、Redoは同一402bytes・有効120。Cancel/最後Close gray/再Open noPromptを挟んでもUndoRedoを保持。作者16276/復元6752は二通常exit0、別processでは3tracks120・空UndoRedo、再保存全Segment bytes同一。他2文書は各17音符/Tempo/GUID全bytes不変。3Projectは宣言済Segment1 size234→402とFILETIME以外不変、292ソース/4出力一致。新build/ソース/PCMなし。関連は新timeline-range117、既存同候補3/168参照で再実行ではない。

追加BandはDMBTのbdth=1と空lbdlだけ。追加Tempoは時刻0/120、3trackのposition0/1/2/group1、20小節/4拍子。宣言した追加Trackを外してRIFF長を再計算すると元234bytesに完全一致し、名前/GUID/opaque情報を保持。Cancelと窓開閉後の次UndoがTempoを除くこと、Redoが同じnativeへ戻ることを確認した。登録前120・Bandのみの空欄・Tempo追加後120のUI状態も一連で確認した。

原版比較はユーザー画像7/8/12と既存の初期Band選択/Cancel観測に限定。新しい追加/history/120の原版動的比較・全外観を合格とはしない。空選択OKの既知不成立操作やdisabled Patternによる固有の解除は再試行していない。segment-creation Mixer.sgpとcomponent-name Names.proのOS拒否は保全し、専用GUIの成功を代替CLI受入にしない。

一次入口：`work/analysis/ui-segment-add-band-tempo/20261109T125633600Z/unit-record-v2.json`、`gui-proof-v1.json`、`native-proof-v1.json`。

SSP02の未実装Cycle Bandsを継続する。正式CHM bandtrack.htmは同じbeatに重なったBand名のitalic表示と右メニューCycle Bandsを規定するが、候補は常に無効。実装前に物理/論理時刻・同一beat判定・描画/hit/選択・Propertiesの重複許可を照合し、表示/選択だけの操作として文書bytes/dirty/historyを保護する。専用重複Band入力・保存build・未凍結関連・本体cycle/Properties/子editor所有先・保存通常終了/別復元へ進む。原版同入力動的/全外観比較・他残責務は未達のまま。component-name/segment-creationの既知OS拒否は代替合格にせず50native/30driver凍結、全40/全8/fullAcceptance=false。 goal active。

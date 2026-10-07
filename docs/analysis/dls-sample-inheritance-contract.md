# DLS Wave 既定設定と Region 上書きの継承契約

対象は `DLSDesigner.ocx` と `Conductor.dll` の既存 DLS サンプル設定である。Wave Track のループ有効化・終端解釈は別の未確認責務として維持する。

## 根拠と限界

保全した原版 help の `wavetab.htm` は、Wave の既定ループを Region と Track のプロパティで上書きできること、Root Note を DLS Region に設定することを説明する。`settinglooppoints.htm` のループ点はサンプル数単位である。Windows SDK 10.0.26100.0 の `shared/dls1.h` は `WSMPL::usUnityNote` を MIDI Unity Playback Note、`cSampleLoops == 0` を one shot と定義する。MIDI ノートの値域は 0–127 である（[Microsoft の DMUS_NOTE_PMSG 文書](https://learn.microsoft.com/en-us/previous-versions/ms808226(v=msdn.10))）。この値域を既定サンプルの採用・再生準備に適用するのは上記の型と意味からの判断であり、不正値を入力した原版 GUI の動的挙動を観測したという主張ではない。

原版の現行 New ダイアログは Project のみ、Add-Ins は空で対象編集画面を利用できない。原版動的比較は障害ありである。RIFF 監査、ソース版 UI、PCM の差分検証はこの比較を代替しない。

## 編集と再生の境界

- Region に `wsmp` があれば全サンプル設定の明示的上書きである。ループ数 0 でも Wave のループを採用せず、one shot として扱う。
- Region に `wsmp` がなければ `wlnk` と pool cue が指す Wave の設定を継承する。「Wave の設定を使用」は Region の `wsmp` 全体を取り除く一つの Undo/Redo 操作である。Root Note・Fine Tune・音量・ループの上書きをまとめて外すことを UI で示す。
- 継承状態からループを編集する場合、Wave のサンプルヘッダを複製して明示設定を作り、ループ部分だけを変更する。ヘッダ拡張、ループレコード拡張、末尾、独立した `smpl` は既存契約どおり保持する。
- 0 と 127 の Unity Note を許可し、128–65535 を採用・再生前に拒否する。ループがなくても検証する。破損ヘッダ、曖昧 chunk、cue、ループ境界の既存検証を維持する。
- 拒否は文書公開前に行い、保存 bytes、dirty checkpoint、Undo と保留 Redo、再生準備の呼出元スナップショットを変更しない。不正値を含む原版入力の lossless load/save 自体は保持し、黙って丸めない。
- 編集済み owned DLS は次の Play で採用する。保存前の編集・Undo がディスクの古い DLS に置換されないことを本体の録音差分で検証する。
- 再生用の私有 DLS コピーでは、Region に `wsmp` がない場合だけ、cue が指す Wave の `wsmp` 全体を複製する。編集文書・ディスク・履歴には上書きを追加せず、明示的なゼロループ Region は変更しない。これは候補 `234743632Z` の本体 PCM で、継承と Undo 復元が約75msで途切れ、明示 one shot が約1秒鳴った実測を受けた再生境界の修正である。OS 再生器に渡す有効値を明示する判断であり、原版 Producer の動的比較済みという意味ではない。

## 今回の終了条件

専用 `--dls-sample-policy` で one shot/継承、許可境界、不正 Wave/Region、履歴不変、再生準備の拒否、native Project の保存復元を検証する。既存 UI で継承、Undo/Redo、保存、正常終了、別 PID 再読込を実施する。同じ別 PID で継承ループ、未保存の明示 one shot、Undo した継承を再生し、自動録音で一秒 PCM を越える持続と one shot の減衰、Stop 後の無音を確認する。

Forward/Loop And Release の詳細、ADSR と note-off、Wave Track のループ上書き形式、原版動的比較、原版を配置しない独立環境、全対象・8 項目の最終受入は別に残る。

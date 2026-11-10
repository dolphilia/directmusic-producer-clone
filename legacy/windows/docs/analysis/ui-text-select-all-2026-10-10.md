# Main入力欄のCtrl+A修正

2026-10-10（日本時間）。候補1643 `20261009T164331136Z`。MainのWindows Edit欄でCtrl+Aを押しても全文選択されず、既存BPM文字列へ入力が追加される不整合を修正した。対象欄へ`EM_SETSEL(0,-1)`を送り、入力bufferのUndoと文書履歴を維持する。

[単位記録](../../work/analysis/ui-text-select-all/20261009T164257905Z/unit-record-v1.json)から開始契約、保存ソース、生成物、入力、操作、画面、判定器へ辿れる。236ソースと4出力が保存/作業木に一致。関連8試験、[GUI結合監査13](../../work/analysis/ui-text-select-all/20261009T164257905Z/gui-proof-v1.json)、[独立native監査5](../../work/analysis/ui-text-select-all/20261009T164257905Z/native-proof-v2.json)が合格。これは全体受入ではない。

実本体でBPM9→入力123.25→局所Undo9、文書Change10→Ctrl+S、入力44.5→局所Undo10を確認した。局所Undoは文書のUndoを消費しない。listのCtrl+Aは2件のテンポを選択し、文書Undo9/Redo10も成立。Style117→入力42.5→局所Undo117、Band音量96→入力123→局所Undo96は文書bytesを変えず、Ctrl+S後も不変だった。

保存ダイアログのファイル名はScratch→Ctrl+A→Other→Ctrl+Z→Scratchを画素で確認し、Escapeで取消した。モーダルの入力欄番号を使用できないエラー2件と、SaveAsメニュー番号clickの無応答を保持し、最新画面で位置とフォーカスを確認して操作した。UIAのSearchEditBoxという古いフォーカス表示をファイル名の判定に使っていない。

nativeは事前に独立生成したSourceSequenceのテンポ10 bytesと完全一致。明示Save Projectでは保存したSourceSequence/Style/Bandの3参照`filh`の時刻・サイズだけが更新され、他のleaf bytesは保持された。開始時の1参照という記述と全3文書Ctrl+Sという手順の矛盾は[GUI実行前の訂正](../../work/analysis/ui-text-select-all/20261009T164257905Z/contract-clarification-v1.json)で固定し、旧契約・旧判定器を保持した。

author22944/reload21384/empty24584はすべて通常exit0。別processで10/7.5 BPM、Style117、Band96と空の履歴を復元。空Projectの入力120→11→局所Undo120、Ctrl+Sでdialogなし・ファイル不変も確認した。正常close後のwindow失効はexit0と結合した。

OS表示200%を維持し、2行の選択色、Save SourcePiano.bnpとCtrl+S、モーダル文字置換の画面を保存した。[画面証拠](../../work/analysis/ui-text-select-all/20261009T164257905Z/visual-proof-v1.json)は自作候補の表示確認に限定する。原版の配置・字体・アイコン・操作比較は未確認で、原版approval障害を維持する。

音声はキー選択経路だけの変更で非適用。fixtureのテンポ変更はnativebytesまでの判定であり、音声合格やcoldWave初回onset1.021秒の失敗解消を主張しない。0735限定採用、2137原始一巡、native94/driver164、全8の6作業中/2障害/0合格を保持。`completed=false`、`fullAcceptance=false`。

次はWaveのFind Best Loop End From Selection。Helpの操作入口・選択範囲・既存loop始点保持を契約として固定し、未知の原版探索アルゴリズムを推論と区別して、PCM/native/history/本体保存復元と必要音声へ進む。

# 空のDLS選択・DEFAULTGMと再生 — 2026-10-12

20261011T224147887Z DLS空コレクション：未編集Save対照から選択操作のflagsを分離。原版の0x10e3/DEFAULTGM無効時0x83に合わせ、既存DEFAULTGMとzero transposeを保持。作者文書の参照を保ったまま、空DLS＋DEFAULTGM有効だけ再生用GMへ投影。関連5/387、native32、原版PCM23/候補PCM24、閉鎖21、291ソース/4出力と保護入力不変。候補の保存/UndoRedo/別process復元/通常終了を確認。既定GM無効時の空DLS再生・一覧全体の解決・既定再open・厳密外観/音色は未完。 全体fullAcceptance=false。

原版では専用コピーを未編集Saveし、共通serializerの変更を先に固定した。その対照と明示的な空コレクション選択を比較するとPCh1は0x1163→0x10e3、DEFAULTGM無効のPCh2は0x3→0x83となった。選択によるTRANSPOSEの有効化とzero初期化、既存USE_DEFAULT_GM_SETの維持を実装し、非空DLSで0x1700を解除する既存assertは保持した。全ノート・対象外のcandidate文書bytesは保存前後不変。

最初の候補215501は保存結果が合ったが、ノート試聴で空DLS読込0x88781184が発生した。220914は私有runtimeコピーから空参照を除去してエラーを解消したが、固定録音は無音で失敗。20261011T224147887Zでは同じ私有投影にGMコレクションの分類フラグも加えた。作者のGUID/DMRF/0x10e3は変更しない。dependency context・identity・sample・path/GUID衝突検査を投影前に実行し、非空DLSとDEFAULTGM無効は従来どおり維持する。以前の候補・録音・失敗・操作ログを保持した。

音声比較には同一のPRO/SGP/DLSコピーと、120BPM4/4、MIDI60/velocity96/Organ1、pan63/volume100、空DLSとDEFAULTGM有効、長い1ノート・CCなしの専用入力を使用。編集用3ノートfixture全体の音色完全一致を示すものではない。固定のonset1秒、音高・発音中RMS、停止後2秒以降の無音、操作間隔3〜20秒、packet/clip/入力hashを双方で検査した。原版はAlt+<、候補はStopボタンを使用し、同じ停止機能の比較とマウス手順の完全一致を分けた。

|測定|正式原版|候補20261011T224147887Z|
|---|---:|---:|
|初回発音遅延|0.240秒|0.480秒|
|再開発音遅延|0.240秒|0.485秒|
|停止後最大RMS|0|0|

録音先endpoint IDは両方のendpoint.txtで一致。前回Count-In比較についても既存endpoint.txtが同じIDを記録しており、LA41の「ID未記録」は広すぎる記述だったため[訂正証拠](../../work/analysis/ui-dls-empty-semantics/20261011T213700Z/countin-endpoint-addendum-v1.json)を追加した。旧報告・旧判定器は保持する。原版内部のAudioPath型と実ロードGM.DLS、厳密音色/波形は未計測。

原版の短いクリック試聴は固定音高窓で失敗し、合格へ書き換えていない。90秒録音の時間超過、回帰音声混入、toolbar Stop不適用、継続再開時に録音窓終了した試行を保持し、新しい条件の120秒録音だけを独立判定した。候補関連回帰・他の発音は正式採用録音に重ねていない。原版の設定はStereo & Reverbへ復旧し、同一processを開いたまま保持した。

現行画面は原版961×752、候補1207×794。OS倍率は未測定で旧4K/200%を現在の測定値にしない。厳密DPI/font/theme/寸法比較、global catalogの同名・未一致locale解決、既定値のまま再open、DEFAULTGM無効の空DLS再生は別残件。

[単位](../../work/analysis/ui-dls-empty-semantics/20261011T213700Z/unit-record-v3.json)、[閉鎖監査](../../work/analysis/ui-dls-empty-semantics/20261011T213700Z/final-audit-v3.json)、[native監査](../../work/analysis/ui-dls-empty-semantics/20261011T213700Z/native-proof-v4.json)、[原版PCM](../../work/analysis/ui-dls-empty-semantics/20261011T213700Z/original-transport-proof-v5.json)、[候補PCM](../../work/analysis/ui-dls-empty-semantics/20261011T213700Z/candidate-transport-proof-v4.json)。

次：SSP08 同じ20261011T224147887Zで登録native100とdriver172の一巡を実施する。48native/30driverの既知拒否は実行せず保持し、必要GUI/audioが未実行のdriverを完了にしない。原始run/input/source/output hashを照合し、旧193237の成功を転用しない。全40/全8・SSP04親残件・6640未保存Project破棄回答待ち・独立Style015244/Q2を保持。

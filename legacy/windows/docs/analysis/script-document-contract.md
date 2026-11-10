# Script文書契約と残責務

対象はScriptDesigner.ocx、ScriptStripMgr.dll、ContainerDesigner.ocxの必要責務。全体未完了。現在の単位はScript文書の所有、ソース・言語・名称・読込設定、履歴、本体/native Project保存復元を閉じる。routine/変数/Script時刻起動、Container参照・埋込・alias編集を省略した完成にはしない。

一次資料は原版FarmGame/FarmMusic.spt、Tutorial/FinishedProject/FarmMusic.spp、保全済みDirectX SDK `work/analysis/sources/dmusicf.h` 1638〜1710行。sampleの構造/hashは `work/analysis/q3-script-container/20261005T022200Z/original-sample-observation.json`。原版動的比較はstartup registry/window障害で未確認。静的RIFF観測を動的比較としない。

両sampleはRIFF DMSC、schd DWORD flags=3、scve version、GUID、UNFO/UNAM、RIFF DMCN、scla UTF-16 VBScript、scsr UTF-16ソースを持つ。`.spt` と `.spp` を別の根本形式として扱わない。SDKはscsrに代えてGUID_NULL classのDMRF外部ソース参照を認めるため、外部参照を読込不能へ狭めない。外部ファイル読込・実行はこの文書モデルで暗黙に行わない。

言語/ソース/名称のNULL終端UTF-16とUnicodeを所有する。ソース編集はscsrのみ変更し、内蔵Containerと他chunkを全bytes維持する。外部参照をソース編集で置換する場合は明示的な操作とし、Undoで元の参照へ復元する。名称/言語とLOAD_ALL_CONTENT(1)、DOWNLOAD_ALL_SEGMENTS(2)はまとめて一履歴として変更する。2は1が未設定の場合もSDKに意味があるため禁止しない。未定義flagと拡張header bytesは保持する。

不正・曖昧な入力は採用前に拒否し、正常文書・保存checkpoint・Undo/Redoを保持する。保存は既存のatomic writeで、OS拒否時はcheckpointを進めない。異常入力試験にはUTF-16、重複source、短いheader、NUL、無対surrogate、external source、未知flagsを含める。新規文書は空の所有ソース、VBScript、flags0、空DMCNを持つ。原版新規文書のデフォルトとの一致は未観測であり主張しない。

候補20261005T023607765ZでFramework所有、native Projectメタデータ/保存/復元/外部sourceコピーと本体editorを実装。専用55/core791/smoke合格。本体Unicodeソース/名称編集、Undo/Redo、Script/native Project保存、通常終了exit0を確認。別PID6900は起動し応答するが、sky一覧に画面が現れずGUI再読込と終了を未確認として保持。証拠: work/analysis/q3-script-container/20261005T022200Z/unit-record.json。

現行025654288ZではContainer Alias本体編集・同一native Project保存・別プロセス復元/双方終了0を確認（[最新単位](../../work/analysis/q3-container-graph/20261005T025700Z/unit-record.json)）。旧023607765Zの画面取得障害は候補別履歴として保持。

未完了: 原版相互編集、routine一覧/呼出しと変数・エラー位置、Script track timing、ContainerのCRUD/参照・埋込置換/loader寿命、runtime export/移動時の参照調整、main共通OpenへのScript接続。音声・原版依存解消と全8受入は別判定。


20261006T081405935Zの限定実行契約：所有VBScript/JScriptは元scsrを変更せずIActiveScriptで実行し、OS Script/Containerは公開APIのprivate helperとして保持する。TraceはSDK inline UTF16 PMsg（offset56、64+2*n、flags0x41/type14）をGraph経由で送る。Container aliasのLoadは著者呼出時にhelper Script文脈で実施し、遅延Load後のGetVariableObjectは置換されたhelperの実aliasから取得する。先行Loadや保持済み古いIDispatchの再利用でlazy契約を壊さない。Container42とmanual/scheduled/replay/hidden exact Unicode Trace27が同候補で合格。本体Script保存/独立PID復元、native Project保存/作者破棄なし閉鎖まで限定成立。原版ScriptDesigner利用不可のため動的比較は未確認。Qualified Performance、他methods/HRESULT/variant/任意alias/graphなし、AudioVBScript/外部source経路、全40/8は残責務。証拠：[最新単位](../../work/analysis/q3-source-script-host/20261006T074300Z/unit-record.json)。上記旧候補の未完了表記は履歴であり既存機能の再実装指示としない。

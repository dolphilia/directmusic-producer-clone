# Container graph契約と残責務

一次資料: 保全済みSDK dmusicf.h 1053–1145、原版FarmMusic.spt/.sppのDMCN/cosl/cobl。原版静的sampleは10のSegment参照、conh NOLOADS=2、cobh flags0、LIST/DMRF型を持つ。原版動的操作はstartup/window障害により未確認。

conh DWORD flags、cobh GUID class/flags/ckid/type、optional coba NULL終端WCHAR aliasを型付き所有する。cobhが示すchunk/typeを一つだけ採用し、DMRF refh classとの一致を検証。未知chunk・flags・拡張header/order/paddingは保存する。別の任意RIFF readable chunkをembedded payloadとして扱い、既知Producer形式に狭めない。NOLOADS(2)とKEEP(1)は既知bitだけ変更する。alias重複を原版未観測の禁止契約にせず、lookupの曖昧さを扱う。

独立Container/ProjectのReference Runtime Segment文書経路は候補201655513Zで限定成立。Main新規/name/alias/KEEP/NOLOADS/削除/各UndoRedo/.cop/native Project保存、別PID16448の復元とhash不変、専用21checksを確認した。参照ファイル探索/GUID解決の全class、ロード順序・KEEP lifetime、Embed Runtime出力、実行時接続、原版相互編集、全8受入は未完了。未知data保持だけを実装済みとしない。

版別履歴: 候補20261005T025654288Z（124保存ソース）のconfigure/build/install exit0、core791/ScriptContainer70/smoke合格。EXE SHA256 `0e52990af5fef2d391cd00195cf2442bcb5b4c3f29c2a4ab1eebb954c3029812`、build summary SHA256 `3d2af72fcc9c5b554b44e763e96aaf2ef3ecd3b049ea33757d340b7bf3be93b5`。本体Alias Apply/Undo/Redo、Script/native Project保存、正常終了0、同一Project別PID16728再読込と正常終了0。保存FarmMusic SHA256 `468623fb2ba444e8db18b317b9187104ffb9db1395c6e115ceccd2b29a9a78c6`、Alias以外のchunkは完全一致。KEEP/NOLOADSはnative試験とGUI復元のみで、GUI変更・runtime効果は未確認。

固定回帰46は29合格17障害、94driverは29合格11障害54未実行。証拠と再現コマンド：[最新単位](../../work/analysis/q3-container-graph/20261005T025700Z/unit-record.json)、[一巡](../../work/acceptance/regression/20261005T031011496Z/run.json)、[driver一巡](../../work/acceptance/registered-drivers/20261005T031023275Z/run.json)。原版動的比較は起動/window障害で未完了。静的SDK/保存監査は代替しない。

現行証拠: [独立Container単位](../../work/analysis/q3-standalone-container/20261005T201500Z/unit-record.json)。原版helpのcopリンク/con埋込/Reference Runtimeリンクを区別し、未知data保持と文書経路だけで全Container互換を完成としない。alias衝突の編集制限は原版追加契約が必要であり残queue。

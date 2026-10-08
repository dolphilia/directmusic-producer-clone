# Farm score player接続（未完）

同梱 `FarmGame/src/FARM.CPP`・README・FarmScript.txtを一次契約として、Add-InsへFarm Score Playerを追加した。12routineボタン、所有Scriptの選択・初期化、数値変数get/setを既存Framework/ScriptSessionで行い、専用Conductorをwindow終了時に解放する。原版Farm.exeやDMUtil/DXUtil binaryを製品へ追加していない。

**compile/install/native/GUI/audioは未実行。** 直前の208-source候補084224157ZはSDK探索MSB4184でconfigure失敗し、現在216ソース相当の木に新しい生成物はない。SDKアクセス判断待ちで同条件再試行は行わない。本単位は限定完了にも昇格しない。

`script_dependencies.*`を製品coreへ追加し、`Conductor.load_script`に接続した。明示したnative入力を事前に所有し、クラス/GUIDを優先して選び、それがなければ明示fileを使う。入れ子DMRFは私有コピーだけをGUID参照にし、`ScriptSession`がbacking bytesを保持してLoaderへ`SetObject`を行う。事前`GetObject`は追加せず、NOLOADS/KEEP/aliasとDLSの既存WSMP policyを維持する。登録済みOS moduleを読み取りで照合し、Chord/Command/Style/Pattern/Muteをdmstyleへ正しく振り分けた。これは未コンパイルの実装であり、runtime効果は未確認。

独立RIFF監査は19 native files/24 references/10 aliasesを確認し、40対照（原入力1、欠落・切断38、Wave GUID変更1）を実行した。10個のSegment参照GUIDは実ファイルと異なる。最初の厳密GUID一致監査失敗と、DLS `dlid`を見落とした初回観測、alias順序判定器の失敗を保存している。公開[GetObject契約](https://learn.microsoft.com/en-us/previous-versions/ms809373(v=msdn.10))は全識別属性の一致を要求しないため、原入力の不一致を隠さず、選択契約を修正した。[SetObject契約](https://learn.microsoft.com/en-au/previous-versions/ms809415(v=msdn.10))に従い、選ばれたオブジェクトの元GUIDを保持する。原版動的比較は未確認である。

専用`--script-dependencies`と`--farm-script-runtime`、各driverと入力監査・対照をmanifestへ登録した。入力は原native bytesの正確な複製で、原版Farm.exeを含まない。再現は新しく用意した入力directoryを`Create-FarmRuntimeFixture.mjs`で作り、新候補の`Test-RegressionManifest.ps1 -Only script-dependencies,farm-script-runtime`へ渡す。native2モードは未実行。関連既存Script runtime/track、Sequence26、DLS sample/runtimeを新候補で確認した後、Farm本体操作→native保存→通常exit→別PID復元→WASAPIの背景Stop/再開/secondary/SFX→原版同条件比較へ進む。

所有GUID-only参照は下記の未コンパイル実装を追加した。name/category/埋込graph/config/未知class/循環/KEEP外部寿命の全Loader互換は残責務であり、この限定resolverを全Container完成と呼ばない。APFarm.aud経路とWave loop/end、Q2、全40全8も未完。

再開入口はproduct-state.currentのinProgressUnit（Tempo表示修正）とindependentInProgressUnit（Waves Reverb単位）、Farmを含むunfinishedUnits。最新完了単位は0749 Timeline keyboardであり、現候補の成功結果には転用しない。
## GUID-only継続（C++未検証）

同じ進行中Farm単位の [開始範囲](../../work/analysis/q3-farm-player/20261007T085530733Z/guid-only-scope.json) に従い、file属性のないGUID+CLASS参照を、明示file/埋込から索引済みの所有データで解決する修正を追加した。トップaliasとnested参照のforward ownership、inactive file文字列保持、GUID/クラス不一致と欠落の拒否を既存専用モードへ追加した。新しい検索先・registry探索・COM activationは追加していない。Send編集単位に続く独立ソース作業であり、Farmの終了条件は変えていない。

GUID-only継続時点は214ソース相当、登録85 native/130 driver。現在の件数は下記継続記録とcurrentを使用する。SDK拒否で新生成物がなく、C++/本体/native/音声/原版実行は未検証。この時点ではname/category/full descriptor、到達不能の明示fileを含む優先規則、追加class/config/graph、identity-less/recursive graph、KEEP外部寿命が残っていた。到達不能fileの後続修正と未検証状態は下記を参照する。以前のScriptDesigner.dllという対象名を40対象表に照合し、ScriptDesigner.ocxへ修正した。旧記録を保持した [最新checkpoint](../../work/analysis/q3-farm-player/20261007T085530733Z/guid-only-checkpoint.json) を使用する。`fullAcceptance=false`。


## 到達不能fileと所有GUID優先の継続（C++未検証）

同じ未完Farm単位の [開始範囲](../../work/analysis/q3-farm-player/20261007T085530733Z/file-priority-20261007T112558244Z/continuation-scope.json) と [契約照合](../../work/analysis/q3-farm-player/20261007T085530733Z/file-priority-20261007T112558244Z/contract-review.md) に従って修正した。先行file索引でopen/readが失敗すると後の所有GUIDを選べない不整合に対し、IO例外だけを保存して選択時に必要なら元例外を返す。所有class/GUIDがある場合はそのデータを選び、未使用fileの失敗を観測として保持する。読取方法・エラーメッセージと64MiB/形式/GUID衝突/class/NUL/深さ・個数ガードは維持する。

既存script-dependenciesモードにトップ/nested/前方・逆順所有、欠落file、directoryなし、GUID不一致・欠落、file-only、形式不正、共有拒否の対照を追加した。既存Farm入力とNOLOADS/KEEP/alias/opaque保存条件を維持する。共有拒否は専用試験が新しく作るcontrol fileで、既知Windows5への再試行ではない。

現在216ソース、失敗候補からpending20件、登録87 native/133 driver。 [保存ソース](../../work/analysis/q3-farm-player/20261007T085530733Z/file-priority-20261007T112558244Z/unbuilt-source-snapshot.json) は未ビルドである。 [整合性run](../../work/analysis/q3-farm-player/20261007T085530733Z/file-priority-20261007T112558244Z/checkpoint-run.json) と [proof](../../work/analysis/q3-farm-player/20261007T085530733Z/file-priority-20261007T112558244Z/checkpoint-proof.json) はsource/state/provenanceのみを検証し、C++試験の合格を示さない。

新しいSDK解除・環境回答の証拠はなく、configure/build/install/native/本体/PCM/原版比較は未実行。同条件buildは再試行していない。riff.cppの例外型変更を含め、新候補の通常core/Sequence/DLS/Script依存/Farm/Container/AudioPath/FileOutput/Waves回帰が必要である。まずTempo本体/native/二process正常終了を完結し、未完Farm/Send/Wavesの本体・音声へ進む。name/category/full descriptor、追加class/config/graph、identity-less/cycles/KEEP、APFarm/Wave loop/end、各Windows5・原版approval・Q2、全40全8を保持し、fullAcceptance=falseを維持する。

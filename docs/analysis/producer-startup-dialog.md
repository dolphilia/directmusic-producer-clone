# 起動時ダイアログの撮影と自動OK

2026-10-02、ユーザーから起動後のダイアログを撮影し、自動でOKを押す依頼を受けた。既存Sky操作では編集画面を取得できるが、起動時の `Failed to update the system registry. Please try using REGEDIT.` ダイアログは対象ウィンドウ一覧に現れない。起動要求が失敗を返しても、実プロセスは警告待ちで生存する場合がある。

`tests/native/producer_startup_dialog.cpp` と `scripts/Handle-ProducerStartupDialog.ps1` を追加した。補助プログラムの既定動作は読取りだけであり、以下を実装する。

- 試験用EXEのSHA-256、準備済み試験ディレクトリとplan.appをスクリプトで照合する。
- 指定PIDの実行ファイルパス、可視・有効な標準ダイアログ、タイトル、既知の警告文、所有PIDを照合する。
- OKボタンのID・クラス・文面・親・状態を照合し、ダイアログだけをPrintWindowで撮影してPNGへ保存する。
- ダイアログとOKボタンの画面座標、画像内相対座標と中心をJSONへ保存する。
- 明示した `-Click` の場合だけ、撮影後に再照合して対象ダイアログへ標準の `WM_COMMAND/IDOK` を送る。前面ウィンドウへの座標クリックには依存しない。
- 未知の警告・複数の候補・撮影失敗・PIDや文面の変化には入力を送らず失敗する。

実装とビルドは完了したが、実行確認は未完了。SHA-256 `70bb796ed5e86f460969e3eb107d37a397bfc38f0a2e1dcacc5728b9bb5ac604` の補助EXEは、最初の読取り専用実行でWindowsのアプリケーション制御ポリシーに拒否された。記録は `work/integration/user-trial/20261002T074920855Z/startup-dialog-20261002T094457999Z-observe/run.json`。launchErrorに拒否を保持し、exitCodeはnull、dialog.jsonlは空である。PNG生成・実ボタン位置・自動OK成功を確認したとは扱わない。セキュリティ設定は変更していない。

既存読取りプローブではPID14908の試験用Producerに、タイトルDirectMusic Producer、クラス#32770、既知レジストリ警告、子Button「OK」が存在することを確認した。`auto-ok-startup-observe.jsonl` はその観測であり、スクリーンショットやクリックの証拠ではない。

ユーザー提供の18:46:24スクリーンショットは3840×2160、SHA-256 `048b1d45c39f32ea4551c0fae9d8cbb8d231b2e58c04ea84433af671f6e24173`。画像を原寸で確認し、同じ警告文とOKを読めた。画像内のOK中心は概ね(x=2085,y=1135)、ダイアログ左上は概ね(1651,885)。これは提供画像上の目視位置であり、現在の操作座標や補助プログラムによる撮影結果ではない。自動処理には固定座標を使わず、実行時に所有PIDとボタンを再検出する。

実行が許可される環境での確認手順：

```powershell
cmake -S tests/native/startup_dialog -B work/build/startup-dialog -G 'Visual Studio 17 2022' -A Win32
cmake --build work/build/startup-dialog --config Release
./scripts/Handle-ProducerStartupDialog.ps1 -TrialDirectory (Resolve-Path work/integration/user-trial/20261002T074920855Z) -ProcessId <起動中の試験用PID>
# dialog.pngとdialog.jsonlを確認してから、同じPIDへ次を実行する。
./scripts/Handle-ProducerStartupDialog.ps1 -TrialDirectory (Resolve-Path work/integration/user-trial/20261002T074920855Z) -ProcessId <同じPID> -Click
```

`dialogDismissed=true` に加え、Producerの編集画面を取得し、終了・別の警告を区別する必要がある。この補助処理はProducer固有モジュールの互換実装や、全体再構築の完了を示すものではない。

## 外部ツールによる撮影・自動OKの確認

ユーザーの外部ツール調査・試用依頼に従い、2026-10-02 19:04（日本時間）に公式AutoIt 3.3.18.0のポータブル版で成功した。通常ユーザー環境にAutoItの既存配置は見つからず、公式配布から `work/tools/autoit-3.3.18.0` へ取得した。インストール、COM登録、Windowsのセキュリティ設定変更は行っていない。以前拒否された補助EXEを再包装したり、AutoItからロードしたりはしていない。

- [公式配布ページ](https://www.autoitscript.com/site/autoit/downloads/)：署名付きポータブル版。取得ZIP SHA-256 `ceb666a993a9f62621c3a0d4ee602774b2e7f543de1f08ec0380632ee3f89beb`。
- 使用したx86 AutoIt3.exe SHA-256 `bdd2b7236a110b04c288380ad56e8d7909411da93eed2921301206de0cb0dda1`。AuthenticodeはValid、署名者AUTOIT CONSULTING LTD。
- [WinList](https://www.autoitscript.com/autoit3/docs/functions/WinList.htm)、[ControlGetHandle](https://www.autoitscript.com/autoit3/docs/functions/ControlGetHandle.htm)、[ControlClick](https://www.autoitscript.com/autoit3/docs/functions/ControlClick.htm)で対象を検出・操作する。
- `ProducerStartupDialog.au3` と `Handle-ProducerStartupDialogAutoIt.ps1` が試験ディレクトリ、EXEハッシュ、実プロセスパス、PID、タイトル、ダイアログクラス、既知文面、ボタンの親・PID・文面・可視・有効状態を照合する。撮影はAutoIt付属ScreenCaptureライブラリ。スクリプトのスレッドだけに [物理ピクセルのDPIコンテキスト](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setthreaddpiawarenesscontext) を設定する。

実際のレジストリ警告のOKボタンはID **2** だった。以前のC++補助コードのIDOK=1という想定はこのダイアログには合わない。したがって上のC++実行手順は今回の警告に使用せず、AutoItの確認済み手順を使う。AutoItはButton1を取得してIDが1または2か、表示文面がOKかを確認し、その実ボタンのハンドルへControlClickを送る。画面固定座標やID1への強制コマンドには依存しない。

証拠は試験ディレクトリ `work/integration/user-trial/20261002T074920855Z` 内に保持した。

| 実行 | 結果 |
| --- | --- |
| startup-autoit-20261002T100221923Z-observe、100242153Z、100300031Z | AutoIt実行は成功したが、ID1照合により対象0件で終了1。クリックなし。診断によりID2を確認 |
| startup-autoit-20261002T100322560Z-observe | 終了0・PNG生成。ただし目視でDPIによる誤った領域の撮影を確認。成功撮影とは扱わない |
| startup-autoit-20261002T100357530Z-observe | 終了0。550×298のPNGで警告文とOKを目視確認。ボタンの画像内中心(439,251)、画面上中心(2085,1135) |
| startup-autoit-20261002T100417090Z-click | 同じPID18600と同じソースを再照合・再撮影後、ControlClick=1、dialogDismissed=true、終了0 |

クリック後のSky一覧に、同じProducer実行ファイルの編集ウィンドウが現れた。`get_window_state` のスクリーンショットとアクセシビリティでFile/Edit/Viewメニュー、ツリー、Transport/Synth Statusの各ツールバーと作業領域を確認し、警告待ちやプロセス終了とは区別した。保存記録は `startup-autoit-frame-confirmation.json`。今回のクリックで文書編集は行っていない。

次回の起動時にも、対象プロセスのPIDで次を実行する。既定は撮影のみ、`-Click` 指定時に撮影と再照合後のOK操作を行う。未知の警告や候補数が1以外の場合は入力を送らず失敗する。

```powershell
./scripts/Handle-ProducerStartupDialogAutoIt.ps1 -TrialDirectory (Resolve-Path work/integration/user-trial/20261002T074920855Z) -ProcessId <起動中の試験用PID> -Click
```

今回確認したのは既知レジストリ警告の撮影と自動OK、続く編集画面の表示である。Components欠落警告の自動OK、全警告への汎用対応、Producer全体の互換実装の完了は確認していない。原版起動試験の待ちを解消できたため、次はこの手段で候補DLLの終了・再起動後のUI比較を進める。

続く候補PID6548と原版PID17040でも、同じ既知警告の撮影・実ボタンの自動OKと編集画面への移行に成功した（`startup-autoit-20261002T100926320Z-click`、`startup-autoit-20261002T103020465Z-click`）。この操作を使い、候補の終了再起動読込と原版の選択表示比較を進めた。旧式Open Projectダイアログも公式AutoItで明示パスの設定・読戻し・開くボタン操作に成功し、プロジェクトツリーの読取りと観測済みheartland.sgpの起動も確認した。追加スクリプトは `Open-ProducerTrialProjectAutoIt.ps1` と `Invoke-ProducerTrialTreeAutoIt.ps1`。対象は準備済み試験コピーのPIDとUiTest入力に限定する。

新候補のPID5896でも同じ処理が成功した（`startup-autoit-20261002T104201682Z-click`）。550×298の警告画像、実ボタンID2、画像内中心(439,251)、ControlClick=1、dialogDismissed=trueを保存した。続く編集画面で実ロードDLLのハッシュと選択表示を確認し、試験後に本体を終了して原版DLLへ復元した。これで原版と複数の候補起動を自動OKで進められることを実測した。コードと実行手順は上記AutoIt版を使う。

## 実行アカウントによるウィンドウ取得の違い

続くPID20076では、既定のサンドボックス実行ユーザー `DESKTOP-PR7GO31\CodexSandboxOffline` から同じ署名済みAutoItを実行すると、既知ダイアログ0件・終了1になった（`startup-autoit-20261002T111040473Z-click`）。Producerは生存していた。この0件はダイアログ不存在や候補DLLの起動拒否を示さない。

画面ユーザー `DESKTOP-PR7GO31\dolph` と同じ実行環境で実行すると、同じPIDの警告を取得・撮影できた（`startup-autoit-20261002T111232142Z-observe`）。550×298のPNGで警告文とOKを確認し、続く `startup-autoit-20261002T111302187Z-click` でControlClick=1・dialogDismissed=true・終了0、Skyで編集画面の表示を確認した。`tempo-host-rounds/20261002T110939964Z/desktop-identity.json` に実行ユーザー・SID、Producer所有ユーザーとセッションを保持した。Windowsのセキュリティ設定変更や拒否された補助EXEの再実行は行っていない。

このCodex環境では、AutoIt補助スクリプトの実行に `require_escalated` を指定して画面ユーザー側で動かす。これは今回の実行アカウントの一致を確保する条件であり、Windows管理者権限が常に必要という結論ではない。以後の3つのPowerShellラッパーは `desktopIdentity` として実行ユーザー・SID・実行セッション・Producerセッションも記録する。過去の保存ソースとrun.jsonは変更していない。

同じPIDで旧式Open Projectの入力・読戻し・開くボタン操作（`project-autoit-20261002T111652255Z-open`）と、観測済みツリー `#5|#2` のheartland.sgp起動（`tree-autoit-20261002T111732941Z-open`）も終了0で成功した。今回の候補DLLによるコピー・切り取り・再貼付け、Undo／Redoの保存結果と終了・復元は [統合試験](tempo-integration.md) に記録する。未知の警告や別プロセスの汎用クリックは引き続き実施していない。

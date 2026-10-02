# Producer 本体への統合試験

更新日：2026-10-02。工程5は未完了。原版と代替 TempoStripMgr の両方で、QuickStart の読込、テンポ変更、保存、Undo・Redo、プロジェクト閉鎖後の再読込に成功した。候補は本体終了・再起動後の再読込も確認した。クリップボード、ドラッグ、音声出力などの受入は継続する。

## 原版の起動結果

ユーザーから原版の試験用コピーを表示して起動する承認を受けた。起動時のエラーは OK で進む指示も受けた。使用した DMUSProd.exe は 518,144 bytes、SHA-256 `fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011`。`scripts/Run-ProducerStartup.ps1` は app 内の42ファイルを run ごとに複製し、起動前のハッシュと登録状態を保存する。元の配布物は変更していない。

`work/integration/startup/20261002T064049208Z/` のコピーを Computer Use の `sky.launch_app` で起動した。PID 7716、Default デスクトップ上で、以下のモーダルダイアログを確認した。

> Unable to load DirectMusic Producer Components.  Please run Setup and reinstall.

根拠は `default-desktop-windows.jsonl`。読み取り専用の `producer_startup_probe` が指定 PID の実行ファイルパスを検証し、そのプロセスのウィンドウだけを列挙した。メインフレーム、MDIClient、Project Tree、Properties のウィンドウは作られたが、フレームは非表示・無効で、編集画面へは進んでいなかった。

Computer Use はこのモーダルを対象ウィンドウとして取得できず、こちらでは OK を押せなかった。ユーザーが OK を押した後も編集画面は開かなかった。`processes-after-user-ok.json` に DMUSProd プロセスが残っていないことを記録した。これは今回の解析用配置での結果であり、Windows 11 全般で起動できないことの証拠ではない。ユーザーは以前 Windows 11 で OK 後に起動できた経験を報告している。

同じコピーでの先行する非表示起動 PID 6704、および `20261002T065057196Z/` の PID 16468 は、PowerShell のサンドボックス専用デスクトップ上で起動した。後者では app 配下の DMUSProducer ディレクトリ作成についてアクセス拒否を観測した。この結果を Default デスクトップでの Components エラーと混同しない。いずれも試験プロセスを終了し、デスクトップ名を記録した。

`files-before.json`、`file-checks-after-launch.json` で42ファイルの一致を確認した。この初回試験では COM 登録、フォントのインストール、セキュリティ設定変更は行っていない。後続の一時登録試験は以下に区別して記録する。

## Components 登録の静的根拠

`scripts/Inspect-ProducerRegistration.mjs` で再抽出できる。成果は [producer-registration.json](producer-registration.json)。全モジュールのバイト列を PE 記録のハッシュと照合してから、文字列・リソース・命令を読む。DLL の登録関数を実行するスクリプトではない。

DMUSProd の RVA `0x18c93` は、x86 の既定ビューで `HKLM\Software\Microsoft\DMUSProducer\Components` を `KEY_READ` で開く。失敗すると文字列リソース `0xee76` の上記エラーを表示して false を返す。今回、起動前に HKLM/HKCU の32/64ビット各ビューで DMUSProducer キーが存在しないことを `registry-before.json` に記録した。この分岐と表示内容が一致するため、Components 登録の欠落を今回の起動中断の原因として扱う。

キーを開けた場合は、GUID 名のサブキーを列挙する。既定の REG_SZ を表示名として読み、DWORD `Skip=1` の項目を飛ばす。GUID を変換し、RVA `0x18e02` の CoCreateInstance で IID `9F3ED901-46B7-11D0-89AC-00A0C9054129` のコンポーネントを要求する。したがって空の Components キーを作るだけでは、必要な編集モジュールの初期化を保証しない。

SegmentDesigner の DllRegisterServer は RVA `0x1f9be` の登録ヘルパーへ進む。このヘルパーは、HKCR の CLSID 登録に加え、HKLM の Components と Container Objects を書く。Segment の Components 登録は次の内容である。

| 項目 | 内容 |
| --- | --- |
| コンポーネント CLSID | `DFCE860B-A6FA-11D1-8881-00C04FBF8D15`、GUID RVA `0x50dc` |
| Components サブキーの既定値 | `Segment Designer`、文字列リソース201 |
| Skip | DWORD 0 |
| RefNode | `DFCE8609-A6FA-11D1-8881-00C04FBF8D15` |
| Segment ノード | `DFCE860A-A6FA-11D1-8881-00C04FBF8D15` |
| DirectMusic オブジェクト | `D2AC2882-B39B-11D1-8704-00600893B1BD` |

Components 既定値の書込みは RVA `0x1fcd2`、Skip の書込みは `0x1fcf5`。REG_SZ ヘルパー `0x1f411` は RegCreateKeyExA / RegSetValueExA / RegCloseKey を呼ぶ。28モジュールから Producer 固有の登録パスを抽出したが、他モジュールの役割と値はまだ完全に復元していない。JSON 内の文字列参照を完全なインストール用マニフェストとは扱わない。

## 比較用プログラムへの起動拒否

原版の Components エラーとは別に、新規ビルドした `work/build/repro-command-20261002T070351228Z/Release/com_window_probe.exe` が Windows のアプリケーション制御により起動を拒否された。`work/integration/startup/code-integrity-events-20261002.json` に CodeIntegrity Operational の記録を保存した。

2026-10-02 16:08:15・16:08:18 JST のイベント3077はこのパスを明示し、対応する3118には Smart App Control のブロックが記録されている。3077の SHA256 Flat Hash は `e73f41c8fc5c2577f2adcb4c367fa38b94ac2a7b2a7bba5c55ec94ae8014ce36` で run.json のプローブハッシュと一致する。ユーザーも起動を止めた警告通知を報告した。

保存35ソースから全7ターゲットのクリーンビルドは成功した。ただし、この新規生成物による実行比較は未確認。6,887件の成功比較は既存の `work/build/probes` の生成物による。セキュリティ機能を無効化して再試験していない。

## 次の統合条件

1. 他の必須コンポーネントについて、登録関数の値・CLSID・初期化順序を復元する。
2. 復元した登録内容のバックアップと復元手順を用意し、正式インストールとの差を確認できる試験配置を作る。
3. 原版で起動・サンプル読込・編集・保存・再読込を確認する。その後に TempoStripMgr を置換して同じ操作を比較する。
4. 起動拒否のある新規生成物は未検証のまま区別する。本体での表示、キーボード配送、文書 Undo と音声出力は別途検証する。

## 専用ハイブでの登録観測の進展

`tests/native/registration/CMakeLists.txt` と `producer_registration_probe` を追加し、`scripts/Run-RegistrationCapture.ps1` で33個の Producer 固有 DLL/OCX を個別に観測した。最初の Segment 実行は `work/integration/registration/20261002T072808095Z/`、残る32モジュールは `20261002T072922522Z` から `20261002T072939518Z`。30件が登録・列挙・参照先復元まで成功し、10 Components と43 COMクラスを取得した。[登録観測一覧](registration-capture.md) と元のキー・値・バイト列をまとめた [registration-capture.json](registration-capture.json) を参照。

Windows の [RegLoadAppKeyW](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regloadappkeyw) で新しいファイルハイブを作り、[RegOverridePredefKey](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regoverridepredefkey) で HKCR/HKLM/HKCU の参照先をその試験プロセス内だけで切り替えた。変更先は各 run の capture.hiv である。本体の実インストールや、グローバルへの COM 登録は行っていない。元モジュールのハッシュと、グローバルの DMUSProducer キーおよび Segment の2 CLSIDの存在状態は起動前後で一致した。

ADSREnvelope、PanVol、RegionKeyboard は `0x80040200`（SELFREG_E_TYPELIB）で失敗し、完全な登録内容は未取得。COM 初期化と TypeLib の読取診断を追加した新しいプローブは、20261002T073350140Z、073351148Z、073351803Z の各実行で起動を拒否された。診断による原因特定は未完了。先行する30件の成功結果と3件の登録失敗、新しいプローブの起動拒否を区別して保持した。

取得済み40 PEモジュールの [全体検証状態](implementation-status.md) も作成した。互換ソースがあるのは現時点では TempoStripMgr の限定経路であり、他モジュールの登録観測を再実装の完成とは扱わない。

## 通常ユーザーでの一時登録試験

`work/integration/user-trial/20261002T074920855Z/` に原版42ファイル、登録計画、実行・復元記録を保存する。`scripts/Prepare-ProducerUserTrial.ps1` は、登録先が未使用であることを確認し、計画の SHA-256 と実行アカウントの SID を固定してから変更する。部分失敗でも復元できるよう、最初の変更前に所有記録を書く。

通常ユーザー dolph の HKCU に87個の未使用ルート、471値を追加し、全値の読戻しに成功した（`install-state.json`）。内訳は32ビット COM 登録の43 CLSID・42 ProgID、および64ビットビューの VirtualStore に設けた Producer 専用2ルート。既存の DirectShow 名前空間への追加は除外した。HKLM とセキュリティ設定には、この段階では変更を加えていない。

専用ハイブで取得した43 InprocServer32 のうち20個は、日本語を含むパスが文字化けして解決できなかった（`work/integration/registration/server-path-validation.json`）。そのため、新しい ASCII パスの NTFS ジャンクションを作業コピーへ向け、検証済みの原版ファイルを登録した。原版バイナリは加工していない。これはパスに関する試験対策であり、TypeLib 登録失敗の原因を確定したものではない。

この構成で再起動しても、PID 20060 は同じ Components エラーを表示した。`relaunch-windows-canonical.jsonl` は実行ファイルの実パスを照合し、Default デスクトップ上の正確な文面を記録する。ユーザー提供のスクリーンショットでも同文面を確認した。QueryFullProcessImageName はジャンクション先の実パスを返すため、別途保存した alias 指定の照合失敗を本体の異常とは扱わない。編集画面、文書読込、保存、再生は未検証。

`machine-access.json` に、通常ユーザーが実際の32ビット HKLM Components キーを見つけられず、HKLM Software\\Microsoft への書込権限もないことを保存した。VirtualStore への値配置では今回の探索を成立させられなかった。EnableVirtualization=1 と EnableLUA=1 は読取値であり、本体のトークンで仮想化が有効なことを証明しない。

通常のサンドボックス実行アカウント CodexSandboxOffline と、画面上の実行アカウント dolph は SID が異なる。HKCU の試験結果はアカウントを明示し、サンドボックス側の準備のみの run `20261002T074747964Z` と、実際に登録した上記 run を混同しない。

ユーザー承認を受け、未使用の32ビット HKLM\\Software\\Microsoft\\DMUSProducer に105値だけを追加する `machine-plan.json` と `scripts/Set-ProducerMachineTrial.ps1` を用意した。Windows の管理者承認後に105値の追加・読戻し確認が成功した（`machine-install-state.json`、`admin-install-result.json`）。このスクリプトは既存キーを上書きせず、COM クラス・フォント・セキュリティ設定を変更しない。HKCU・HKLM の一時登録は、後続の置換試験用に保持中。試験終了後は同じ SID で HKCU の Restore を実行し、87ルートの不存在を確認する。HKLM も所有記録と計画ハッシュを照合して Producer 専用キーだけを削除し、不存在を確認する。アプリ自身による設定保存を含む全レジストリの完全復元を確認したという意味ではない。

## 原版の UI 編集・保存・再読込

105値の登録後の PID 19088 は Components エラーを越え、代わりに `Failed to update the system registry. Please try using REGEDIT.` を表示した（`machine-startup-windows-settled.jsonl`）。ユーザーが OK を押すと編集画面が開き、Computer Use でも一意の対象ウィンドウを取得できた。本体から10個の Components DLL/OCX、および dmusic、dmime、dmloader、dmsynth、DSOUND のロードを確認した（`machine-startup-loaded-modules.json`）。ロードは音声出力の合格を意味しない。

同梱 QuickStart の5ファイルを新しい UiTest/QuickStart へコピーし、元ファイルとのハッシュ一致を保存した（`ui-sample-inputs.json`）。UI で QuickStart.pro を開き、プロジェクトツリーから heartland.sgp を開いた。Tempo ストリップの先頭112 BPMを選択し、同梱ヘルプに記載された F11 で Tempo Properties を表示した。小節1・拍1・tick 0 を確認して137 BPMへ変更し、Tab で確定、Ctrl+Sで保存した。保存物を `heartland-original-137.sgp` として別途保持した。

保存後に文書 Undo で112 BPMへ戻ること、Edit メニューの Redo 操作後に137 BPMとなることを確認した。操作直後の UI スナップショットには古い値が残る場合があり、後続の確定状態と区別して `ui-actions.jsonl` に保持する。プロジェクトを閉じ、最近使った QuickStart.pro を再び読み込んで heartland を開くと、Tempo ストリップと Transport に137 BPMを表示した。Alt+F4 でアプリを終了し、`original-ui-exit.json` に PID の終了を保存した。

読取専用の `scripts/Inspect-SegmentRiff.mjs` でコンテナ境界を検証した。元の2,962 bytesのファイルは時刻0・112 BPM、保存した2,986 bytesのファイルは時刻0・137 BPM。どちらも `RIFF:DMSG / LIST:trkl / RIFF:DMTK / tetr` の16バイトイベントである。`ui-riff-tempo-comparison.json` にチャンク一覧・元ハッシュ・保存ハッシュ・イベントを残した。元の配布サンプル5ファイルは不変（`source-samples-check.json`）。この実測は限定した原版試験であり、コピー・貼付け、複数文書、再生・音声出力の合格を示さない。

## 候補 DLL の基本編集・保存・再読込

原版の UI 終了後、`scripts/Set-TempoIntegrationTrial.ps1` で試験コピーの TempoStripMgr.dll だけを置換した。元 DLL のバックアップを保存し、元42ファイルと候補ハッシュを確認してから変更した。元 DLL は `bb9811c74f68dcf0b37d32fe2ae89d3e45962e59b95f1ec93ddf7a12635a5c95`、候補は6,887レコード比較と同じ `9d73a68d5460571aa63e02d78b3fa65da261a35b51e14ca7a0e74159a192d12d`。所有記録は `tempo-replacement.json`。候補をクリーンビルド生成物の実行成功とは扱わない。

候補を置いた本体でも同じレジストリ更新警告が出た。ユーザーの OK 後、PID 17644 の編集画面で試験を行った。実ロードのパスとハッシュは `candidate-loaded-tempo-identity.json` に記録し、候補 SHA-256 の一致を確認した。比較入力5ファイルは元サンプルと同じハッシュ（`candidate-ui-sample-inputs.json`）。最初に用意した CandidateQuickStart は「プロジェクト名とフォルダー名が異なる」として本体が読込を拒否したため、`app/UiTest/Candidate/QuickStart/QuickStart.pro` に入力を複製した。元サンプルは変更していない。

原版と同じ操作で、heartland.sgp の112.00 BPMイベントを選び、F11のプロパティから137へ変更し、Tabで確定、Ctrl+Sで保存した。Ctrl+Zで112に戻り、Editメニューの Redo Change Tempo で137へ復元した。再保存、プロジェクト閉鎖・再読込後も137が表示された。操作ごとの落ち着いた UI ツリーは `ui-actions.jsonl` の `candidate_*` 記録に保存した。入力直後のスナップショットが前の表示を返す場合があるため、再観測してから判定した。

PID 17644はAlt+F4で正常終了した（`candidate-ui-exit.json`）。同じ候補のまま再起動した PID 17968でも、警告をユーザーがOKした後に保存した137を表示した。再起動後の候補ハッシュは `candidate-restarted-loaded-tempo-identity.json`、プロジェクトの実パスは UI の Project Properties と `candidate_restart_project_path_confirmed` で確認した。この過程は `scripts/Summarize-TempoIntegration.mjs` が入力・DLL・保存データ・UI観測を照合する。原版の成功から候補の成功を推定していない。

候補の保存スナップショット `heartland-candidate-137.sgp` は2986 bytes、SHA-256 `d712903bd25385b94c207aa96a4e72689910e4782cded41a675aebab987c2cea`。RIFFの `tetr` は16 byteイベント1件、time=0、BPM=137（`candidate-ui-riff-tempo-comparison.json`）。`scripts/Compare-SegmentRiff.mjs` で原版保存と全leaf payloadを無正規化で比較すると、58個中53個が一致し、テンポの20 byte payloadも一致する。残る5個は別トラック `LIST:cord/crdb`、各132 bytesの一部（payload内offset10〜31）の差分である。原因は未確定であり、文書全体の完全一致や、差分が無害であるとは判定しない。全差分を `original-candidate-saved-chunks.json` に保持した。

PID17968も正常終了後、Set-TempoIntegrationTrial の Restore を実行し、試験コピーを原版 DLL のハッシュへ戻した。HKCU・HKLMの一時登録は後続の原版クリップボード観測に使うため保持中。今回の最小機能は本体で成立したが、追加・削除・システムクリップボード・実ドラッグ・複数文書の独立性・音声出力は今回の成功に含めない。工程5全体と全体再構築は完了していない。

復元した原版のPID7868で候補保存済みのheartlandを開き、137 BPMを表示した（`original_reloaded_candidate_saved_137`）。実ロードの元DLLハッシュは `original-restored-loaded-tempo-identity.json`、復元記録は `tempo-restoration.json`。続くクリップボード観測には保存済みスナップショットを入力として別の試験文書を用意し、基本試験の証拠を上書きしない。次の一手は原版UIのコピー・貼付けを観測し、現在 E_NOTIMPL のシステムクリップボード経路と本体・Timelineの責務を確定することである。

再検査コマンド：

```powershell
node scripts/Summarize-TempoIntegration.mjs work/integration/user-trial/20261002T074920855Z
node scripts/Compare-SegmentRiff.mjs work/integration/user-trial/20261002T074920855Z/heartland-original-137.sgp work/integration/user-trial/20261002T074920855Z/heartland-candidate-137.sgp
```

## 原版UIのシステムクリップボード

PID7868の原版で元サンプル5ファイルを新しい `UiTest/ClipboardReference/QuickStart` へコピーした。初期112のイベントをCtrl+Cし、2小節目をクリックしてCtrl+V、Ctrl+S。保存スナップショット `heartland-clipboard-reference-paste.sgp` はtime=0/3072、BPM=112/112。2小節目をCtrl+Xし保存した `heartland-clipboard-reference-cut.sgp` はtime=0のみ。3小節目へCtrl+Vし保存した `heartland-clipboard-reference-repaste.sgp` はtime=0/6144、BPM=112/112。操作はui-actions.jsonlのclipboard_reference_*、元5入力はclipboard-reference-inputs.json、3保存の構造とハッシュはclipboard-reference-riff-sequence.jsonに保持した。基本テンポ編集の証拠は上書きしていない。

PID7868はAlt+F4で終了した（clipboard-reference-ui-exit.json）。4ケースの限定したネイティブ比較後、`Set-TempoClipboardTrial.ps1` で試験コピーだけを候補 `69029024…` に切り替えた。原版42ファイル、元DLLバックアップ、候補ハッシュと比較記録を検査し、tempo-clipboard-replacement.jsonにfullNativeRunsPassed=falseも明記した。元へ戻すRestoreを同スクリプトに用意した。

実プロセスPID5564は、実パス照合後に `Failed to update the system registry. Please try using REGEDIT.` で待機した（clipboard-candidate-startup.jsonl）。Skyの起動要求と一時PID3812の終了は、実プロセスの終了と混同しない。ユーザーがOKを押すと編集画面が開いた。操作ツールの古いウィンドウIDによる接続失敗は、JavaScriptセッションを初期化して解消した。

## 候補UIのシステムクリップボード

元サンプル5ファイルと同一ハッシュの `UiTest/ClipboardCandidate/QuickStart` を明示パスで開き、heartlandを編集した。PID5564へ実ロードされた候補DLLのSHA-256は `69029024df3a082e14343cc8684d2baeb9162b3087fa8f310a2d29e3b5bdd733`（clipboard-candidate-loaded-identity.json）。112 BPMをCtrl+Cし、2小節目へEdit/Paste/Mergeで貼付けて保存した。最初のCtrl+VではViewメニューが開いたため、貼付け成功に数えずメニュー操作へ切り替えた。Edit/Cutで2小節目のイベントを切り取り保存し、3小節目へCtrl+V、Ctrl+Sで再貼付けを保存した。

3保存のtetrは原版UIと同じtime=[0,3072]、[0]、[0,6144]、BPMはすべて112。clipboard-candidate-riff-sequence.jsonに構造と各ファイルハッシュを保持する。最後の `heartland-clipboard-candidate-repaste.sgp` はSHA-256 `e0e44c37ed4d5c6064ec9da533138f3f7a6fbd127cb454ca8c7a9845ae4f419c`。原版保存とのleaf比較はテンポを含む53/58一致、残る5件は既存の別トラックcrdb差分で、文書全体の完全一致とはしない（clipboard-ui-repaste-comparison.json）。

再貼付けのUndoで3小節目が消え、Redoで復元された。保存後、File/Close Projectで候補プロジェクトを閉じ、同じ明示パスから読み直すと1・3小節目の112を表示した。閉鎖時に共有styleリンクを外す確認が出たため、その確認と実際の閉鎖を区別して記録する。独立した複数文書の受入試験には数えない。ui-actions.jsonlのclipboard_candidate_*と、入力・DLL・3保存・UI操作・再読込・終了を照合するSummarize-TempoIntegrationで再検査できる。

PID5564をAlt+F4で終了し、プロセス不存在を確認した（clipboard-candidate-ui-exit.json）。Set-TempoClipboardTrialのRestoreで元DLLへ戻し、元ハッシュを確認した（tempo-clipboard-restoration.json）。候補保存物の原版再読込用PID19980は起動警告の手動OK待ち（clipboard-original-reload-startup-windows.jsonl）。この候補690のプロセス再起動後の再読込、原版での相互読込は未確認。HKCU471値／87ルート、HKLM105値は引き続き試験用に保持し、復元待ちである。

クリップボードのネイティブ比較については、原版TimelineのExportでカウンターが残ることを動的にも確認した。[寿命の調査](timeline-clipboard-lifetime.md)に、復元処理の異常終了と修正、同一36ソース・プローブによる再比較を記録する。候補UIの成功をTimelineアンロードの成功や全体再構築の完了へ拡大しない。

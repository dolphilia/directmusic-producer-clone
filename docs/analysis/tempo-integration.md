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

PID5564をAlt+F4で終了し、プロセス不存在を確認した（clipboard-candidate-ui-exit.json）。Set-TempoClipboardTrialのRestoreで元DLLへ戻し、元ハッシュを確認した（tempo-clipboard-restoration.json）。候補保存物の原版再読込用PID19980は、起動警告の手動OK後に編集画面が開いた。明示パスでClipboardCandidateのQuickStartを開き、プロジェクトのプロパティでも同じパスを照合した。heartlandの1・3小節目に112を表示し、実ロードDLLのハッシュは元版bb9811c74…だった（clipboard-original-reload-loaded-identity.json、ui-actions.jsonlのclipboard_original_reload_*）。原版終了後も保存ファイルのSHA-256はe0e44c37…のままで、元へ戻したDLLのハッシュも一致した（clipboard-original-reload-ui-exit.json）。候補690のプロセス再起動後の再読込は未確認。HKCU471値／87ルート、HKLM105値は引き続き試験用に保持し、復元待ちである。

PID19980の原版で同じheartlandを再生した。ElapsedとOffsetが進み、発音数17・最大19、その後最大20を表示した。ユーザーから「音楽が聞こえた」と聴取確認を得た。停止後の発音数0とAlt+F4によるプロセス終了も確認した（clipboard_original_playback_*）。これは原版DLL群による候補保存物の再生であり、候補TempoDLLをロードした状態の音声出力、録音波形の比較、Producer全体の互換実装による再生は未検証である。

クリップボードのネイティブ比較については、原版TimelineのExportでカウンターが残ることを動的にも確認した。[寿命の調査](timeline-clipboard-lifetime.md)に、復元処理の異常終了と修正、同一36ソース・プローブによる再比較を記録する。候補UIの成功をTimelineアンロードの成功や全体再構築の完了へ拡大しない。

## 候補690の終了・別プロセスでの再読込

2026-10-02 19:08〜19:21（日本時間）。前回のPID5564終了記録と原版バックアップ・復元を照合し、同じ候補690を再配置した。`Set-TempoClipboardTrial.ps1 -Mode Reinstall` は以前の置換記録と元バックアップを上書きせず、新しい `tempo-clipboard-reinstall-20261002T100850085Z.json` を残す。原版PID18600の終了を確認してから配置した。

新しいPID6548の起動警告を公式署名付きAutoItで撮影・照合し、実ボタンのControlClickで進めた（`startup-autoit-20261002T100926320Z-click`）。Skyの旧式Open Project入力は、キャッシュ要素不一致、geometry unavailable、settable不一致で成功しなかった。入力成功に数えず、同じ生存プロセスのままAutoItで対象ダイアログと入力欄・実ボタンを照合した。ID1検索は一覧を返す場合があり、Button1は読取り専用チェックだったため、ボタン群の実ID1・親ダイアログを確認して開くButton2を特定した。失敗観測を `project-autoit-*` に保持した。

`project-autoit-20261002T101446001Z-open` のログは明示パス `app/UiTest/ClipboardCandidate/QuickStart/QuickStart.pro` の入力文字列とControlClick=1、ダイアログ閉鎖を保持する。`tree-autoit-20261002T101700911Z-observe` が追加された5番目のQuickStartと4子項目を観測し、`tree-autoit-20261002T101813571Z-open` がそのheartland.sgpをテキスト・PID・実行ファイル照合後に開いた。F11でプロジェクトルートのプロパティを要求した際はNo properties availableであり、プロジェクトの実パスをプロパティ画面で確認できたとはしない。

セグメント読込後、実ロードの候補SHA-256 `69029024df3a082e14343cc8684d2baeb9162b3087fa8f310a2d29e3b5bdd733` を確認した（`clipboard-candidate-restarted-loaded-identity.json`）。画面の1・3小節目に112.00を表示し、3小節目のTempo PropertiesはMeasure=3、Beat=1、Tick=0、Tempo=112.00だった。`ui-actions.jsonl` の `clipboard_candidate_restart_reloaded_overview` と `clipboard_candidate_restart_measure3_112`、`clipboard-candidate-restart-frame.png` が証拠。前回保存のheartland.sgpは読込前後・終了後もSHA-256 `e0e44c37ed4d5c6064ec9da533138f3f7a6fbd127cb454ca8c7a9845ae4f419c` のままである。

別の未確認点を二つ残す。先頭イベントを選んだ表示後もTempo PropertiesがMeasure=3を示した（`clipboard-candidate-restart-property-selection.png`、`clipboard_candidate_restart_selection_property_pending`）。選択の更新、ページの更新、操作ツールの配送のどこに原因があるか、同じ操作を原版で比較する必要がある。また `.pro` は読込前SHA-256 `4a9209d66d56e5604afd1628f7f16981f7000ad47ed1e750a3cd06077d1deea1`、終了後 `5698951e97c71f94002b896d17c997ca4bbdf6614dd629a6533f739ba654cd44` に変わった。終了後ファイルを `clipboard-candidate-restart-project-after.pro` に保持したが、今回は読込前のバイト列を保存しておらず、差分の原因は未確定。次回はOpen-ProducerTrialProjectAutoItが読込前コピーも保持する。文書を意図的に編集しなかったことを、プロジェクトファイル全体の不変性とは扱わない。

PID6548はAlt+F4後に不存在を確認し（`clipboard-candidate-restarted-ui-exit.json`）、元DLLへ戻した。現在の試験コピーのTempoStripMgrは原版 `bb9811c74…`。Summarize-TempoIntegrationのschema4が別PID・実ロードDLL・元入力・ツール成功・保存ソース・明示パス・セグメント開閉・PNGハッシュを照合し、限定した再起動読込を確認する。同時に選択／ページ更新とプロジェクトメタデータの未確認を残す。音声出力、実ドラッグ、複数文書と全体再構築の合格は増やしていない。

```powershell
node scripts/Summarize-TempoIntegration.mjs work/integration/user-trial/20261002T074920855Z
```

次は復元した原版で同じ1・3小節目の選択とプロパティ切替を比較し、`.pro` の読込前後をコピーして差分のチャンクを特定する。

## 原版による選択表示とプロジェクトメタデータの切り分け

2026-10-02 19:30〜19:33（日本時間）、復元済み原版でPID17040を起動した。公式AutoItによる撮影・自動OKは再び成功した（`startup-autoit-20261002T103020465Z-click`、ControlClick=1、dialogDismissed=true）。原版フレームのプロジェクトプロパティには、候補保存物の `UiTest/ClipboardCandidate/QuickStart/QuickStart.pro` が表示された。ここからセグメントを開く前に `.pro` をコピーした。これは起動前のコピーではなく、自動再開後・セグメント操作前のコピーである。ハッシュ5698951e…は前回候補終了後のスナップショットとも一致した。

AutoItで5番目のQuickStartとその4子項目を読取り、heartland.sgpを開いた（`tree-autoit-20261002T103100745Z-observe`、`tree-autoit-20261002T103110869Z-open`）。実ロードのTempo DLLは原版bb9811c74…だった。画面の3小節目の112.00をクリックするとTempo PropertiesはMeasure=3、続いて1小節目をクリックするとMeasure=1へ更新された。閉じてF11で再表示してもMeasure=1だった。UI観測は `clipboard_original_selection_measure3`、`clipboard_original_selection_measure1`、`clipboard_original_selection_measure1_reopened`、画像は `clipboard-original-selection-measure1.png` と `clipboard-original-selection-measure1-reopened.png`。候補PID6548のMeasure=3残存と原版の表示は異なる。原因と修正は未確定で、選択／プロパティ更新の受入は未合格とする。

同一対象のSetObjectがGetDataを再取得しない契約は、既存の原版プローブと静的コードの両方で確認済みである。この契約を変えて表示差を隠さず、マウス選択後にページ更新を要求する原版の経路を調べる。

Alt+F4で終了後、PID17040の不存在を確認した（`clipboard-original-selection-ui-exit.json`）。heartland.sgpはe0e44c37…のまま。`.pro` は5698951e…から89ff596d…へ変わった。`clipboard-original-selection-project-before.pro` と `clipboard-original-selection-project-after.pro` の全leaf payloadを正規化なしで比較し、63件中56件が一致、7件が変更された（`clipboard-original-project-metadata-comparison.json`）。差分はpjctのoffset 2〜17、open/guidの16 bytes、edtwと4つのfilhの先頭16 bytesに限られる。GUID配置に見える識別子領域であり、各識別子の意味と再生成理由はまだ仕様化していない。原版でも管理情報が変わることは分かったが、候補の読込前バイト列がないため、候補側の変更全体を説明できたとはしない。

Summarize-TempoIntegrationのschema5は原版DLL・同じ保存セグメント・プロセス終了・AutoItソース・UI値・PNGと前後ファイルのハッシュを検査し、RIFF比較を再実行して記録と照合する。原版比較待ちは解除し、候補の表示差と原因未確定を残す。現在はProducer終了済み、試験用Tempo DLLは原版へ復元済み。一時登録は後続試験用に保持中。

## 選択後のプロパティ更新の修正

原版TempoStripMgrの左ボタン処理では、選択変更・Invalidate・ShowPropertiesに続いてページ管理のRefreshDataを呼んでいた。WM_LBUTTONDOWNのRVA aec8〜aee1と、複数選択を単一へ縮小するWM_LBUTTONUPのb0bb〜b121で確認した。候補にこの更新要求を追加した。同一対象のSetObjectがGetDataを省略する既存契約は維持した。

実ネイティブページを表示したまま、3小節目・1小節目・Ctrlで3小節目を追加・1小節目への縮小・2小節目の空拍を順にクリックし、各ボタン押下／解放の10段階で選択モデルと実コントロールの表示を照合する回帰を追加した。テスト側からクリック後のRefreshDataは呼ばない。修正前候補690の `work/candidate/tempo/20261002T103841783Z` は全10段階で古い137.25／Measure 1／Beat 2／Tick 42が残り終了1、原版 `work/reference/tempo/20261002T103829395Z` は終了0。保存データの不変性は両方で確認した。これはロード拒否ではなく表示回帰の失敗である。

新候補SHA-256 `00c3a3aaf7a8cbfeb2a6a0aa9c954f8421dba5e5ec75d74ee865bc6ecfd8448f` と原版を、同じプローブSHA-256 `3d93c73cf4970a794063cd1973e572a0228f78629823e4a19e4394d0f8713f1c` と36ソースで比較した。原版 `20261002T103949808Z` と候補 `20261002T104000544Z` はともに終了0。比較 `work/comparison/tempo-dll/20261002T104031478Z/comparison.json` は6,914観測、445通常ファイル、123コピー用データ、66画像で差異0、選択更新10段階と保存不変を検査する。比較器のページ呼出し期待数を追加ケースに対応させた。ネイティブ実行時の比較器スナップショットと、この集計時の比較器ハッシュは区別して保持する。

`Set-TempoSelectionTrial.ps1` はこの成功比較、実行メタデータとログ、対象ソース・DLLのハッシュ、原版42ファイルを照合し、原版と候補の両バイナリを保存して試験コピーだけを置換した（`tempo-selection-replacement.json`）。新PID5896の既知警告を公式AutoItで撮影・自動OKし、Project Propertiesに表示された同じClipboardCandidateの実パスを照合してheartlandを開いた。実ロードの新候補ハッシュは `selection-fixed-loaded-identity.json` に保持する。

本体画面で3小節目を選ぶとMeasure 3、続けて1小節目を選ぶとMeasure 1に更新された。閉じてF11で再表示してもMeasure 1／Beat 1／Tick 0／112.00だった。`ui-actions.jsonl` の `selection_fixed_measure3`、`selection_fixed_measure1`、`selection_fixed_measure1_reopened` と、`selection-fixed-measure1.png`、`selection-fixed-measure1-reopened.png` が証拠である。Alt+F4後にPID5896の不存在、heartland.sgpのe0e44c37…不変を確認し（`selection-fixed-ui-exit.json`）、原版bb9811c74…へ復元した（`tempo-selection-restoration.json`）。

Summarize-TempoIntegrationのschema6は新候補の成功を `selectionFix` に分けて検査する。候補690の失敗履歴は残す。今回のネイティブ比較はsystemClipboard=falseであり、旧候補のクリップボードUI・終了再起動・原版での聴取結果を新候補の合格へ引き継がない。新候補のそれらの受入、実ドラッグ、複数文書、プロジェクト管理情報と非テンポ保存差分、TimeSigの比較と全体再構築を継続する。
## 2026-10-02 選択表示修正候補の新しいクリップボード試験

状態：候補SHA-256 `00c3a3aaf7a8cbfeb2a6a0aa9c954f8421dba5e5ec75d74ee865bc6ecfd8448f` の本体コピー・切り取り・再貼付け・保存・Undo／Redoを確認した。今回の保存データの再起動読込・原版相互運用・音声出力は未確認。全体の完了判定は変更しない。

独立した所有記録とバックアップを作る `Set-TempoHostRound.ps1` を追加し、検証済み候補の保存DLLだけを試験コピーへ設置した。新しい原版QuickStart入力5ファイルを `UiTest/SelectionClipboard/QuickStart` へコピーし、以前の `ClipboardCandidate` 入力と証拠は保持した。PID20076で起動警告の自動OK、明示したプロジェクトの読込、ツリーからのheartland.sgp起動を公式AutoItで実行した。実ロードDLLのパス・基底アドレス・ハッシュを確認した。

証拠は `work/integration/user-trial/20261002T074920855Z/tempo-host-rounds/20261002T110939964Z`。保存した5つのRIFFを境界検査付きで再解析した結果は次の通り。

| 操作 | 保存テンポ（時刻、BPM） | スナップショット |
| --- | --- | --- |
| 先頭をコピーして2小節目へ貼付け | (0,112)、(3072,112) | paste-second.sgp |
| 2小節目を切り取り | (0,112) | cut-second.sgp |
| 3小節目へ再貼付け | (0,112)、(6144,112) | paste-third.sgp |
| Undo | (0,112) | undo-third.sgp |
| Redo | (0,112)、(6144,112) | redo-third.sgp |

Undo保存はcut-secondと、Redo保存はpaste-thirdとファイル全体のSHA-256が一致した。PID20076終了後の保存ファイルもRedoと一致した。終了を確認してから原版TempoのSHA-256 `bb9811c74f68dcf0b37d32fe2ae89d3e45962e59b95f1ec93ddf7a12635a5c95` へ復元した。42個の元アプリファイルの照合、候補のネイティブ比較6914件、公式AutoItの保存ソースと実行ログ、保存RIFF、終了・復元を `Summarize-TempoHostRound.mjs` で再検査し、integration-summary schema7へ追加した。

記録上の制限：画面で選択3→1への表示更新は観測したが、今回の `recordHostUi` は以前の変数束縛をクロージャに保持し、25件すべてに起動直後の古いアクセシビリティツリーを保存していた。元ログを改変せず `ui-log-quality.json` に原因とハッシュを残し、今回のツリーをプロジェクト／選択プロパティの証拠から除外した。AutoItの独立ログ・実ロードDLL照合・保存ファイルと復元はこの不具合の影響を受けない。前回PID5896の別証拠による選択修正の確認は維持する。次の記録は現在の観測を明示的な関数引数として渡す。

再検査コマンド：

```powershell
node scripts/Summarize-TempoHostRound.mjs work/integration/user-trial/20261002T074920855Z/tempo-host-rounds/20261002T110939964Z
node scripts/Summarize-TempoIntegration.mjs work/integration/user-trial/20261002T074920855Z
```

次はこの候補と保存した新しいSGPで終了・再起動読込と選択プロパティの証拠を取り、原版で同じファイルを開く。候補音声出力、保存時のテンポ以外のメタデータの差、実ドラッグ、複数文書、TimeSig接続とProducer全体の再構築は残っている。登録試験の87ルート471値と32ビットHKLM105値は継続試験のため復元待ちである。

# 原版の試験環境

更新日：2026-10-02。工程2は作業中。以下の表は専用プロセスからの DLL 単体試験の条件。後続の一時登録による本体 UI 試験は末尾と [統合試験](tempo-integration.md) に区別して記録する。正式インストーラーによる完全な比較環境は未構築。

| 項目 | 現在の実測・状態 |
| --- | --- |
| ホスト OS | `Microsoft Windows NT 10.0.26200.0` |
| プローブ | ネイティブ C++、Windows x86、STA |
| ビルド | Visual Studio 2022、MSVC 19.44.35228、Windows SDK 10.0.26100.0 |
| ロード方法 | 絶対パスと `LoadLibraryExW` の `LOAD_WITH_ALTERED_SEARCH_PATH` |
| COM の生成 | 編集 DLL は `DllGetClassObject` から非集約で CreateInstance。ランタイムは CoCreateInstance で IDirectMusicTrack を取得 |
| 登録 | DllRegisterServer / DllUnregisterServer を呼んでいない |
| 実行管理 | 子プロセス、15秒制限、終了コードとログを保存。タイムアウト時はその子プロセスのみ終了 |
| 原版 UI・インストーラー | 単体試験では未使用。後続の原版 UI 試験は末尾を参照。正式インストーラーは未実行 |
| 実際にロードされた依存 DLL の全パス・版 | テンポランタイム `dmime.dll` を収集済み。依存 DLL 全体は未完了 |
| ランタイムの再生能力、音声出力、フォント | 未検証 |

## 後続の本体 UI 試験環境

`work/integration/user-trial/20261002T074920855Z` は通常ユーザー dolph の Default デスクトップで実行した。サンドボックス用アカウントの HKCU と区別して SID を記録した。原版42ファイルを複製し、ASCII パスのジャンクション経由で登録・起動した。全値が未使用であることを確認して HKCU の87ルートに471値を追加し、ユーザー承認と Windows の管理者承認後に32ビット HKLM の Producer 専用キーへ105値を追加した。正式インストーラー、フォント、セキュリティ設定の変更は使用していない。

Components エラーは解消し、続くレジストリ更新警告をユーザーが OK で進めると原版編集画面が開いた。QuickStart のコピーを読み込み、heartland のテンポ112→137 BPM変更・保存・文書 Undo・メニュー Redo・プロジェクト閉鎖後の再読込・アプリ終了を記録した。`integration-summary.json` は UI の確定観測と実際の RIFF データを照合する。音声出力は未検証。候補 DLL の本体置換でも同じ基本編集・保存・Undo・Redo・再読込が通り、候補は終了・再起動後も保存結果を復元した。候補試験後の原版DLLへの復元を照合済み。一時登録は続く原版クリップボード観測に使うため保持し、後続試験の終了後に専用キーの復元を確認する。現在の成否・所有記録・復元手順は [統合試験](tempo-integration.md) を参照。

## 再実行

```powershell
cmake -S . -B work/build/probes -G 'Visual Studio 17 2022' -A Win32
cmake --build work/build/probes --config Release --parallel 4
.\scripts\Run-ReferenceProbe.ps1
```

SDK 参照がサンドボックスに拒否される環境では、ビルドの実行権限を調整する必要がある。ビルド出力はリポジトリ内に置く。

生成直後の保存と再読込を調べる場合：

```powershell
.\scripts\Run-ReferenceProbe.ps1 -SaveInitialStream
```

**この指定は Framework 未接続の負例であり、Load 時のアクセス違反で失敗する。** ライフサイクルのみの試験は成功している。出力は `work/reference/tempo/<UTC日時>/` に保存され、失敗時も残す。旧ログの `saveEmptyStream` は現在の `saveInitialStream` と同じ試験指定を意味する。保存データ自体は空ではない。

限定 Framework と実ランタイムを接続する試験：

```powershell
.\scripts\Run-ReferenceProbe.ps1 -Connected
```

`20261002T003412534Z` のビルドでは、初期データの保存・再読込、バイト一致、ランタイム90→120 BPM更新、参照解放が成功した。限定 Framework は `tests/native/tempo_host_fixture.h` にあり、空のコンポーネント一覧とメタデータ付きメモリストリームを提供する。Timeline、Conductor、文書管理、UI、音声出力を提供していない。

その後、複数イベント等を追加したビルドは一度 **Windows Smart App Control / Code Integrity に起動を拒否された**。通常のサンドボックス外実行でも同じ結果で、イベント3033/3077、ポリシー ID `{0283ac0f-fff1-49ae-ada1-8a933130cad6}` を確認した。証拠は `work/reference/tempo/application-control-20261002.json`。これは自動承認レビューの拒否ではない。

後続の実行では同じ拡張バイナリが起動し、`20261002T004437044Z` に成功を記録できた。セキュリティ設定は変更しておらず、拒否と解除の理由は確定していない。その時点では動的試験を再開できた。起動前に失敗しても run.json を残し、ソースのハッシュと実ランタイムのパス・版・ハッシュも保存する。

## 原版 Timeline を接続した試験

```powershell
.\scripts\Run-ReferenceProbe.ps1 -Timeline
```

この指定は `-Connected` を含み、STA を OleInitialize で初期化する。原版 `Timeline.dll` をハッシュ照合後に直接ロードし、クラスファクトリーから生成する。StripMgr の登録・取得の委譲・登録解除、COM 経由の選択・変更・削除を試験する。HWND を持つ編集画面、マウス操作、Conductor、音声出力は含まない。

`20261002T005936693Z` で全ケース成功、終了コード0、両 DLL のアンロード可能状態を確認した。原版 Timeline の SHA-256 は `bebc8149e31f3b4b0b74bffbf482acc38ba8b5b28a1468c9741e71961c6db176`。

実ランタイムは `C:\WINDOWS\SysWOW64\dmime.dll`、版 `10.0.26100.9278 (WinBuild.160101.0800)`、SHA-256 `efdb7c341b5decbe8784f9c5b498af1e2e76814556decaf0133f3f9078ca29c5`。x86 プローブが報告する System32 パスを64-bit PowerShellから読む際は WOW64 の対応先へ解決し、報告値と実ファイルのパスを両方残す。これは当時の DirectX 9 ランタイムの再現ではなく、現在の Windows のランタイムである。

次に依存 DLL 全体の来歴、原版本体の比較環境、追加・移動・Undo と UI 操作を進める。DLL 接続試験の成功を、アプリ全体の動作確認の代わりにはしない。

## 原版 TimeSigStripMgr を接続した位置編集試験

`Run-ReferenceProbe.ps1 -TimeSignature` は `-Timeline` を含む。TimeSigStripMgr の CLSID `{8C6005D2-ABDA-11D2-B0D9-00105A26620B}` を直接生成し、自作の20 bytesの `tims` データ（時刻0、4/4、grids=4）を IPersistStream::Load へ渡す。生成直後の空モデルでは位置変換が失敗することを確認し、初期拍子データを明示的に用意した。拍子側の Framework とランタイムトラックは接続しておらず、その同期・編集 UI は検証対象外。

Timeline の長さを30720 clocksとしてから TempoStripMgr を挿入する。複数 strip が存在するため、列挙順を仮定せず、プロパティ12で取得した管理オブジェクトの IUnknown 同一性によりテンポ strip を選ぶ。`20261002T014530015Z` の原版と `work/candidate/tempo/20261002T015126510Z` の代替版で、非0位置の変更・1小節移動と3 DLLの解放が成功した。

TimeSigStripMgr.dll の SHA-256 は `898258cfbf1b17bef0054695d25331c530e2ee2846c5d45073a8fd5670484bb7`、版は5.3.0.900。パス・版・ハッシュは run.json の `timeSignatureModule` に保存する。

その後の境界・複数選択用プローブ（SHA-256 `54ba04e3dd93c41389ed6e5571f3a3295e47a765d522d37db27f981490da7a62`）は、`20261002T015417818Z` と `20261002T015454759Z` で Smart App Control に起動を拒否された。前者の `code-integrity-events.json` にイベント3033/3077/3118と上記と同じポリシー ID を記録した。原版 DLL の呼出し前の失敗で、自動承認レビューによる拒否ではない。セキュリティ設定は変更していない。

以後の実行スクリプトは、run.json に記録した参照ソースを run 内の `sources/` へコピーし、コピー後のハッシュも照合する。過去の実行でソースのコピーを保存していないものは、run.json のハッシュのみが残る。

## 境界・拍子変更の実行成功と追加試験の制限

拍子変更の試験を追加したプローブ（SHA-256 `2dbb6db197efd9c0b81c15da69177a4ac6d3a2f80b837fea7a1c4231780821e5`）は起動でき、`work/reference/tempo/20261002T020601617Z/` に境界6ケースと拍子変更3ケースの成功を記録した。同一プローブで代替版 `work/candidate/tempo/20261002T020912325Z/` も成功した。セキュリティ設定は変更していない。元の拒否されたハッシュの許可を確認した結果ではない。

さらに通知登録の重複・解除漏れを確認するコードを追加したプローブ（SHA-256 `4fe07449dbfd718e5165515305d4ff634cf9ebbc5133f189d1b3cc98282c2700`）は、原版側 `20261002T021055436Z` と代替版側 `20261002T021058879Z` で再び起動拒否となった。原版側の `code-integrity-events.json` に OS イベントを保存した。

モデル単体の実行ファイル（SHA-256 `0286119aec230c965c877b200c6d96df5767f3f70f4c3aafdaf8d5fb479b08fa`）も拒否された。`work/comparison/tempo/20261002T020947860Z/comparison.json` の起動失敗は同じ OS イベントと対応する。その試行では単体試験を実行できなかった。

現在のソース保存対象は、原版・代替版の両 run で全 CMake 入力と比較スクリプトを含む。これ以前のコピーは記録されたファイルだけを含み、単独でプロジェクト全体を構成できるとは限らない。

後続の再試行では、通知登録チェックを含む同じプローブ SHA-256 `4fe07449…82c2700` が起動し、原版 `20261002T021855247Z` と代替版 `20261002T021919993Z` の両方に成功を記録した。6 GUIDの登録・重複なし・切断後不在を確認し、全750レコード・55ファイルが一致した。モデル単体も同じ SHA-256 `0286119a…79b08fa` で起動でき、`work/comparison/tempo/20261002T021855016Z` に24件比較の成功を記録した。セキュリティ設定は変更していない。拒否と後続の実行許可の理由は確定しておらず、恒久的に解消したとは断定しない。

`work/reference/tempo/20261002T021855247Z/sources` だけをソースディレクトリとして、新規 `work/build/repro-20261002T021855247Z` に CMake 構成と Release ビルドを行い、全4ターゲットの生成に成功した。コマンドと確認範囲は [代替 DLL 比較](tempo-comparison.md) に記載している。

## 追加試験の座標条件

追加処理を比較した初回は原版 `20261002T022716737Z` と代替版 `20261002T023458250Z` で保存76ファイルが一致したが、ClocksToPosition のX座標が6件異なった。表示倍率を指定しておらず、実行間で値が異なった原因は未確定。失敗した比較記録は `work/comparison/tempo-dll/20261002T023506895Z` に保存している。

Timeline の SetTimelineProperty スロット10・プロパティ8に VT_R8=0.125 を渡すようにプローブを変更した。原版 `20261002T023617410Z` と代替版 `20261002T023621317Z` は同じプローブ（SHA-256 `118f785d4fd83e41a3247065ab75aaa73fd159077c0993fe5d1a5dc30459ac60`）で成功し、座標を含む977レコード・76ファイルが一致した。比較から座標を除外していない。両 run に現ソースのコピーとハッシュがある。実画面や本体インストールを必要としない直接 COM 試験であり、本体上の統合確認は未実施。

## コピー・切り取り試験の起動と再試行

コピー試験追加後のプローブSHA-256 b535676f1b5d5d58bb66e0376bb7467c7eb3680d536ec7db463608c61b525795は、原版実行20261002T024443535Zでアプリケーション制御による起動拒否となった。同じハッシュを再試行した20261002T024644475Zでは成功した。両run.jsonに起動結果とソースを保存しており、セキュリティ設定は変更していない。

Undo操作名の観測を加えたプローブSHA-256 2f29dd111f2ff1a5c3f816007f833509a64d1b565096c6d36ad70584762370c5では、原版20261002T025003151Zと代替版20261002T025007727Zの両方が終了コード0・タイムアウトなしで成功した。コピー用データは原版Timelineが生成したCOMデータオブジェクトへ保存しており、システムクリップボードの内容は使用していない。本体のインストール・UI・音声再生の確認とは区別する。

## 貼付け試験の入力消費とカーソル

原版実行20261002T030126199Zは、事前検査で入力ストリームを取得したため形式の利用可能状態が消費され、PasteがE_UNEXPECTEDとなった。終了コード1の記録を保持する。観測用のデータオブジェクトを分離した20261002T030234031Zは終了コード0だが、TimeStripMgrが未接続で実カーソルは0のままだった。この実行を非0位置での貼付けの証拠には使用しない。

同じTimeline.dll内のTimeStripMgrを接続し、カーソルを読み戻す検査を追加したプローブSHA-256 88e56a64cd1f3cae0cc223af801eef5edc69e2c53145cbec5d696f61eaa3f11aで、原版20261002T030521345Zと代替版20261002T030704686Zが終了コード0・タイムアウトなしとなった。比較20261002T031256359Zは1587レコード、通常116ファイル、paddingを限定して正規化したコピー用15ファイルの一致を示す。新しいCOM登録やシステムクリップボードの変更は行っていない。

## 通知と範囲選択の後続試験

通知用プローブの初回ビルドはローカル型とシリアライズ補助関数の宣言不足で失敗した。その直後の原版20261002T031916609Zは旧プローブ88e56a64…を実行しており、保存した作業中ソースから生成されたものではない。通知試験やソースと実行ファイルの対応の証拠には使用しない。修正後のビルド成功を確認してから後続試験を行った。

原版20261002T031958165Zはアプリケーション制御による起動拒否。同一プローブ50970b68e41b2007372d6d396a21625a9c20059fd053a679be2e8e3a6a08c64dは20261002T032151973Zで成功した。セキュリティ設定は変更していない。代替版20261002T032208076Zと比較し、テンポ変更通知8ケースを含む1868レコード・通常140ファイル・コピー用15ファイルの内容が一致した。

範囲選択追加後は原版20261002T032622430Z、代替版20261002T032636899Zが終了コード0・タイムアウトなし。全2501レコード・通常172ファイル・コピー用23ファイルの内容一致を20261002T032657289Zに保存した。保存ソースだけを新規ディレクトリwork/build/repro-20261002T032636899Zで構成・ビルドし、全4ターゲットの生成にも成功した。


## 非表示の Timeline ウィンドウ

tests/native/hidden_ole_site.h に IOleClientSite・IOleInPlaceSite・IOleInPlaceFrame の限定実装を作った。親は WS_VISIBLE を付けない STATIC ウィンドウ。原版を直接ロードして IOleObject を取得し、SetClientSite、DoVerb(OLEIVERB_INPLACEACTIVATE)、IOleInPlaceObject::GetWindow を通す。子 HWND の実在・親子関係と親が非表示であることを検査する。レジストリ登録、インストーラー実行、原版ファイルの変更は行っていない。

work/reference/timeline-window/20261002T035012355Z は DoVerb が戻らず、15秒で試験プロセスを終了した。初期長さ・倍率を先に設定した20261002T035235126Zでも同じ位置で停止した。診断追加後の20261002T035458939Zでは、例外0xC0000005、操作種別8（実行アクセス）を観測した。診断の stack_timeline_rvas はスタック上の候補値であり、復元済みの呼出し履歴ではない。

旧 ATL の thunk は実行不可ページ上で動く場合があり、Windows は DEP を有効にしたままエミュレートする互換機能を提供する。[SetProcessDEPPolicy の公式仕様](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setprocessdeppolicy) と、[/NXCOMPAT と ATL の関係を説明する Microsoft の資料](https://www.microsoft.com/en-us/msrc/blog/2009/06/understanding-dep-as-a-mitigation-technology-part-1) に基づき、ウィンドウ用の2プローブだけを /NXCOMPAT:NO でリンクし、開始直後に SetProcessDEPPolicy(PROCESS_DEP_ENABLE) を呼ぶ。GetProcessDEPPolicy で flags=1・permanent=true を確認できなければ、原版をロードする前に終了する。DEP を無効化する呼出しやシステム全体の設定変更は使用しない。

20261002T035622533Zは終了コード0で、DEP設定、ウィンドウ生成、表示位置の設定・読戻し、終了・アンロードが成功した。共通プローブへ接続した最新実行は原版20261002T040055150Z、代替版20261002T040059698Z。20ケース・40画像の比較と、それ以前の全編集試験が通る。再現コマンドは次のとおり。

    cmake --build work/build/probes --config Release --parallel 4
    .\scripts\Run-TimelineWindowProbe.ps1
    .\scripts\Run-ReferenceProbe.ps1 -Windowed
    .\scripts\Run-ReferenceProbe.ps1 -Windowed -Candidate
    node scripts/Compare-TempoDll.mjs <原版の実行ディレクトリ> <代替版の実行ディレクトリ>

アプリケーション制御による起動拒否20261002T035134450Zは、DoVerb内の例外と別の事象として記録した。同じプローブは20261002T035235126Zで起動できた。制御ポリシーの変更はしていない。

## プロパティページ接続用の限定シート

tests/native/property_page_probe.hは、Timelineプロパティ3（VT_UNKNOWN）へ接続するFrameworkと、そこからQueryInterfaceで取得されるプロパティシートを用意する。実際のシートウィンドウは生成しない。表示状態とSetPageManagerの失敗を制御し、ページ管理オブジェクトの受領、再利用、解除と参照数を観測する。

初回20261002T043503957Zは、非表示TimelineウィンドウのClose後にShowPropertiesを呼び、原版TempoStripMgrのRVA 0x6285でnull参照となった。Closeによる接続解除後の呼出しを避けるため、ページ試験を描画用ウィンドウの生成・終了より前へ移した。修正後の原版20261002T043814633Zと代替版20261002T043816216Zは正常終了した。未接続時に原版が成功するという契約にはしない。

この実行にはクリック選択13ケースも含む。非表示の原版Timelineに接続した状態でStripのメソッドを直接呼び、画面上のマウスやキーボードは操作しない。依存DLL、OS、プローブ、ソース28ファイルの識別情報は各run.jsonに保存した。

## 実ページの生成・編集試験

2026-10-02の原版20261002T050317203Z・代替版20261002T050322356Zは、非表示子シート内に実プロパティページを生成して終了コード0となった。描画用Timelineと同じDEP条件を使用し、OSや登録は変更していない。GetPropertySheetPagesが返すHPROPSHEETPAGEをPropertySheetへ渡し、生成後にページ・親子関係・非表示を確認する。EN_CHANGE / EN_KILLFOCUS / UDN_DELTAPOS / PSN_RESETを直接配送する。画面での実フォーカス移動は使用しない。

対象はDIALOG/105/1033と8コントロール。テンポ・小節・拍・tickの値渡し、境界入力、状態別有効化と解除時の参照を比較した。実モデルへの編集5ケースでは、原版Timeline、TimeSigStripMgr、dmime.dllを従来どおり接続し、保存・再読込・ランタイム同期を確認した。入力ハッシュ・依存モジュール・29ソースは各run.jsonとsourcesへ保存。再現コマンドは従来の-Windowed / -Windowed -Candidateである。本体のページ配置や本体による更新配送は検証待ち。

最初の通常サンドボックスのMSBuildはSDK検索用ユーザーディレクトリへのアクセス拒否で失敗した。承認レビューを通ったサンドボックス外ビルドでは成功した。アプリケーション制御による起動拒否や自動承認レビューの拒否とは区別する。新規ビルドの実ログはwork/build/repro-pages-20261002T050322356Z/{configure.log,build.log}。

## OLE直接試験の実行条件と保存ソースからの再実行

2026-10-02の原版20261002T053143638Z・代替版20261002T053148670Zは、非表示の原版Timelineウィンドウを接続したままStrip::OnWMMessage(WM_CREATE)、IDropSource、IDropTargetを直接呼んで終了コード0となった。tests/native/drag_probe.hのIDataObjectは独立IStreamを返し、OSのDoDragDrop・システムクリップボード・実入力は使用しない。DEPは以前と同じく永久有効を確認してから原版ATLを接続する。

保存30ソースからwork/build/repro-drag-20261002T053100438Zに全6ターゲットを新規ビルドし、Run-ReferenceProbe.ps1の-BuildDirectoryでその生成物を使用した。configure.log / build.logは同ディレクトリに保存。原版Timeline・TimeSigStripMgr・OSのdmime依存は継続し、登録や配置は変更していない。OS・共通プローブ・代替DLL・依存モジュール・保存ソースのハッシュは各run.jsonで識別できる。4980件の比較成功と負例検査は [比較結果](tempo-comparison.md) を参照。ドラッグ画像、実OLEループ、本体上の操作と音声出力は未検証。

## ドラッグ開始のインポート差し替え試験

原版20261002T060034989Z・代替版20261002T060040582Zは、非表示の原版Timelineへ接続したまま、tests/native/ole_drag_boundary.hでロード済みDLLのole32.dll!DoDragDropインポートだけを差し替えた。PEのimport名を解析し、IATセルのページ保護を一時的に読書可へ変更し、書換後は元の保護へ戻す。試験終了前に元の関数ポインターを復元し、その結果を記録する。DLLの命令・ディスク上のバイト・OSのDEP設定は変更しない。

元の開始・データ生成・終了処理は実行するが、OSのドラッグループへは入らない。開始境界で中止・失敗・外部COPY/MOVEの結果を与え、セルフドロップでは実IDropTargetを呼ぶ。物理マウス・キーボード・システムクリップボードは操作しない。右メニューやドラッグ画像は未生成。本体の起動・文書編集・音声出力の証拠とは区別する。

work/build/repro-drag-start-20261002T055947351Zは保存32ソースだけからの新規ビルドで、configure.log / build.logに全6ターゲットの成功を保存した。MSVC 19.44.35228.0、Windows SDK 10.0.26100.0、終了コード0、警告・エラーなし。-BuildDirectoryでこの生成物を指定し、最新5905件比較を行った。固定原版・同一プローブ・同一保存ソース・Timeline・TimeSigStripMgr・dmimeはrun.jsonで識別する。原版TempoStripMgr.dllのSHA-256は終了後もbb9811c74f68dcf0b37d32fe2ae89d3e45962e59b95f1ec93ddf7a12635a5c95で一致する。依存と登録は以前の限定環境から変更していない。

## 右メニューとWindows実OLE中止の限定条件

20261002T062859215Zの比較は、右メニュー表示境界の観測7件とWindowsの実DoDragDrop中止2件を追加した。右メニューはロード済み対象DLLのuser32!TrackPopupMenu IATセルだけを一時差し替え、実HMENUを検査してFALSEを返す。対話表示・メニュー選択は行わない。所有HWNDはTimelineから借りたHDCをWindowFromDCで取得し、ReleaseDCする。元ファイルを変更せず、終了前にセルを復元した。

実中止はDoDragDrop境界の既存差替え内でWindowsのAPIへ入る。別スレッドから自身のSTAのスレッドキューだけへEscapeメッセージを配送し、代理IDropSourceが実ソースへescape=TRUEを転送する。起床スレッドは終了時にjoinし、代理参照数1・転送データの最終解放0も検査した。送信先はGetCurrentThreadIdで特定した試験自身で、入力デバイス・他アプリ・システムクリップボードは操作していない。入力配送前3試行の15秒タイムアウトは比較資料に保存した。成功転送・物理入力の証拠とは区別する。

work/build/repro-right-menu-20261002T062717325Zは保存33ソースからの新規ビルド。MSVC19.44.35228.0／SDK10.0.26100.0で全6ターゲット成功・警告とエラーなし。ログはwork/build/repro-right-menu-configure.log、repro-right-menu-build.log。最新原版20261002T062839769Zと代替版20261002T062845815Zは-BuildDirectoryでこの生成物を使用した。両run.jsonにOS・プローブ・DLL・元依存と保存ソースのハッシュを記録した。登録・配置・フォント・ランタイムの環境変更は行っていない。右コマンド・ドラッグ画像、本体の起動・編集・音声は未検証。


# DirectMusic Producer recovery

DirectMusic Producer の解析と、原版固有モジュールに依存しない本体の再構築を進めるリポジトリです。全体完成は未達です。

現行192511898Zは保存46ソースの構成・ビルド・installとnative314件が成功、警告なし。所有DLSをmemory loaderへ登録し、patch256のEnum/GetInstrument、Segment Download/Play/Stop/Unload・再開を接続。GUID生成は再生用コピーだけ、保存文書/初期DLSは全bytes保持。headless24/通常55/DLS55原版40hash一致0。現行GUI・実音色・DLS編集全体・全八受入は未完了。

```powershell
cmake --preset product-win32
cmake --build --preset product-release
cmake --install work/build/product --config Release --prefix work/install/product
```

保存ソースと生成物ハッシュを結び付ける製品ビルドは `scripts/Build-ProductSnapshot.ps1`。比較ツールは既存構成に残し、製品presetでは構築しません。以下の原版・限定DLLの成功記録は、それぞれのソース版・試験構成に限られます。

解析は [DirectMusic Producer 解析と再構築の実施計画](docs/analysis-plan.md) に沿って進めます。本体の構造把握から小さな DLL の仕様復元・再実装・比較検証へ進む順序と、各工程の完了条件を定めています。

工程0の [解析基準と再現手順](docs/analysis/baseline.md) を整備しました。[本体の接続調査](docs/analysis/host-map.md) と [TempoStripMgr の仕様復元](docs/analysis/tempo-strip-manager.md) を進めています。代替 DLL をソースからビルドし、限定 Framework、原版 Timeline・TimeSigStripMgr、実ランタイムへ接続しました。通常ストリーム、境界・複数選択の編集、拍子変更通知、通知登録・解除、COM 経由の追加・コピー・切り取り・貼付け、カーソル位置でのテンポ変更通知、範囲選択、Undo 用操作名取得、保存・同期・解放を含む比較で、原版の6887件の観測結果、通常の入出力443ファイル、コピー用データ123ファイルの内容が一致しました。非表示の原版 Timeline ウィンドウを接続し、横スクロール・灰色の直前テンポ表示20ケースとクリック選択13ケースで、画像66枚も一致しました。プロパティページ管理の生成・対象切替・解除に加え、実ページの生成、境界を含む18入力、スピン通知5ケース、キャンセル通知を比較しました。ページから実モデルを編集する5ケースと、OLEコールバック120条件・左ボタンドロップ17ケースを比較しました。ドロップは既存選択・コピー内容・保存・再読込・ランタイム同期まで一致しました。DoDragDropの境界を試験中に差し替えた12ケースでは、開始・中止・転送ストリーム・元イベント削除・同一ストリップへのドロップも一致しました。右メニューを閉じた7ケースと、Windowsの実OLEループを中止する2ケースも一致しました。WM_COMMAND の削除・挿入・全選択・プロパティ・未知コマンドなど11ケースも比較済みです。コピー用データは原版が初期化しないレコード内の8バイトだけを正規化して比較しています。[代替 DLL の比較と未対応機能](docs/analysis/tempo-comparison.md) を記録しています。保存した35ソースだけから全7ターゲットの構成・ビルドも確認しました。今回の新規生成物は Smart App Control による起動拒否を記録しており、6887件の成功比較には既存のビルドを使用しました。[本体の統合試験](docs/analysis/tempo-integration.md) では、限定した一時登録後に原版が起動し、QuickStart のテンポ112→137 BPM変更・保存・Undo・Redo・プロジェクト再読込を確認しました。候補 DLL でも同じテンポ変更・保存・Undo・Redo・プロジェクト再読込と、終了・再起動後の復元を確認しました。原版DLLへの復元も確認済みです。保存結果のテンポpayloadは一致しますが、別トラックのコード用データ5箇所の差分は未解明です。システムクリップボード、実OLEの成功転送・ドラッグ画像・右メニューのコマンド選択、複数文書の独立性、音声出力と全体再構築の検証は未完了です。

## 現在の状態

起動時ダイアログは公式署名付きAutoItで撮影・自動OKに成功し、続く原版編集画面も確認しました。実際のOKボタンはID2で、DPI倍率を考慮した撮影と実ボタンへのControlClickを使用します。以前のC++補助EXEの実行拒否は履歴として保持しています。[確認済みの手順と記録](docs/analysis/producer-startup-dialog.md) を残しました。

次モジュールの [TimeSigStripMgrの仕様調査](docs/analysis/time-signature-strip-manager.md) では、サービス参照管理に加え、試験用Timelineへの接続・切断とStripの基本プロパティを実装しました。保存17ソースからビルドした候補630bは、接続11ケース・160観測と既存322観測・27入出力ファイルが原版と正規化なしで一致しました。接続比較器の7テストも成功しています。実Timeline・Style拍子データの取込み・描画・拍子編集・通知配送・ランタイム同期・本体置換は継続します。

追加の実Timelineプローブは保存19ソース・5ターゲットからビルドしましたが、修正後のEXEはWindowsのアプリケーション制御ポリシーで起動を拒否されました。初回の不完全な合成Style条件での保存結果は互換基準から除外し、修正後のStyle取込み・候補比較は未完了として記録しています。SDK照合により、以前テンポ管理対象と記した `d2ac28a1` はStyle識別子と訂正しました。既に成功した自動OK処理には公式AutoItを使用できます。

共通CMakeもTimeSigを含む構成へ更新し、保存48ファイルだけから全11ターゲットをビルドしました（`work/build/recovery-snapshot/20261002T115108613Z/build-summary.json`）。`scripts/Build-RecoverySnapshot.ps1` で再現できます。共通ビルドの生成物は今回実行しておらず、上記TimeSig比較は独立した17ソースのビルドを使用しました。11ターゲットには比較用プローブと静的ライブラリを含み、互換ソースのあるProducerモジュールは依然としてTempo／TimeSigの限定経路だけです。Producer全体の再構築は未完了です。

続く現行構成はStyleプローブを加え、保存49ファイルから全12ターゲットをビルドしました（`work/build/recovery-snapshot/20261002T120616172Z/build-summary.json`）。TimeSig単独の保存19ソース・5ターゲットもビルド成功、検証器の17テストが成功しています。これらの新規生成物は実行しておらず、既存630bの原版比較結果とは区別します。

クリップボードの追加進捗：原版と候補690のUIでコピー・貼付け・切り取り・再貼付けを保存結果まで確認しました。候補のUndo・Redo、プロジェクト再読込、別プロセスでの再起動読込と、原版による候補保存物の読込も確認しました。限定4ケースの機能比較は一致しています。原版TimelineではExport4回後にアンロード用カウンター4が残るため、システムクリップボードを含む全体試験は失敗記録を保持します。

候補690の先頭選択でプロパティに3小節目が残る不具合は、原版の選択後RefreshData呼出しを復元して修正しました。新候補00c3は実ページの選択更新10段階を含む6,914観測・445通常ファイル・123コピー用データ・66画像で原版と一致し、本体でも3→1小節目の切替とF11再表示を確認しました。この新しい比較はシステムクリップボードを含みません。新候補のクリップボードUI・終了再起動・音声、実ドラッグ・複数文書の受入は継続中です。試験終了後の原版DLLへの復元も照合済みです。

続く新候補00c3の本体試験では、公式AutoItで起動警告の自動OK・Open Project・ツリー操作を進め、コピー・切り取り・再貼付けの保存とUndo／Redoのファイル全体の一致を確認しました。Producerと同じ画面ユーザー環境でAutoItを実行する条件も記録しています。今回の新しい保存データの再起動・原版読込・候補音声は検証待ちです。画面ツリーの保存に古い変数束縛の問題があったため、今回のプロパティの証拠から除外しました。試験後は本体を終了し、原版DLLへ復元しました。

2026-10-02（日本時間）、DirectX 9 用 DirectMusic Producer の配布物を取得し、本体を展開しました。

| 項目 | 保存先 / 確認結果 |
| --- | --- |
| 元の配布物 | `artifacts/downloads/dx90_directmusicproducer.exe` |
| 配布物のサイズ | 33,139,712 bytes |
| 本体 | `artifacts/extracted/producer/Release_Program_Executable_Files/DMUSProd.exe` |
| 本体バージョン | 5.3.0.900（リソース表示: `5.3.0000000.900 built by: DIRECTX`） |
| 編集コンポーネント | `artifacts/extracted/producer/Release_Program_DLLs/`（33ファイル） |
| ヘルプ | `artifacts/extracted/producer/Help_Files/dmusprod.chm` |
| リリース説明 | `artifacts/extracted/producer/Help_Files/dmusprod.txt` |
| インストーラー一式 | `artifacts/extracted/dx90/Essentials/DirectMusic Producer/` |
| Style Library | `artifacts/extracted/dx90/Essentials/DirectMusic Style Library/` |

配布物の SHA-256:

```text
087b43e9efc43915fdbd3352c1144a69a7f3a728e088b187f823b6b293839d6f
```

別々のミラーからダウンロードした2ファイルのサイズとハッシュが一致しました。元の配布物と本体の Authenticode 検査結果は `NotSigned` です。ミラー間の一致は確認できましたが、Microsoft の署名による真正性や無害性を証明するものではありません。

正式インストーラーの動作は未検証です。限定した一時登録による原版起動・編集と、原版DLL群による候補保存物の再生・ユーザーの聴取確認は上記の統合試験に記録しています。候補DLLロード時の音声受入は未完了です。現状の展開先は InstallShield のファイルグループ別配置であり、そのまま使えるポータブル版ではありません。

Producer 本体のソースコードはこの配布物からは見つかっていません。`FarmGameAppSrc/` にある C++ / Visual C++ 6 プロジェクトはチュートリアルのサンプルゲーム用です。再ビルドに向けた本体ソースの調査は次の段階になります。

## 解析用の作業コピー

解析の入口は `work/producer/app/DMUSProd.exe` です。2026-10-02 に展開済みファイルを次の配置へコピーし、全332ファイルの SHA-256 がコピー元と一致することを確認しました。

| 作業場所 | 内容 |
| --- | --- |
| `work/producer/app/` | 本体、編集 DLL/OCX、同梱 MFC/CRT、FlexGrid、INI、ヘルプ（42ファイル） |
| `work/producer/resources/fonts/` | 同梱の MusicSym フォント |
| `work/producer/docs/` | Word チュートリアル |
| `work/producer/samples/` | QuickStart、FarmGame（ソースを含む）、チュートリアル素材 |
| `work/producer/style-library/` | Style Library と StylePlayer（216ファイル） |
| `work/producer/manifest.json` | コピー元・コピー先・サイズ・コピー時の SHA-256 |

元の `artifacts/` は保管用、`work/` は解析や変更用です。`work/` も Git の追跡対象外です。この配置は解析用で、インストール先の完全な再現や起動を保証するものではありません。COM 登録、フォントのインストール、ランタイムの導入は行っていません。

## 再取得・展開

PowerShell と `curl.exe`、7-Zip が必要です。リポジトリのルートで実行します。

```powershell
.\scripts\Get-DirectMusicProducer.ps1 -SevenZip '7z.exe' -VerifySecondMirror
```

既存ファイルがあればハッシュを確認して再利用します。第一ミラーからの新規取得が失敗した場合は第二ミラーを試します。異なるハッシュのファイルは展開しません。

本体の InstallShield CAB まで展開する場合は、unshield を指定します。この作業環境では次のコマンドで再展開を確認済みです。

```powershell
.\scripts\Get-DirectMusicProducer.ps1 `
  -SevenZip 'C:\Users\dolph\Tools\7-Zip\7z.exe' `
  -Unshield '.\artifacts\tools\unshield-build\src\Release\unshield.exe' `
  -VerifySecondMirror
```

元のインストーラー EXE は実行せず、書庫として展開します。[取得・検証記録](docs/acquisition-2026-10-02.md)に入手元、展開ツールのビルド手順と確認範囲を記載しています。

`artifacts/` にはダウンロード物・展開物・ツールをローカル保存し、Git の追跡からは除外しています。Git clone だけでは取得済みバイナリは復元されないため、長期保全にはこのディレクトリも別途バックアップしてください。取得物のライセンスは元の権利者に帰属します。

本体の起動に必要な登録を調べるため、プロセス内だけで参照先を専用ファイルハイブへ切り替える [登録観測](docs/analysis/registration-capture.md) を追加しました。33モジュール中30件で登録内容を取得し、10 Components と43 COMクラスを確認しました。3つの補助コントロールは登録に失敗しており、実環境へのインストールと本体上での編集試験は未完了です。[全体の検証状態](docs/analysis/implementation-status.md) を40モジュールの台帳で管理します。


最新のBand/DLS作業単位は [本体の引継ぎ](docs/analysis/product-host.md) と [機械可読状態](docs/analysis/product-state.json)。現行195748806Zは保存48sources/3targetsの構成・compile・install成功、警告なし、native345件とDLS再生API成功。DlsDocumentの楽器locale/Region範囲・wave cue/8・16bit PCM音量編集と全bytes履歴・保存・Framework所有/dirty/project reloadを接続。試験曲72..84が原版DLS Region72..111に入ることを独立監査。headless24/DLS55原版40hash一致0。DLS編集GUI未接続、実音声回答待ち、現行GUI/通常/Style再生と全八受入は未完了。

現在の限定本体201437359ZではDLS文書の楽器・Region・PCM音量を本体GUIから編集し、保存/UndoRedo/別起動復元を全bytesで確認しました。保存50sources、native345件、DLS再生API成功。編集音源のGUI再生・実音声・全40責務と全八受入は未完了。[版別の記録](docs/analysis/product-host.md)、[現行状態](docs/analysis/product-state.json)。

現行限定本体214943975ZはPCM WAV入出力を接続し、保存50sourcesの構成/build/install、395件、Framework保存復元、製品読込み生成DLSの再生API成功。今回GUI WAV書出し/読込み/UndoRedo/保存別起動再書出し再保存全bytes・Play自然終了/GUI132由来確認。音声回答待ち。前版証拠を分離保持。可変長PCM/loop・全40責務/全八受入は未完了。詳細はdocs/analysis/product-host.md末尾。

現行限定本体221251264Zは可変長PCM/loop境界/alias cue再配置を文書所有へ接続。保存50sourcesの構成/build/install、native410件、Framework保存復元・製品生成DLS API成功。同版GUI長変更import/UndoRedo/保存別起動再書出し再保存全bytes・GUI109由来passed。今回GUI Play/音声・原版比較、空Region/Wave削除/loop編集/articulation/groupと全40責務/全八受入は未完了。[最新引継ぎ](docs/analysis/product-host.md)。

現行限定本体223434780Zは空Region新規作成を本体/Frameworkへ接続。構成/build/install・native425件・製品生成Region再生API、同版GUI Create/UndoRedo/保存別起動復元再保存全bytes・GUI104由来passed。capture metadata復旧制限あり。DLS別名保存のBand参照問題、GUI Play/音声・原版比較・全40責務/全八受入未完了。[最新引継ぎ](docs/analysis/product-host.md)。

現行限定本体230152798ZはDLS/Band別名保存と所有Band/Style/Segment参照更新を接続。構成/build/install・native449件・同run別名DLS再生API、同版GUI別名保存/依存文書保存/別起動復元/DLS再保存全bytes・GUI104由来passed。Wave削除/loop、Style/Segment自身SaveAs/履歴、GUI Play/音声・原版比較・全40責務/全八受入未完了。[最新引継ぎ](docs/analysis/product-host.md)。

現行限定本体232131882ZはWave参照付き削除/明示置換とalias cue/Region再配置をFramework保存復元へ接続。構成/build/install・native471件・同run削除生成DLS再生API・native24/55由来passed。現行GUI/音声未確認、全40責務/全八受入未完了。[最新引継ぎ](docs/analysis/product-host.md)。

現行232641307ZはWave削除GUI/置換cue指定を実装。構成/build/install・native471件・同run削除DLS API成功。試験用コピーのGUI削除確認を表示、Yes未送信/保存再起動未実行。native24/55・確認前GUI104由来passed。全40/全八受入未完了。

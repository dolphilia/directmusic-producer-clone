# DirectMusic Producer recovery

DirectMusic Producer の復旧と、将来的な再ビルドを目指すリポジトリです。

解析は [DirectMusic Producer 解析と再構築の実施計画](docs/analysis-plan.md) に沿って進めます。本体の構造把握から小さな DLL の仕様復元・再実装・比較検証へ進む順序と、各工程の完了条件を定めています。

工程0の [解析基準と再現手順](docs/analysis/baseline.md) を整備しました。[本体の接続調査](docs/analysis/host-map.md) と [TempoStripMgr の仕様復元](docs/analysis/tempo-strip-manager.md) を進めています。代替 DLL をソースからビルドし、限定 Framework、原版 Timeline・TimeSigStripMgr、実ランタイムへ接続しました。通常ストリーム、境界・複数選択の編集、拍子変更通知、通知登録・解除、COM 経由の追加・コピー・切り取り・貼付け、カーソル位置でのテンポ変更通知、範囲選択、Undo 用操作名取得、保存・同期・解放を含む比較で、原版の6887件の観測結果、通常の入出力443ファイル、コピー用データ123ファイルの内容が一致しました。非表示の原版 Timeline ウィンドウを接続し、横スクロール・灰色の直前テンポ表示20ケースとクリック選択13ケースで、画像66枚も一致しました。プロパティページ管理の生成・対象切替・解除に加え、実ページの生成、境界を含む18入力、スピン通知5ケース、キャンセル通知を比較しました。ページから実モデルを編集する5ケースと、OLEコールバック120条件・左ボタンドロップ17ケースを比較しました。ドロップは既存選択・コピー内容・保存・再読込・ランタイム同期まで一致しました。DoDragDropの境界を試験中に差し替えた12ケースでは、開始・中止・転送ストリーム・元イベント削除・同一ストリップへのドロップも一致しました。右メニューを閉じた7ケースと、Windowsの実OLEループを中止する2ケースも一致しました。WM_COMMAND の削除・挿入・全選択・プロパティ・未知コマンドなど11ケースも比較済みです。コピー用データは原版が初期化しないレコード内の8バイトだけを正規化して比較しています。[代替 DLL の比較と未対応機能](docs/analysis/tempo-comparison.md) を記録しています。保存した35ソースだけから全7ターゲットの構成・ビルドも確認しました。今回の新規生成物は Smart App Control による起動拒否を記録しており、6887件の成功比較には既存のビルドを使用しました。[本体の統合試験](docs/analysis/tempo-integration.md) では、限定した一時登録後に原版が起動し、QuickStart のテンポ112→137 BPM変更・保存・Undo・Redo・プロジェクト再読込を確認しました。候補 DLL でも同じテンポ変更・保存・Undo・Redo・プロジェクト再読込と、終了・再起動後の復元を確認しました。原版DLLへの復元も確認済みです。保存結果のテンポpayloadは一致しますが、別トラックのコード用データ5箇所の差分は未解明です。システムクリップボード、実OLEの成功転送・ドラッグ画像・右メニューのコマンド選択、複数文書の独立性、音声出力と全体再構築の検証は未完了です。

## 現在の状態

クリップボードの追加進捗：原版UIでコピー・貼付け・切り取り・再貼付けを保存結果まで確認し、候補DLLへシステムクリップボード経路を実装しました。限定4ケースの機能比較は一致しています。原版・候補ともTimelineの参照保持が未解決なので、全体試験の合格数は更新していません。候補の本体クリップボードUI試験は起動警告待ちです。

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

正式インストーラーの動作と音楽再生は未検証です。限定した一時登録による原版起動・編集の実測は上記の統合試験に記録しています。現状の展開先は InstallShield のファイルグループ別配置であり、そのまま使えるポータブル版ではありません。

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

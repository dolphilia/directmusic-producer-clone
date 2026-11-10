# DirectMusic Producer 取得記録

取得日: 2026-10-02（Asia/Tokyo）。HTTP ヘッダーは UTC のため 2026-10-01 と表示されています。

## 入手元

| URL | 実際の確認結果 |
| --- | --- |
| `https://download.microsoft.com/download/c/d/d/cdd61e5e-dd4d-4c5f-8f8f-d2b0edb61746/dx90_directmusicproducer.exe` | HTTP 404 |
| `https://w2krepo.somnolescent.net/Audio/dx90_directmusicproducer.exe` | HTTP 200、33,139,712 bytes を取得 |
| `https://files.rajko.info/dx90_directmusicproducer.exe` | HTTP 200、33,139,712 bytes を取得、第一ミラーと SHA-256 一致 |

発見に使った公開ページ:

- [w2krepo 配布ディレクトリ](https://w2krepo.somnolescent.net/Audio/)
- [配布者 rajkosto の Music Tools 投稿](https://mxoemu.info/forum/showthread.php?tid=1967)
- [VGMPF の DirectMusic Producer ページ](https://www.vgmpf.com/Wiki/index.php?title=DirectMusic_Producer)（検索結果から候補を発見。ページ本文の直接取得は失敗）

ミラー2の保存名は `artifacts/downloads/dx90_directmusicproducer.rajko.exe`。HTTP 応答ヘッダーは `artifacts/evidence/somnolescent-headers.txt` と `artifacts/evidence/rajko-headers.txt` に保存しました。

## 静的に確認できた内容

- 外側は x86 の Microsoft Cabinet Self-Extractor。中に `Essentials.exe` が1ファイル。
- `Essentials.exe` は埋め込み ZIP を持つ自己解凍形式。Producer の InstallShield セットアップ、Style Library、デモ配布物を含む。
- `data1.cab` は通常の Microsoft CAB ではなく InstallShield CAB。7-Zip 26.02 では開けず、unshield 1.6.2 で116ファイルを展開できた。
- `DMUSProd.exe` は 518,144 bytes。ファイルリソースは Microsoft Corporation / DirectMusic Producer Application / `5.3.0000000.900 built by: DIRECTX`。
- 本体 SHA-256: `fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011`。
- `Release_Program_DLLs/` に33ファイル。StyleDesigner、SegmentDesigner、DLSDesigner、BandEditor、ChordMapDesigner、ScriptDesigner、AudioPathDesigner、ToolGraphDesigner、各 StripMgr など。
- CHM ヘルプ、`dmusprod.txt`、Word チュートリアル、QuickStart、FarmGame のソースとプロジェクトを含む。
- 配布物の README と `dmusprod.txt` は DirectX 9 release と明記する。ミラー紹介の「9.0b」という呼称だけで細かいリリース名を確定せず、本体のファイルバージョンを記録した。

署名検査: 外側の配布物、本体とも `Get-AuthenticodeSignature` は `NotSigned`。バージョンリソースはファイル内の自己申告情報であり、署名検証の代わりにはならない。インストーラー・本体は実行しておらず、起動、COM 登録、DirectMusic ランタイム互換性、再生は未検証。ウイルススキャンはこの作業では実施していない。

## unshield のビルド

使用ツール: 7-Zip 26.02、CMake 4.4.2、Visual Studio 2022 / MSVC 19.44.35228、Windows SDK 10.0.26100.0。

公開ソースを `artifacts/tools/` に保存してローカルビルドした。これは展開ツールのビルドであり、Producer の再ビルドではない。

| ソース | URL | SHA-256 |
| --- | --- | --- |
| unshield 1.6.2 | `https://github.com/twogood/unshield/archive/refs/tags/1.6.2.tar.gz` | `a937ef596ad94d16e7ed2c8553ad7be305798dcdcfd65ae60210b1e54ab51a2f` |
| zlib 1.3.2 | `https://zlib.net/zlib-1.3.2.tar.gz` | `bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16` |

zlib のハッシュは [公式サイト](https://zlib.net/) 掲載値とも一致。[unshield upstream](https://github.com/twogood/unshield) は InstallShield 5 以降の書庫に対応している。

取得した2つの tar.gz を `artifacts/tools/` に展開した後、以下でビルドできる。各コマンドが成功したことを確認してから次へ進む。

```powershell
cmake -S artifacts/tools/zlib-1.3.2 -B artifacts/tools/zlib-build `
  -G 'Visual Studio 17 2022' -A x64 `
  -DZLIB_BUILD_TESTING=OFF -DZLIB_BUILD_SHARED=OFF
cmake --build artifacts/tools/zlib-build --config Release --parallel 4
$zlibPrefix = Join-Path (Get-Location).Path 'artifacts/tools/zlib-install'
cmake --install artifacts/tools/zlib-build --config Release --prefix $zlibPrefix
cmake -S artifacts/tools/unshield-1.6.2 -B artifacts/tools/unshield-build `
  -G 'Visual Studio 17 2022' -A x64 `
  -DBUILD_STATIC=ON -DUSE_OUR_OWN_MD5=ON -DBUILD_TESTING=OFF `
  "-DZLIB_LIBRARY=$zlibPrefix/lib/zs.lib" "-DZLIB_INCLUDE_DIR=$zlibPrefix/include"
cmake --build artifacts/tools/unshield-build --config Release --parallel 4
```

zlib 1.3.2 の静的ライブラリ名は `zs.lib`。自動検出では見つからなかったため明示した。初回はサンドボックスにより SDK 参照が拒否されたが、アクセス許可を伴う再実行でビルドは成功した。

## 検証と保存範囲

- 2つの配布物の全体 SHA-256 とサイズを比較。
- 7-Zip による二段階展開が成功。
- 7-Zip の `t` による外側 CAB・内側 ZIP の検査が成功（終了コード0）。内側の自己解凍 EXE には ZIP 終端後の2,292 bytesについて `There are data after the end of archive` の警告があり、記録を保持した。元ファイルは変更していない。
- unshield による116ファイルの展開が成功。
- 再取得スクリプトを取得済みファイルで実行し、ハッシュ照合と全段階の再展開が成功。ネットワークからの初回取得は curl で実施済み。スクリプトの通信失敗時フォールバックは未検証。
- ファイル別のサイズ・SHA-256 は `acquisition-manifest.json` に記録。
- Producer 本体のソースは確認できていない。サンプルゲームのソースとの混同を避けること。

元の配布物にはデモコンテンツ用の別の自己解凍 EXE も含まれる。これは元の形で保全しており、今回の本体確認のためには追加展開していない。

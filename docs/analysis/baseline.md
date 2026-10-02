# 解析基準と再現手順

更新日：2026-10-02（日本時間）。工程0は完了。全体の復旧・再構築は未完了。

## 固定した入力

`work/producer/manifest.json` の332ファイルをサイズ・SHA-256で照合した。そのうち PE ファイルは40個で、すべて x86。識別情報と件数は [modules.csv](modules.csv) に保存した。本体と33個の編集モジュールはファイルバージョン 5.3.0.900。その他はアンインストール DLL、MFC/CRT、FlexGrid、サンプル実行ファイル等である。

解析基準となる SHA-256：

| ファイル | SHA-256 |
| --- | --- |
| DMUSProd.exe | `fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011` |
| TempoStripMgr.dll | `bb9811c74f68dcf0b37d32fe2ae89d3e45962e59b95f1ec93ddf7a12635a5c95` |
| SegmentDesigner.ocx | `541e5dd87c62cb0b449c571aba4e7ac863b093877ded7793eea89e4c6877c1bb` |
| Timeline.dll | `bebc8149e31f3b4b0b74bffbf482acc38ba8b5b28a1468c9741e71961c6db176` |

## 静的解析の再実行

リポジトリのルートで実行する。Node.js、LLVM の `llvm-readobj.exe` / `llvm-objdump.exe`、7-Zip が必要。

```powershell
.\scripts\Collect-AnalysisBaseline.ps1 -SevenZip 'C:\Users\dolph\Tools\7-Zip\7z.exe'
```

実施時のツールは Node.js 24.18.1、LLVM 19.1.7、7-Zip 26.02。スクリプトは入力のハッシュが変わっていれば抽出を中止する。調査対象の EXE/DLL は実行しない。

| 出力 | 内容 |
| --- | --- |
| `docs/analysis/modules.csv` | 40ファイルのバージョン、アーキテクチャ、ハッシュ、各種件数 |
| `docs/analysis/com-classes.csv` | REGISTRY リソース内で確認した20件のクラス登録記述。全 COM クラスの網羅表ではない |
| `work/analysis/pe/collection.json` | 入力照合件数、ツール版、抽出スクリプトのハッシュ、ツールで処理できなかった項目 |
| `work/analysis/pe/app__TempoStripMgr.dll/` 等 | `pe.json`、文字列、個別リソース、LLVM 出力、RTTI 候補 |
| `work/analysis/pe/*/disassembly.txt` | 本体、TempoStripMgr、SegmentDesigner、Timeline の逆アセンブル |
| `work/analysis/help/` | CHM から展開した863ファイル |

文字列の一覧は ASCII と UTF-16LE の ASCII 範囲の連続文字が対象。STRINGTABLE リソースは別途、長さフィールドに従って UTF-16LE で復号している。非 ASCII の任意の文字列をすべて拾うものではない。

RTTI の結果はクラス・基底クラス・サブオブジェクト位置・仮想関数テーブルの候補である。旧バイナリでは実行属性のある `.text` にデータも含まれるため、命令として表示された全領域をコードと見なしてはいけない。仮想関数の名前、正確な個数、型は別途確認する。

## 検証結果と例外

- 同じラッパースクリプトを再実行し、332入力の照合、40 PE の抽出、CHM の展開、4モジュールの逆アセンブルが成功した。記録は `work/analysis/baseline-rerun.log`。
- 独自の PE 読み取り結果について、通常インポートの DLL 数と有効エクスポート数を LLVM の出力と照合した。MFC42 のインポート照合には後述の例外がある。
- バージョンリソースがある38ファイルは、Windows の `FileVersionInfo` が返す数値と一致した。残り2ファイルは固定バージョンを取得できていない。
- `MFC42.DLL` の遅延インポートについて LLVM が `RVA 0x5f4aefa8 ... not found` で失敗する。終了コードとエラーを `collection.json` に保存している。通常インポートは独自読み取り結果があるが、遅延インポートは未解決で、完全に照合済みとは扱わない。
- PDB のパスを示す CodeView 情報を抽出できるが、PDB 本体を入手できたという意味ではない。現時点の詳細調査はリソース、RTTI、逆アセンブルを組み合わせている。

## 同梱ヘルプから得た操作と形式の手掛かり

以下はヘルプの記述であり、UI の実動作を確認した結果ではない。原文は `work/analysis/help/htm/` にある。

| 原文ファイル | 比較試験へ取り込む事項 |
| --- | --- |
| `creatinganewproject.htm` | 編集対象はプロジェクトに属する。起動後の基準操作はプロジェクト作成から始める |
| `lesson2importingamidifileintoasegment.htm` | MIDI をセグメントとして取り込む経路を調べる |
| `lesson3savingthesegment.htm` | 編集用ファイルとランタイム用ファイルを区別する。どちらの保存を試したかを記録する |
| `lesson4playingthesegment.htm` | 一回再生、ループ再生、再生開始位置を別項目にする |
| `changethevalueofthefirsttempomark.htm` | テンポの Properties、F11、UI 入力範囲として10〜350 BPMの説明がある |
| `insertanewtempomark.htm` | テンポトラックを選択して Insert、Properties で値を変更する |
| `tempotrack.htm` | 既定テンポ120 BPM、主セグメントと副セグメントの扱い、複数テンポトラックの優先順位が説明されている |

`lesson6changingthetempo.htm` には1小節につき1マークという説明がある一方、`tempotrack.htm` は任意数のテンポマークを説明している。正確な配置制限は原版で確認するまで未確定とする。UI 上の入力範囲を、読込形式の許容範囲と同一視しない。

## SDK と追加資料の探索記録

SDK のランタイム API 宣言は [sdk-reference-sources.json](sdk-reference-sources.json) の3ファイルを取得し、[接続調査](host-map.md) に照合結果を記録した。Producer 固有のヘッダーや本体ソースを取得したものではない。

以前の推測パスへの404だけで候補全体を除外しないため、2026-10-02に次の公開候補のディレクトリ一覧を取得した。取得したものはパスと Git オブジェクト ID の一覧で、これらの候補からソースファイル自体は取得していない。

| 対象の範囲 | Git tree SHA | 件数・結果 | ローカル証拠 |
| --- | --- | --- | --- |
| `tongzx/nt5src` の `Source/XPSP1/NT/multimedia/directx` | `6be46eff64ba8a30accd160bf406313d0d9ccad6` | 8,268件、truncated=false | `work/analysis/sources/nt5-directx-tree.json` |
| `selfrender/Windows-Server-2003` の `multimedia/directx` | `dac142cd15b719ed801fe03cd84cfbd9b84b4604` | 6,005件、truncated=false | `work/analysis/sources/ws2003-directx-tree.json` |

どちらもパス名の `producer` / `tempostrip` / `dmusprod` / `stripmgr` 検索では一致しなかった。この結果は上記 DirectX 配下に対するものに限り、リポジトリ全体や別配布物にないことの証明とはしない。

## 次に行う作業

本体と編集モジュールの接続調査を [host-map.md](host-map.md) に蓄積する。Framework と実ランタイムの接続により、TempoStripMgr の初期ストリームの読込・再保存と同期を確認できた。次は Timeline と編集操作の経路、追加ストリーム試験、実行環境のアプリケーション制御への対応である。詳細は [tempo-strip-manager.md](tempo-strip-manager.md) と [reference-environment.md](reference-environment.md) を参照。

# 配置変更の実施記録

実施日：2026-11-10（日本時間）。対象：ユーザーが採用した[リポジトリ構成](../architecture/repository-layout.md)への配置変更。

## 変更内容

| 移動前 | 現在の配置 |
| --- | --- |
| `src/` | `legacy/windows/src/` |
| `tests/` | `legacy/windows/tests/` |
| `scripts/` | `legacy/windows/scripts/` |
| `Q1DlsOracle/` | `legacy/windows/Q1DlsOracle/` |
| 旧ルートCMake／preset | `legacy/windows/CMakeLists.txt`／`CMakePresets.json` |
| 旧解析資料・計画・状態台帳・配布説明 | `legacy/windows/docs/` |
| Windows版READMEの履歴本文 | `legacy/windows/README.md` |
| 技術スタック調査 | `docs/reference/macos-technology-strategy-2026-11-10.md` |
| 構成提案書 | `docs/proposals/repository-structure-proposal-2026-11-10.md` |

新しく、`apps/desktop/`、4つの`modules/`、新しい`tests/`、`tools/`、`third_party/`、`cmake/`の責務をREADMEで明示した。製品ソースのない下位ディレクトリや仮の製品ターゲットは追加していない。

ルートREADMEを現在の開発入口へ整理し、`AGENTS.md`、文書索引、構成の正本、採用記録、ビルド手順、旧資料索引を追加した。実施計画書は作成していない。

ルートのCMakeとpresetは新製品用の構成確認の骨組みへ置き換えた。旧WindowsのCMakeとpresetは内容を変えずに移した。`.gitignore`には新しい生成先、個人設定、macOSメタデータ、入れ子になった旧作業先を追加した。

## 内容の保全確認

移動前の作業ツリーから、相対パス、SHA-256、Markdownの相対リンクの存在状態を採取した。対象は追跡ファイル1,030件と、移動対象ディレクトリにあった未追跡の`src/.DS_Store`1件の計1,031件。既に行っていた文書変更を含む、その時点の作業内容を基準にした。

| 確認 | 結果 |
| --- | --- |
| 移動した1,031件の所在 | 全件確認 |
| 内容がバイト単位で同一 | 1,030件。うち追跡ファイル1,029件 |
| 意図した内容変更 | 追跡ファイル1件：旧`docs/analysis-plan.md`の新文書への案内リンクだけを更新。リンクを戻した内容のSHA-256は移動前と一致 |
| 予期しない内容変更 | 0件 |
| 移動前に存在していた相対リンク286件 | リンク先を保持。上記の案内リンク1件は新しい配置へ付け替え |
| Windows版READMEの履歴本文 | 移動前の本文と一致。旧作業ルートの説明を別途追記 |

旧文書に元から存在する、Git対象外の素材・生成物へのリンクは未解決のまま保持した。過去の証拠JSON、CSV、ソース、試験、スクリプトの内容は変更していない。

個別の監査データはGit対象外の`local/evidence/repository-layout-2026-11-10/`へ保存した。

- `baseline.json` SHA-256：`6199b3b03cdf9ce2d3ffc8938fa3b0ac76cf667ea8e5e4fa27fbd472195765c0`
- `relocation-result.json` SHA-256：`197404120db9c62b2cd34e03a230939d2b255c7e81c28fb54e6535c70cdd29fc`
- 移動前のルートREADMEも同じ場所に保管した。

このローカル監査データは通常のcloneには含まれない。追跡する変更の説明と結果は本書に残す。

## 構成の確認

Apple Silicon Mac、Apple Clang 17.0.0、CMake 4.0.1、Ninja 1.13.1で確認した。

```sh
cmake --list-presets=all
cmake --preset macos-debug
cmake --build --preset macos-debug
cmake --preset macos-release
cmake --build --preset macos-release
```

Debug／Releaseともにconfigure・generate・buildがexit 0。現在は製品ターゲットがないため、buildの結果は`ninja: no work to do.`である。新しいビルドグラフに旧Windows環境やRCコンパイラが入っていないことを確認した。

`legacy/windows/`内で`cmake --list-presets=all`を実行し、旧`product-win32`、`product-release`、`product-core`が読み取れることを確認した。Windowsでの構成・コンパイル・試験を再実行した結果ではない。

新しい配置のMarkdownの相対リンクと末尾空白を検査し、旧資料については移動前に存在したリンクが失われていないことを比較した。新旧の生成先、`.DS_Store`、`CMakeUserPresets.json`について7種類のパスがignoreされることも確認した。

## 今回の範囲

完了したのは配置変更、開発入口・文書の整備、内容保全と構成の確認。JUCEの取得、音楽モデルの抽出、製品アプリ、新製品の実行可能な試験は今後の実装で扱う。

旧試験の一部はGit対象外の原版観測データを必要とし、このcheckoutには素材がない。Windowsの実行・GUI・音声を含め、旧試験の成功を今回のMac版の成功として転用していない。

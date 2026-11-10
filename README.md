# DirectMusic Producer-inspired music authoring

DirectMusic Producerの解析・再構築で得た知見とC++資産をもとに、macOSで使えるMIDIシーケンサー／音楽オーサリングツールへ発展させるプロジェクトです。Pattern、和声、Variation、音楽の遷移という思想を受け継ぎ、現代の制作環境として開発します。

採用スタックは**C++20＋JUCE 9＋CMake**。macOSではCore Audio／Core MIDIを利用します。完全再現は目標にしません。

## 現在の状態

2026-11-10、新しい開発構成への配置変更を実施しました。旧Windows環境を`legacy/windows/`へ移し、新製品の置き場所とCMakeの入口を用意しています。

新製品のソース、JUCE依存、アプリの`.app`、実行可能な新製品の試験はまだありません。ルートのCMakeは配置確認用の骨組みです。機能の移植と実施計画書の作成は後の工程で行います。

## 開発の入口

- [文書索引](docs/README.md)
- [リポジトリ構成と依存方向](docs/architecture/repository-layout.md)
- [技術スタックの採用](docs/decisions/0001-technology-stack.md)
- [開発環境・ビルド](docs/development/building.md)
- [配置変更の記録](docs/development/repository-layout-migration-2026-11-10.md)
- [構成提案と採用理由](docs/proposals/repository-structure-proposal-2026-11-10.md)

macOSのリポジトリルートで構成を確認できます。現在は製品ターゲットがないため、buildを実行してもアプリは生成しません。

```sh
cmake --preset macos-debug
cmake --build --preset macos-debug
```

## 主な配置

| 配置 | 内容 |
| --- | --- |
| `apps/desktop/` | JUCEアプリと編集画面 |
| `modules/` | music、engine、project、formatsの4責務 |
| `tests/`、`tools/` | 新製品の試験・補助処理 |
| `docs/` | 現行設計、採用判断、開発手順 |
| `legacy/windows/` | 旧Windowsソース・試験・スクリプト・解析記録 |
| `build/`、`local/` | Git対象外の生成物・開発用データ |

## Windows再構築の記録

[旧README](legacy/windows/README.md)と[旧資料索引](docs/reference/windows-recovery.md)を参照してください。旧コマンドの作業ルートは`legacy/windows/`です。旧資料・証拠の内部配置を保っていますが、Git対象外だった原版素材や録音は同梱していません。

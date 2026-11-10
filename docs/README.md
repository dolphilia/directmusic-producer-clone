# 文書の入口

現在の開発は、DirectMusic Producerの知見を活かしたmacOS向け音楽オーサリングツールを対象とする。

| 文書 | 内容 |
| --- | --- |
| [リポジトリ構成](architecture/repository-layout.md) | 現在の配置、責務、依存方向 |
| [技術スタックの採用](decisions/0001-technology-stack.md) | C++20＋JUCE 9＋CMakeを採用した判断 |
| [構成案の採用](decisions/0002-repository-layout.md) | 新旧環境の分離と配置の判断 |
| [開発環境・ビルド](development/building.md) | 現在実行できるコマンドと未実装範囲 |
| [配置変更の記録](development/repository-layout-migration-2026-11-10.md) | 移動、内容保全、リンクと構成の確認結果 |
| [技術スタック調査](reference/macos-technology-strategy-2026-11-10.md) | 採用前の比較、公式資料、移植の論点 |
| [構成提案書](proposals/repository-structure-proposal-2026-11-10.md) | 今回採用した構成の背景と到達形 |
| [Windows資料索引](reference/windows-recovery.md) | 旧ソース・契約・検証記録への案内 |
| [実施計画の置き場所](plans/README.md) | 後の工程で作成する計画書 |

現行設計は`architecture/`、採用理由は`decisions/`、操作・検証手順は`development/`へ置く。旧資料内の「次の作業」は、現在の実施指示ではない。

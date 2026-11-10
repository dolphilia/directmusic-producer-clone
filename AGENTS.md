# 開発時の共通ルール

- 新製品はC++20＋JUCE 9＋CMakeを採用する。現在は配置整理を終えた段階で、JUCEの導入と製品機能の移植は未実施。
- 配置と依存方向の正本は[リポジトリ構成](docs/architecture/repository-layout.md)。`apps/desktop`が製品を組み立て、`modules/music`はJUCE・OS・GUIに依存させない。
- 通常の新機能は`apps/`、`modules/`、新しい`tests/`へ追加する。新コードから`legacy/windows/src/`をinclude・リンクしない。
- 旧Windows環境は`legacy/windows/`で独立して管理する。旧文書の「次の作業」、原版UI一致、全40責務等を新製品の自動的な要件にしない。旧環境で作業する場合のsource rootも`legacy/windows/`。
- 新ビルド入口はルートのCMake。macOSでは`cmake --preset macos-debug`または`macos-release`で構成する。現在は骨組みだけなので、構成成功を製品のコンパイル・起動・音声動作の成功とは報告しない。
- 実行手順と検証範囲は[開発環境](docs/development/building.md)を参照する。Windows実行や必要素材がない試験を合格扱いにしない。
- 外部依存はcommitを固定し、取得方式を重複させない。JUCEは今後`third_party/JUCE/`へGit submoduleとして追加する。
- 生成物は`build/`、開発用データは`local/`へ置く。正式なfixtureは出自・生成条件とともに`tests/fixtures/`で管理する。
- 過去の証拠のhashやパスを現状に合わせて一括置換しない。旧作業ルートとの対応を文書で説明する。
- 実施計画書は後の工程で作成する。採用済みの判断は`docs/decisions/`、現行設計は`docs/architecture/`へ記録する。

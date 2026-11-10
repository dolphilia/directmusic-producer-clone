# 旧Windows再構築資料の索引

旧環境のルートは[legacy/windows](../../legacy/windows/README.md)。Windows x86向けのソース、試験、スクリプト、文書、Q1DlsOracleを一緒に移動し、内部の相対配置を保っている。

| 入口 | 内容 |
| --- | --- |
| [旧実施計画](../../legacy/windows/docs/analysis-plan.md) | 方針転換前の計画と進行記録 |
| [本体・文書の引継ぎ](../../legacy/windows/docs/analysis/product-host.md) | 候補ごとの実装・検証の記録 |
| [機械可読の旧状態](../../legacy/windows/docs/analysis/product-state.json) | 旧Windows候補・入力・結果 |
| [再生契約](../../legacy/windows/docs/analysis/playback-contract.md) | 旧再生経路とその制約 |
| [Style契約](../../legacy/windows/docs/analysis/style-contract.md) | Styleの形式・参照等の知見 |
| [Sequence編集契約](../../legacy/windows/docs/analysis/sequence-crud-contract.md) | 音符編集と保持するデータ |
| [Wave文書契約](../../legacy/windows/docs/analysis/wave-document-contract.md) | 音声文書の編集・保存 |
| [旧ソース](../../legacy/windows/src) | Win32本体、形式処理、互換ABI等 |
| [旧ツール](../../legacy/windows/scripts) | 解析、ビルド、検証、画面操作等 |

## パスと証拠の読み方

過去の記録にある`src/...`、`tests/...`、`scripts/...`、`docs/...`は、当時の作業ルートに対するパスである。追跡ファイルの現在の配置は、それらの前に`legacy/windows/`を付けたものに対応する。

`work/...`、`artifacts/...`、`screenshot/...`はGit対象外だった素材・生成物を指す。これらは今回のcheckoutには存在しない。旧スクリプトを動かす場合は`legacy/windows/`を作業ルートとし、必要な素材と所在を個別に確認する。旧Windowsの絶対パスをMacのパスへ機械的に変換しない。

原文のhashと証拠パスは履歴として保持している。原版比較・GUI・音声の過去の成功を、移動後の実行やMac版の成功へ昇格させない。旧資料の継続指示も、現在の作業指示ではない。

現行の設計へ知見を採る場合は要点と出典を現行文書へ記載し、新しく決めた規則と区別する。

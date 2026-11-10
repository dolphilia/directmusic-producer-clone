# 製品ライブラリ

| モジュール | 責務 |
| --- | --- |
| [music](music/README.md) | 曲、時刻、音符、和声、Pattern |
| [engine](engine/README.md) | 再生プラン、Transport、音声・MIDI |
| [project](project/README.md) | 編集コマンド、履歴、保存、資産参照 |
| [formats](formats/README.md) | SMF・WAV・旧DirectMusic形式 |

現在は責務と配置だけを用意した段階。実装のないINTERFACEターゲットや仮のソースを作らず、最初の実装とともに静的ライブラリを追加する。

公開ヘッダーは各モジュールの`include/producer/<module>/`、実装と内部ヘッダーは`src/`へ置く。依存方向の正本は[リポジトリ構成](../docs/architecture/repository-layout.md)。

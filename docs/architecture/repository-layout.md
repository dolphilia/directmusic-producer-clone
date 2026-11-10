# リポジトリ構成

状態：2026-11-10、採用した配置への移動と新しい開発入口の整備を実施。新製品の実装・JUCE取得は未実施。

| 配置 | 責務 |
| --- | --- |
| `apps/desktop/` | JUCEアプリ、編集画面、文書と再生の接続 |
| `modules/music/` | 曲、音符、テンポ・拍子、和声、Pattern、音符生成 |
| `modules/engine/` | 再生プラン、Transport、音声・MIDI、デバイス |
| `modules/project/` | 編集コマンド、履歴、保存復元、資産参照 |
| `modules/formats/` | SMF、WAV、RIFF、旧DirectMusicの入出力 |
| `tests/` | 新製品の単体・接続・音声試験、追跡するfixture |
| `tools/` | fixture生成、音声解析等の開発補助 |
| `cmake/` | 共通ビルド設定の置き場所 |
| `third_party/` | commitを固定する外部依存。JUCEの導入は今後実施 |
| `docs/` | 現在の設計、採用判断、開発手順、提案、資料索引 |
| `legacy/windows/` | 独立した旧Windows再構築環境 |
| `build/` | Git対象外の生成物 |
| `local/` | Git対象外の取得物、録音、調査・実験データ |

## 依存方向

```text
desktop ──→ project ──→ formats ──→ music
   │            └────────────────→ music
   └──────→ engine ──────────────→ music
```

`music`はJUCE、OS API、GUI、ファイルパスへ依存させない。`engine`は音声系のJUCEを利用し、文書保存や編集画面を呼ばない。`project`と`formats`から`engine`へ逆依存しない。アプリが文書から再生への接続を組み立てる。

各モジュールは最初の実装時にCMakeターゲットを追加し、`include/producer/<module>/`に公開ヘッダー、`src/`に実装と内部ヘッダーを置く。他モジュールの内部ヘッダーを直接includeしない。将来のターゲット名は`producer_music`、`producer_engine`、`producer_project`、`producer_formats`、`producer_desktop`。

## 新旧の境界

ルートCMakeは新製品用。旧WindowsのCMake、preset、ソース、試験、スクリプト、資料は`legacy/windows/`内で相対配置を保っている。通常のMac構成では旧環境を追加せず、新コードから旧ソースを直接include・リンクしない。

旧実装を参考に機能を抽出するときは、型・OS依存・公開API・試験を新しい責務へ適合させる。移動だけで旧コードが移植されたとは扱わない。

旧資料のパスは当時の作業ルートに対するものとして読む。過去の証拠のhashや参照パスを一括で書き換えず、[旧資料索引](../reference/windows-recovery.md)から辿る。

## 現段階で作成していないもの

製品ソース、アプリの`.app`、JUCE submodule、新製品の実行可能な試験はまだない。`src/`、`include/`、`resources/`等の空の下位構造は、実ファイルが生じた単位で追加する。配置確認のCMake成功と、製品の動作成功を区別する。

採用理由は[構成の採用記録](../decisions/0002-repository-layout.md)、現在のコマンドは[開発環境](../development/building.md)を参照する。

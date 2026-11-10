# 今後の開発に向けたリポジトリ構成の提案

作成日：2026-11-10（日本時間）

状態：2026-11-10、ユーザーが本案を採用。同日、旧Windows環境の移動と新しい開発入口の整備を実施した。現在の配置の正本は[リポジトリ構成](../architecture/repository-layout.md)、確認結果は[配置変更の記録](../development/repository-layout-migration-2026-11-10.md)を参照する。以下は採用した提案の背景と到達形を保持したもの。実施計画書は別途作成する。

## 1. 推奨する整理方針

**ルートを新しい製品の入口にし、既存のWindows再構築環境は内部構造を保ったまま`legacy/windows/`へまとめる。新規コードは`apps/`と`modules/`へ、現行の設計文書は`docs/`へ置く。**

新規コードはOS名より責務で分ける。デスクトップアプリは`apps/desktop/`、曲のモデルや再生処理は`modules/`とする。今後WindowsやLinuxにも展開する場合、同じ製品コードを育てられる配置にする。`legacy/windows/`の「legacy」は旧Windows実装を指し、今後のWindows対応全体をそこへ置く意味ではない。

新コードと旧コードは同じGitリポジトリで管理する。今の段階で別リポジトリや多数のサブモジュールへ分割する必要はない。最初のライブラリ分割は4つに抑え、さらに分けるのは独立した利用先や依存上の理由が生じたときにする。

## 2. 移動前の調査で分かった制約

- `src/producer/`には文書、RIFF、音楽モデル、再生、Win32 UIが同居し、`Framework`や`Conductor`のヘッダーもWindows型を公開している。フォルダー名を変えるだけでは移植可能なライブラリにはならない。
- `scripts/`には479ファイルある。静的な文字列検索では120ファイルが`$PSScriptRoot`、164ファイルが`work/`等のパスを参照していた。これらは分類の手掛かりであり、実行互換を検証した件数ではない。
- [Build-ProductSnapshot.ps1](../../legacy/windows/scripts/Build-ProductSnapshot.ps1)は`CMakeLists.txt`、`src/`、`tests/producer/`、配布用の`docs/`を同じルートから集める。[TimeSig用CMake](../../legacy/windows/tests/native/time_signature/CMakeLists.txt)も`../../../src`を参照する。
- 解析資料のMarkdown、JSON、CSVには、当時のソース・生成物・録音・候補のパスとhashが記録されている。文書とスクリプトを一度に細分化すると、この対応を追いにくくなる。
- `Q1DlsOracle/`の3ファイルは旧試験の入力資産として扱う。新アプリのサンプルや配布素材として、そのままルートに残す必要はない。
- 現在の`.gitignore`はルートの`work/`、`artifacts/`、`screenshot/`を対象にしている。旧環境を入れ子にした場合は除外設定も見直す必要がある。

このため、旧ソース・旧試験・旧スクリプト・旧文書は、まず一つのまとまりとして移す構成を推奨する。旧環境の移動と、新しい責務へのコード抽出は別の変更として扱う。

## 3. 目指すディレクトリ構成

以下は到達形の例。全ての空ディレクトリを先に作る必要はなく、実ファイルが生じた単位で追加する。

```text
directmusic-producer-clone/
├── README.md                         # 現在の目的と開発入口
├── AGENTS.md                         # 開発時の短い共通ルール
├── CMakeLists.txt                    # 新製品のビルド入口
├── CMakePresets.json                 # macOS Debug / Release等
├── .gitignore
├── cmake/                            # 依存物・警告・共通設定
├── apps/
│   └── desktop/
│       ├── CMakeLists.txt
│       ├── src/                      # 起動、文書と再生の接続、編集画面
│       └── resources/                # アプリアイコン、表示用素材
├── modules/
│   ├── music/                        # 曲、時刻、音符、和声、Pattern
│   ├── engine/                       # Transport、スケジュール、音声・MIDI
│   ├── project/                      # 編集コマンド、履歴、保存、参照管理
│   └── formats/                      # SMF、WAV、旧DirectMusicの入出力
├── tests/
│   ├── unit/                         # モジュール単位の試験
│   ├── integration/                  # 編集・保存復元等の接続試験
│   ├── audio/                        # レンダリング・発音時刻等
│   └── fixtures/                     # 小さな入力、期待値、生成条件
├── tools/                            # fixture生成、音声解析等
├── third_party/
│   └── JUCE/                         # commitを固定した外部ソース
├── docs/
│   ├── README.md                     # 現行文書と旧資料への索引
│   ├── architecture/                 # 現在採用している設計
│   ├── decisions/                    # 技術選定と判断理由
│   ├── development/                  # 環境構築、ビルド、試験方法
│   ├── proposals/                    # 未採用・検討中の提案
│   ├── reference/                    # 制作概念、形式仕様、旧資料の案内
│   └── plans/                        # 後の工程で作る実施計画
├── legacy/
│   └── windows/
│       ├── README.md                 # Windows再構築の説明・旧作業ルート
│       ├── CMakeLists.txt
│       ├── CMakePresets.json
│       ├── src/                      # 現在のsrcを内部配置ごと保持
│       ├── tests/                    # 現在のWindows試験
│       ├── scripts/                  # 現在の解析・実行スクリプト
│       ├── docs/                     # 過去の契約、観測、状態台帳
│       └── Q1DlsOracle/              # 旧試験入力
├── build/                            # Git対象外：ビルド生成物
└── local/                            # Git対象外：取得物、録音、作業用データ
```

`apps/`は起動可能な製品、`modules/`はライブラリの単位にする。ここでいうmoduleはCMakeターゲットの区分であり、C++20 ModulesやJUCE独自のモジュール形式を必須にする意味ではない。初期は静的ライブラリを基本とする。

将来オフラインレンダラーが独立した実行物になったら`apps/render/`等を追加できるが、利用先のない枠を先に増やさない。

## 4. 新しい製品コードの境界

| 配置 | 所有する責務 | 依存の範囲 |
| --- | --- | --- |
| `modules/music` | 音符、Clip、テンポ・拍子、和声、Pattern、決定的な音符生成 | 標準C++を中心にする。JUCE、OS API、ファイルパス、GUIに依存させない |
| `modules/engine` | 再生プラン、Transport、音声レンダリング、デバイスとMIDIの接続 | `music`とJUCEの音声系。編集画面や保存形式の処理を呼ばない |
| `modules/project` | 文書の編集、Undo／Redo、dirty、資産参照、保存・復元 | `music`、`formats`、非実時間の保存用ライブラリ。再生エンジンに依存させない |
| `modules/formats` | RIFF・SMF・WAVのcodec、旧形式からの変換、変換レポート | `music`と必要なcodec。`project`やGUIへ逆依存しない |
| `apps/desktop` | JUCEアプリ、メニュー、タイムライン、Inspector、文書と再生の接続 | 上記4つとJUCEのGUI系。アプリケーション全体の組立てを担当する |

依存の向きは次の形にする。

```text
desktop ──→ project ──→ formats ──→ music
   │            └────────────────→ music
   └──────→ engine ──────────────→ music
```

モジュール間の公開ヘッダーを`include/producer/<module>/`、実装と内部ヘッダーを`src/`に置く。例えば次のようになる。

```text
modules/music/
├── CMakeLists.txt
├── include/producer/music/
│   ├── musical_time.h
│   └── sequence.h
└── src/
    ├── timing/
    └── sequence/
```

新しいターゲット名は`producer_music`、`producer_engine`、`producer_project`、`producer_formats`、`producer_desktop`等に揃える。公開ヘッダーに内部のフォルダー構造を漏らさず、他モジュールから`src/`を直接includeしない。

最初から独立した`platform`や`generation`ライブラリは作らない。Core Audio／Core MIDIへJUCE経由で接続する処理は`engine`内、アプリ固有のMac連携は`apps/desktop`内、音楽生成は`music`内の責務として始める。将来、依存や利用先が増えた場合に抽出する。

JUCEを使う箇所と純粋な音楽モデルの境界は保つが、全てのJUCEクラスを独自クラスで包むことはしない。デバイスやプラグイン等の外部接続、再生プランの受渡しに必要な境界へ絞る。

## 5. 旧Windows資産の配置と保全

| 現在の配置 | 提案する配置・扱い |
| --- | --- |
| `src/` | `legacy/windows/src/`へ内部構造ごと移す。新ライブラリに採る機能は、依存を除去して別途抽出する |
| `tests/producer/`、`tests/native/`、既存`tests/audio/` | `legacy/windows/tests/`へ。新しい`tests/`にはMac・共通モデル用の試験だけを追加する |
| `scripts/` | `legacy/windows/scripts/`へ。再利用する処理だけ依存と引数を整理して`tools/`へ採る |
| `Q1DlsOracle/` | `legacy/windows/Q1DlsOracle/`へ。出自を確認した上で、必要な小さな入力のみ新fixtureの候補にする |
| 旧`CMakeLists.txt`／旧preset | `legacy/windows/`内に保管し、新しいルートCMakeから独立させる |
| `docs/analysis/`、`analysis-plan*.md` | `legacy/windows/docs/`へ。契約、状態JSON、CSVもまとまりを保つ |
| 取得記録、旧配布説明 | `legacy/windows/docs/`へ。旧スクリプトやCMakeからの相対参照を保つ |
| 長いWindows開発履歴を含むREADME | 旧記録を`legacy/windows/README.md`へ整理し、ルートREADMEを新開発の入口にする |
| 今回の技術調査文書 | 現行の`docs/`側に残す。整理時には`docs/decisions/`で採用理由を短く示し、詳細調査と相互参照する |

旧文書だけを`docs/archive/`へ分離すると、旧スクリプトが期待する`docs/`と、文書内の相対リンクを別々に直す必要がある。今回は旧環境を`legacy/windows/`内で完結させ、現行の`docs/reference/`から重要な資料へ案内する方が扱いやすい。

元ファイルの内容を保って移動すれば、Gitは履歴を辿る際にrenameを検出しやすい。大規模な移動と書換えを同時に行わず、まず配置変更、次に必要な参照修正、機能抽出は別の差分として扱う。ただし、移動だけで旧試験の再現性が保証されるわけではない。

旧スクリプトが想定する作業ルートは`legacy/windows/`とする。カレントディレクトリや絶対パスに依存するものは個別に確認する。旧`work/`等の素材を引き継ぐ場合も、このルートとの対応を記録する。Mac上でWindows試験が実行済みと扱わない。

過去のJSONやログにある`src/...`、`work/...`は、当時の作業ルートからのパスである。新しいルートからのパスへ全件置換せず、旧作業ルートと現在の保存先の対応を説明する。過去のhashを持つ証拠ファイルも編集して辻褄を合わせない。

新コードから`legacy/windows/src/`をinclude・リンクする構成にはしない。移植時は旧実装を基準として残し、必要な部分を新しい公開API・型・試験へ適合させる。この間に似たコードが並ぶことは許容するが、両方を新機能の実装先にはしない。通常の新機能は新コード側へ追加する。

## 6. ビルドと依存ライブラリの扱い

ルートCMakeは新製品だけを構成する。旧Windows本体や比較プローブをルートから無条件に`add_subdirectory()`しない。旧構成にはWindows x86の判定、RC言語、SDKリンクがあり、Macビルドへ巻き込むと開発入口が再び分かりにくくなるためである。

通常の生成先は`build/macos-debug/`、`build/macos-release/`等に統一する。将来の新製品Windowsビルドも同じルートに別presetを追加できる。旧Windows再構築は`legacy/windows/`をsource rootとする独立ビルドにする。

JUCEは`third_party/JUCE/`にGit submoduleで置き、commitを固定する構成を推奨する。外部依存の取得方式を一つに揃え、同じJUCEをsubmoduleとFetchContentの両方で持たない。ほかの小さな依存も採用時に固定し、アプリ・モジュールごとに別コピーを増やさない。

`cmake/`は実際に複数ターゲットで使う設定だけを置く。依存物の取得・組立てと、製品コードのコンパイル設定を分け、ルートCMakeはモジュールとアプリの接続が読める長さに保つ。

## 7. 文書、試験、補助ツールの役割

現行文書と歴史資料の入口を分ける。

| 置き場所 | 内容 |
| --- | --- |
| ルート`README.md` | 製品の目的、採用スタック、現在できること、ビルド手順へのリンク |
| `docs/architecture/` | 実装が従うモジュール境界、時間モデル、再生、保存形式 |
| `docs/decisions/` | 採用済みの判断と理由。技術スタック採用を最初の記録にする |
| `docs/development/` | 環境準備、ビルド、テスト、デバッグ、依存更新の手順 |
| `docs/proposals/` | 今回のような検討段階の提案。採用状態と採用先を明示する |
| `docs/reference/` | DirectMusicの制作概念、再利用する形式知識、旧資料の索引 |
| `docs/plans/` | 後の工程で作成する実施計画。設計文書へ進捗ログを混ぜない |
| `legacy/windows/docs/` | 原版比較、UI観測、旧契約、候補ごとの結果、旧継続指示 |

旧資料から得た知見を現行の仕様に採る場合は、現行文書へ要点と出典を書き、新しく決めた規則と区別する。原版の操作契約が、そのまま新アプリの必須要件になる扱いを避ける。

新しい試験は`tests/`に集約し、`unit/`と`integration/`の中を`music/`、`project/`等で対応付ける。製品コードには試験用mainや原版プローブを混ぜない。fixturesは小さな合成入力、期待値、生成方法、素材の出自を追跡する。録音結果や実行ログをfixturesに蓄積しない。

`tools/`には開発を助ける実行可能な処理を置く。例えばfixture生成、音声解析、文書リンク検査など。最初から言語別の`python/`、`node/`を作らず、必要に応じて用途で分ける。旧`.mjs`でもOS非依存にできる処理は再利用でき、拡張子だけで廃棄対象とは判断しない。

`AGENTS.md`は現在の開発ルールへの入口とし、採用構成、依存方向、ビルド・試験文書、旧Windows資料の位置を短く記載する。旧資料中の「次の作業」や凍結条件を、新しいMac開発への自動指示にしないことも明示する。長期の計画や作業ログは別文書へ置く。

## 8. Git管理するものとローカル生成物

| 種類 | 配置・管理方針 |
| --- | --- |
| 製品コード、CMake、固定依存の参照 | Git管理 |
| アイコン、同梱が必要な少量の素材 | `apps/desktop/resources/`。出自・利用条件とともにGit管理 |
| 合成fixture、期待値、生成器 | `tests/fixtures/`と`tools/`。Git管理 |
| 大きな旧配布物・外部音源・参考資料 | `local/downloads/`、`local/reference/`等。Git対象外。必要なら所在・hashを別途記録 |
| ビルド成果物、依存物のビルドキャッシュ | `build/`。Git対象外 |
| 録音、スクリーンショット、調査ログ、実験 | `local/evidence/`、`local/scratch/`等。Git対象外。重要な結果の要約は現行文書へ残す |
| 旧環境の`work/`、`artifacts/`、`screenshot/` | 移行時は旧スクリプトとの対応を維持し、入れ子になったパスも明示的にignoreする |

ルートの`.gitignore`には、新しい`/build/`、`/local/`、旧環境のローカル作業先、macOSの`.DS_Store`、個人設定用の`CMakeUserPresets.json`等を追加する方針とする。共有する`CMakePresets.json`はGit管理する。全てのJSONや音声ファイルを一律にignoreする指定は使わない。

`local/`は開発用の一時データであり、アプリ利用者の正式なプロジェクト保存先や設定保存先にはしない。

## 9. 構造変更を進める際の考え方

構造整理の目的は、次の実装の置き場所と依存方向を明確にすることにある。全旧ファイルの細かな再分類や全面的な命名変更を、Mac版開発の前提にはしない。

実際の移動では、旧環境のまとまりを保つ変更と、新ビルド入口を成立させる変更を対応付ける。新しいルートCMakeやpresetがない期間に既存入口だけを移すと、どこからビルドするか分からなくなるためである。さらに、C++機能の抽出やUIの作成は配置変更と別の差分にする。

整理後に確認するのは、新製品の通常ビルドが旧Windowsターゲットや旧素材を要求しないこと、旧環境の参照元が分かること、現行文書のリンクが辿れること、未追跡素材の移動・削除を勝手に行っていないことである。Windowsでの旧試験の再実行は、利用可能なWindows環境と素材がある範囲で別に確認する。

本案の採用後、旧環境の移動と新しい配置・文書・構成確認用CMakeを整備した。機能の抽出、JUCE導入、アプリの実装は後の工程で扱う。今後の具体的な作業順と完了条件は、別途作成する実施計画書で定める。

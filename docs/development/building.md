# 開発環境とビルド入口

## 現在できること

ルートにある新製品用CMakeの構成と、生成したビルドシステムの実行を確認できる。現在は配置整理の骨組みであり、JUCE、製品のソース・バイナリ、実行可能な新製品の試験は未作成。buildが成功しても`.app`は生成しない。

必要な環境は、macOS、Apple Clangを利用できるXcodeまたはCommand Line Tools、CMake 3.22以上、Ninja。共有presetはApple Silicon arm64向け。deployment targetの14.0は仮の設定であり、macOS 14上での製品動作を確認した意味ではない。

リポジトリのルートで実行する。

```sh
cmake --list-presets
cmake --preset macos-debug
cmake --build --preset macos-debug
cmake --preset macos-release
cmake --build --preset macos-release
```

生成先は`build/macos-debug/`、`build/macos-release/`。製品ターゲットがまだないため、現段階のbuildはコンパイルする製品ソースがなく終了する。

個人用の上書きはGit対象外の`CMakeUserPresets.json`で管理する。共有presetをローカルの絶対パスや個人設定で変更しない。試験を実装したときにCTestとtest presetを追加し、試験が0件の状態を合格結果として扱わない。

## 旧Windows環境

旧製品用presetと比較用CMakeは[legacy/windows](../../legacy/windows/README.md)にある。Windowsで旧コマンドを使う際は、まずそのディレクトリへ移動する。旧ツールの相対パスもそこを基準とする。

原版配布物・過去の録音・旧`work/`等はGit対象外であり、このcheckoutには引き継いでいない。Windowsのビルド・実行・音声試験は今回再実行していない。

## 今後の実装

最初の実装時にJUCEを固定commitのsubmoduleとして追加し、必要なモジュールとアプリのターゲットを作成する。今回の配置変更では依存物を取得しない。対象と作業順は別途作成する実施計画で定める。

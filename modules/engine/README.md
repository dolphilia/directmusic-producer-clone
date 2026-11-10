# engine

再生プラン、Transport、音声レンダリング、デバイスとMIDI入出力を所有する。`music`とJUCEの音声系に依存し、編集画面やプロジェクト保存処理には依存しない。

想定ターゲット名は`producer_engine`。実装開始時に`include/producer/engine/`、`src/`、`CMakeLists.txt`を追加する。

旧[Conductor](../../legacy/windows/src/producer/conductor.cpp)は再生・停止等の規則を読む参照元。WindowsのCOM実装をそのまま新しいバックエンドとして使わない。

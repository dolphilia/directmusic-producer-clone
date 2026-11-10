# project

文書の編集コマンド、Undo／Redo、dirty、資産参照、保存・復元を所有する。`music`、`formats`、非実時間の保存用ライブラリへ依存し、再生エンジンへは依存しない。

想定ターゲット名は`producer_project`。実装開始時に`include/producer/project/`、`src/`、`CMakeLists.txt`を追加する。

旧[Framework](../../legacy/windows/src/producer/framework.cpp)と[文書モデル](../../legacy/windows/src/producer/document.h)の規則を参考にし、Windows型や旧RIFF文書の所有方式を新APIへ持ち込まない。

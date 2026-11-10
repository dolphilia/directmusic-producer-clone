# music

音符、Clip、テンポ・拍子、音楽時刻、和声、Pattern、決定的な音符生成を所有する。JUCE、OS API、GUI、ファイルパスへ依存させない。

想定ターゲット名は`producer_music`。実装開始時に`include/producer/music/`、`src/`、`CMakeLists.txt`を追加する。

旧実装を参照する入口は[テンポ](../../legacy/windows/src/tempo/tempo_track.h)、[Timeline](../../legacy/windows/src/producer/timeline.h)、[Sequence](../../legacy/windows/src/producer/sequence.h)。これらを直接リンクせず、必要な型・規則から依存を除去して抽出する。

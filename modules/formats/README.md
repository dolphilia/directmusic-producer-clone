# formats

SMF、WAV、RIFF、旧DirectMusic形式のcodec、旧形式からの変換、変換レポートを所有する。`music`と必要なcodecへ依存し、`project`、再生エンジン、GUIへは依存しない。

想定ターゲット名は`producer_formats`。実装開始時に`include/producer/formats/`、`src/`、`CMakeLists.txt`を追加する。新プロジェクトの保存トランザクションは`project`側で扱う。

旧[RIFF処理](../../legacy/windows/src/producer/riff.cpp)、[MIDI import](../../legacy/windows/src/producer/midi_import.cpp)、[Style](../../legacy/windows/src/producer/style.cpp)等を参照する。未知データ保持と、そのデータを演奏できることを区別する。

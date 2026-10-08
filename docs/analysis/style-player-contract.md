# StylePlayer Shape生成・試聴の契約

Q3G、2026-10-07。原版起動は `work/analysis/q3-style-player/20261007T022658Z/original-launch-block.json` の承認タイムアウトで未観測。以下は同梱 `work/producer/style-library/StylePlayer.txt` と公開SDKの契約であり、原版動的比較の合格ではない。

同梱txtではStyle/ChordMap/Shape/intro/end変更とRe-Composeは先頭から再開始、Band変更は再開始なし、Motifは演奏へ重ねる。Shapeはgrooveと対応しQuietは低いgroove、Risingは低→高。起動時のランダムStyle選択・自動再生、4個のMotifボタンも補助アプリの責務として保持する。本体の明示的な試聴開始から接続し、未検証部分を残す。

[Microsoft ComposeSegmentFromShape](https://learn.microsoft.com/en-us/previous-versions/ms808992(v=msdn.10)) はStyle、measures、Shape、activity(0..3)、intro/end、ChordMapから新規Segmentを生成する。固定公開header `work/analysis/sources/dmusici.h` のABIとDMUS_SHAPET値0..8を用いる。既存 `compose_chord_track` はユーザーSegmentのテンプレートからChord trackだけを採用する責務で、Shape生成と区別する。

宣言したWindows dmloader/dmstyle/dmcompos/dmimeとDirectMusic/DirectSound/GM.DLSはOS依存。原版StylePlayer.exeやProducer designerのロードは実装要件に含めない。Q2独立環境は未用意の障害を維持する。

終了条件・変更範囲・試験は同単位 `resume-scope.json`。生成Segmentの全trackをnative DMSGへ保存し、source-owned Style参照とnative Projectの既存保存・解決を再利用する。自動録音の成功を原版互換・全40・全8受入へ広げない。

現候補20261007T043410164Zでは停止中のStyle/ChordMap/settings変更とRe-Composeもstartを呼び、先頭から再開始する。不正設定は変更前に拒否し、Bandだけの変更は停止中にも再生を始めず、再生中のprimaryを保つ。64項目のnative契約試験と本体の操作で検証する。

OS Composer生成のSegment/trackのSaveはE_NOTIMPLとなる場合があり、公開GetParamで取得したBand/Tempo/Style/ChordMap/Command/Chordの六責務をnative DMSGへ保存する。未対応trackを省略して成功にはしない。匿名の生成Bandはclass-only descriptorを持つため、所有StyleのGetDefaultBandから明示的な既定Band identityを束縛する。これは原版互換を観測した根拠ではない。私有再生用Bandのみのcarrierから既定Tempo trackを取り除き、120BPMへの意図しない上書きを防ぐ。

残責務: legacy AASY Style Library入力のDMST変換/編集互換、Style既定ChordMapのみの生成物でのSave E_NOTIMPL、複数Band changeの生成保存は未完。選択Bandは現在の生成snapshotにも反映し、再生成時にも適用する。複数Bandイベントの一般的な保存は未対応として保持する。原版動的比較、外部Producer ABI、Q2独立環境と全40/全8の完成は別判定。

進行中既定ChordMap単位 `work/analysis/q3-style-player-default/20261007T071213536Z` の修正前候補071303345Zでは、null入力がComposeSegmentFromShapeのE_POINTER（0x80004003）で拒否され、track保存には到達しなかった。現行修正は公開dmusicf.hのprrf/DMRFを既存の所有参照resolverで解決し、OS GetDefaultChordMapの選択identityと完全なsource bytesを照合してComposer・native保存・Conductorへ渡す。既定選択順序の推測、legacy.PER→.CDMの推測置換、mapグラフの合成、省略はしない。検証完了は未判定。

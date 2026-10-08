# AudioPath Send宛先編集・FileOutput identity修正（未完）

単位：[記録](../../work/analysis/q3-audiopath-send/20261007T094855774Z/unit-record.json)。対象は40対象表の `AudioPathDesigner.ocx`、`FileOutputDMO.dll`、Conductor、本体。計画Q3FのSend編集不足を減らすソース変更であり、受入合格ではない。基準候補084224157Zの208保存ソースは保持し、SDK構成失敗後の新生成物はない。

原版helpの [Send Effects](../../work/analysis/help/htm/sendeffects.htm) は、宛先をPChannelとsynth busのない1バッファのmix groupに限定し、mono送信元はmono/stereo、それ以外は同じ音声チャンネル数を要求する。効果の順序によってSendへ入る音が変わる。[挿入手順](../../work/analysis/help/htm/toinsertasendeffect.htm) の標準Environmental Reverb、別AudioPath、Other GUID、減衰設定は引き続き残責務。[FileOutput help](../../work/analysis/help/htm/fileoutputinanaudiopath.htm) の効果後段録音、番号順、Producer専用効果のruntime export除去も保持する。

保存SDK `dmusicf.h` のDMUS_BUFFERF_MIXIN=8、DSOUND_IO_DSBUFFERDESCのWORD nChannels、fxhrのguidSendBufferとreserved fieldsを使用した。Microsoftの [DirectSound return values](https://learn.microsoft.com/de-at/previous-versions/windows/desktop/ee416775(v=vs.85)) も無効なmix-in宛先とSend循環の拒否を記載している。GUI原版観測ではない。GetObjectInPathの旧Microsoftページは本文を取得できず、実行時のglobal mix-in index/factory契約の根拠にはしていない。

変更は既存Send宛先の編集、所有mix-inバッファ作成、チャンネル数・synth bus・PChannel接続・ローカル循環の検査と本体UI接続。既存外部宛先、effect class/flags/data/順序/尾部/未知chunkは保持する。未観測のSend classを生成したり、未知factoryを実行許可したりしていない。

FileOutputのpredefinedバッファ変換は、従来PChannel参照しか書き換えず、mix-inフラグも消していた。incoming Send GUIDを同じ変更で更新し、mix-inの役割とbusなしを維持するよう修正した。既存二バッファ録音は再実装していない。ConductorのSend録音未対応guardは残る。

Send helpで使用する公開SDK Waves Reverbが既存効果allowlistから漏れていたため、9番目の標準効果として追加した。[読み取り契約/変更範囲](../../work/analysis/q3-audiopath-send/20261007T094855774Z/waves-effect-scope.json) にはSDK header hashとRegistry32の既存dsdmo.dll登録を保存した。実行許可は従来のsystem-directory provenance gateを通る。登録・セキュリティ変更やCOM activationは行わず、native/audioは未検証。

新しい `audio-path-send` モードは、宛先編集の正確なbyte差分、Undo/Redo、拒否時不変、mono/stereo/multichannel、循環、predefined materializationとincoming Send更新、fresh native roundtripを検査する。fixtureのeffect classは人工値で、COMへ渡さない。これは本物のSend factory/PCMの試験ではない。専用wrapperは実行結果の合格/exit0を確認する。

登録は85 native / 130 driver。C++ compile/install/native/GUI/PCM/原版比較は未実行。wrapper ASTとdiff whitespaceの確認だけを製品合格にしない。manifest生成時に消えたFarm独立入力監査の参照を復元し、以後保持する修正も追加した。checkpointの最初の対象名誤り `.dll` は失敗記録と初版を残して `.ocx` へ修正した。

残る条件は、宣言SDKを読める環境で新候補を保存してビルドし、関連回帰、本体操作/native保存、通常終了、別プロセス復元/通常終了、実際のSend factory/録音・効果順序・Stop/再開/PCM比較を行うこと。Producer `papd` のmix-group編集表示/順序、標準predefined variants、他AudioPath宛先の寿命・減衰・原版編集互換も未確認。`fullAcceptance=false` を維持する。

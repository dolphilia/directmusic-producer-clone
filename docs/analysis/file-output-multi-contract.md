# FileOutput 複数バッファの契約

既存 FileOutputWriter、source IMediaObject/InPlace、AudioPath の効果挿入・履歴・runtime除外を維持し、Conductor の単一control制限だけを拡張する。対象は FileOutputDMO.dll / Conductor.dll / AudioPathDesigner.ocx の直接PChannel接続バッファである。

保全原版help `work/analysis/help/htm/fileoutputinanaudiopath.htm` は、効果を使用するmix-groupのPChannelだけを記録し、複数バッファならRecord.wav、Record1.wav、Record2.wavの順に作成すると説明する。順序はmix-group順、PChannelを持つものが先である。runtimeにはProducer-only効果を保存しない。Sendによる後段録音も責務だが、今回の直接接続とは別queueである。原版対象designerを利用できず、同入力動的比較は障害あり。以下のsource検証を原版互換完成とは扱わない。

- シリアライズされたport/route順に、直接接続する一意のFileOutputバッファを列挙する。物理dbfl保存順だけで番号を付けない。共有バッファの同一DMOを重複開始しない。直接接続されない録音バッファは、Send接続が実装されるまで開始前に明示拒否する。
- 各バッファのsource runtime class/controlを実際のAudioPathから取得し、それぞれに選択名、選択stem+1+extension、stem+2+extensionを設定する。UIの既定拡張子は.wav。既存出力を上書きせず、全予定出力の衝突を開始前に検証する。
- 一つの録音開始に複数controlを所有する。全control取得・開始を完了してから録音状態を公開する。途中失敗は全controlを停止・解放し、作成済み出力をfinalizeして失敗証拠として保持する。部分録音をactive成功としない。
- 楽曲Play/Stopと録音Start/Stopは独立する。楽曲Stop後も両バッファ録音を保持し、再生の再開を同じファイルへ記録する。録音Stopは全controlをfinalizeし、一つの停止が失敗しても残りの停止を実行する。正常終了cleanupも全controlを処理する。
- 録音中に異なるAudioPathを黙って既存録音経路へ置き換えない。今回の固定経路は同じauthoring bytesを要求し、変更・欠落を楽曲状態変更前に拒否する。live切替は残責務として維持する。
- authoring文書・native所有・UndoRedo・元の効果順・未知chunkを維持する。runtime exportのProducer-only除外は、複数バッファについても検証する。

専用native試験でdbflとroute順を逆にした二バッファ、PChannel0/8の異音高、重複/出力衝突/未接続/異なる再生経路拒否、独立Stop、native保存復元とruntime除外を検証する。本体でAudioPath履歴・native保存・通常終了・別復元後、同じPIDの実buffer WAV二本とWASAPIを記録する。RIFF/PCMの独立監査は原版動的比較の代替ではない。

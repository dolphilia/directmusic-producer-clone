# Wave共通プロパティの本体接続

2026-10-10（日本時間）。候補1446で共通modeless PropertiesのWave/Infoを接続した。単位は未完、fullAcceptance=false。限定採用0735と原始一巡2137を維持する。

[開始契約](../../work/analysis/ui-wave-properties/20261009T135815190Z/unit-start-v1.json)は原版Help/GIFを基準に、TreeのWave componentからF11/右メニュー、選択追従、Root、最初のWSMP loop、Info Subject/形式表示を対象とする。既存WaveDocumentの編集・履歴・保存を再利用した。C5はMIDI60、EndはinclusiveでLength=End-Start+1。Unlocked StartのEnd保持方針は推論として残す。

[1446 build](../../work/build/product-snapshot/20261009T144612503Z/build-summary.json)の236保存/作業ソース・4生成物は一致し、[関連native7](../../work/analysis/ui-wave-properties/20261009T135815190Z/related-regression-binding-v3.json)は合格した。Producer SHA256はb286e3973a7ce952d7a6d7dba0d29997a8fccda0e5ebaff832d7e69d4113fe41。旧1404のCtrlTab不成立、1433のInfoラベル切れを保持し、1446で修正した。

[GUI監査](../../work/analysis/ui-wave-properties/20261009T135815190Z/gui-proof-v3.json)28項目が成立。F11/context、CtrlTab/CtrlShiftTab/矢印、E5/不正H5、UndoRedo、Loop/Start/End/Length/Lock、範囲外拒否、別Wave選択追従、Info Subject、Segment回帰、Save Wave/Project、別process復元を確認。merged UIAの古いコピーは保持し、最後のcontrol行とsettled観測で判定した。checkbox/radio選択は画像・nativeで確認する。

[独立native監査v4](../../work/analysis/ui-wave-properties/20261009T135815190Z/native-proof-v4.json)は4項目合格。保存Waveは操作前に独立作成したexpected-properties.wvpの全bytesと一致し、SHA256は863ae7741e35adcfb701c61acf13f6f9ed21de943aaa5c8b6bc65119494ea9e3。root64、WSMPtype1/Start66150/Length44100、SMPLroot64、名前Properties tones、PCM/未知chunk/残りmetadataを照合。ProjectのWave表示名UNFO/nnamも更新される既存framework.cpp契約を最初の判定器が取り落としたため、v3失敗を保存し、[訂正根拠](../../work/analysis/ui-wave-properties/20261009T135815190Z/native-auditor-correction-v4.json)を添えた。Segment/Otherは不変。author17064/reload20300/audio17164/繰り返しaudio25516は全て異なる起動の通常exit0。

[外観QA](../../work/analysis/ui-wave-properties/20261009T135815190Z/visual-layout-proof-v3.json)はOS200%を維持し、二行Sample Rate/Sizeラベルと基本属性群を確認した。原版396×233 Help画像に対して現在796×484物理pixelの枠・字体・配置差を残す。原版のattenuation/fineTune、Selection行、reduction options、Info Author/Copyright/Information/GUID、Compression/Streamingは未実装。原版の動的外観・操作一致とはしない。

音声は未合格。[初回](../../work/analysis/ui-wave-properties/20261009T135815190Z/audio-proof-v1.json)はnewWindow入力geometryなし、[2回目](../../work/analysis/ui-wave-properties/20261009T135815190Z/audio-proof-v2.json)は操作間17.33秒で5秒音源終了後のStopとなり、失敗を保持した。録音前にSegment-only repeatコピーを作りdwRepeatsの4bytesだけ0→infiniteへ変更、Wave全bytes不変、16秒周期の36秒Stopを固定して180秒録音した。[v3固定判定](../../work/analysis/ui-wave-properties/20261009T135815190Z/audio-proof-v3.json)は初回onsetがPlayafterから1.021秒となり、固定1.000秒を超えて不合格。[診断](../../work/analysis/ui-wave-properties/20261009T135815190Z/audio-partial-diagnostic-v3.json)では両経路の440/660/880Hz、発音中Stop、停止後RMS0、再開、peak0.09494が成立するが、全体判定には転用しない。判定閾値は維持。合格PCMがないためnegative controlsは未実行。

[単位記録](../../work/analysis/ui-wave-properties/20261009T135815190Z/unit-record-v3.json)に終了条件・入力/実行物/観測/PCM/失敗を結合した。残差は初回coldWave onset、全Wave属性/Editor/DLS埋込、原版approval比較障害、Q2、全40/全8/個別Windows5/Clipboard5。同条件の録音を再試行せず、既実装のstartup Conductor.prepareを保護する。Wave3単位連続のため、次は共通File CtrlSを原版HelpのEditor-open条件に沿って接続する。latestUnit/latestReportの0735採用は更新しない。

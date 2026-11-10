# Part Variation候補の所有編集


## 2026-10-03 Part Variation候補の編集

DMUS_IO_STYLEPARTのtimeSig直後にある32個のdwVariationChoicesを、StylePartとset_part_variation_choiceへ公開。0-based index0..31の単一DWORDだけを変更し、全32bitを保持する。0を許容し、SDKのmode/将来bitを推測で除去しない。音符のvariation membership maskとは独立。共有Partへは同じ変更が届き、所有貼付け後の別GUID Partへは伝播しない。変更なし/無効indexは履歴を作らず、切断/重複headerは変更前に拒否する。Frameworkは一回のStyle transactionで依存Segment snapshotを更新、Segment bytes/dirtyを保持。

本体にVariation1..32選択、候補DWORD入力、Set Chord Choicesを追加。原版のUIと同じ表現とは未確認。中間120434753Zはnative18/独立全bytesとclipboard32成功だが、GUI120706988ZでBand欄との重なりを確認して修正した。中間GUIは未編集で通常終了exit0。最初の120407456ZはMSBuildのSDK探索がsandboxで拒否され構成失敗、通常権限の承認済みビルドへ切替。OS制御ポリシーの変更や迂回はしていない。

最終20261003T120854267Zは保存56sources/3targets、構成0/compile0/install0。EXE SHA256 9a83ac2d6c1f2dbf62fa11c9e9457ddee1a51d53c086156eda714ef768044e46。work/acceptance/part-variation/20261003T121131490Z/run.jsonは18件/exit0、variation-proof.jsonが入力加工/候補DWORD以外の全bytes保持/Framework別復元/再保存を独立確認。work/acceptance/pattern-clipboard/20261003T121132566Zの32件/全byte監査も同版で合格。work/acceptance/product-host/20261003T121133520Zの本体起動通常終了/実moduleを別判定。

原版Clipboard観測は登録済み原版launchAppへのsky.launch_appが「did not expose a targetable window」。list_windows再照会も原版なし。Get-Processで既存PID5400（reference-track-position/20261003t041600z/app/dmusprod.exe、開始13:16:20JST、MainWindowHandle0）を確認。新たなlaunchは繰り返さず、原版Clipboardは未観測のまま保持した。

現行GUIの結果はwork/acceptance/product-project-gui/20261003T121157754Zへ分離。実際のVariation選択/Compose音響効果・原版動的変更比較・Motif・空Style Band・全40責務/全八受入は未完了。旧1133版GUI/audioを新しいEXEへ転用しない。Windows DirectMusic/DirectSound/GM.DLSは残る外部依存。次は同版GUI候補編集/保存/別起動と、固定候補入力のCompose生成音・録音、その後Motifと原版観測を進める。

再現：Build-ProductSnapshot.ps1、Test-PartVariation.ps1 -BuildSummaryPath <summary> -Segment <source selection.sgp> -Style <source Heartlnd.stp>、Inspect-PartVariation.mjs <run.json>。SDK根拠はwork/analysis/sources/dmusicf.hのDMUS_IO_STYLEPARTとDMUS_VARIATIONF_*。

最終GUI121157754Z/PID4244はVariation1を0→127、Undo0、Redo127、Save Document、通常終了exit0を確認。独立variation-gui-proof.jsonは候補DWORDだけの全bytes変更、project/Segment不変、画像hashを照合。保存Style SHA256 c1f47f8805eaa9f3381e5870a0cf3ff48759766423267af6e46cd07480dd603d。45modules原版40hash一致0。右上へ移した欄はBand欄と重ならない。UIAメニュー位置がwindow外になる場合は再観測した画像から一操作し、Undo直後の遅延UIAは安定したundo-readyで値を確認。GUI別起動/Variation32選択は次の検証。原版警告は遅れて現れた「Failed to update the system registry. Please try using REGEDIT.」をstartup画像へ保全した。list_windowsに原版対象は返らず、警告・レジストリの操作はしていない。


## 2026-10-03 現行Variation再生・無人録音とGUI別起動

製品は120854267Z（保存56sources、EXE 9a83ac2d6c1f2dbf62fa11c9e9457ddee1a51d53c086156eda714ef768044e46）を再利用。今回の製品ソース変更/新ビルドはない。構成・compile・installは当該保存build-summaryの各exit0、実行と本体受入は別判定。検証スクリプトと固定入力を追加し、前版の音声を転用せず同じ現行EXEで新規録音した。

Create-PartVariationFixture.mjsは所有Part/Patternを各1個持つ入力の4音をC4/C5へ複製し、membership mask1/0x80000000を指定。候補配列はVariation1だけまたは32だけFFFFFFFF、他を0にする。両Styleはその32DWORD以外の全bytesが同じで、Segment/Band/Pattern/GUIDも同一。独立Inspect-PartVariationPlayback.mjsはraw RIFFを再読取し、この差分・保存56sources・生成物・入力コピー・4実行を照合。work/analysis/part-variation-playback/20261003T122000Z/playback-proof.jsonが合格。API work/acceptance/product-notes/20261003T121912515Z と work/acceptance/product-notes/20261003T121936079Zは各12音、C4=60/C5=72、768clock間隔、duration384、PChannel5、velocity96、通常終了を確認。

新規録音work/acceptance/audio-loopback/20261003T122059286Z と work/acceptance/audio-loopback/20261003T122213065Zは48kHz/2ch/float32、各16秒、player/capture exit0。候補1 RMS0.022930106555072483/C4、候補32 RMS0.028394606261839195/C5、onsetはいずれも3.4秒、前後RMS0、最大packet gap2frames。各生成12音と録音12区間の音程を照合。音色入力はstrings、pianoとは記述しない。録音器102354462Zの保存生成物を使用した。初回録音後の説明scope修正は解析のみ再実行し、旧proofをinitial-proof-before-scope-correction.jsonへ保存、WAV/APIは変更していない。

work/acceptance/audio-auditor-controls/20261003T122500Z-first と work/acceptance/audio-auditor-controls/20261003T122500Z-lastは各4対照が合格。未変更コピーexit0、無音/中間の逆音程/基準区間への背景音混入はexit1。生成API成功を保持したまま録音だけの失敗を拒否する。派生コピーであり、新録音数には含めない。

GUI work/acceptance/product-project-gui/20261003T122419895Zは初回GUI PID4244の保存Styleを別PID5392で読込み、Variation1=127、Variation32=2435007847(0x91234567)を確認。Save Document後も全bytes一致、Style SHA c1f47f8805eaa9f3381e5870a0cf3ff48759766423267af6e46cd07480dd603d、通常終了exit0。45modules原版40hash一致0。variation-gui-reload-proof.jsonに画像/入力/前回proof/module/生成物を結合。GUI Playの音声試験とは別。

残る依存はWindows DirectMusic/DirectSound/GM.DLS。原版動的Variation/Clipboard比較、一般Variation組合せ、GUI Play、物理スピーカー、Motif、空StyleのBand生成、JAZP、全40責務/全八受入は未完了。原版の対象ウィンドウ未公開/registry警告という既存障害は保持し、起動を同条件で繰り返していない。次はMotifの所有文書/編集/保存復元/生成音と、空Style Bandの成立を順に進める。

再現：Test-PlaybackNotes.ps1へ現行summaryとfixtureのfirst/lastのselection.sgpおよびInputPaths Heartlnd.stpを指定。Test-LoopbackAudio.ps1へ同じsummary、RecorderBuildSummaryPath work/build/audio-capture/20261003T102354462Z/build-summary.json、Recorder work/build/audio-capture/20261003T102354462Z/build/Release/producer_loopback.exe、Segmentを各fixture、InputPathsを対応Style、Profile variation-first/variation-last、Nodeを環境のNode実パスへ指定。Test-LoopbackAuditor.mjs <audio run dir> <未作成の一意dir>で対照を作る。Inspect-PartVariationPlayback.mjs <fixture.json> <first API run.json> <last API run.json> <first audio run.json> <last audio run.json>で独立照合。GUI再起動はTest-ProductProjectGui.ps1とInspect-PartVariationGuiReload.mjs <reload dir> <first GUI dir>。work/acceptance/part-variation-playback/20261003T123500Z/unit-record.jsonは記録時刻・最終scripts/docsの保存コピーと入力/生成物/WAV/証拠を結合する。

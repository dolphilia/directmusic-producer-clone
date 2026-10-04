# 自作Sequence・Band・Conductorの最小再生契約

更新：2026-10-03（日本時間）。限定経路の仕様であり、Producer全体の互換性・受入完了ではない。

根拠は保存済み公開SDK `work/analysis/sources/dmusici.h`、`dmusicf.h`、`dmplugin.h`。取得元・ハッシュは `sdk-reference-sources.json`。dmusicf.hの48行以降のSequence構造、782行以降のBandTrack、877行以降のInstrument、dmusici.hの917/1032/1302行以降のLoader8/Segment8/Performance8を照合した。原版QuickStartのRIFF観測は `product-inputs/20261002T132547062Z`。今回の新規Sequence文書を原版本体で開く比較は未実施。

`sequence.*` はDMTK/trkhでSequenceTrackを識別する。seqtはRIFF/LISTコンテナーではなく、evtl/curlを含むleafデータとして保持する。evtlの先頭DWORDはrecord stride、生成値20。time/duration/PChannel/offset/status/pitch/velocityはそれぞれ0/4/8/12/14/15/16 byte。既存のMIDIイベント、拡張record bytes、curve、未知subchunkとpaddingを保存する。今回の編集は一つのSequenceトラックへnote-onレコードを追加する経路だけ。durationは正、timeと終端はSegment length内、PChannel0–15、pitch0–127、velocity1–127。複数Sequenceへの編集、削除、curve編集、MIDI import/exportは未実装。全RIFFを一つのUndo/Redo履歴として更新する。

短い試験文書はlength6144 ticks、PPQ768、8音（60/62/64/65/67/69/71/72）、各間隔768/duration384/velocity96/PChannel0。Tempoは0で120、3072で180 BPM。BandTrackはRIFF:DMBT、bdth、LIST:lbdl/lbnd/bd2h、RIFF:DMBD/lbil/lbin/binsを持つ。44-byte binsはGM piano patch0、PChannel0、pan64、volume100、flags0x1161。独立Band文書の編集・外部DLS参照・他楽器は未実装。

`playback_runtime.h` は公開OS COM ABIの使用部分を宣言する。Producer独自COMとは区別し、x86 ObjectDesc848 byte、memoryLength832/memory840をstatic_assertする。インターフェースは継承部分を含むメソッド順を保持し、参照はIUnknown経由で解放する。未使用の後続スロットを呼ばない。

ConductorはSTAの同じスレッドで初期化・終了する。x86 HKCRからPerformance/LoaderのInprocServer32を読み、dmime.dll/dmloader.dllがWindows system directory配下であることを確認して生成する。登録を変更しない。InitAudioはdynamic stereo/16 PChannels。文書を毎回save_bytesでserializeし、loader cacheを無効にして現在のbytesをmemory descriptorからロードする。保存文書の親directoryを外部参照の探索先とする。descriptorのbacking bytesはloader/segmentの解放まで保持する。

Download、PlaySegmentEx、IsPlaying、GetTime/GetStartTime、実再生中のGUID_TempoParam照会、StopEx、停止後IsPlaying=S_FALSEの確認、Unload、CloseDownの結果を記録する。Stopは最大2秒で終了状態を待ち、終了しなければ失敗にする。失敗時も部分DownloadのUnloadとCOM参照解放を試みる。Release/CoUninitialize自体の参照リーク検査は未実施。GUIは現在文書のsnapshotを再生し、編集中の変更は次のPlayで反映する。ライブ編集同期は未実装。

OS由来の再生ランタイム・標準GM音源を宣言依存とする。今回のRegistry32 GMFilePathはWindowsのGM.DLS、SHA `3229b09b9d7d9f3f4793b0d9b34fe6abc75cfa4a2503c0c90f43ff651ba7f2c0`。設定値とファイルをread-onlyで照合したが、実際のfile-open traceと出力endpointの名前は未取得。原版固有のEXE/DLL/OCXは製品経路から生成・ロードしない。再生中55 modulesのhash/PE/由来照合は現行試験に対応する。API成功・人の聴取・GUI・全機能を別々に判定する。

次はStyle参照の拍子と既存QuickStart文書の再生へ拡張する。音符の全編集、Band/DLS/AudioPath、通知・ライブ同期、原版相互読込、正確な音声計測と同じ最終構成での全体受入は残る。

## 追記：Styleのメモリ所有と照会（2026-10-03）

現在154730195ZはStyle references/context/GUID/meterとidentity競合を停止前に検証し、バイトをコピーして所有する。登録先dmstyle.dllもWindows system directoryをread-only照合する。保存済みdmusici.hのIDirectMusicStyle/IID、GetTimeSignature/GetTempoのprefix ABIと8バイトDMUS_TIMESIGNATUREを使用する。StyleはGUID+DMUS_OBJ_MEMORYでSetObject/GetObjectへ渡し、filename-validは立てない。最初の併記版のSetObject失敗を別記録に保持した。

Segment/Loader解放後までStyle bytesと参照を保持する。実行で取得したStyle拍子/tempoをsource readerと比較し、Segment GetParam(GUID_TimeSignature)も照合する。native Style caseの入力/依存SHA、ヘッダーだけの3/4メモリ変化と実runtime値、cleanupと実ロード57件を同じrun154812445Zへ記録した。原版module一致0件。GUI Playにも現在文書stylesを渡す変更を加えたが、この版のGUI操作と音声は未検証。filename-only mapping・GUID-only catalog・全Pattern/Band/音源通知は継続作業。
# 2026-10-03 追加：filename-only／GUIDなしStyle再生

164907492ZのConductorは`prepare_style_playback`で元DMSG/DMSTを保持したまま再生用copyのidentityを正規化し、filename-only参照とGUIDなしStyleをGUID+memory登録へ接続する。`playback_bytes()`/`playback_styles()`で実際のruntime入力を試験へ返す。構成/build/installとrun165035676Zのhost/core147/minimal/Style APIは成功した。独立RIFF監査は`style-playback/mapping-proof.json`。3/4・tempo132・groove5..50編集後のDMPJ移動/reloadとGUIDなしケースのplay/stopが通った。Segment performance tempo132、Pattern選択と聴取は未確認。再生開始は固定100msの仮定を除き、試験で最大2秒の50ms pollingを行う。

module provenanceの57件は初期GUID Style再生時のcaptureで、後続filename/missing-GUID caseの一覧ではない。現行GUI/audio受入も未実行。原版固有moduleへのfallbackは追加していない。旧版の音階/加速の聴取確認はその版にだけ帰属する。全体八受入は未完了。



## 2026-10-03 所有BandをSegment/BandTrack再生へ接続

現行184224946Zは保存46ソースの構成・ビルド・installとnative263件が成功、警告なし。所有Bandを時刻付きでSegmentへコピー/同時刻置換し、UndoRedo・project保存/reload・再生snapshotへ接続。現行APIは0/3072 clocksのpiano/violinを含む全bytes一致、Play/Stop/tempo/再開が成功。GUI・実音色・DLS・全八受入は未完了。

原版静的観測: heartland.sgp SHA cfff2d948051dc7dbdaca2009982086e50cfd00e046b9717ad96cb31c6fbec05、BandTrack[4]のbd2hはoffset1838/8byte、logical0/physical0。work/analysis/bandtrack-headers-observed.json。ローカルSDK dmusicf.hのDMBT/lbdl/lbnd/bdih/bd2h/DMBDと論理・実行時刻を使用。原版編集/saveとの動的比較は未実行。

SegmentDocument::set_bandとFramework::assign_bandは所有Band全bytesをコピー。独立Bandの後続編集が既存Segmentへ暗黙伝播しないため、本体にCopy Band at clocksと所有Band選択を追加。同時刻はBand bytesだけ置換し、旧bdih/既存bd2h実行時刻・未知tail/padding・他項目を維持。新イベントはlogical=physical=time、group1で新しいDMBT/bdth autoDownload1。複数BandTracks/group2/同時刻重複/欠落ヘッダは変更前に拒否。イベント移動・削除・負の時刻生成・group選択は未実装。失敗時はSegment全体を保持する。

work/build/product-snapshot/20261002T184224946Z/build-summary.jsonは46保存ソース/3targets/configure/build/install成功、EXE a776d9f8c3e974a174e740dd741f1501088fb58183ec4b84322a7aee24e71fe7、work/acceptance/product/20261002T184310612Z/run.jsonでhost/core/playback exit0、263checks。保存source/workspace/EXE/入力をbandtrack-proofで照合。playback.sgpはprojectへ保存し別Frameworkでreload、runtime-band.sgpと全bytes一致、piano時刻0とviolin40時刻3072のBandを保持。violin.bnpと埋込Band全bytes一致。現行OSロード/Download/Play/Stop/120→180/90編集/earlyStop/restart成功、実際に選択された音色はAPIでは未確認。headless24/playback55実ロードの原版40hash一致0。Style専用API/GUIは現行未実行、前回版の証拠を転用しない。聴取質問は184224946Zに限定して回答待ち。

失敗履歴:183954191Zはビルドだけ、試験fixtureがSequenceをBandTrackと誤選択する問題を実行前に修正。184048191Z/run184143817Zはhost/playback成功、core Invalid chunk identifierで失敗。fixtureのvector追加後に古いheader pointerを読む無効参照を修正し、check数付き診断を追加して184224946Zで263件成功。製品コードの同条件失敗再試行やOS拒否回避ではない。旧生成物の成功を現行へ転用せず、新版を実行した。

残る依存:Windows DirectMusic/DirectSound/GM.DLS。捕捉経路の原版hash一致0は全GUI/全機能非依存の完成証拠ではない。次はCopy Band GUI/save/reloadと版別音色確認、BandTrack move/delete/physical時刻とDLS source-owned参照/download。その後残る40責務と全八受入。全体目標active/incomplete。


## 2026-10-03 BandTrack論理/実行時刻編集と本体復元

現行185105419Zは保存46ソースの構成・ビルド・installとnative277件が成功。BandTrackの選択・論理/実行時刻移動・削除を本体/文書/再生へ接続。GUIで所有Bandコピー、768/720 clocksへの移動、UndoRedo、保存・別process復元を確認。現行APIは移動/削除後も保存とruntime全bytes一致。実音色・DLS・全八受入は未完了。

原版静的観測はwork/analysis/bandtrack-headers-observed.jsonとSDK bdih/bd2h契約を継続利用。動的編集比較は未実行。group1の選択イベントをcopy-on-editで移動・削除、論理重複/範囲外/無効index/曖昧trackは変更前に拒否。物理時刻はsigned preroll可。legacy bdihは同一時刻なら保持し、異なる物理時刻ならbd2hへ変換して未知tail/paddingを保持。event整列はopaque siblingのslotを保持し、最後の削除でも空BandTrack/metadataを保持。Segment全RIFF UndoRedoへ接続。

work/build/product-snapshot/20261002T185105419Z/build-summary.json EXE d029683835fd0fa2d407adfc831763f94904a47576d142c132a85656462f1f76、46source/3targetsの構成・コンパイル・install、work/acceptance/product/20261002T185204038Z/run.json host/core/playback exit0、277checks。最初の184955731Zは構成・compileのみ、playback fixture追加前の版で未実行。現行runtimeは初期0/3072、移動1536/1500、削除後0のみの三snapshotが各保存SGP全bytesと一致。headless24/playback55で原版40hash一致0。現行GUI/Styleモジュール一覧は未取得。

work/acceptance/product-ui/20261002T185300Z/unit-audit.jsonは同一現行EXE/source46/native277、入力SHA、7直接取得画像、保存差分、2process closeを連結。GUI所有Band73をclocks0へコピー、選択event0を768/720へ移動、Undo0/0・Redo768/720・Save・別process project読込768/720を確認。全ファイル期待値はbd2hの2値とbinsのpatch73/flags0x1163だけを書いたbeforeと一致、他11leafと全opaque bytes保持。独立Band/projectの入力bytes不変。GUI削除/Playは未実行。window消失は確認、process exitcodeは未確認。accessibilityが1操作遅れる場合は画像の視認結果を使用。選択dialogのmixed slash入力エラーはbackslash形式へ修正して解消、セキュリティ拒否ではない。

ユーザーの最新「聞こえた。途中から速くなった」は最初の4秒聴取質問への回答で、既存145048176Z/audio-confirmation.jsonと同じ内容。144958360Zの証拠として保持し、新しい185105419Zへ転用しない。前回184224946Zの音色質問は未回答、現行聴取/音色は未確認。

依存はWindows DirectMusic/DirectSound/GM.DLS。全40責務/全八受入は維持。次の具体的一手はDLS reference descriptorのtyped編集・project相対所有collection解決・runtime download。group-aware選択、原版動的比較、GUI削除/再生、実音色・全文書/COM通知は未完了。全体goal active/incomplete。


## 2026-10-03 DLS参照編集とFrameworkの音源所有

現行191312627Zは保存46ソースの構成・ビルド・installとnative304件が成功、警告なし。DLS参照の型付き編集/UndoRedo、Frameworkの音源snapshot所有、相対参照・GUID解決、project保存/移動/reloadを接続。実FarmGame.dls全bytes保持を確認。本体割当UIはビルドのみ、DLS download・現行GUI/再生/音色・全八受入は未完了。

前回は進捗あり（BandTrack実装/native277/GUI保存復元/版別記録）。本単位はSDK dmusicf.h DMUS_IO_REFERENCE、dmusici.h object flags、Windows SDK10.0.26100 dmusicc.h line746 CLSID_DirectMusicCollection、Microsoft [DMUS_IO_REFERENCE](https://learn.microsoft.com/en-us/previous-versions/ms808108%28v%3Dmsdn.10%29)を照合した。work/analysis/dls-references-observed.jsonは4原版sampleを静的読取。FarmGame.dls SHA615eab9a6eb2eaeadfb7f69afa5d6b53d1a2ce8e4699b4d18c5f1f0994b399df、163588bytes、RIFF DLS /dlid79bdcd0caada3c4cb91b993a398acdc3/colh1。FarmGame.dlpもDLS form。抽出23DMRFはStyle1/Script22でCollection参照0、原版DLS割当編集/saveの動的比較は未実施。

BandDocument::set_collection_referenceはclass/GUID/relative filenameをtyped編集、classとvalidflagsをSDKに合わせる。GM/GS/XG/default GM bitsを解除、他のinstrument fields/opaque descriptor/tails/paddingを保持、全Band UndoRedo。古いmalformed/fullpath/URL/memory/stream referenceはlossless load/saveを維持し、typed編集/解決は明示的拒否。filename最大259WCHAR、GUID-onlyはFramework所有catalog必須。relative filenameはdocument directoryへ解決し、読込時はcanonical containment/identity照合。missing/重複GUID/型不一致は成功扱いしない。

Frameworkは.dls全bytesを所有snapshotとして開き、Band割当、DMPJ参照/SaveAs/別Framework reloadへ接続。collection_identityはDLS root/dlidを確認する関数であり、全instrument/region/wave意味検証や編集の完成証拠ではない。所有snapshotはdisk後続変更に依存せず、GUID-onlyでもregistryを検索しない。音源参照の解決はband_collections呼出時。未保存Bandの割当は拒否し、保存済みBandのdirectory内に音源を配置する。mainにAssign DLS Collection追加、GUIは本単位未実行。Style内DLS、copied Segment directoryを跨ぐ再配置、runtime download/音源編集は次単位。

失敗:191012578Z構成/compile成功、run191147147Z host成功/core after140 Expected one RIFF root失敗。LIST DMRFをRIFF-root-only parserへ渡した実装不具合を、別API Chunk::parse_listで修正。既存RIFF rootの拒否契約は保持。修正後work/build/product-snapshot/20261002T191312627Z/build-summary.jsonの46source/3targets構成・compile・install成功、EXE 09bfb854b32e8e853a03cc48df77b454f7051e96662ae0b4b359846290c957a1、work/acceptance/product/20261002T191412418Z/run.json host/core exit0、304checks。実FarmGame DLSをcopy/assign/Framework project reloadし全bytes保持、input/run/build/sourceをdls-proofに結合。headless24原版40hash一致0、現行再生/GUI/音声未実行。旧277件版のGUI/API成功は転用しない。

依存:Windows DirectMusic/DirectSound/GM.DLS、原版モジュールfallbackなし。全40責務・全八受入は維持。次はFramework所有DLS snapshotをConductor memory loaderへ登録し、Band referenceのGUID解決とdownload/unload寿命を接続・版別検証。その後割当GUI/save/reload、instrument/region/wave編集、原版動的比較/実音色。全体active/incomplete。


## 2026-10-03 所有DLSをConductorのmemoryロード/downloadへ接続

現行192511898Zは保存46ソースの構成・ビルド・installとnative314件が成功、警告なし。所有DLSをmemory loaderへ登録し、patch256のEnum/GetInstrument、Segment Download/Play/Stop/Unload・再開を接続。GUID生成は再生用コピーだけ、保存文書/初期DLSは全bytes保持。headless24/通常55/DLS55原版40hash一致0。現行GUI・実音色・DLS編集全体・全八受入は未完了。

前回は進捗あり（DLS descriptor/Framework所有/保存移動reload/native304）。SDK10.0.26100 dmusicc.hのCollection class/IIDとGetInstrument/EnumInstrument prefixを実装し、公式 [GetInstrument](https://learn.microsoft.com/en-us/previous-versions/ms808982%28v%3Dmsdn.10%29)、[Band Download](https://learn.microsoft.com/en-au/previous-versions/ms808954%28v%3Dmsdn.10%29)でpatchとdownload契約を確認。Download=S_OKだけでは実音声/port対応の証拠にならないため聴取は別判定。

Framework::playback_collectionsはSegment内Bandと所有Style内Bandの参照を順序付きで所有catalog/文書directoryへ解決。prepare_collection_playbackは参照数/identity/context/path/GUIDの衝突をStop前に検証し、runtime copyでfilename valid bitを外してGUIDへ置換。既存DLIDは保持、ないときだけCoCreateGuidでDLSコピーにdlid追加、同pathは共有。保存bytes/元ファイルは変更しない。fullpath/URL/malformed参照、未知の他形式は未対応として拒否。

ConductorはWindows system由来dmusic.dllをos_serverで確認し、GUID+MEMORYだけでSetObject/GetObject。backing bytesとCollection interfaceを所有、Enum first patch/Get assigned instrumentを照合してからSegmentロード/Download/Play。partial load/downloadは成功扱いしない。StopEx→IsPlaying S_FALSE→Unload→Segment/Style/Collection/Loader release→CloseDown/COM解除、異常時もshutdownで所有を解放。UI PlayにFramework解決を接続した。現行Style内DLSの純粋mappingは対応するが、そのOS runtime/GUI経路は本単位未実行。

ビルド192339879Zは構成/compileだけで未実行。fixture判定がStop後IsPlaying=S_FALSEを異常とすることを実行前に修正、GetInstrument照合を追加してwork/build/product-snapshot/20261002T192511898Z/build-summary.jsonを保存46source/3targetsから構成・compile・install。EXE 53a9642d8bc6565da79e87c41341c59889ae1fc24bf1052c4f5ac0e4b6e4f8fa、work/acceptance/product/20261002T192602557Z/run.jsonでhost/core/normal/DLS API exit0、314checks。通常Band move/delete exact snapshotとtempo120→180/90・Stop/再開を回帰。DLS APIはprojectを別FrameworkでreloadしFarmGame.dlsのpatch256をEnum/GetInstrument取得、memoryロード/Segment Download/Play/Stop/Unloadが2回S_OK。再生中のmissing-context replacementはStop前に拒否、DLIDなしfilename-only caseも生成GUIDでPlay/再開成功。

独立監査work/acceptance/product/20261002T192602557Z/dls-playback/dls-playback-proof.jsonはsource/workspace46/build/run/EXE/input/output/modulesをhashで結合。初期DLS全bytes一致、初期Segmentはfilename valid bitの解除のみ。generated DLSはdlid appendのみ、generated Segmentはvalidflagsとguid追加のみの全ファイル期待値と一致。空runtime directoryにDLSなし。headless24/通常55/DLS55のcaptured原版40hash一致0。現行GUI実ロード、Style DLS、endpoint・音声波形・実音色は未確認。直前のDLS約2秒と再開について聴取質問を出し、work/acceptance/product/20261002T192602557Z/dls-playback/audio-question.jsonへ版限定で記録、回答待ち。古いピアノ回答を現行へ転用しない。

依存はWindows dmime/dmloader/dmusic/dmband/dmsynth/DirectSound/GM.DLS。原版固有module fallbackなし。DLSDesigner全instrument/region/wave/loop/articulation編集、copied Bandのdirectory移動、group対応/原版動的比較/COM通知と全八受入は未完了。次は同一現行版の割当GUI/copy/save/project reload/Playと版別聴取、それからsource-owned DLS instrument/region/wave編集。全体goal active/incomplete。

再現: scripts/Build-ProductSnapshot.ps1で原版なし製品snapshotを作り、scripts/Test-ProductSnapshot.ps1 -BuildSummaryPath <build-summary.json> -ReferenceSegment work/producer/samples/QuickStart/heartland.sgp -ReferenceStyleDirectory work/acceptance/product-ui/20261002T180100Z/runtime-input -ReferenceCollection work/producer/samples/FarmGame/FarmGame.dls -DlsPlayback -Playback。fixtureはDLS bank1/program0=patch256のFarmGame用限定API受入。他入力を同試験へ転用しない。Inspect-ProductModules.ps1で各CaseName host-smoke/playback-api/dls-playback-api、Inspect-DlsPlayback.mjsとInspect-BandTrackPlayback.mjsへそのrun.jsonを指定。原版sampleは構成・ビルドに不要で、read-only検証入力。全体導入/配布手順の完成ではない。


## 2026-10-03 DLS Bandコピーのdirectory解決と本体GUI受入

現行193427048Zは保存46ソース/3targetsの構成・compile・install成功、警告なし、native318件と通常/DLS再生API成功。所有DLSのBand割当→Segmentコピー→保存→終了→別process再読込→Play/Stop→再保存をGUIで確認、全bytes期待値一致。別directoryへのコピーをGUIDで解決。captured headless24/通常55/DLS55原版40hash一致0。GUI実ロード/終了コード/実音声、DLS instrument/region/wave編集と全八受入は未完了。

Framework::assign_bandはコピー前にBand自身のdirectory/所有catalogで全collectionを解決する。DLID付きの所有音源はコピーしたBandをGUID-only参照にし、コピー先Segmentのdirectoryに依存しない。元Bandのfilename/GUIDは保持。DLIDなしは保存済みSegmentのdirectoryへ相対参照を作り直し、未保存/外側../相対化不能は変異前に拒否。未解決参照もコピー前に拒否。coreへ別directory・GUID-only解決・元Band不変/UndoRedo・project別Framework reloadを追加、314→318。UIの古い「DLS解決/download未実装」説明を接続済み経路へ更新し、instrument/region/wave編集が残ることを明示。

work/build/product-snapshot/20261002T193427048Z/build-summary.jsonは保存46sources/3targets、configure/build/install exit0、警告なし。EXE 3859f988146b0fe4801f6ef4ccbf1aa70855448376d4baa4741ebabaf9fd0331、core fc9df9ca20ffaa5390e9ccbfb79f6016ff6d446acd466d8652b67506d530c038。work/acceptance/product/20261002T193550235Z/run.jsonではhost/core/通常/DLS API exit0。両独立auditorが現workspace/frozen sources46とbuild/run/EXE/input/output/modulesを照合。初期DLS全bytes保持、現行初期SegmentはGUID-onlyのためruntimeと全bytes同一（proofのinitialSegmentOnlyFilenameFlagChangedは0変化も許容する名称）。DLIDなしは再生コピーにGUIDを追加する旧契約を維持。旧版の成功を流用しない。

GUIはcomputer-use @oai/skyで明示した同一install EXEのwindow7276470と再起動window14158300を操作。work/acceptance/product-ui/20261002T193700789Z/inputs.jsonのbefore-band（patch73）とbefore-playback（0/3072 clocks piano/violin）とproject/DLSを保全。Band patch256→Set Band Instrument→Assign DLS Collectionでowned.dlsを選択→Save Document、Segmentへ戻り0 clocks Copy Band→Save Document→Close。新processでproject.dmpjを開き、起動時の生成空文書だけDiscard、8notes/tempo120/180とBand patch256を復元。Play後Playing document snapshot、Stop後Stoppedを直接capture、両文書を再保存して終了。

work/acceptance/product-ui/20261002T193700789Z/unit-audit.jsonとscripts/Inspect-DlsGui.mjsは12直接captureのimagehash重複なしとEXE/windowを結合。保存Bandの全bytes期待値はpatch73→256、flags GM bits解除、collection-class refh19/guid/file owned.dls追加だけ。Segmentの全bytes期待値はtime0のembedded Bandのみ置換、copied参照はrefh3/GUID-onlyでfileなし、3072イベント/tempo/notes/他payloadを保持。project/DLS元bytes不変、別process再保存のBand/Segment全bytes同一。二windowが閉じたことをinventoryで確認したがGUIprocess exitCodeは未確認。GUI DLS参照UndoRedoは本単位未操作、API履歴検証とは区別する。

Native実ロードはheadless24/通常55/DLS55、captured原版40hash一致0。GUI inventoryとStyle DLS runtime未実施、実波形/出力先/実音色は未確認。previous192511898Z音声質問は未回答のまま版限定で保持、今回の「聞こえた/途中から速くなった」は144958360Zの同一question回答で現行へ転用しない。OS拒否の迂回なし。Windows dmime/dmloader/dmusic/dmband/dmsynth/DirectSound/GM.DLS依存は継続。DLSDesignerのinstrument/region/wave/loop/articulation editor、全40責務と全八受入は未完了。次はsource-owned DLS編集model/文書選択/save/UndoRedo接続、原版編集保存との比較、通知/lifetimeとgroup対応。goal active/incomplete。

再現:既存Build-ProductSnapshot.ps1/Test-ProductSnapshot.ps1の版別引数は前節と同じ（生成物/runを新しい記録へ置換）。scripts/Inspect-DlsGui.mjsへ本GUIdirectoryを指定すれば保存before/after期待値、再保存、EXE/build/native/sourceを再監査できる。GUI試験入力のbeforeファイルを新しい試験dirへコピーして上記手順を行い、状態は各操作直後に直接captureする。音声/全対象導入の完成手順ではない。


## 2026-10-03 DLS編集文書とFramework所有・音域付き再生入力

現行195748806Zは保存48sources/3targetsの構成・compile・install成功、警告なし、native345件とDLS再生API成功。DlsDocumentの楽器locale/Region範囲・wave cue/8・16bit PCM音量編集と全bytes履歴・保存・Framework所有/dirty/project reloadを接続。試験曲72..84が原版DLS Region72..111に入ることを独立監査。headless24/DLS55原版40hash一致0。DLS編集GUI未接続、実音声回答待ち、現行GUI/通常/Style再生と全八受入は未完了。

原版FarmGame.dls SHA615eab9a6eb2eaeadfb7f69afa5d6b53d1a2ce8e4699b4d18c5f1f0994b399dfをscripts/Inspect-DlsDocument.mjsでread-only観測、work/analysis/dls-editor-observed.jsonへ全chunk offset/小payloadを保存。inshのbank1/program0/region1、rgnh14bytesのkey72..111/velocity0..127、ptbl cbSize8/cue0offset0、mono PCM16 sampleRate44601/data162876bytes=81438framesを確認。rgnhの標準12byte後の拡張2bytes、wsmp/smpl loops/lar2/art1/INFO/未知wave chunksを保持。SDK10.0.26100 shared/dls1.hのINSTHEADER/RGNHEADER/WAVELINK/POOLTABLEと [Microsoft DMUS_REGION](https://learn.microsoft.com/nl-nl/previous-versions/ms808241%28v%3Dmsdn.10%29)を根拠に、region-to-pool cueの契約を確認。原版アプリの編集保存動的比較は未実施。

src/producer/dls.h/cppをproducer_coreへ追加。DlsDocumentは所有RIFF/保存checkpoint/全bytes UndoRedoを持ち、typed instruments/wavesでunique chunks/count/range/ptbl offsetを検証。locale bank/program・Region key/velocity/keygroup/cueを固定長で更新し、PCM8/16の音量倍率0..400percentを飽和付きで編集する。pool cueはwvpl先頭から各wave LISTへのoffsetで照合、ファイルサイズが変わる操作をadoptで拒否、未知chunk/order/padding/tailsを保持。compressed/noninteger PCMの編集は明示拒否。raw loadはDLS RIFFを保持し、未対応typedレイアウトは表示/編集時に拒否する（全DLS妥当性をloadで証明するものではない）。作成/追加削除/波形import/export/loop/articulation editorやptbl relocationは未実装、責務を縮小しない。Region keygroup編集はDLS1の0..15に限定、DLS2詳細未比較。

OpenCollectionのraw BytesをDlsDocumentへ置換し、Framework::collection_documentとsave_collectionを接続。Band/Segment/Styleのplayback依存catalogは常に所有文書のsave_bytesを採用、未保存PCM編集がBandへ反映する。Framework dirtyはcollection checkpointを含み、dirty collectionのproject saveは変異前に拒否。atomic保存が成功してからcheckpoint/pathを更新。GUI文書選択/編集controlは本単位未接続、原版音源の直接編集なし。

195424522Z/buildは48sources configure/build/install成功、195518414Z/runで344件/host/DLS API成功、同版auditors保存。原版Region観測により従来DLS試験notes60..72が大半音域外と判明したため中間版と明記。SegmentDocument::playback_test(firstPitch=60)の通常既定値を保ち、DLSfixtureだけfirstPitch72へ変更。原版instrument bank1/program0と全8notesのkey/velocity coverageをPlay前に検証。195748806Zを別snapshotとして構成・compile・install、work/acceptance/product/20261002T195839530Z/run.jsonで345件/host/DLS API exit0。EXE f95cd70c239e9391d532e9c068dc3aa6d54b468b1024c9e05e6e371b8912bf59、core 6bf30f4143b229346129ba5433e850f548706dac764685fffaee70b466cdb1c8。警告なし、現行regular playbackは変更が関係するcore既定fixtureを通したがAPIは未実行。

26編集検証＋1音域検証を追加。原版whole-byte roundtrip、locale bank2/program7だけ、Region keyLow60/velocityLow12/keygroup3だけ、PCM全sample50percentだけの期待値、UndoRedo/保存checkpoint、invalid range/cue/locale/count/ptbl/compressed formatのatomic拒否、Framework dirty/project保存拒否/別Framework reloadとBand edited依存を確認。work/acceptance/product/20261002T195839530Z/core/dls-editor/editor-proof.jsonは独立JSでoriginal→editedの直接offset8byte locale/6byte region/全PCM half期待値と全filesを照合、owned sourceはPCMだけ変更、ptbl/loop/未知chunks不変。保存bytesはedited/project/sourceへ保全。work/acceptance/product/20261002T195839530Z/dls-playback/dls-playback-proof.jsonはsource/runtime全8notes72..84・velocity96がoriginalRegion72..111/velocity0..127に含まれることを独立に照合し、memory register/GetInstrument/Download/Play/Stop/Unload/生成GUID再開を版別に確認。編集済みPCMのOS発音は本単位未実行。

Native捕捉headless24/DLS55原版40hash一致0。現行GUI/Style/通常API・実音声/波形/timbre/出力先は未確認。現行Region内試験の約2sec＋短い再開について聴取質問をwork/acceptance/product/20261002T195839530Z/dls-playback/audio-question.jsonへ版限定保存、回答待ち。旧DLS API成功は大半音域外のため実発音受入へ転用しない。前回193427048ZのGUI/保存再起動はその版で保持。Windows dmime/dmloader/dmusic/dmband/dmsynth/DirectSound/GM.DLSへの宣言依存は残る。原版固有fallbackなし、全40/全八受入未完了。次はDlsDocumentを本体document selectorと楽器/Region/Wave control/Save/UndoRedoへ接続、同新EXEのGUI保存/再起動とedited DLS playbackを検証、CRUD/loop/articulation/原版動的比較を進める。goal active/incomplete。

再現: Build-ProductSnapshot.ps1、Test-ProductSnapshot.ps1 -BuildSummaryPath <build-summary.json> -ReferenceSegment work/producer/samples/QuickStart/heartland.sgp -ReferenceStyleDirectory work/acceptance/product-ui/20261002T180100Z/runtime-input -ReferenceCollection work/producer/samples/FarmGame/FarmGame.dls -DlsPlayback。FarmGame固定bank/Region fixture、他入力へ成功転用しない。Inspect-ProductModules.ps1をhost-smoke/dls-playback-api、Inspect-DlsEditor.mjs/Inspect-DlsPlayback.mjsへそのrun.json。原版sampleは検証入力だけ、構成/ビルドに不要。


## 2026-10-03 本体DLS編集画面・保存履歴・再起動復元

現行201437359Zは保存50sources/3targetsから構成・compile・install成功。EXE SHA088efef6ced5cef31b0a71018ef2765dac5d24b1717a91d9716a5fa8e7784cf3、core EXE SHA39019c250f511215cf13b5ad9ebcbd34db696543174e4c88dfe48f44d1d3ddc9。native345件・host/DLS API exit0、headless24/DLS55の原版40hash一致0。構成と実行と本体受入を分離し、現行GUIはDLS編集の限定受入のみ。全40責務と全八受入は未完了。

src/producer/dls_editor.h/cppを本体へ追加。File/Open *.dlsとDocumentsのDLS項目からFramework所有文書の楽器bank/program、Region key/velocity/group/pool cue、PCM volume、Save DLS、Undo/Redoを接続。編集画面を閉じても所有文書を保持し、メインを無効にする同期message loopでindex/lifetimeを守る。保存済み変更は次のPlay用snapshotから取得する既存契約。DLSEditor全体の再構築ではなく、新規作成/CRUD/import/export/loop/articulation/サイズ変更pool relocationは未実装。

中間200441108Z/build・200820347Z/runは345/API成功。product-ui/201000Z wave-unit-audit.jsonでGUI PCM50percent、Undo元全bytes、Redo編集全bytes・保存を確認したが、owned secondary windowが操作ツールのlist_windowsで独立targetとして露出せず、Program7 typingが親activation後に届かなかった。UIA cache index unavailableも記録。入力の再試行を止め、OS拒否とは区別する。201437359Zでは独立したWS_EX_APPWINDOWを用い、文字欄を直接targetできることを実観測。中間成功は新版へ転用せず、新版でも別に345/API・module監査を実施した。

現行GUIはtask-owned FarmGame.dlsコピーを本体から開き、bank1→2/program0→7、Region keyLow72→60/velocityLow0→12/group0→3を適用しPCM50percentへ編集、Save、Wave Undo/Save、Wave Redo/Saveを操作。完全終了後に同一EXEを再起動してowned.dlsを開き、各値復元と無変更Save全bytes一致、Documentsから所有DLSの再表示を確認。7枚は別hashの直接capture。GUI保存edited.dlsは独立native auditorの原版field/全PCM期待値と全bytes一致し、Undoはlocale/Regionを残してPCMだけ原版へ復元。GUI Window IDs初回main6359392/editor49219108、再起動main64161920/editor354748100/切替76220664。終了はwindow不在を観測、process exit codeとGUI module inventoryは未確認。編集DLSのGUI再生/実音色も未実施。

証拠: work/build/product-snapshot/20261002T201437359Z/build-summary.json、work/acceptance/product/20261002T201540305Z/run.json、work/acceptance/product/20261002T201540305Z/core/dls-editor/editor-proof.json、work/acceptance/product/20261002T201540305Z/dls-playback/dls-playback-proof.json、work/acceptance/product-ui/20261002T201700Z/unit-audit.json。GUI before/edited/undo/redo/resaved/owned.dlsとstates/capturesを同directoryに保持。scripts/Inspect-DlsEditorGui.mjs <GUI directory> は保存ソース/build/run/EXE/入力hashと全保存物・新規capture/restartを再監査する。scripts/Inspect-DlsWaveGui.mjsは中間版のwave記録専用。

再現: Build-ProductSnapshot.ps1で保存snapshotを作成。Test-ProductSnapshot.ps1 -BuildSummaryPath <summary> -ReferenceSegment work/producer/samples/QuickStart/heartland.sgp -ReferenceStyleDirectory work/acceptance/product-ui/20261002T180100Z/runtime-input -ReferenceCollection work/producer/samples/FarmGame/FarmGame.dls -DlsPlayback、Inspect-ProductModules.ps1でhost-smoke/dls-playback-api、Inspect-DlsPlayback.mjs/Inspect-DlsEditor.mjsを新版runに実行。GUIは新しい試験directoryへ原版DLSをコピーして上記操作を行い版に結び付けて記録する。全対象導入/音声/完成手順ではない。

次の具体的一手はGUI編集音源をmatching Band patch519へ割当て、Region内音符72..84のSegmentへコピーして保存/restart/Playし、編集snapshot・runtime登録・GUI modules・音声を版別に確認する。その後DLS CRUD/pool relocation、group-aware trackと他文書へ進む。原版アプリとの動的編集保存比較、全通知/COM ABI/全形式、残る40責務と全八受入は保持する。


## 2026-10-03 編集DLSのlocaleをBandへ一括割当・現行再生

現行203140584Zは保存50sources/3targetsの構成・compile・install成功、build.logにwarning/errorなし。EXE SHAb84d1e0c782fc1d6ab6edab48410d01187e29fb69c222e60faf14d7bcd1e5351、core SHA4dd9a6306d604c4f29fab84d58e86f793733cdb0fe3c63fbd8a7e3317db09562。work/acceptance/product/20261002T203232589Z/run.jsonのhost/core/DLS API exit0、native354件。構成・compile・実行・本体受入を分離し全体は未完了。

BandDocument::set_dls_instrumentとFramework::set_band_collection_instrumentを追加。DLSのbank bits0..6/8..14をpacked Band bits8..14/16..22、percussion bit31とprogram bits0..6へ写し、typed相対filename/GUID参照と一度にcommit。PChannel/pan/volume、opaque chunksを保持。作業copyの検証・依存解決を先に行い、失敗時はBand全bytes/履歴を変更しない。一回のUndo/Redo、未変更、無効選択/参照、MSB/LSB/percussionと保存project reloadを9件追加。GUI Assign DLS Collectionは楽器が一つの入力へlocaleを自動割当。複数楽器は明示拒否、選択UIは次の作業。

入力は前版201437359Z GUIで編集・保存したproduct-ui/201700Z/owned.dls（SHA0303ed077421c8d04179b521ebb2544847ce0cbbab8850bb110d0542e94de0fa、bank2/program7、Region key60..111/velocity12..127/group3、PCM50percent）。現行本体でBand patch519、Segment72..84、project保存/reload、owned memory snapshot/GetInstrument/Download/Play/Stop/Unload/generated GUID restartを実行。独立監査scripts/Inspect-DlsPlayback.mjs schema2は入力inshからlocaleを導出し保存Bandとruntime519を照合、入力とruntimeDLS全bytes、Segment flags/GUIDだけの変化、note音域、source hashと生成物を結合。headless24/DLS55に原版40hash一致0。Windows dmime/dmloader/dmusic/dmband/dmsynth/DirectSound/GM.DLSは依存宣言のまま。現行GUI/全機能inventoryは未確認。

失敗203059340Zは編集済みDLSを原版専用coreにも渡したため313件目に固定fixture比較失敗。DLS API519はexit0だがrun全体失敗として保持。Test-ProductSnapshotにDlsPlaybackCollectionを追加し、ReferenceCollectionは原版回帰専用、再生入力は別path/SHAとしてrun.jsonに記録。次の成功を失敗runへ転用しない。

GUI work/acceptance/product-ui/20261002T203400Z/binding-gui-proof.json：操作用コピー4filesは現行native監査済み入力と全bytes一致。画面でpatch0→DLS割当519→Undo0→Redo519、同じprojectのSegmentでPlaying document snapshot→Stopped(segment ended)、閉じた後window不在を確認。7 distinct画像/SHA、状態tree、returned app/exeを記録。UIA一覧が一操作遅れる場合は実画像を確認し、機械監査で画像の数値を読んだとは主張しない。GUIはnative生成済みSegmentを読み込んだため新たなGUI recopy/save/restartは未実施、GUI Undo全bytes/exitcode/modules/audioも未確認。前版のGUI編集Save/restart成功は現行へ転用しない。

音声質問はwork/acceptance/product/20261002T203232589Z/dls-playback/audio-question.jsonで今回APIのpath/SHAへ結合して回答待ち。今回GUI音声は別未確認。ユーザーが再回答した「聞こえた。途中から速くなった」は旧144958360Zピアノ質問への返信であり今回へ流用しない。

再現：scripts/Build-ProductSnapshot.ps1。現行をTest-ProductSnapshot.ps1 -BuildSummaryPath work/build/product-snapshot/20261002T203140584Z/build-summary.json -ReferenceSegment work/producer/samples/QuickStart/heartland.sgp -ReferenceStyleDirectory work/acceptance/product-ui/20261002T180100Z/runtime-input -ReferenceCollection work/producer/samples/FarmGame/FarmGame.dls -DlsPlayback -DlsPlaybackCollection work/acceptance/product-ui/20261002T201700Z/owned.dls。その後Inspect-ProductModules.ps1のhost-smoke/dls-playback-api、Inspect-DlsEditor.mjs、Inspect-DlsPlayback.mjs、GUIはInspect-DlsBindingGui.mjsで保存証拠を監査。

次の具体的変更は複数DLS楽器を明示選択するUIとGUI新規Segmentコピー保存・再起動・再生/modules、続いてDLS CRUD/loop/articulation/pool relocationとgroup-aware tracks・他40責務。全八受入は維持。


## 2026-10-03 複数DLS楽器の選択・GUI Segmentコピー保存と再起動

現行204408500Zは保存50sources/3targetsの構成・compile・install成功。build.logのwarning/error検索は該当なし。EXE SHAf14210ba45c22f775fadd21fdab0c84c07fc16dc39fb18b8e83394443c031c4c、core SHA617a07cac453d3cb68f31814b7c77fa0a72d615ea1547ed95e9af4a3f014cdec。work/acceptance/product/20261002T204527348Z/run.jsonのhost/core/DLS API exit0、native358件。構成・compile・実行・本体受入を分離し、全40責務・全八受入は未完了。

本体のAssign DLS Collectionへ独立した楽器選択画面を実装した。DlsDocumentの全instrumentをbank/program/Region数付きで列挙し、選択indexをFrameworkの一括locale/reference割当に渡す。CancelはBand変更を行わないが、先に開いたcollectionのcatalog所有は残る。空collection/無効indexは失敗にする。従来の一楽器限定拒否を解除した。既存のPChannel/pan/volume・参照解決・一回UndoRedo契約を維持。四件のnative追加は複数楽器index1→777、全bytes UndoRedo、index0→256、依存DLS全bytes不変を確認。元入力の動的な楽器選択操作との比較は未実行。

入力work/analysis/dls-selection-input/multi.dls SHA85e86c728823c34a07eeea1eb85f843315a3ada08823d970a51bf2d105be801bは前版201437359Z GUI編集owned.dlsを基にinsを複製しcolhを2へ変更、第二instrumentをbank3/program9へ変更した合成fixture。同wave poolを共有し、製品の新規作成/CRUD成功とは数えない。Create-DlsSelectionFixture.mjsとmulti.dls.jsonに変換根拠・元hashを保持。native試験の最初のEnum patch519と、選択したBand patch777を別々に照合した。APIのassignedPatchは期待locale値、実呼出しGet assigned instrument S_OKと保存Bandの値を独立照合するもので、別Enumで777を取得したとは主張しない。Inspect-DlsPlayback.mjs schema3で選択instrumentのRegion、全入力/runtimebytes、flags/GUID変化、保存Band、ソース/EXEを照合。headless24/DLS55は原版40hash一致0。

GUI work/acceptance/product-ui/20261002T204700Z/selection-gui-proof.json：同EXEで新規試験directoryのprojectを開き、第一instrument519を割当、再度chooserから第二bank3/program9→777を選択。Band volume100→90を確定・Save、SegmentのCopy Band at clocks0を実行してSave。完全終了後に同EXEの別window/processでprojectを開き、8音/テンポ120→180とBand777/pan64/volume90を復元し両文書を再Save。初回window113968496、再起動52038000。独立RIFF auditorはnative保存Band/Segmentへvolume byteのみ100→90の期待変化を適用し、GUI saved/resaved/current全bytes一致、DLS/project不変を確認。14別hashのcaptureを保持し、再起動後のPlay→Stopped(segment ended)、終了後window不在を確認。GUI process exit codeは未確認。以前203140584ZのGUI UndoRedoは旧版証拠として保持、今回GUI UndoRedoを行ったとはしない。

GUI module captureはGet-Process読み取りで同EXE唯一processから127 pathsを取得。Strict監査work/acceptance/product-ui/20261002T204700Z/gui-module-provenance.jsonはpassed=false。64bit外部inventoryが両ntdllを同System32名で報告しbase addressを保存していないため二つの由来を確定できない。x86 virtual namespaceはSysWOW64で解決したが、この二entryを確定済みとはしない。さらにMicrosoft ink tiptsf.dll、OneDrive FileSyncShell.dllが標準宣言directory外でunresolved。WOW64補助5DLLはinstalled Windows host supportとして別分類。OS拒否・製品再生失敗ではなく外部inventoryの証拠不足と環境integrationであり、strict全依存受入を合格にしない。最初のauditorがwow64.dllをSysWOW64へ機械変換してfile-not-foundになった問題を、存在/PEを検証する処理へ修正。原版回避やsecurity変更はしていない。次に製品内inventoryまたはaddress付きcaptureでこの曖昧さを解消する。

現在の実音声/波形/音色/出力先は未確認。work/acceptance/product-ui/20261002T204700Z/audio-question.jsonに再起動GUI再生時刻と生成物を結び付けた聴取質問を保存し回答待ち。旧ピアノの「聞こえた。途中から速くなった」は今回DLSへ転用しない。Windows DirectMusic/DirectSound/GM.DLS依存は宣言のまま。Style runtime、原版動的比較、CRUD/loop/articulation/pool relocation、group/全通知/COM ABI・他40責務は未完了。

再現：scripts/Build-ProductSnapshot.ps1。Test-ProductSnapshot.ps1 -BuildSummaryPath work/build/product-snapshot/20261002T204408500Z/build-summary.json -ReferenceSegment work/producer/samples/QuickStart/heartland.sgp -ReferenceStyleDirectory work/acceptance/product-ui/20261002T180100Z/runtime-input -ReferenceCollection work/producer/samples/FarmGame/FarmGame.dls -DlsPlayback -DlsPlaybackCollection work/analysis/dls-selection-input/multi.dls -DlsPlaybackInstrument 1。Inspect-ProductModules.ps1をhost-smoke/dls-playback-api、Inspect-DlsEditor.mjs/Inspect-DlsPlayback.mjsを新版run.jsonへ実行。GUIはnative DLS directoryの四filesを新directoryへコピーし上記操作、Inspect-DlsSelectionGui.mjsをGUI directoryへ実行。GUI dependency監査はInspect-ProductGuiModules.ps1、現記録は厳格判定失敗として再現する。各auditor copyと入力・保存物・captureをevidenceに保存。完成用の全体導入手順ではない。

次の具体的一手：GUI依存captureのnamespace/address不足を解消し、今回のDLS聴取回答を当該生成物のみへ記録。その後DLS Region/Wave CRUDとpool relocationをモデル・Framework・本体へ接続して保存/再生/比較する。group-aware tracks・全40責務と全八受入の対象・完了条件は維持する。goal active/incomplete。


## 2026-10-03 DLS Region複製・削除とGUI依存由来の確定

現行210809084Zは保存50sources/3targetsで構成・compile・install成功。build.logのwarning/error検索は該当なし。EXE SHAde0846668dd5a5bc9b40dce28cb853c53e28a4faf51e8843a39cbbc1305d172a、core SHA584f003655221c4497b5bc8d2d9f9fb22ee0198b58601e6a524387105e557855。work/acceptance/product/20261002T211008875Z/run.jsonのhost/core/DLS API exit0、native369件。構成・compile・実行・本体受入を分離し、全40責務・全八受入は未完了。中間210643246Zはcompileのみで実行していない。新テストが既存PCM編集fixtureを上書きする出力設計を、別region-crud directory/Frameworkへ分離してから現行版を作成した。旧成功を新生成物へ転用しない。

DlsDocumentへduplicate_region/remove_regionを追加した。選択rgn/rgn2の全チャンク・未知child/tail/paddingを複製し、inshのRegion countだけを更新する。削除は未知兄弟を飛ばして型付きindexを対象にし、最後のRegion削除後もwave poolを保持する。無効indexは文書/history不変。可変長adoptはptbl/wvpl全bytes不変の場合だけ許し、型付き全instrument/waveを事前検証して一回のUndoへ記録する。pool cueはwvpl内相対offsetなので、lrgnサイズ変化でpool全体がファイル内移動してもcueは変更しない。Waveサイズ変更は依然拒否し、pool relocationは未実装。空Region一覧からの新規作成も未実装。本体DLS editorへDuplicate Region/Delete Regionを接続し、選択と空一覧のcontrol状態を更新する。

追加11件はFarmの全bytes期待値、可変長UndoRedo、無効indexの原子性、複製削除で元bytes復元、最後削除/UndoRedoでwave保持、rgn2/未知兄弟/odd paddingの保持、別FrameworkでDLS/Band/project保存と再読込を確認する。work/acceptance/product/20261002T211008875Z/core/dls-editor/region-proof.jsonは独立RIFFパーサで元Farm SHA615eab9a6eb2eaeadfb7f69afa5d6b53d1a2ce8e4699b4d18c5f1f0994b399dfへRegion全体複製とcount2のみを適用し、native/Framework保存全bytes一致を照合。期待SHA8c10ac40e0f883ef8cace34c622a94ef66b3df153c4a23ca359659f7227f738e。未知articulationは保持するだけで編集互換成功とはしない。原版の動的CRUD操作/相互読込比較は未実行。

再生入力work/analysis/dls-region-input/duplicated.dlsは同独立期待値で作成した二Region合成fixture。製品CRUDの成功はnativeとGUI保存物で別途判定した。bank1/program0 patch256、共通wave cue0、key72..111/velocity0..127。同じrangeの二Regionが実際にどう重なって聞こえるかは未確認。Inspect-DlsPlayback.mjsで全入力/runtimebytes、保存Band/Segment/project、Get assigned instrument S_OK、memory registration/Download/Play/Stop/Unloadとidentity restartを照合。headless24/DLS55は原版40hash一致0。

GUI work/acceptance/product-ui/20261002T211200Z/region-gui-proof.json：native保存Band/Segment/projectをコピーし、owned.dlsは元Farm一Regionへ戻した独立試験入力。同現行EXEでprojectを開きRegion1を複製してRegion2を選択、Save、UndoしてSave、RedoしてSave。PlayからStopped(segment ended)を観測後、完全終了・別起動でprojectを再読込。初回window12191918、再起動9308328。再起動Region dropdownに1/2を観測しRegion2のkey72..111/velocity0..127/cue0を選択、再Save。saved/redo/resaved/current全bytesが上記独立期待値、undo全bytesは原入力、Band/Segment/projectはnative baseline不変。12capture別hashを保存。duplicate/playingの直後UIAは一action遅延したが画面pixelsは確認し、次のsaved treeのRegion2とfinished treeの自然終了を使用。raw observationsを書き換えない。GUI削除は未実行、削除の証拠はnativeのみ。GUI process exit codeは未確認、終了後window不在のみ観測。以前のGUI複数楽器選択/volume変更は旧204408500Z証拠として保持、今回へ転用しない。

Capture-ProductGuiModules.ps1は同EXE唯一GUI processのreadonly127module paths/base addresses/sizeを取得。初回GUIのPlay後・終了前の一点snapshotで、再起動processや全生命周期inventoryではない。低addressの仮想System32を実SysWOW64へ、高address AMD64をWOW64 host supportへPE machine/hashで分離し、二ntdllの曖昧さを解消。Microsoft ink tiptsf.dllとOneDrive26.173.0906.0008/i386/FileSyncShell.dllはAuthenticode Valid、Microsoft Corporation署名subject/thumbprintを保存。環境由来input/shell integrationとして宣言し、製品配布/必須依存とはしない。work/acceptance/product-ui/20261002T211200Z/gui-module-provenance.jsonはpassed=true、118 Windows x86・6 WOW64 support・2 signed Microsoft ambient・1自作EXE、原版40hash一致0。以前204700Zのaddressなしstrict不合格記録は保持。最初の署名結果JSON化でPowerShell自動変数Matchesがregexに上書きされた記録処理の失敗を、originalMatchesへ分離して修正。環境制御/OS拒否を迂回していない。

残る依存：宣言済みWindows DirectMusic/DirectSound/GM.DLS、OS UI/shell/ambient extensions。現行音声は未確認。ユーザーの旧ピアノ「聞こえた。途中から速くなった」は旧144958360Zだけ。旧204700Z DLS聴取質問も当該版に保持し現行へ転用しない。Style runtime/GUI、group対応、loop/articulation、Waveサイズ/pool relocation・PCM import/export・DLS新規作成、他40責務/全八受入は未完了。

再現：Build-ProductSnapshot.ps1。Inspect-DlsRegions.mjs --fixture work/producer/samples/FarmGame/FarmGame.dls work/analysis/dls-region-input/duplicated.dls。Test-ProductSnapshot.ps1 -BuildSummaryPath work/build/product-snapshot/20261002T210809084Z/build-summary.json -ReferenceSegment work/producer/samples/QuickStart/heartland.sgp -ReferenceStyleDirectory work/acceptance/product-ui/20261002T180100Z/runtime-input -ReferenceCollection work/producer/samples/FarmGame/FarmGame.dls -DlsPlayback -DlsPlaybackCollection work/analysis/dls-region-input/duplicated.dls。Inspect-ProductModules.ps1をhost-smoke/dls-playback-api、Inspect-DlsEditor.mjs/Inspect-DlsRegions.mjs/Inspect-DlsPlayback.mjsを現行runへ実行。GUIは上記手順後Capture-ProductGuiModules.ps1とInspect-ProductGuiModules.ps1、Inspect-DlsRegionGui.mjsを実行。各auditor/capture scriptのcopyをevidenceへ保存。GUI再試験は別evidence directory/新snapshotへ記録する。

次の具体的一手：Wave複製とpool cue offset再計算を未知チャンク保持/一回Undoで実装し、Wave入出力と空一覧へのRegion新規作成へ進む。GUI依存address不足が解消したため、細部inventory再試行を続けず本体所有音源編集の統合へ戻る。原版観測/動的比較・音声・group対応は残作業として継続。全40責務と全八受入の対象・完了条件は維持。goal active/incomplete。


## 2026-10-03 Wave複製・cue再配置と本体の保存再起動再生

現行212937659Zは保存50sources/3targetsの構成・compile・install成功、build.log warning/error該当なし。EXE SHA7377201d4932567b3ea5c716d44d4663823bc311b8fe26ad30f6f4eff18a00df、core SHA6183919e74df8451d5ea4ddcab017fc166b73b9960f617fb061af2d6c7f6b6b7。work/acceptance/product/20261002T213115778Z/run.jsonはhost/core/DLS API exit0、381件。全40責務と全八受入は未完了。直前210809084ZのGUI Region複製証拠は旧版として保持。

根拠work/analysis/dls-wave-observation.jsonは原版FarmGame全bytes SHA615eab9a6eb2eaeadfb7f69afa5d6b53d1a2ce8e4699b4d18c5f1f0994b399df、wvpl346/firstWave358/cueoffset0、Producer Wave guidとfmt/data/未知chunkを記録する。インストール済みSDK10.0.26100.0/shared/dls1.hのWAVELINK.ulTableIndex、POOLCUE.ulOffset、POOLTABLE.cbSize/cCuesの行とhashを保持。原版アプリでのWave操作は未実行。MIDI公式の仕様案内を調べたが仕様PDF本文取得は403のため採用しない/回避しない。実装根拠は原入力の静的構造とSDK、動的比較は残作業。

DlsDocument::duplicate_waveは選択Waveの前へcloneを挿入する。新規cueはtable末尾へ追加し、既存cue番号とRegion.wlnkを維持したまま各offsetを再計算。未知pool children/RIFF paddingを含めたencode長でoffsetを算出し、cueの任意順・aliasを維持する。ptblヘッダー拡張/未知tail/padding、元Wave全bytesを保持。cloneのguid/dlidがあれば16bytesをCoCreateGuidで新規発行し、identity拡張tailは保持。曖昧/短いidentityは編集前に拒否。一回Undoで全bytes復元しRedoは同GUIDを復元する。private adoptの旧サイズ拒否を全typed instrument/wave/cue事前検証へ置換して、全編集をhistory変更前に検証する。本体DLS editorにDuplicate Waveを接続、空Wave一覧ではdisabled。PCM入出力/サイズ置換/削除は今回未実装。

追加12件は新GUID/元Wave不変/clone全bytes、既存Region cue番号維持/offset relocation、UndoRedo、invalid index、複数Wave/逆順alias cue/opaque pool child/odd padding/extended ptbl headerとtailの全bytes保持、短いGUIDのatomic rejection、複製PCM50%と新cue1のFramework再生snapshot/保存/別Framework復元。新しい再生試験オプションDlsPlaybackCoreWaveは成功したcurrent coreのwave-crud/source.dlsを直接API入力にする。別合成fixtureの成功へ置換せず、当該製品が保存した音源を試す。core failureなら依存APIを実行せず、未生成file hashも補完しない。current inputとruntime全bytesをInspect-DlsPlayback.mjsで照合し、memory registration/Download/Play/Stop/Unloadとruntime identity再起動が成功。native headless24/DLS55原版40hash一致0。最初の監査呼出しは誤ったRunJsonPath paramと先行module evidence欠落で停止したが、正しいRunPathでmodule監査後にdependent再生監査を実行。製品再実行は不要で、OS拒否ではない。

独立Inspect-DlsWaves.mjsは元Farmの一Waveをパースしてcloneを挿入、cue0を元Waveへrelocateしcue1をcloneへ追加、clone.guid16bytesだけ新生成値を採用する。他の全bytesは独立期待値。編集後はcloneの16bit PCM全sampleに50%/0方向切捨てとRegion cue1だけを適用し全file照合。native clone/edited/Framework保存、API実入力まで連結。native clone SHA6494a727091881518b31f3b4ca61b0e2e633f852c09b0dcfc32156f7cbf7fe4b、edited SHAf6132224ac9247b901a51a9c263590fe9e088be5ce1a5997a65309b6437562c5。

GUI work/acceptance/product-ui/20261002T213300Z/wave-gui-proof.json：同現行EXEでnative Band/Segment/projectと元Farm一Waveのowned.dlsを入力にする。Duplicate Wave→Save、Undo→Save、Redo→Save、選択clone Wave1のPCM50%をScale、Region cue1をSet、Save。本体を完全終了し、別起動でprojectを開く。初回window7211360、再起動55249468。Region cue1、Wave dropdown1/2、元Wave2を観測し再Save。duplicate/redo全bytes一致、undoは元Farm全bytes、edited saved/resaved/currentは独立期待値全bytes一致、Band/Segment/projectはnative baseline不変。GUIDはnativeとGUIで別発行し、それぞれ同じ操作内のUndoRedo/restartで維持する。16captureを保持。GUI編集後SHA1b12607ae5a565d4b346610cda56a7573918149c68296cf472b1fd502d25109a、newGUID60e2884deeb4284b841b6b4f4a6f3123。再起動後PlayからStopped(segment ended)を観測、終了後window不在。GUI process exit code/音量の実測/音色/出力先は未確認。work/acceptance/product-ui/20261002T213300Z/audio-question.jsonにactual playing UTC2026-10-02T21:37:54.998Z、日本時間6時37分55秒頃（質問では6時38分頃）と当該buildを結び付けて回答待ち。旧ピアノ/旧DLS回答は転用しない。

GUI初回編集終了前105 address modulesをgui-modules-first.jsonへ保持。厳格監査対象は再起動後Play終了時の127modules（gui-modules.json、gui-module-provenance.json）。同現行EXE/PE/hash/namespace/base address/署名を照合し原版40hash一致0、Windows x86/WOW64 support・Microsoft署名済ink/OneDrive ambientを識別。一点snapshotで全生命周期ロード保証ではない。OS DirectMusic/DirectSound/GM.DLS・OS UI/shellの依存は宣言のまま。原版固有COMへfallbackしていない。

再現：Build-ProductSnapshot.ps1。Test-ProductSnapshot.ps1 -BuildSummaryPath work/build/product-snapshot/20261002T212937659Z/build-summary.json -ReferenceSegment work/producer/samples/QuickStart/heartland.sgp -ReferenceStyleDirectory work/acceptance/product-ui/20261002T180100Z/runtime-input -ReferenceCollection work/producer/samples/FarmGame/FarmGame.dls -DlsPlayback -DlsPlaybackCoreWave。Inspect-ProductModules.ps1 -RunPath work/acceptance/product/20261002T213115778Z/run.jsonをhost-smoke/dls-playback-apiへ、その後Inspect-DlsPlayback.mjs/Inspect-DlsWaves.mjs/Inspect-DlsEditor.mjs/Inspect-DlsRegions.mjsへ同run。GUIは上記操作で別directory記録、Capture/Inspect-ProductGuiModules.ps1、Inspect-DlsWaves.mjs --guiを使用。auditorとcapture script copyを証拠に保持。旧大量互換比較や無関係なStyle runtimeを繰り返していない。

次の具体的一手：PCM WAV入出力とWaveサイズ置換に同cue relocationを拡張し、loop boundsと未知metadata整合を検証する。空Regionからの新規作成、Wave削除時の参照整合、loop/articulation・group-aware tracks、原版動的比較・現行聴取、他40責務と全八受入を継続。全体の対象・完了条件を縮小せず、goal active/incomplete。


## 2026-10-03 PCM WAV入出力を文書所有・保存復元・再生へ接続

現行214943975Zは保存50sources/3targets、構成・compile・install成功。EXE SHAfc78f05be7bc1132ef1730b1f1f6d83037a557793f78e4cbfe3d24d476c9697b、core SHA83ce7378b9af621e6cd7c33695078456a2016ee55564206df217025c4c935eb0。work/acceptance/product/20261002T215048185Z/run.jsonはhost/core/DLS API exit0、395件（追加14）。build.log warning/error該当なし。最初の214752021Zは構成/build/install成功だが実行せず、Framework保存復元とcurrent core PCM生成物直接再生オプションを追加した214943975Zを採用。前版GUI212937659Zの成功は今回へ転用しない。全40責務/全八受入は未完了。

根拠work/analysis/dls-pcm-io-observation.jsonは原版Farm入力SHAと既存Wave静的観測、インストール済みSDK shared/mmreg.h hashを保持。最初のum/mmreg.h探索は不存在、sharedの実在ヘッダーを読取。原版アプリのWAV import/export操作は未実行。Producer wavh/wavu等のサイズ依存仕様は未確定のため、サイズ変更を無検証で受け入れずPCM交換を先に接続。サイズ変更/loop/その他対象は残作業として維持。

DlsDocument::export_wave_pcmは選択Waveのfmt/dataを元payload/paddingごとRIFF WAVEへ出力し文書/履歴を変更しない。DLS identity/loop/articulation等はcollection所有のまま。import_wave_pcmはRIFF WAVEと一意fmt/data、PCM8/16、channels/rate非zero、blockAlign/byteRate整合・overflowを検証。先頭16format bytesとdata長が元Waveと一致する入力のみsamplesを置換し、全DLS metadata/GUID/cueoffset/paddingを保持。一回UndoRedo。入力WAVのancillary chunkはDLSへ追加しない。形式/長さ変更は理由付き拒否、対応完了ではない。UI Import PCM WAV/Export PCM WAVと標準Open/Save dialog、overwrite prompt、cancel/no-op、8/16 PCM有効化を接続しcompile。今回UI操作は未実行。

追加14件はfmt/dataだけのexact WAV、export/no-op importのhistory不変、PCM50%全bytes、UndoRedo、長さ/format/byteRate/重複data/非WAV拒否とatomicity、ancillary保持方針、8bit unsigned stereo、Framework Band snapshot/保存projectを別Frameworkへreloadしてexact DLS/WAV復元。work/acceptance/product/20261002T215048185Z/core/dls-editor/pcm-io/original.wav SHA712089fdf9a0a8f5c18c022f7573f171e73a3bd163a1b40b024130bfd191a13e、scaled.wav SHAb661b1b445619c01da93222707fe2e610cb3c89fb876ae2fb5cf9a9ced425920、imported.dls SHA3a60a9663a44a4136e8a659c7b5daf6e38a224ba050c68adb7d1239790940e5c。

Test-ProductSnapshot -DlsPlaybackCorePcmは成功したcurrent coreのimported.dlsを直接API入力へ使用。CoreWave/外部入力との混用は事前拒否、core失敗なら依存APIを実行しない。native headless24/DLS55 module由来passed、原版40hash一致0。Inspect-DlsPlaybackはexact入力/初期runtime全bytes、generated identity変更範囲、patch256、Download/Play/Stop/Unload/restartを検証。独立Inspect-DlsPcmIoは原入力RIFFからfmt/dataだけのexport、16bit全samples50%のimport期待値と全file bytes、actual API入力hashを検証。最初はdependent API proofのfieldをpassedと誤認し監査停止。実在apiPassedへ修正し同証拠を再監査成功、製品再実行なし。Wave/Region/既存PCM独立監査も今回395で成功。GUI/audio/原版動的比較/full lifecycleモジュール一覧は未確認。旧GUI音声質問は旧版に保持。

再現：Build-ProductSnapshot.ps1、その後Test-ProductSnapshot.ps1 -BuildSummaryPath work/build/product-snapshot/20261002T214943975Z/build-summary.json -ReferenceSegment work/producer/samples/QuickStart/heartland.sgp -ReferenceStyleDirectory work/acceptance/product-ui/20261002T180100Z/runtime-input -ReferenceCollection work/producer/samples/FarmGame/FarmGame.dls -DlsPlayback -DlsPlaybackCorePcm。Inspect-ProductModules.ps1 -RunPath work/acceptance/product/20261002T215048185Z/run.jsonをhost-smoke/dls-playback-apiへ、後でInspect-DlsPlayback.mjsとInspect-DlsPcmIo.mjsへ同run。proof/auditor copyを出力directoryに保持。

次の一手は今回GUI PCM export/import/Save/別起動復元と由来、続いてwavh/smpl/wsmpのサイズ/loop観測と可変長PCM置換・cue relocation。空Region新規作成、Wave参照付き削除、articulation/group-aware tracks、原版比較/音声、他全40責務・全八受入を続ける。goal active/incomplete。


## 2026-10-03 現行PCM WAV入出力の本体GUI保存・別起動・再書出し

同214943975Z EXE SHAfc78f05be7bc1132ef1730b1f1f6d83037a557793f78e4cbfe3d24d476c9697b、native395/run215048185Zの生成物を使用し、work/acceptance/product-ui/20261002T215515Z/pcm-gui-proof.jsonで本体受入の不足証拠を追加。ソース50と生成物を再照合し変更なし。native成功を別版へ転用せず、GUIでも実際の標準WAV dialogとDLS文書所有を操作した。今回は必要なGUI受入・監査を追加し、無関係な旧試験やnativeを再実行していない。全40責務/全八受入は未完了。

入力は同runのBand/Segment/projectを分離directoryへcopyし、owned.dlsは原版Farm一Wave（PCM未編集）へ置換。input-scaled.wavは今回nativeが出力したPCM50% WAV。File Open project→DLS editor→Export PCM WAVの新pathへexport-original.wav→Import PCM WAVでinput-scaled.wav→Save→Undo/Save→Redo/Save。本体を終了し、最初のwindow20908578消失を確認。別起動window101255204で同projectを再読込み、DLS Wave1/cue0/81438framesを観測。Export PCM WAVへexport-restarted.wav、Save DLSへresaved.dls。元WAV書出しは独立native fmt/data期待値SHA712089fdf9a0a8f5c18c022f7573f171e73a3bd163a1b40b024130bfd191a13e、復元WAVはinput-scaled全bytes SHAb661b1b445619c01da93222707fe2e610cb3c89fb876ae2fb5cf9a9ced425920と一致。saved/redo/resaved/owned DLSは独立native import期待値SHA3a60a9663a44a4136e8a659c7b5daf6e38a224ba050c68adb7d1239790940e5cに全bytes一致し、Undoは原版Farm全bytesへ一致。Band/Segment/project bytes不変。

15distinct screenshotと返却window/UTC/UIA treeをstates.jsonへ保持。最初のFile UIA indexはoutside bounds、file dialog set_valueはcached element unavailableで失敗。再観測してFile位置のpixelsとfilename caret/Alt+nを用い、盲目的な同条件retryは行わない。入力直後のUIAは1操作遅れることがあり、saved screenshot/実ファイル/次観測で照合した。OS起動拒否やセキュリティ変更はない。未編集のdefault空文書のみDiscardして試験projectを開き、既存変更を保持。

再起動後PlayはPlaying pixels→Stopped(segment ended)を実観測。playing UTC2026-10-02T22:01:40（日本時間7:01:40頃）。最初の質問7:03は誤った概算のため7:01:40と訂正して質問、audio-question.jsonはanswer null/旧回答非転用。実音/endpoint/音色/音量は未確認。GUI終了後windowなし、process exit code未確認。

再起動後Play終了の132address modulesをCapture-ProductGuiModules→Inspect-ProductGuiModulesで由来確認、原版40hash一致0。同EXE/hash/PE/addressとWindows x86/WOW64/Microsoft署名ambient判定により監査passed。一点snapshotで全生命周期ロード保証ではない。OS runtime/UI/shellの宣言依存は維持。独立Inspect-DlsPcmGui.mjs work/acceptance/product-ui/20261002T215515Zはcurrentbuild/run/exe/nativeproof/source50 hash、GUI全file/capture/lifecycle/依存proofを検証しpassed。auditorとcapture/inspect copiesをdirectoryに保持。

次の一手：原版wavh/smpl/wsmpのサイズ/loopを観測して可変長PCM置換とcue relocation/loop boundsを実装する。空Region作成/Wave参照付き削除、articulation/group-aware tracks・他40責務、原版動的比較・current聴取と全八受入を継続。全体goal active/incomplete。


## 2026-10-03 可変長PCMを本体文書・loop境界・cue再配置・保存復元へ接続

現行221251264Zは保存50sources/3targetsの構成・compile・install成功。EXE SHA757399c4b98d7e6945e2bed0948e7c62fc68b64ed6becf2dc877c09bde12f3a2、core SHA34d299f64d97f79664619b0429b630373caac4dcaeebf5de4069f2e1a69bc6fd。work/acceptance/product/20261002T221432238Z/run.jsonはhost/core/DLS API exit0、native410件（追加15）、build.log warning/error該当なし。work/acceptance/product/20261002T221432238Z/core/dls-editor/pcm-resize/pcm-resize-proof.jsonとwork/acceptance/product-ui/20261002T221600Z/pcm-resize-gui-proof.jsonを別々に監査passed。旧395/GUI132/聴取記録はprevious-product-state.jsonを含め別版として保持し、新版へ転用しない。全40責務/全八受入とgoalはactive/incomplete。

観測work/analysis/dls-pcm-resize-observation.jsonは原版Farm SHA・Wave/Region smpl/wsmp/wavh/wavuの全payloadとSDK dls1.h/既存dmusicf.h/原版helpのhashを保持。Farm Wave wsmpとRegion overrideはstart29198+length23176、smplはinclusive end52373、最小52374frames。DMUS_IO_WAVE_HEADERはrtReadAheadとdwFlagsでframe数を持たないので16byte観測の未知tailも保持。SMPLのinclusive endはMicrosoft DirectXTK WAVFileReader.cpp（https://raw.githubusercontent.com/microsoft/DirectXTK/main/Audio/WAVFileReader.cpp）と照合。原版アプリの長変更import動作とwavu意味は未確認。

import_wave_pcmは同PCM8/16 format・完全なnonempty framesで異なる長さを許可。サイズ変更前にWave wsmp/smplと、cue aliasを解決した全参照Regionのwsmp/smpl loopを検証。WSMPL cbSizeと各WLOOP cbSizeによるheader/stride拡張を尊重し未知tail保持、uint64 start+lengthでoverflow防止。loop位置を切り落とす縮小や短い/曖昧metadataはhistory/bytes変更前に拒否。fmt/identity/loop/articulation/Producer chunks/paddingは全保持。fact/cue/plstのサイズ依存metadataは仕様未実装のため明示拒否。loop編集/format conversion/compressed resize/未知metadata全種の意味上整合保証は未実装。PCM変更後に全pool child encode長でptbl offsetsを再計算し、cue番号・逆順/alias・table拡張/tail・opaque pool paddingを保持。

追加15件は81438→81440frames延長、52374frames境界短縮、52373拒否、UndoRedoと拒否後history/bytes、empty拒否、Wave loopを除いたRegion overrideのextended header/strideと独立bounds、2Wave逆順/alias/opaque odd padding/extended ptbl全bytes、malformed loop stride拒否、Framework Band dependency snapshotと保存project別Framework reload。独立JS RIFF監査は原Farmから延長/短縮/2Wave alias入力と再配置全bytesを算出。grown DLS SHA79b19dbda5bc4206516baa5eba14d07480f931214c1cac24cc01bbf2dbdf1ab9、WAV SHA9d550d6251f5ae5f7c818f0924c83ebc76a61a355887a5de9528a31ada11f00c、shrunk DLS SHA8901a06258c5064eb594480ce32e9bf2bea02133bf7888bf8d7404c2b0cb4921、multi SHA2fb4e615737059a402225bc30df394243da0cc18480e56c95ecb86d8b62633e3。-DlsPlaybackCoreResizeは成功した同run coreのgrown.dlsを直接API入力へ使用し、Download/Play/Stop/Unload/restartとruntime全bytesを監査。native24/55由来passed、原版40hash一致0。

GUIは同EXEでnative Band/projectと元Farmを分離directoryへcopyし、input-grown.wavを実際の標準dialogから読込み→Save→Undo/Save→Redo/Save。本体window15403182を通常終了し消失確認、別起動window162268742でproject再読込み、DLS81440frames/cue0を観測、Export PCM WAVへexport-restarted.wav、DLS再保存。saved/redo/resaved/grown全bytesは独立native期待値、undo全bytesは元Farm、export全bytesはgrown.wavに一致。Band/project全bytes不変。11 distinct JPEG captures/UTC/window/UIAをstates.jsonへ保持。最初のforward slashパスは標準dialogで無効、Windows区切りに修正。Modal UIA unavailable/終了index geometry unavailableは再観測後pixels/keyboardへ変更、editor初回検出raceはfresh list後選択。同条件input retry/OS拒否回避なし。保存時画像JPEG拡張子へ修正してhash再監査。GUI module監査の最初はstates未保存で停止し、states保存後同captureを監査成功。

再起動DLS editor時109address modulesの由来passed、原版40hash一致0。一点snapshotで全 lifecycleではなく、今回GUI projectはBand/DLSのみでPlay未実行。音声/endpoint/音量・GUI process exit code・原版動的比較は未確認。Windows runtime/UI/shell等の宣言依存は維持。

再現：Build-ProductSnapshot.ps1。Test-ProductSnapshot.ps1 -BuildSummaryPath work/build/product-snapshot/20261002T221251264Z/build-summary.json -ReferenceSegment work/producer/samples/QuickStart/heartland.sgp -ReferenceStyleDirectory work/acceptance/product-ui/20261002T180100Z/runtime-input -ReferenceCollection work/producer/samples/FarmGame/FarmGame.dls -DlsPlayback -DlsPlaybackCoreResize。その後Inspect-ProductModules.ps1 -RunPath work/acceptance/product/20261002T221432238Z/run.jsonをhost-smoke/dls-playback-apiに実行し、Inspect-DlsPlayback.mjs、Inspect-DlsPcmResize.mjsへ同run。関連Editor/Region/Wave独立監査も410でpassed。GUIは上記実操作で別directoryへ記録、Capture/Inspect-ProductGuiModules.ps1、Inspect-DlsPcmResizeGui.mjs work/acceptance/product-ui/20261002T221600Z。auditor/script copy保持。

次の具体的一手は空Regionからの新規作成を本体/Framework/UI/save/reloadへ接続する。続いて参照拒否付きWave削除とalias relocation、loop点編集/articulation/group-aware tracks、現行GUI再生/実音・原版動的比較、他40責務と全八受入を継続。対象・全体完成条件を縮小しない。


## 2026-10-03 空Region新規作成を本体・Framework・保存復元へ接続

現行223434780Zは保存50sources/3targetsの構成・compile・install成功。work/acceptance/product/20261002T223544561Z/run.jsonはhost/core/DLS API exit0、native425件（追加15）。EXE SHA1fef83745e2628a72c6fdb6cc700475d6263c9ab803eb2c7e8776b65b34dc299、core SHA11a3ce83738995a2dc4529cf3fadb5f839208de61dc66c461b9af5a9e71de88e。独立Region全bytes監査、関連Editor/Region監査と製品生成Region再生API監査passed。native24/55・GUI104address modules由来passed、原版40hash一致0。WindowsランタイムとUI/shell等の宣言依存は維持。旧410/音声の成功を新版へ転用しない。全40責務・全八受入とgoalはactive/incomplete。

観測work/analysis/dls-region-create-observation.jsonにFarm/SDK dls1.h/原版helpのhashとRegion/Wave sample payloadを保持。原Region root85をコピーせず、選択Wave root60のwsmp（未知extension/padding/loopを含む）を新Regionに使用。rgnh12bytes keys0..127/velocity0..127/options0/group0、wlnk12bytes/channel1/cue0、lrgn appendとinsh count更新だけ。Wave wsmpなしはroot60 one-shotを明示生成。mono PCM8/16に限定し、不正ranges/group/cue/instrument/loop/malformed wsmpをhistory/bytes変更前に拒否。alias cueと未知odd Region list/insh tailを保持。layer/重複range規則、stereo placement/compressed/articulation/new instrument/new collectionは未実装。

最初のbuild223211381Z/run223328141Zは構成・build・install成功、core366件後に保存project reloadのBand snapshot不一致で失敗し、依存APIは未実行。work/analysis/dls-region-create-first-failure.jsonへ原因・hash保持。DLS別名保存後もBand filenameが旧empty.dlsを指していた。新規Region機能単位の試験を同じsource.dlsパスの編集保存へ修正して新しいbuildを作成し、425件成功。SaveAs問題自体は未修正であり、次の本体文書管理作業としてWave削除より優先する。

GUI work/acceptance/product-ui/20261002T223650Zは同EXE・nativeの空Region/Band/projectを分離copyし、Create Region→Save→Undo/Save→Redo/Save→通常終了→別起動project reload→Region1/range0..127/cue0復元→再保存。saved/redo/resaved/source全bytes SHA3e3ece5ca81d4c22405a472a3f530abe5b36549da50c2b9e27c81c23b9fed36b、undo全bytes SHAc3047a7c9752d9e2cd6541c3ed51de1aeb464c023866c02be58353c3ecae0124に一致、Band/project不変。10distinct JPEGを保持。capture helperが古い配列をclosureに保持して9件metadataが欠落したため、画像mtimeと実際の返却window対象から明示復旧しstates.jsonに制限を記載。欠落raw UIAを捏造せず、再起動属性はその時点のtool output観測として区別。GUI監査は全ファイル/hash/保存bytes/別window/終了消失/module由来を検証。最初の監査失敗はmetadata欠落、OS拒否ではない。index132がcached modalに無い場合は再観測して画像座標へ変更した。GUI exit code/Play/音声と原版動的比較は未確認。

再現: Build-ProductSnapshot.ps1、Test-ProductSnapshot.ps1 -BuildSummaryPath work/build/product-snapshot/20261002T223434780Z/build-summary.json -ReferenceSegment work/producer/samples/QuickStart/heartland.sgp -ReferenceStyleDirectory work/acceptance/product-ui/20261002T180100Z/runtime-input -ReferenceCollection work/producer/samples/FarmGame/FarmGame.dls -DlsPlayback -DlsPlaybackCoreRegion。Inspect-ProductModules.ps1 -RunPath work/acceptance/product/20261002T223544561Z/run.jsonのhost-smoke/dls-playback-api、Inspect-DlsPlayback.mjs、Inspect-DlsRegionCreate.mjs、関連Inspect-DlsEditor.mjs/Inspect-DlsRegions.mjsへ同run。GUIは上記実操作とCapture/Inspect-ProductGuiModules.ps1、Inspect-DlsRegionCreateGui.mjs work/acceptance/product-ui/20261002T223650Z。保存auditor copy保持。

次はcollection SaveAsのBand filename更新とdirty/Undo/失敗時原子性、Segment/Style内コピーの参照を扱う。本体文書の保存復元を妨げる実際の失敗が根拠。続いてWave削除/loop編集、GUI再生/音声、原版比較、全40責務/全八受入を進める。対象範囲と全体完成条件は維持。


## 2026-10-03 DLS別名保存と依存文書の参照更新

現行230152798Z保存50sources/3targetsの構成・compile・install成功。work/acceptance/product/20261002T230304051Z/run.json host/core/DLS API exit0、native449件（追加24）。EXE SHAfc1034a86349c48f0e02dfe1766e1f1a4a4a9b0e798bc991f4d1d2bb8715b84f、core SHA5d7f24ee8218d9757038b2daa41d91c5c3ce072ebf0599fb387427248d8106bb。独立RIFF全bytes監査、同run別名保存DLSの再生API、GUI監査passed。native24/55・GUI104address modules由来passed、原版40hash一致0。全40責務・全八受入とgoalはactive/incomplete。旧425や旧音声成功は現行へ転用しない。

原版helpのBand SaveAs記述とFarmのhashをwork/analysis/dls-saveas-observation.jsonに保持。原版でのDLS SaveAs参照伝播の動的挙動は未観測。自作Frameworkの保存復元不整合を解消する契約として、所有Collectionの移動先とBand/Style/Segmentの新bytesを事前検証し、DLS書出し成功後のみ一括で文書所有状態へ反映する。active filenameは新相対pathだけ更新し、GUID/refh未知tail/opaque chunk/odd paddingは保持。GUID一致で相対pathが文書directory外ならGUID-onlyに移しinactive filenameは保持、filename-onlyで外なら出力前に拒否。GUID-onlyは維持。Band自身のSaveAsも元の解決contextから新directoryへ相対参照を再計算する。

変更されたBand/Style/Segmentは一回のUndo/Redo単位となりdirty、DLSは保存checkpoint更新、projectはdirty。Style参照Segmentには更新Style cacheを再配布しSegment raw bytesは変えない。依存文書を自動でdisk上書きしない。dirty文書をSave Documentで保存してからproject保存する。別Frameworkでproject再読込とDLS編集snapshotを確認。失敗時にpath/history/checkpoint/bytes不変、元DLS不変、同名所有衝突・GUID不一致・directory外filename-only・missing parentを拒否。複数ファイルdisk transaction、未open外部文書、過去Undo snapshotの参照伝播、reparse alias、Style/Segment自身のSaveAsでの全参照再配置は未完了。

最初の225333322Z/run225438283Zは447件成功だがDLS API exit1。filename-only音源を未保存Segmentにassignしたためdirectory contextを持たず拒否されたと、既存guardと生成物から推定。OS拒否ではない。mainのAPI試験でSegmentを先に保存する手順に修正し225558363Z/run225727694Zは447件/API成功。Style cacheとbefore証拠を追加した225908735Z/run230027964Zは449件/API成功。最後にDLS GUI Save DLS Asボタンを追加した現行だけをGUI試験。中間版はnativeの成功として別記録を保持。

GUI work/acceptance/product-ui/20261002T230350Z は同版native before project/old.dls/3文書/style-songを分離copyし、DLS Scale50→Save DLS As renamed.dls→song/Band/StyleをSave Document→project.dmpj保存→通常終了→別起動project再読込→DLS renamed.dls表示→Save DLS再保存→通常終了。9ファイル（元DLS/新DLS/再保存/3文書/style-song/前後project）全bytesが現行native期待値と一致。13JPEGと都度metadata/raw UIA保持。元DLS SHA27b9797cf2f20ef30c902b64803ab32a287c797c122cf794bb18ec774f51b960、新/再保存DLS SHAe5962ea224e2858384a2f25935b142395d7938c3bfabef75fa53b4d476d841a1。GUI依存3文書の再保存は初回のみ、再起動後はDLS再保存。UIAが一操作遅れる場合は再観測、menuの不正座標は画面再観測後に対応、終了直後のlist raceは新しいlistで消失確認。metadataを推測で補完していない。GUI Play/音声/終了code・原版動的比較は未確認。

再現: Build-ProductSnapshot.ps1。Test-ProductSnapshot.ps1 -BuildSummaryPath work/build/product-snapshot/20261002T230152798Z/build-summary.json -ReferenceSegment work/producer/samples/QuickStart/heartland.sgp -ReferenceStyleDirectory work/acceptance/product-ui/20261002T180100Z/runtime-input -ReferenceCollection work/producer/samples/FarmGame/FarmGame.dls -DlsPlayback -DlsPlaybackCoreSaveAs。Inspect-ProductModules.ps1へ同runのhost-smoke/dls-playback-api、Inspect-DlsPlayback.mjs/Inspect-DlsSaveAs.mjsへ同run。GUIは上記実操作、Capture/Inspect-ProductGuiModules.ps1、Inspect-DlsSaveAsGui.mjs work/acceptance/product-ui/20261002T230350Z。auditor copy保持。監査の拒否出力absence pathをcore/outside.dlsへ訂正し同runの監査のみ再実行、native実行は繰返していない。

次はWave参照付き削除/alias cue再配置とloop編集を文書所有・保存復元へ接続する。Style/Segment自身SaveAsの参照再配置・履歴整合、GUI再生/音声、原版動的比較、全40責務/全八受入も継続。対象と全体完成条件は縮小しない。


## 2026-10-03 Wave削除・明示置換を本体文書所有と保存再生へ接続

現行232131882Z保存50sources/3targets構成・compile・install成功、warning/error一致なし。work/acceptance/product/20261002T232312195Z/run.json host/core/DLS API exit0、native471件（追加22）。EXE SHA7470f83f2efa4377eff8526a5e313c9e036cd325550334327e421a9bf2b519f6、core SHA337615d3b75316cabcd62d054ab5912a6fa0f31452d0811f8c1f4381667d30a9。独立全bytes監査と同run生成deleted.dlsの再生APIpassed、native24/55由来passed、原版40hash一致0。現行GUI/音声/原版動的比較は未確認。前版230152798Z GUI SaveAsの成功はその版だけに保持。全40責務・全八受入、goalはactive/incomplete。

観測work/analysis/dls-wave-remove-observation.jsonはFarmと原版help hashを保持。helpはWave Replaceが利用する全Regionへ反映されると記述し、原版Deleteのdialog/方針は未観測。自作契約としてremove_wave(index,optional replacementCue)を実装。参照がある削除は明示surviving cueなしで拒否。参照Regionのwlnkを置換cueへ向け、削除Waveの全aliasをptblから除き、残cueの順序を保持して全Region/rgn2 indexを圧縮し、全残Wave offsetを再配置。置換はPCM8/16のchannels/rate/depth/byteRateを維持、Region wsmp/smpl loop境界を検証。Region sample/articulation・未知chunk・pool opaque child・ptbl拡張header/tail/padding・他Wave bytesは不変。Regionの暗黙削除は行わない。参照のない最後のWaveは空poolにできる。

native試験は無参照削除/参照先明示置換、reverse aliasと複数Region/rgn2、未知odd bytes全一致、saved checkpointに対するUndoRedo、自己置換/不存在cue/破損pool/rate違い/loop破壊のatomic拒否を確認。本体Framework所有Collection→Band/Segment playback snapshot→same-path DLS/project保存→別Framework再読込で全bytes一致。APIはそのrunで生成したdeleted.dlsを直接使いDownload/Play/Stop/Unload/identity restart。入力は2同一Waveの片方を削除し残Regionをcue0へ戻すため結果Farm全bytesと一致するが、旧API成功を転用せず現行EXEで実行した。

再現: Build-ProductSnapshot.ps1。Test-ProductSnapshot.ps1 -BuildSummaryPath work/build/product-snapshot/20261002T232131882Z/build-summary.json -ReferenceSegment work/producer/samples/QuickStart/heartland.sgp -ReferenceStyleDirectory work/acceptance/product-ui/20261002T180100Z/runtime-input -ReferenceCollection work/producer/samples/FarmGame/FarmGame.dls -DlsPlayback -DlsPlaybackCoreDelete。Inspect-ProductModules.ps1へ同run host-smoke/dls-playback-api、Inspect-DlsPlayback.mjs/Inspect-DlsWaveRemove.mjsへ同run。auditor copy保持。

GUI Wave削除/置換選択は未実装・未実行。Wave Track/外部文書の参照伝播は未実装。次は本体DLSエディターへ削除/置換選択を接続し、保存/別起動を検証する。続いてloop編集、Style/Segment自身SaveAsの参照再配置/履歴整合、現行GUI再生/音声・原版比較・全40/全八受入。対象・全体完成条件は維持。


## 2026-10-03 Wave削除GUI接続候補232641307Z・確定前の引継ぎ

work/build/product-snapshot/20261002T232641307Z/build-summary.json保存50sources/3targets構成・compile・install成功、warning/error一致なし。work/acceptance/product/20261002T232742232Z/run.json host/core/deleted-DLS API exit0、native471件、独立削除全bytes/API監査passed。EXE SHAe6ea133c0810903ae20658b6dd6d8df4ba4f4fe1d0a6761f1216f5d337f65f9b、core SHAa806edae9d1b55a783b503dca2c240a18906fa3653af3c03e77cd9d659e4aed7。native24/55と削除確認前GUI104由来passed、原版40hash一致0。前版232131882Zとは異なる生成物で再実行した。

DLSエディターへDelete Wave...、Replace references with cue checkbox、独立cue入力を追加。置換は初期off、cue欄disabled。削除操作で文書copyを検証し、失敗なら確認前に拒否、取消ならbytes/history不変、Yesだけでprepared documentを所有文書へ反映する。確認はdefault No、Wave番号とcue、保存/Undo説明を表示。増設分の高さを拡げ他controlsの位置を調整。

GUI work/acceptance/product-ui/20261002T232850Z は同run two.dlsをsource.dlsとして、Band/song/before projectをproject.dmpjとして分離copy。現行EXE本体window28903864、DLS editor58525352。projectを開き、Wave1・Region cue0・2Wave入力、replacement checkbox on/cue1を設定しDelete Wave確認を表示。Yes未送信、入力source.dls全bytesはnative two.dlsと一致。4JPEG/metadata/raw UIA・states.json保持。確認時のUIAは1action遅れたが画像modalを観測。GUI全保存/再起動試験の合格とはしない。

Computer Use SKILL.mdの参照先confirmations.mdはAlways Confirm at Action-TimeのDelete data（local via app）を明記するため、GUI最終Yesの直前に試験用コピーWave1のみ削除/参照cue1置換の確認をユーザーへ要求した。許可回答はまだ未観測。goalはactive/incomplete、OSblockではない。確定前dialogを残した。再開時はsky.list_windows/get_window_stateで現在状態と回答を照合し、古いindexを再利用せずYes→Save DLS→全bytes比較→通常終了→別起動project/DLS復元→再保存→module由来を実行する。Undo/Redoはnativeで検証済み、GUIで削除を再適用するRedoはその時点の規則に従う。未承認なら削除確定を行わず、loop編集・Style/Segment自身SaveAs・履歴整合などを続ける。全40/全八受入/音声/原版比較未完了。

2026-10-03: loaded Segment group inventory

Current Conductor exposes segment_track_group(type,groups,index) using the frozen SDK Segment GetTrack/GetTrackGroup ABI. It releases the returned owned reference on all result paths. Playback remains a whole-document snapshot; editor selection does not filter the played tracks.

Build025721417Z/run025830354Z loaded the previous GUI saved seven-track fixture byte-exact, queried23 type/mask combinations, and verified Play/Stop. The independent RIFF auditor verifies group membership per type/mask as a multiset and retains the actual order separately. Sequence returned1/2/6; Band returned6/2/1 for RIFF1/2/6, including reversed mask2 Band6/2 versus RIFF2/6. Do not treat editor RIFF ordinal as a runtime ordinal. The cause and original Producer behavior remain unverified. No additional runtime-track absence or note/Band output is asserted. The new segment_meter index argument is compiled but unexecuted.

Evidence: work/acceptance/product/20261003T025830354Z/group-playback/group-playback-proof.json and group-playback-module-provenance.json (55modules, original40hash matches0). Current GUI/audio unexecuted. Next: observe Band runtime order/position/configuration and original behavior, then define stable mapping before selected parameter synchronization.

2026-10-03: selected Tempo/TimeSig parameters (build031059939Z)

Conductor::segment_tempo(time,groups,index) uses GUID_TempoParam on the loaded Segment. segment_meter now has an executed index path. The group smoke compares every source Tempo/TimeSig event against Segment GetParam at that event time; it supports nested LIST TIMS without changing document bytes. Tempo indices0/1/2 and TimeSig0/1 matched in the indexed edited fixture. Independent raw RIFF oracle verifies18 parameter records,15 group queries and exact whole snapshot. GetParam at3840 proves stored data, not audible scheduling beyond this fixture length. Whole playback remains unfiltered.

Attempted runtime track persistence in build030533135Z returned E_NOTIMPL from Tempo Save after successful IPersistStream QI. That production dependency was removed; no Band Save result is inferred. Band index binding remains unresolved. Evidence: product/20261003T031302411Z/run.json and product-group/20261003T031318943Z/group-playback/group-playback-proof.json under work/acceptance. Runtime55/54 module provenance compared against original40 has zero matches. Current GUI/audio and original Producer comparison remain unexecuted.


## 2026-10-03 所有Styleの名前指定Motif再生と無人録音

全体未完了。Conductor.playへoptional MotifSelection{styleIndex,name}を追加。既存の所有Style snapshotのidentity/拍子/collection解決後、停止前に選択index・非空/NULなしname・Motif種別/名前一意・mtfs読取を検査。Loaderに所有Style memory/GUIDを登録し、そのStyleのGetMotifから得たSegmentをSegment8へQIしてDownload/PlaySegmentExする。GetMotif S_FALSE/null/部分loadを成功にしない。取得pointerはQIの成否に関わらずReleaseし、既存Stop/Unload/Style/collection解放経路を共有する。通常DMSGをMotifの代替としてロードしない。playback_bytes/runtime.sgpは参照解決用の文脈DMSGであり、GetMotif生成Segmentの保存bytesではない。

本体PatternメニューにPlay Selected Motifを接続。現在のSegmentが選択Styleの同じpath/bytesを所有参照している場合に再生する。独立Styleだけの再生は未対応で、明示エラーとする。GUIタイマーは最初の実再生を待ち、準備中のS_FALSEを自然終了と誤判定しないよう5秒の開始期限を設けた。通常Playにも同じ監視を適用。今回のGUI実操作は未実行、compile/APIの成功をGUI受入へ転用しない。

初回132925638Zの133048077Z実行はGetMotif/QI/Download/Play成功後、positionのTempo GetParamで0x88781166(DMUS_E_TRACK_NOT_FOUND)が返り失敗。MotifにはTempo trackがないという実測を受け、Motifのこの値だけを明示的に許容しPlaybackPosition.tempoAvailable=falseとする。他のGetParam失敗は引き続き例外。Tempo値をStyleの112や既定120と推測で埋めない。133222455Z/133325978ZのMotif4音成功は中間版記録として保持。GUIのSegment取得ではStyle mode用document() guardを使わずFrameworkのSegmentを明示参照するよう修正し、準備待ち監視を含む最終133344134Zを別ビルドした。

最終work/build/product-snapshot/20261003T133344134Z/build-summary.jsonは保存59sources/3targets、構成0/compile0/install0、warning/error0。EXE SHA256 b95cddb9dcc9ad849904e9838a67f369f887524e402634b497e2d9cca52de1da、core 1a5a22070adbf811b1d2f84f17394a0bf8fd617b8a493e62dd246d5ba4410363。work/acceptance/motif/20261003T133554816Zは27件と独立全bytes監査、work/acceptance/product-host/20261003T133554768Zは本体起動/終了exit0・24modules原版40hash0を確認。GUI/core全suiteは未実行。

SDK根拠は保存dmusici.hのGetMotif(WCHAR*,IDirectMusicSegment**)とdmusicf.hのDMUS_IO_MOTIFSETTINGS。Create-MotifPlaybackFixture.mjsは以前の所有Motif保存入力からNormal PartをC4、Motif PartをC5の各4音にし、Motifへ明示Bandを所有コピー。二入力はrepeat DWORDだけ0/1、playStart0/loopStart768/loopEnd2304/resolution1、Segmentは同一。work/analysis/motif-playback/20261003T134500Z/playback-proof.jsonはraw RIFFの名前/kind/GUID Part binding/音符/Band/loopを読み、有限loopから生成時刻oracleを独立に組立てた。同EXEのAPI133527325ZはC5x4、133500259ZはC5x6、通常DMSG133614350ZはC4x12、各自然終了/Stop/CloseDown/exit0。生成音はduration384/velocity96/PChannel5、startから768clock間隔。API modules57/58/57は原版40hash0。GetMotif呼出と通常DMSGロードの排他性も照合し、Pattern通常再生をMotif成功と混同しない。

work/acceptance/audio-loopback/20261003T133532974Zは同EXEの名前指定Motifを、既存のソース製録音器102354462ZでWASAPI default render endpoint loopback録音。48kHz/2ch/float32/16秒、player/capture exit0、6音C5のAPI属性と録音音程一致、onset 3.3秒、active RMS 0.021948113274515218、前後RMS 0/0、最大packet gap 2frames。57modules原版40hash0。work/acceptance/audio-auditor-controls/20261003T133700Z-motifは未変更copyの合格、無音/誤音程/基準区間背景音の拒否を確認（APIは成功のまま）。録音は一回、対照は派生copy。Stereo Mix/microphone/OS設定変更や人の聴取は不要。物理スピーカーの可聴性はこのデジタル検証の範囲外。

再現：Build-ProductSnapshot.ps1。Create-MotifPlaybackFixture.mjs <Segment> work/acceptance/motif/20261003T124256919Z/core/motif/original.stp <新しいdir>。Test-PlaybackNotes.ps1へ同summary、repeat-0/1のSegment/InputPaths Style、-MotifName 'Owned Motif'、通常比較ではMotifNameを省略。各Inspect-ProductModules.ps1 -CaseName notes-api。Test-LoopbackAudio.ps1へ同summary、既存102354462Z recorder/summary、repeat-1 Segment、-Profile motif-repeat -MotifName 'Owned Motif' -Node <Node実パス>。Test-LoopbackAuditor.mjs <audio dir> <新しい対照dir>。Inspect-MotifPlayback.mjs <fixture.json> <repeat0 run.json> <repeat1 run.json> <normal run.json> <audio dir>。Test-Motif/Inspect-MotifとTest-ProductHost/Inspect-ProductModulesは同版へ別実行。

Windows DirectMusic/DirectSound/GM.DLS依存は残る。原版Producer40固有hash一致は点観測で0、全40責務の完成を意味しない。Motif GUI Play/Stop/再開と別起動復元、再生中のStop/restart録音、独立Style再生、Motifの実テンポ/指定時刻/secondary playback、原版動的比較/Clipboard、空Style Band、JAZP/全40全八は未完了。次は同版GUI Play/準備待ち/Stop/再開・通常終了と保存別復元を確認し、次に長いMotifのStop/restart無人録音と独立Style再生を進める。原版警告はユーザーが閉じて対象可能になった状態から観測を再開する。全体条件を縮小しない。

記録訂正：133532974Z録音時の実module数は監査JSONの57。初版unitの本文に58と誤記したため訂正し、初版の凍結ファイルは保持する。最新凍結記録はwork/analysis/motif-playback/20261003T134500Z/unit-record-v2.json。実際の生成物/入力/音声/監査結果は変更していない。


## 2026-10-03 現行Motif GUI保存復元と再生ライフサイクル

全体未完了。製品は保存59sourcesの133344134Zを再利用し、今回の製品ソース変更/新ビルドはない。構成/compile/installは当該build-summaryの各exit0。EXE b95cddb9dcc9ad849904e9838a67f369f887524e402634b497e2d9cca52de1da。今回は固定長いMotif入力の生成スクリプトと独立GUI監査を追加した。過去GUI/録音成功を新しい実行の結果として転用しない。

GUI134503517Z/PID4272は以前130219053Zの保存Style(3b839a6e7ef0541df39718516d3176a4b0cdd0245d6f1a83a2d79f03b4fa55c6)を別processで開き、repeats2/playStart384/loopStart768/loopEnd2304/resolution1を表示。Save Style後のresaved.stpは全bytes一致、project/Segment不変、通常終了exit0。Inspect-MotifGuiReload.mjsは保存59sources/同現行EXE/入力/画像/以前の保存proofを結合する。以前のEXEは異なり旧起動監視の終了コードは不明のまま保持。今回の通常終了を過去の終了判定へ転用しない。実ロード45modules、原版40hash一致0。

Create-MotifLongFixture.mjsは既存有限loop固定入力のmtfs repeat DWORDだけ1から63へ変更し、その他全bytes/Segment/projectを保持。Style SHA 8c2ab246096068b5d7fd99c931b976607e09b4b9c938b7b9fddb4b795ebc6d5a。入力は明示BandとC5のMotif Partを持つ。GUI134919847Z/PID19232でStyle/Owned Motifを選択しPattern > Play Selected Motifを実操作。5秒の準備期限を過ぎたPlaying表示、再生中のStopとStopped表示、同メニューで再開後のPlaying表示、後のStopped(segment ended)表示、通常終了exit0を確認した。再開後の終了を観測した時点では既に自然終了しており、二度目の再生中Stopは試験していない。Inspect-MotifGuiPlayback.mjsがraw mtfs差分、同現行版、入力不変、操作/安定UI状態の時系列、画像hashを独立照合。再生中の実ロード75modules、原版40hash一致0。

このGUI実行の音声は未録音。Playing/Stoppedという画面表示を実際の発音/無音やexact tempoの証明には使わない。以前の同EXE有限Motif録音133532974Zは独立したCLI実行の証拠として保持する。Style画面の112 BPMをGetMotif実tempoと推測しない。人の聴取確認は不要で、次の録音もWASAPI default render endpoint loopbackを使う。OS設定/registry変更なし。原版の警告はユーザーが閉じて既存targetが取得可能になったため、次の原版比較はその既存processから再開できる。

再現：Create-MotifLongFixture.mjs <repeat1 Style> <Segment> <project> <新dir>。Test-ProductProjectGui.ps1へ現行summaryと生成project/Style/Segmentを指定し、Computer Useで上記メニュー/Stop/再開、通常終了を操作。画像/全UIAをmotif-gui-states.jsonへ保存し、各操作前UTCをactions.jsonへ保存。再生中Capture-ProductGuiModules.ps1。終了後Node scripts/Inspect-MotifGuiPlayback.mjs <GUI dir> <fixture.json> <133554768Z host run.json>、Inspect-ProductGuiModules.ps1 -EvidenceDirectory <GUI dir>。復元はTest-ProductProjectGui.ps1で旧保存Styleのcopyを開き、Settings表示/Save/終了後Inspect-MotifGuiReload.mjs <新GUI dir> <130219053Z dir> <host run.json>。起動は承認済みの通常対話環境を使用し、対象を公開しないsandbox条件で再試行しない。

残る依存はWindows DirectMusic/DirectSound/GM.DLS。点観測原版固有hash0は全40責務の完成を意味しない。次の具体的な一手は既存audio-lifecycle経路を名前指定Motifにも接続し、長い固定入力を新規loopback録音してStop前発音/Stop後無音/再開後発音をAPIの時刻/音符と照合する。次に独立StyleだけのMotif再生を成立させる。原版動的Motif設定/Clipboard比較、実tempo/指定時刻/secondary playback、空Style Band生成、JAZP、全40責務/全八受入は未完了。全体条件は縮小しない。凍結記録：work/analysis/motif-gui-long/20261003T135000Z/unit-record.json。


## 2026-10-03 名前指定MotifのStop/restart無人録音

全体未完了。main.cppのaudio_lifecycleへoptional MotifSelectionを追加し、--motif-lifecycleで所有StyleのGetMotif経路を再生・早期Stop・3秒hold・再開・早期Stopへ接続した。両PlayでFramework所有collectionも渡す。通常Segmentだけの長さ49152clock条件をMotifへ誤適用せず、開始待ちとStop直前の実IsPlayingで早期停止を検査する。生成音符をrun1/run2・各start付きで記録し、observer overflow/forwarding failureを拒否、全API呼出を保存する。note_observeも文字列空判定で通常再生へfallbackせずoptional選択をそのまま渡すため、CLIの空Motif名はConductorのpreflightで拒否される（空名CLIの個別実行は未試験）。

最終work/build/product-snapshot/20261003T140334534Z/build-summary.jsonは保存59sources/3targets、構成0/compile0/install0、build.log warning/error0。EXE 4a2188b97f39eb484dd2d572f9e52363d67e3ccd3f52cab098a20bdcce716524、core b56d26af239285603190e37c715ea51ca6066b8a6f0fa31fd30d6fb893913759。今回native文書27/core全suite/GUIは再実行していない。変更対象CLI/APIと共有ライフサイクル、本体hostを検証し、過去1333のGUI/native成功を現行版へ転用しない。

新規Motif録音work/acceptance/audio-loopback/20261003T140509870Zはplayer/capture exit0、ソース製録音器102354462Z、48kHz/2ch/float32/16秒。固定入力は135000Zの長いOwned Motif、mtfs[63,0,768,2304,1]・C5/384clock/velocity96/PChannel5・所有GUID Part binding/明示Bandをraw RIFFで独立確認、実ロードStyle/input Segment bytesも照合。Get owned Motifは二回成功、通常DMSGロードなし。APIは両再生各6音を768clock間隔で観測。音符観測は先行スケジュールも含むため、6音全ての可聴性を主張しない。録音検査はplay-ready+.55からstop-request-.15の区間でC5成分/発音RMSを確認し、Stop後1.2秒以降のhold区間を無音として判定。Stop直後のrelease tailが即時0であるという判定には使わない。

初回発音RMS0.03307359600306563、再開RMS0.033081916320508636、baseline/Stop hold/final holdはRMS0、peak0.1343252956867218、最大packet gap2frames。QPCとWASAPIpacket時刻を照合して録音区間を決める。C5成分はC4成分の2倍超を要求。初期proofのtone欄名c4がC5にも使われていたため、initial-proof-before-tone-label.jsonへ保存してexpectedToneへ訂正。raw入力照合追加前のproofもproof-before-raw-fixture-audit.jsonへ保存し、同WAVを解析再実行。録音は繰り返していない。

work/acceptance/audio-auditor-controls/20261003T140900Z-motif-lifecycleの対照4件は未変更copy合格、Stop区間へ音混入/再開区間無音/再開区間C4への置換を拒否。API成功は保持した派生WAVであり新録音ではない。旧140600Z/140700Z対照も保持。Inspect-AudioLifecycleは通常Segment profileも維持し、共有経路の新規録音work/acceptance/audio-loopback/20261003T140722303Zで早期Stop/hold/restart/finalStopを確認した。有限Motif API140618882ZはC5x6、通常API140646678ZはC4x12、exact属性・自然終了・正常終了をwork/analysis/motif-audio-lifecycle/20261003T141000Z/related-api-proof.jsonで独立照合。host140523324Zもexit0。

現行点module観測はhost23/Motif録音58/通常録音57/両有限API57。各監査passed、原版40固有hash一致0。Windows DirectMusic/DirectSound/GM.DLS依存は残る。Stereo Mix/microphone/registry/OS設定変更、人の聴取は不要。このCLI無人録音をGUI実操作や物理スピーカーの成功へ転用しない。

再現：Build-ProductSnapshot.ps1。Test-LoopbackAudio.ps1 -BuildSummaryPath <summary> -RecorderBuildSummaryPath work/build/audio-capture/20261003T102354462Z/build-summary.json -Recorder 同build/Release/producer_loopback.exe -Segment work/analysis/motif-gui-long/20261003T135000Z/selection.sgp -Profile motif-lifecycle -MotifName 'Owned Motif' -Node <Node実パス>。同dir Heartlnd.stpを入力とする。Inspect-AudioLifecycle.mjs <録音dir>、Test-AudioLifecycleAuditor.mjs <録音dir> <未作成の対照dir>。通常回帰はProfile lifecycleと保存long Segment入力。有限回帰はTest-PlaybackNotes.ps1へrepeat1入力とMotifNameの有無。Test-ProductHost/Inspect-ProductModulesを同summaryへ指定する。承認済み通常環境で実行し、OS制御回避は行わない。

次の具体的な一手はSegmentを開かなくても、所有Styleのみの選択MotifをConductorへ渡せる本体/Framework文脈を実装し、同じ生成音符/無人録音で確認する。Motif指定時刻/secondary/実tempo、原版設定・Clipboard比較、空Style Band生成、JAZP、全40責務/全八受入は未完了。過去GUI設定保存復元/Play状態の証拠は以前の生成物のまま保持。全体条件を縮小しない。凍結記録：work/analysis/motif-audio-lifecycle/20261003T141000Z/unit-record.json。


## 2026-10-03 Style単独Motif再生の本体・Framework接続

全体未完了。Frameworkへstyle_playback_snapshot/style_playback_collectionsを追加し、所有Styleの現在save_bytesとその文書のcollection参照を取得する。Conductor.play_motifはStyleCatalogEntryからStyleを検査し、既存GUIDがなければruntime copyにだけGUIDを追加して共通play_snapshotへ渡す。snapshot.segmentは空のまま、DMSG文脈/仮のSegment/新規Segment文書は作らない。Style/Band/DLSのpreflight、名前の一意/型/mtfs検査を停止前に行い、GetMotif生成SegmentをDownload/Playして既存Stop/Unload/所有解放を共有する。通常SegmentはこれまでのStyle参照・command変換後に同じ共通経路へ入る。

本体Play Selected Motifは選択Styleの所有snapshotから直接再生する。参照Segment有無による無効化と一致Segment探索を除去し、Style単独/Style-only projectでも有効にした。CLI --style-motif-observeを追加。Framework.open_styleだけを呼び、Segment所有が空/Conductor文脈bytesが空であることを実行時検査し、input.stp/source-style/runtime-styleと生成音符を保存する。通常/文脈Motif観測も同関数を使用する。Frameworkの文書自体は変更しない。所有custom DLSのStyle単独音響は今回未試験。

最終work/build/product-snapshot/20261003T141622797Z/build-summary.jsonは保存59sources/3targets、構成0/build0/install0、build.log warning/error0。EXE 92a428c698cb8613fc8cb978d95170e469b4c78fc1555faf19849c32d951f2b0、core a496f91fc25a7f08f5353013d59477f6be6a9b02f15744b4ceed4f52c029751a。今回native27/core全suiteは再実行していない。過去1403のStop/restart録音/GUI/native成功は別生成物の記録として保持し、現行版の成功へ転用しない。

Create-StandaloneMotifFixture.mjsは保存有限Motif入力からStyle-only folders/projectを作る。with-id Style e2d2630d2f9b311c5e284f66f08a8ad2c6fa0af1e7633868cbabef788b0cd4ca、without-id 909e371d2e117f1f7acd3ca286b7424198d070bae592646a11e6f64e05fa1c77、差分はroot guid chunk除去だけ。両projectはfile参照がHeartlnd.stp一つ、Segmentファイルなし。API142020540Zはwith-id Style単独C5x6、GUIDなし新規録音142029205ZもC5x6、同版文脈Motif141815445ZはC5x6、通常DMSG141849323ZはC4x12、各自然終了/正常終了。Inspect-StandaloneMotif.mjsはraw Motif/name/kind/mtfs[1,0,768,2304,1]/Part GUID binding/明示Band/音符を読み、finite loopから6音oracleを独立構成。入力不変/runtime Style非GUID全childbytes一致/生成GUID16bytesと元GUID非同一/全属性と時刻/同版59sources/生成物/各moduleproofを照合した。Standaloneのinput.sgp/runtime.sgp不存在、Get owned Motif一回/Load current DMSGなしも検査。これらの実行には保存Segmentの暗黙fallbackはない。

work/acceptance/audio-loopback/20261003T142029205Zの新規WASAPI録音は48kHz/2ch/float32/16秒、player/capture exit0、6音C5のAPIと録音6区間を照合。onset3.3秒、active RMS0.021961072917854547、baseline/tail RMS0、最大packet gap2frames。work/acceptance/audio-auditor-controls/20261003T142400Z-standalone対照4件は未変更合格、無音/誤音程/背景音混入を拒否（native API成功は保持した派生copy）。先のwith-id録音work/acceptance/audio-loopback/20261003T141800649Zも同EXEでC5x6/exit0、RMS0.021943645939851382、onset4.3秒、前後RMS0/packet gap2。最終独立4実行比較はno-ID録音を使い、先の録音を省略/置換せず別記録として保存する。ソース製録音器102354462Zを使用し、Stereo Mix/microphone/OS設定変更や人の聴取不要。物理スピーカーは範囲外。

GUIwork/acceptance/product-project-gui/20261003T142334515ZはStyleだけのlong Motif projectをPID2908で開き、一覧の文書はStyle一つ。Owned Motif選択、Pattern > Play Selected Motif有効、操作後5秒準備期限を過ぎてもPlaying表示、再生中Stop、Stopped表示、通常終了exit0を確認した。Style/projectは不変。GUI入力はrepeat63で、有限CLI音声入力repeat1とは別。Inspect-StandaloneMotifGui.mjsはraw project file参照一つ/Segment不存在/現行EXEと保存59sources/操作時刻・安定UIA/画像hashを結合する。Dropdown直後はUIA nullが返り、画像を保存した後の補助ログ出力でエラー。再観測では対象と画像を取得でき、操作を重ねず結果を保存した。即時Stop UIAが旧Playingを返したためstable stopped-readyを別記録。GUI音声/GUI restartは未実行でCLI録音をその成功へ転用しない。

点module観測は現行host23/各三API57/with-ID録音58/no-ID録音57/GUI75、各原版40固有hash一致0。依存Windows DirectMusic/DirectSound/GM.DLSは残る。全40責務/全八受入の完成を意味しない。原版既存windowは取得可能なまま、新規原版launch/registry変更は行っていない。

再現：Build-ProductSnapshot.ps1。Create-StandaloneMotifFixture.mjs <finite repeat1 Style> <新dir>。Test-PlaybackNotes.ps1 -BuildSummaryPath <summary> -Segment <with-id Style> -MotifName 'Owned Motif' -StandaloneStyle。Test-LoopbackAudio.ps1へ同summary/既存102354462Z recorder・summary/-Segment <without-id Style>/-Profile motif-standalone/-MotifName 'Owned Motif'/-Node <Node実パス>。既存引数名SegmentにStyle入力を渡すが、driverは--style-motif-observeを選びSegmentを開かない。文脈Motif/通常回帰は同repeat1 SegmentでStandaloneStyleなし、MotifNameあり/なし。各Inspect-ProductModules.ps1。Inspect-StandaloneMotif.mjs <fixture.json> <standalone run.json> <no-ID audio dir> <context run.json> <normal run.json>、Test-LoopbackAuditor.mjs <audio dir> <新対照dir>。GUIはlong Styleから同fixture builderで作ったprojectをTest-ProductProjectGui.ps1へ渡し、Computer Use操作・再生中Capture-ProductGuiModules、終了後Inspect-StandaloneMotifGui.mjs <GUI dir> <long fixture.json> <host run.json>とInspect-ProductGuiModules。承認済み通常のWindows対話環境で検証する。

次の具体的な一手は既存原版GUIからMotif settings/Style単独再生の動作を観測し、現在の製品方針との差を記録する。同時に空Styleへ所有Bandを生成する本体/Framework経路を実装する。Motif custom DLS/指定時刻/secondary/実tempo、原版Clipboard、JAZP、全40責務/全八受入は未完了。全体範囲/完了条件は縮小しない。凍結記録：work/analysis/standalone-motif/20261003T142000Z/unit-record.json。


## 2026-10-04 製品で新規作成したMotifへの明示Band割り当て

全体未完了。前回のBand作成保存を進め、空Styleから製品APIでGM48/PChannel5/pan64/volume100のBand、Authored Motif、専用Part、C5の4音を生成する対象試験を追加した。Style GUID/Part GUIDは作成時に新規生成。元のStyle/Pattern/Band sampleコピーは使わない。参照Segmentを別作成して所有Style cacheの更新、Band一回Undo/Redo、Segment bytes/dirty不変、保存別Framework復元を検証。CLI再生はStyleだけを開き、その参照Segmentをロードしない。

最初のwork/build/product-snapshot/20261003T145358240Zではnative11項目が通り、work/acceptance/product-notes/20261003T145550277ZのStyle-only GetMotif APIはC5x6・自然終了exit0。だがwork/acceptance/audio-loopback/20261003T145621134Zの新規WASAPI録音はAPI成功のまま全16秒無音（RMS0/peak0/onset null）で検査失敗。録音器/player正常終了でpacket最大gap2。Style rootのBandとMotif内Bandは別であり、所有Style BandだけでGetMotifが発音するという仮定は成り立たなかった。保存SDK dmusicf.h lines420..428はMotif pttnにoptional DMBD Bandを定義。新規Style styh12はOS GetTimeSignature/GetTempo比較が成功しており、この失敗をheader不一致と推定しない。

StyleDocument.assign_motif_band(patternIndex,bandIndex)を追加し、所有Style Bandを選択MotifのDMBDへ明示copyする。既存一つは置換、同bytes/invalid index/非Motifは変更なし、複数既存Bandは拒否、他のPattern/Part/chunksを保持。一回のUndo/Redo。Framework.assign_style_motif_bandが所有文書と参照cacheへ接続。本体PatternメニューにAssign Selected Style Band to Motifを追加し、選択MotifとBand instrument一覧で選んだStyle Bandを使用する。割り当てはsnapshotのcopyであり、後のroot Band編集をMotifへ自動伝播しない。暗黙fallback/無人試験限定のruntimeデータ修正は加えない。GUI操作と原版同等性は今回は未確認。

最終work/build/product-snapshot/20261003T145909887Z/build-summary.jsonは保存59sources/3targets、構成0/build0/install0。EXE 5b56b22cc64028b851274ffcf54084d65601090df6a83209af9cfab696a1cdd0、core 8f238b53d72999e8682be5c95db80d3313f728211821ff696006ef0360ac2f7a。work/acceptance/authored-style-band/20261003T150036596Z/run.json対象14項目exit0。明示copy/no-op/invalid index/一回UndoRedoと依存cacheを追加検査。各fixtureは作成ごとにGUIDが異なるため旧/新の全bytes同一とは主張しない。新版内ではunassigned.stpからfinal Heartlnd.stpへの変更が選択Motifへ追加したDMBDだけであることを独立監査した。native全suite/新GUI試験は未実行。

work/acceptance/audio-loopback/20261003T150057747Zの新規default render endpoint WASAPI loopbackは既存ソース製録音器102354462Z、48kHz/2ch/float32/16秒、player/capture exit0。Style-only GetMotifでC5x6、768clock間隔/duration384/velocity96/PChannel5/group1、自然終了。onset 3.2秒、active RMS 0.021547966736025284、baseline/tail RMS 0/0、peak 0.12410677969455719、最大packet gap 2frames。入力mtfs[1,0,768,2304,1]からpreloop3+repeat2+tail1の有限6音。Inspect-AuthoredStyleBand.mjsはraw rootBand bins44・flags0x1163/patch48/channel5/pan64/volume100/他zero、Motif内Band全bytes一致、name/type/settings/Part GUID binding/4音、空Style→Band→Motif非変更chunk保持、runtime Style/input.stp全一致、input.sgp/runtime.sgp不存在、GetMotif一回/Load DMSGなし、同保存sources/workspace/build/exe/core/driver/入力hashを照合。work/acceptance/audio-auditor-controls/20261003T150200Z-authored対照4件は未変更合格、無音/誤音程/背景音混入を拒否し、API成功は保持した派生copy。過去の無音を成功へ置換せず保持する。

現行host/録音module監査は各原版40hash一致0の点観測。Windows DirectMusic/DirectSound/GM.DLS依存は残り、全40責務完成を意味しない。人の聴取/ステレオミキサー/マイク/OS設定変更は不要。デジタル録音の結果を物理スピーカーやGUI Playの成功へ転用しない。前回143452471ZのGUI成功は旧生成物の証拠として保持。

再現：Build-ProductSnapshot.ps1、Test-AuthoredStyleBand.ps1 -BuildSummaryPath <summary>。作成されたcore/Heartlnd.stpへTest-LoopbackAudio.ps1を同summary/recorder102354462Z/-Profile motif-standalone/-MotifName 'Authored Motif'で実行。Node scripts/Inspect-AuthoredStyleBand.mjs <native run.json> <audio dir>。Test-LoopbackAuditor.mjs <audio dir> <新対照dir>。Test-ProductHost.ps1/Inspect-ProductModules.ps1を同summaryへ指定。旧失敗再試行は行わず明示assignmentという条件変更後だけ新録音。通常の承認済みWindows環境を使用。

次の具体的な一手は同版本体GUIで未割り当てStyleのMotifを選び、Band選択/新メニュー/UndoRedo/保存/別起動復元を実操作し、保存bytesと明示copyを検証する。次にMotif Bandの再割り当て/既存copyの編集UIとcustom DLSを進める。原版新Style/Band/Motif defaults比較、Motif指定時刻/secondary/実tempo、Clipboard/JAZP/全40責務/全八受入は未完了。全体条件を縮小しない。凍結記録：work/analysis/authored-style-band/20261003T150300Z/unit-record.json。

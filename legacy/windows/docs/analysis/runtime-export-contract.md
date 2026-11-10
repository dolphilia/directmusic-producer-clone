# Runtime Save All Files初期統合

現行072633758Z保存67sources構成/build/install0。本体Runtime Save All Files/Framework export_runtimeで4形式(.sgt/.sty/.bnd/.dls)と入れ子の参照名変換、観測済み編集専用チャンク除去、元所有モデル/履歴/Project保持、一時folder検証後publishを実装。関連245checksと独立runtime/nested/copy解析・host smoke passed、24modules原版40hash0。root AudioPathは設定損失を避け未対応拒否/後始末確認。現行DirectMusic書出物Load/音声/GUI/原版動的export比較/全core/全40/全八未完了。

根拠：既存CHM展開htmのtoconvertallfilesinaprojecttodirectmusicruntimefiles、toconvertafiletoadirectmusicruntimefile、componentfileproperties、projectpropertiesを読んだ。保存済みsamples Tutorial/FinishedProjectのBGDawn.sgp/FarmGame.stp/FarmGame.dlpとFarmGameの.sgt/.sty/.dlsを静的chunk比較。source/runtime extensionsと参照名変更、編集専用sgdl/segd、styu、pptd/ptnu/pref UI、jzfr/psrd、Region dmpr等の差を根拠とした。これらは新原版動的試験ではなく、制作時点同一性や全payload等価を保証しない。参照hashはreference-sources.json。新CHM抽出・原版unsupported editor再試行なし。

実装：clean保存済みProjectから既存copy_projectで全所有file＋filename-only依存閉包をunique stageへcopyする。stageは元Project folderの兄弟で、Project内Runtime Filesを出力先にできる。native stage folder==Project stem条件を守る。拡張子case-insensitiveで.sg[p/t]→.sgt、.st[p/y]→.sty、.bn[p/d]→.bnd、.dl[p/s]→.dls、未対応form/ext・出力filename衝突を拒否する。DMRF/fileの同拡張子を変換し、GUIDを保持。観測した親form/contextに限定した編集専用chunkを除去、他chunk/payload/order/paddingを保持する。source metadataをruntimeへ流用するProjectファイルは出さない。stageの元fileを削除し変換物だけを書く。別FrameworkがDLS/Style/Band/Segmentを順にopenし、Segmentの依存を検証後rename公開。失敗時は新unique stageのみ削除し、元Framework/path/model/history/Projectを採用変更しない。出力先が既存folderなら拒否、既存runtime更新は残る。

AudioPath（訂正：2026-10-04）：従来の「BGDawn設計版root DMAPが実行版ではruntime trackへ移る」は誤った推測だった。供給runtime BGDawnにはDMAP自体がなく、同一制作状態を示さない。凍結SDK dmusicf.hはDMSG直下のDMAPを明示的に許可する。以下は初期実装の履歴で、現在の実装方針ではない。初期draftはroot DMAP除去だったため、最終版は変換未実装として明示拒否し、設定損失を起こす出力をpublishしない。完全対応への残課題として継続し、対象除外ではない。最終3checksは専用AudioPath inputを保持してoutputなし/stage残留なしを確認。

試験：native関連245checks（前231＋4形式6/nested runtime5/AudioPath拒否3）exit0。RuntimeSource内Runtime Filesへ4形式を出力、Segment sgdlだけ除去/embedded Band Sound.dlp→Sound.dls、Band同参照変換、Style styu除去、DLS全bytes同一、GUIDとunknown zzzzの3bytes/padC7を保持。別Frameworkがstandalone/embedded Band collectionを解決し元4model/Project clean bytesを保持。Nested filename-only Style→DLS closureも.styへ変換しruntime文書復元、元保持・既存folder拒否を確認。Inspect-RuntimeExportはserializerを独立に持ち上記全bytes変換を照合、3形式opaque/pad、nested filenames、DLS全bytes、noProject/no stageを照合。同run NestedProjectCopy/ProjectCopyもpassed。nativeモデルopenはDirectMusic COM Loadや発音ではない。host smoke0/24modules原版40hash0の点監査。

版の分離：072104706Zはcompile/install成功後未実行（testのBand track選択がtempo index0を指すとreadで判明、実行前修正）。072305024Zは242checks/raw/hostを実行成功したがAudioPath損失防止未修正のdraft、原版AudioPathを実際に書出した証拠ではない。最終072633758Zを245checks/raw/hostで改めて検証し、draft成功を転用しない。各snapshot/log/runを保持。

限界と未完了：4formsだけで全componentのRuntime Saveを完成扱いにしない。AudioPath track、Wave/Script/Container/ChordMap等、圧縮音源、DMRF date valid-bit/unknown metadata意味、非保存dirty拒否個別試験・filename衝突個別試験、Runtime Save As、既定rdir/rfld/rnamやnative runtime履歴、既存出力更新、原版バイト/意味比較、書出物DirectMusic Load/Play/WASAPI録音、GUIメニュー、全core/全40/全八が残る。今回空DLSを使った基本変換試験なので発音は未実行。前版コピーPCM録音の成功を新版に転用しない。Windows DirectMusic/DirectSound/GM.DLS/WASAPI依存継続。旧18804 GUI前面化回答pendingを保持し新GUIを同条件で増やさない。

再現：Build-ProductSnapshot.ps1→Test-JazpSave.ps1 -BuildSummaryPath [同summary]→Node Inspect-RuntimeExport.mjs [run dir]、Inspect-NestedProjectCopy、Inspect-ProjectCopy。同summary Test-ProductHost→Inspect-ProductModules -RunPath [host/run.json]。製品UI File Runtime Save All Files...は既存Common Save dialogで新folder名を選び出力（GUI未実行）。

次：First integrate exported real Envelope PCM/Instrument Project into a same-build native/runtime catalog fixture and calibrated WASAPI Play/early Stop/restart, with source folder unavailable and exported input hashes bound. Then implement original root AudioPath to runtime AudioPath track transformation using retained BGDawn sample/header contracts. Verify DMRF date valid flags, output filename collisions, dirty rejection, unknown runtime metadata and compressed data. Runtime Save As/per-component default names/folders/rdir/rfld/native runtime metadata and update-existing-folder transaction remain required, not reduced away. GUI and original dynamic export compare remain unexecuted; preserve pending old18804 GUI state.


## 2026-10-04 runtime実音源の無人録音

現行074253912Z保存67sources構成/build/install0。実PCM/Instrumentのruntime準備コマンドと入力/録音監査を追加、長いpathで失敗した一時runtime Project名を短縮。最終native準備0/独立Segment layout・DLS Region dmpr除去raw解析passed。元Source folder不在で書出物専用catalogからDirectMusic Play/途中Stop/再開、校正済WASAPI録音440Hz/Slow Attack/停止無音passed、派生負対照3拒否、55再生modules原版40hash0。長path247checksは修正途中073923915Z版のみ（現行へ転用しない）。現行GUI/全core/全40/全八未完了。 証拠：work/analysis/runtime-export-audio/20261004T074800Z/report.md。次：Implement root AudioPath to runtime AudioPath track conversion using retained BGDawn design/runtime pair and SDK track class/header contract; preserve actual configuration rather than dropping or shrinking scope. Verify DMRF date validity flags and filename collisions/dirty export/failure transaction. Expand runtime output beyond3-doc PCM path to Style normal/Motif and standalone Band activation; auxiliary DMPJ is a test preload catalog, not runtime Project format or all-file compatibility. Runtime Save As, component defaults/native rdir/rfld/rnam/history, update-existing output, original dynamic export comparison/GUI and all40/all8 remain.


## AudioPath設定の保持と再生契約（2026-10-04訂正）

根拠とhashはwork/analysis/runtime-audiopath/20261004T081400Z/reference-observation.json。原版APFarm.aup/.audはいずれもPChannel10/11と2つのbufferを持つが、GUID等が違い全bytes一致ではない。Frameworkは.aup/.audを.audへ書出し、Segment root DMAPも保持する。削除はDMAP/papd、pcfl・pchl・DSFX/UNFO、DSFX/pegdに限定し、port/routing/DSBC/FX/identity/unknown/paddingは維持する。元Project・モデル・依存・履歴を変更しない。

Conductorは完全なSegment Load後にGetAudioPathConfig、CreateAudioPath(TRUE)、同じAudioPathへのSegment Download、PlaySegmentEx、Unload、Releaseを行う。標準Performanceへ黙って置き換えない。SDK IDirectMusicAudioPath::ConvertPChannelはローカル番号からPerformance番号への対応を返すので、Sequence音符の接続を事前確認し対応を記録する。Performance graphで観測する番号は保存された番号と一致するとは限らない。元APFarmのPChannel0入力はDMUS_E_NOT_FOUND(0x88781161)で失敗しており、その結果は保持する。演奏試験は元設定を変えず、編集APIで音符・Bandを接続済み10へ割り当てる。

独立AudioPath新規作成・typed文書所有・native filh登録・port/buffer/FX編集・GUI・原版動的比較は未完了。現在はcatalog既存file保持と設定の書出し/再生の部分実装であり、AudioPathDesigner全責務の完成ではない。供給.audとの全byte一致・物理スピーカー確認は主張しない。


## 2026-10-04 AudioPathの設定保持と無人録音

現行081843304Z保存67sources構成/build/install0。FrameworkはAudioPath .aup→.audとSegment root DMAP保持、限定編集metadata除去を実装。Conductorはembedded設定取得/明示AudioPath作成/同path Download・Play・UnloadとSequence接続確認を実装。関連247＋専用7checks/独立raw/host passed。元Source不在でAPFarm設定PChannel10→Performance16のAPI・音符一致、WASAPI440Hz/Slow Attack/途中Stop/再開/停止無音passed、派生PCM3拒否。同版AudioPathなし回帰録音passed。再生56/host23modules原版40hash0。AudioPath ownedモデル/編集GUI・全core/全40/全八は未完了。 証拠：work/analysis/runtime-audiopath/20261004T081400Z/report.md。次：Implement a typed standalone AudioPath document and Framework/native Project ownership (.aup/.aud, factory/new/load/save/filh); retain original DMAP ports/buffers/FX/unknown bytes and add first routing edit with history and separate-Framework reload. Read retained original AudioPath help/native catalog fields and record observations first. Continue runtime Save As/default paths/rdir/rfld/rnam/update-existing transaction, Style/Motif runtime output, DMRF flags/collisions/dirty export and all40/all8. Old GUI18804 and pending front-window question remain; no same-condition startup or original unsupported-editor retries, no listening questions.


## 2026-10-04 AudioPath typed文書とnative所有

現行083455625Z保存71sources構成/build/install0。AudioPath typed文書/既定16ch stereo factory/名前・buffer接続編集/UndoRedo、Framework所有・native filh登録/metadata同期/dirty・コピー・runtime export、本体AudioPath Documents画面を接続。専用20＋関連247＋export7checksと独立全bytes/GUID/サイズ/更新時刻/nnam/rnam/元Source不在コピー解析passed。同版APFarm embedded再生のWASAPI440Hz/Slow Attack/途中Stop/再開/停止無音、派生PCM3拒否passed。再生56/host24modules原版40hash0。新画面操作・自作default設定の実再生・Segmentへのowned AudioPath割当・全core/全40/全八は未完了。 証拠：work/analysis/audiopath-document/20261004T084000Z/report.md。次：Connect owned AudioPath to Segment through an undoable Framework assignment and product control; preserve source/Segment identity and original configuration, verify save/separate reopen/runtime export and actual playback. Fix native preparation to allow already-matching PChannel0/Band routing, then verify source-created default stereo path and buffer-edit output via same-build DirectMusic/API/WASAPI, rather than only APFarm. AudioPath GUI New/Open/route/name/Save/Project Save/exit/reopen remains unexecuted; preserve old pending18804 without same-condition GUI launches. Continue PChannel range editing, buffer/FX properties, runtime Save As/defaults/native metadata and all40/all8.


## 2026-10-04 owned AudioPath割当とdefault実再生

現行090137091Z保存71sources構成/build/install0。Framework/Segmentのowned AudioPath独立copy割当・置換・削除/UndoRedo/native保存復元、編集画面のSegment操作を接続。自作default Stereoの必須pprhを追加、既存一致PChannel0の準備とowned/embedded同時書出しを実装。専用14＋文書20/raw/host passed。元Source不在・自作設定PChannel0→Performance16でWASAPI440Hz/Slow Attack/途中Stop/再開/停止無音、派生PCM3拒否passed。再生55/host24modules原版40hash0。保存回帰は中間085553818Zで146件後Windows error5、未解決・同条件再試行なし。現行全core/GUI/原版動的比較/全40/全八は未完了。 証拠：work/analysis/audiopath-assignment/20261004T085500Z/report.md。訂正：旧default factoryのpprh省略は不正であり現行では必須36byte headerを持つ。埋込みは独立copy、後の単独文書編集は自動伝播させない。Next：Verify edited routing output and Segment embedded independent-copy behavior with real APFarm buffers and same-build calibrated audio; implement PChannel range/route edit while retaining port/buffer/FX metadata, and complete AudioPath GUI New/Open/name/route/assign/remove/Save/Project Save/normal exit/separate reload when a targetable context is available. Investigate retained FreshDls.pro MoveFileEx Windows error5 through read-only lock/event evidence; no unchanged JAZP replay, security-setting changes or permission bypass. Retest full affected save suite only after cause/conditions change is established. Continue native runtime Save As/default paths/rdir/rfld/rnam/update-existing transaction, Style/Motif output and all40/all8.


## 2026-10-04 AudioPath範囲編集／Sequence PChannel

現行20261004T092316964Z保存71sources構成/build/install0。AudioPath port/route PChannel範囲と既存buffer接続編集・UndoRedo/独立Segment copy/native保存復元/runtime全bytes保持を実装。Sequenceの誤った0～15制限をDWORD PChannelへ修正、予約broadcast拒否、UI接続。専用20＋文書20＋Sequence26/raw/host passed。編集APFarm local22の明示AudioPath実再生、元Source不在WASAPI440Hz/Slow Attack/Stop/再開/停止無音、派生PCM3拒否passed。再生56/host24modules原版40hash0。旧save Windows error5未解決、現行全core/GUI/原版動的比較/全40/全八未完了。 証拠：work/analysis/audiopath-range/20261004T091800Z/report.md。旧Sequence0～15制限はMIDIチャンネルとの混同であり、現行DWORD PChannelとreserved broadcast拒否へ訂正。


## 2026-10-04 Runtime Save As／所有snapshot

現行20261004T094117154Z保存71sources構成/build/install0。Framework Runtime Save AsをSegment/Style/Band/DLS/AudioPathの現行snapshotから個別書出し・既存runtime更新へ実装し、各画面/メニューを接続。共通変換で一括出力全bytes一致、dirty/UndoRedo/元Project・source/物理alias保護、native別復元を専用20＋AudioPath export7/raw/hostで確認。個別生成runtimeのみ・Source不在のDirectMusic/WASAPI440Hz/Slow Attack/Stop/再開/停止無音と3PCM反例拒否passed。再生56/host24modules原版40hash0。既定folder/name記憶rdir/rfld/rnam、既存folder一括transaction、旧save Windows error5、GUI/全core/原版動的比較/全40/全八は未完了。 証拠：work/analysis/runtime-save-as/20261004T093900Z/report.md。個別保存の成功はrdir/rfld/rnam/default metadataや既存folder一括transactionの完成を意味しない。


## 2026-10-04 Runtime Settingsとdefault保存

現行095806565Z保存73sources構成/build/install0。native runtime rdir・rfld/fldr/path+fltr・rnamを型付きで編集/dirty/別native・bridge復元し、五文書のRuntime Propertiesと既定先保存を接続。全metadataの指定field以外/filh/GUID/time/opaque保持を設定19＋個別保存20/raw/hostで確認。保存済み設定から四runtime生成・元Source不在DirectMusic/WASAPI440Hz/Slow Attack/Stop/再開/停止無音、3PCM反例拒否passed。再生56/host24modules原版40hash0。文書単独folder記憶・原版動的比較・既存folder一括transaction・旧save Windows error5・GUI/全core/全40/全八は未完了。 証拠：work/analysis/runtime-settings/20261004T100300Z/report.md。文書単独folder記憶・原版動的既定変更比較をcomponent filter既定先で代用しない。


## 2026-10-04 既存Runtime更新

現行102324053Z保存73sources構成/build/install0。既存の明示単一folderへのRuntime一括更新、元source保護、途中失敗時の旧bytes/creation/write日時復元と新規出力除去、変更なし出力の再置換抑止を実装。専用22＋個別保存20/raw/host passed。更新したruntimeのみ・元Source不在のWASAPI440Hz/Slow Attack/Stop/再開/無音、3PCM反例拒否passed。再生56/host24modules原版40hash0。既定の複数folder/name一括、crash復旧/復元失敗分岐、旧save Windows error5原因、GUI/全core/全40/全八は未完了。 証拠：work/analysis/runtime-update/20261004T102000Z/report.md。新設定の複数folder/name一括保存は次単位。


## 2026-10-04 Native既定先Runtime一括

現行104006863Z保存73sources構成/build/install0。native rdir/rfld/rnamによる五種類の別folder/改名一括保存を本体へ接続し、DMRF/fileを出力相対pathへ再配置。Runtime形式だけ兄弟folder参照を読込み、source traversal拒否を保持。専用16＋更新22/五形式全bytes・3参照raw/別Framework元Source不在復元/host passed。同APIで生成した四runtimeのWASAPI440Hz/Slow Attack/Stop/再開/無音、3反例拒否passed。再生56/host24modules原版40hash0。別folderの実音声、文書folder記憶/原版動的比較/GUI、旧save error5、crash復旧/全core/全40/全八未完了。 証拠：work/analysis/runtime-defaults/20261004T104300Z/report.md。


## 2026-10-04 別folder改名Runtime再生

現行104959023Z保存73sources構成/build/install0。configured参照再配置後の五形式/DMRF型検証とcommit直前parent属性確認を追加。別四folder/改名四文書とfilename-only Band/DLSを本体準備・復元・実DirectMusicへ接続。既定16＋更新22/raw/host passed。元Source不在、改名Collectionにsource/runtime/restart DLS snapshot一致、WASAPI440Hz/Slow Attack/途中Stop/再開/無音と3反例拒否passed。再生56/host24modules原版40hash0。GUI/原版動的比較/文書folder記憶、旧save error5、crash復旧/全core/全40/全八未完了。 証拠：work/analysis/runtime-multifolder/20261004T105300Z/report.md。


## 2026-10-04 Runtime GUI設定ライフサイクル

現行104959023Z本体GUIでRuntime名変更→native Project保存→四設定folderへの一括Runtime出力→通常終了0→別GUIプロセスで再読込・設定復元→通常終了0を確認。独立監査でProject変更はSegment rnam一件だけ、所有文書8全bytes保持、四Runtime全bytes一致。GUI105modules由来passed/原版40hash一致0。全体受入は未完了。 証拠：work/analysis/runtime-gui/20261004T112100Z/report.md。


## 2026-10-04 RTUP復旧記録v2

現行112605179Z保存73sources構成/build/install0。Runtime更新前にRTUP v2で旧/新全bytes、存在flag、属性、作成/アクセス/更新FILETIMEを一つのdurable記録へ保存。native7と独立raw監査、関連更新22/既定16、本体host passed。host24modules原版40hash一致0。現行GUI/音声・中断transaction復元は未実行。旧104959023ZのGUI別起動復元/無人録音は別版の証拠。全体未完了。 証拠：work/analysis/runtime-recovery-record/20261004T113000Z/report.md。


## 2026-10-04 Runtime復旧記録の読取判定

現行113830705Z保存73sources構成/build/install0。RTUP v2 strict parserとbefore/after/conflict読取判定、Framework原文書/Project alias保護、本体--inspect-runtime-recoveryへ接続。native25＋別本体2process/read-only入力5hash保持、記録7/既定16/raw/host passed。host23modules原版40hash一致0。関連更新は9checks後、意図したlock解除後のSound.dls replace error5でexit1。旧出力/Project保持・新出力/stage不在を別確認、原因未確定/再試行なし。復元書込・現行GUI/音声・全40/全八未完了。 証拠：work/analysis/runtime-recovery-inspect/20261004T114300Z/report.md。


## 2026-10-04 Project-bound Runtime実復旧

現行115315336Z保存73sources構成/build/install0。RTUP v3を元Project全bytes・完全source closure・explicit/configured mode/output scopeへ結合し、本体--recover-runtime-updateで条件付き旧bytes/属性/時刻復元と新出力削除を実装。実際の二Segment rollback未完了記録から別本体復元成功。外部bytes conflictで全出力保持・自分のsentinel解除後復元・再実行no-op、13prepare/5verify/本体6process/raw passed。関連v2読取25＋別2process/既定16/raw/host passed。host23modules原版40hash一致0。全五形式/configured復旧・forced crash・現行GUI/音声・全40/全八未完了。 証拠：work/analysis/runtime-recovery-write/20261004T120000Z/report.md。


## 2026-10-04 configured五形式Runtime実復旧

現行120354862Z保存73sources構成/build/install0。configured既定出力の観測callbackをFrameworkへ接続。五形式/五folder/改名五文書/filename-only三参照の実rollback未完了記録から別本体:defaults:復旧成功。設定raw/五形式全bytes・三参照変換一致、旧Segment/AP bytes・creation/write/属性復元、新Style/Band/DLS削除。外部Style変更拒否で五出力保持、解除後復旧、再実行no-op。18prepare/14verify/本体6process＋explicit13/5/6process/既定16/raw/host passed。host23modules原版40hash一致0。forced crash/大容量/metadata途中拒否/現行GUI・音声・全40/全八未完了。 証拠：work/analysis/runtime-recovery-configured/20261004T120900Z/report.md。


## 2026-10-04 configured normal Style実音声

現行123002863Z保存73sources構成/build/install0。Style PartのAudioPath ConvertPChannel検証を追加し、本体でnormal Style/DLSのconfigured四folder改名Runtime準備・元Source不在復元を接続。native17と全bytes/二filename参照/補助catalog監査passed。PChannel5→21、WASAPI440Hz/120→180 BPM/途中Stop・無音・再開passed、派生反例3拒否。再生56/host24modules原版40hash一致0。現行GUI/Motif・原版比較・全core/全40/全八未完了。 証拠：work/analysis/runtime-style-audio/20261004T123400Z/report.md。


## 2026-10-04 Runtime Recovery UI

現行134007961Z保存75sources構成/build/install0。本体FileにRuntime Recovery画面を接続し、RTUPv3の保存Project・依存元・許可された出力先をread-only照合。二文書16＋復元5、五形式18＋復元14と独立raw監査で実復旧/競合保持/再実行を確認。実GUIは保存Project読込・復旧済み表示/ボタン無効・別Project拒否・正常終了0。GUI実復旧操作と現行音声/全core/原版比較/全40/全八は未完了。 証拠：work/analysis/runtime-recovery-ui/20261004T135100Z/report.md。次：Prepare a fresh recovery GUI fixture with both outputs already existing before publication, so the GUI restore path can verify replacement without deleting data; verify ready state, confirmation cancellation, actual restore, external conflict refusal, dirty/source/journal changes and normal exit. Continue current Motif/AudioPath GUI route with calibrated unattended capture. Retain known Windows error5 refusals without unchanged replay; continue per-file runtime folder memory, original comparisons, large journals, partial metadata restoration, writer races, directory ownership and all40/all8.


## 2026-10-04 GUI Runtime Recovery Lifecycle

現行135310286Z保存75sources構成/build/install0。復旧画面でプレビュー後の出力変更を確認前に拒否、確認中のjournal差替えを書込み前に拒否する修正。新規GUI専用fixture二既存出力を本体から復旧し、取消し保持・stale output拒否/Conflict表示・確認中journal差替え拒否・全Before/Restore無効・正常終了0を確認。独立raw全bytes/creation・write FILETIME/attributes一致、元Project/source/journal保持。GUI105/host24modules原版40hash一致0。現行音声/全core/原版比較/全40/全八は未完了。 証拠：work/analysis/runtime-recovery-gui-lifecycle/20261004T141100Z/report.md。次：Resume product integration beyond recovery: exercise the current owned Motif/AudioPath project through actual GUI Play/Stop/restart with calibrated unattended WASAPI capture, normal exit and separate reopen. Implement remaining per-file runtime folder memory from retained native/CHM evidence without shrinking all40/all8. Recovery GUI Browse/dirty source changes/configured multi-folder/new-output deletion remain unexecuted; retain large-journal/partial metadata restoration/writer race/directory ownership and frozen Windows error5 refusals.

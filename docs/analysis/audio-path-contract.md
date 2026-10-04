# AudioPath文書の所有・編集・native保存

現行083455625Z保存71sources構成/build/install0。AudioPath typed文書/既定16ch stereo factory/名前・buffer接続編集/UndoRedo、Framework所有・native filh登録/metadata同期/dirty・コピー・runtime export、本体AudioPath Documents画面を接続。専用20＋関連247＋export7checksと独立全bytes/GUID/サイズ/更新時刻/nnam/rnam/元Source不在コピー解析passed。同版APFarm embedded再生のWASAPI440Hz/Slow Attack/途中Stop/再開/停止無音、派生PCM3拒否passed。再生56/host24modules原版40hash0。新画面操作・自作default設定の実再生・Segmentへのowned AudioPath割当・全core/全40/全八は未完了。

原版の保存済みhelpとAPFarm.aup、FarmGameProject.pro、SDKを再利用した静的観測（reference-observation.json）。native AudioPathは共通filh44bytesにファイルGUID/実ファイル更新時刻/サイズ/文書GUIDを持ち、UNFOのnnam/rnamを保持する。新規原版GUI試験は行っていない。既存原版警告OK回答を再要求していない。

実装：AudioPathDocumentはDMAPを所有し、原版port/buffer/FX/未知chunk順序・paddingを保持。新規factoryは独立guid/nameとSDK既定synth/16PChannels/stereo predefined buffer（ddah flag2、DSBC省略）を構成。既存APFarmのtyped loadは全bytes不変。routeの既存buffer GUID一覧を変更し、port/channel範囲、buffer欠落・重複・曖昧なheaderを検証。変更履歴は文書内100件、無効edit/loadは状態を変更しない。Unicode名前はUNAMのみ変更。

Frameworkが.aup/.audを所有・再読込し、native Projectに新規登録、document Saveのpending metadata journal、nnam/filh同期、dirty拒否、二重owner拒否、copy closureとruntime検証へ接続。保存に失敗した際の状態確定は既存atomic writeに従う。全40責務の完成を意味しない。本体File AudioPath Documents...にNew/Open/Save As/名前/route→buffer接続/UndoRedoを実装したが、GUI操作は未実行。現行main treeのAudioPath表示、独立編集window寿命と全操作のGUI受入は残る。

検証：work/acceptance/audiopath-document/20261004T083741388Z/run.jsonの20checks。16byte route GUIDだけの変更、UndoRedo exact、missing/repeated/empty buffer拒否、malformed load後保持、別Framework native復元/完全再保存、Unicode名/unsaved Project拒否、二重owner拒否、元Source folder不在で2文書Copied.pro復元、runtime2filesを確認。独立Inspect-AudioPathDocumentは原版+未知3byte/padC7、GUID-only route16byte、UNAMだけ、SDK default stereo GUID、native2entriesのGUID・size・正確FILETIME・nnam/rnam、copy Project全bytes、runtime編集metadataだけの除去を照合。関連247checks/export7/独立runtime監査も同ビルドで合格。全core未実行。

同版の再生はwork/acceptance/runtime-audiopath-input/20261004T083812972Z/audio-20261004T083847701Z/audio-proof.json。APFarm設定PChannel10→Performance16と観測音符、source-created440Hz PCM・Slow Attack、両再生先頭3音、自然終了前Stop/restart/final silence、録音packet/UTC-QPC、3派生反例拒否を確認。原Source不在で書出物のみをロード。新規AudioPath factoryのdefault設定や今回変更したbuffer接続の発音を確認した試験ではない。旧版のdefault-path録音成功を現行へ転用していない。再生56/host24module原版40hash一致0は点snapshot。

製品SHA256 6d0953d23d61a852f10f597c55b81df5683368a08da81d1c76d3bcb5d497248a、71sources snapshotのconfigure/build/install0。最初の候補083455625Zで20/247/7/native host/audioが成功、今回失敗ビルドなし。各tool command/runは対応build/input/hash/結果に結び付く。録音器は固定032918787Z。新画面・default factory DirectMusic・全core/full40/full8は未確認。

再現：Build-ProductSnapshot.ps1で新しいsummary→Test-AudioPathDocument.ps1 -BuildSummaryPath [summary]→Node Inspect-AudioPathDocument.mjs [run dir]。同summary Test-JazpSave→Inspect-RuntimeExport、Test-AudioPathExport→Inspect-AudioPathExport、Test-ProductHost→Inspect-ProductModules。Prepare-RuntimeAudioPath→Inspect-RuntimeAudioPathInput→Test-RuntimeAudioPathAudio→Inspect-RuntimeAudioPathAudio→Test-RuntimeAudioPathAudioAuditor、同audio runのInspect-ProductModules -CaseName audio-lifecycle。GUIメニューは未検証のため再現合格手順として扱わない。

残件：新規default設定の実再生、owned AudioPathをSegmentへ割当・設定同期、PChannel追加/削除/範囲編集、buffer追加/削除/各property/FX/toolgraph/mixin/shared/send、GUI操作/通常終了/別起動復元・原版動的比較、runtime Save As/既定rdir・rfld・rnam/既存出力更新、残る40責務・8受入。Windows標準DirectMusic/DirectSound/GM.DLS/WASAPI依存を継続。旧18804の前面化質問を保持し、新GUIを同条件で増やさない。

次の具体的な一手：Connect owned AudioPath to Segment through an undoable Framework assignment and product control; preserve source/Segment identity and original configuration, verify save/separate reopen/runtime export and actual playback. Fix native preparation to allow already-matching PChannel0/Band routing, then verify source-created default stereo path and buffer-edit output via same-build DirectMusic/API/WASAPI, rather than only APFarm. AudioPath GUI New/Open/route/name/Save/Project Save/exit/reopen remains unexecuted; preserve old pending18804 without same-condition GUI launches. Continue PChannel range editing, buffer/FX properties, runtime Save As/defaults/native metadata and all40/all8.


## 2026-10-04 owned AudioPath割当とdefault実再生

現行090137091Z保存71sources構成/build/install0。Framework/Segmentのowned AudioPath独立copy割当・置換・削除/UndoRedo/native保存復元、編集画面のSegment操作を接続。自作default Stereoの必須pprhを追加、既存一致PChannel0の準備とowned/embedded同時書出しを実装。専用14＋文書20/raw/host passed。元Source不在・自作設定PChannel0→Performance16でWASAPI440Hz/Slow Attack/途中Stop/再開/停止無音、派生PCM3拒否passed。再生55/host24modules原版40hash0。保存回帰は中間085553818Zで146件後Windows error5、未解決・同条件再試行なし。現行全core/GUI/原版動的比較/全40/全八は未完了。 証拠：work/analysis/audiopath-assignment/20261004T085500Z/report.md。訂正：旧default factoryのpprh省略は不正であり現行では必須36byte headerを持つ。埋込みは独立copy、後の単独文書編集は自動伝播させない。Next：Verify edited routing output and Segment embedded independent-copy behavior with real APFarm buffers and same-build calibrated audio; implement PChannel range/route edit while retaining port/buffer/FX metadata, and complete AudioPath GUI New/Open/name/route/assign/remove/Save/Project Save/normal exit/separate reload when a targetable context is available. Investigate retained FreshDls.pro MoveFileEx Windows error5 through read-only lock/event evidence; no unchanged JAZP replay, security-setting changes or permission bypass. Retest full affected save suite only after cause/conditions change is established. Continue native runtime Save As/default paths/rdir/rfld/rnam/update-existing transaction, Style/Motif output and all40/all8.


## 2026-10-04 AudioPath範囲編集／Sequence PChannel

現行20261004T092316964Z保存71sources構成/build/install0。AudioPath port/route PChannel範囲と既存buffer接続編集・UndoRedo/独立Segment copy/native保存復元/runtime全bytes保持を実装。Sequenceの誤った0～15制限をDWORD PChannelへ修正、予約broadcast拒否、UI接続。専用20＋文書20＋Sequence26/raw/host passed。編集APFarm local22の明示AudioPath実再生、元Source不在WASAPI440Hz/Slow Attack/Stop/再開/停止無音、派生PCM3拒否passed。再生56/host24modules原版40hash0。旧save Windows error5未解決、現行全core/GUI/原版動的比較/全40/全八未完了。 証拠：work/analysis/audiopath-range/20261004T091800Z/report.md。旧Sequence0～15制限はMIDIチャンネルとの混同であり、現行DWORD PChannelとreserved broadcast拒否へ訂正。


## 2026-10-04 設定AudioPath付きMotif再生

現行131847474Z保存73sources構成/build/install0。Motifの文脈SegmentからAudioPathを取得する設定carrierを接続し、再生はGetMotif生成物を維持。native28、configured四folder改名Runtime11出力/二filename書換え/元Source不在を独立監査。PChannel5→21、MIDI72/所有440Hz試料の880Hz出力、120 BPM各4onset、Stop無音/再開/最終無音を無人WASAPIで確認。反例3拒否、範囲外routeはDownload/Play前拒否。normal native17/保存監査もpassed。再生56/host24modules原版40hash一致0。現行GUI/全core/原版比較/全40/全八未完了。 証拠：work/analysis/runtime-motif-audio/20261004T133000Z/report.md。GetMotif生成物を再生し文脈SegmentはAudioPath設定取得だけ。現行実GUI/全受入未完了。


## 2026-10-04 Owned AudioPath Motif GUI

現行141957011Z保存75sources構成/build/install0。本体Motif再生画面で所有AudioPathを明示選択できるよう接続し、GetMotif再生を保持した私有設定carrierを実装。native28/Runtime11出力監査/元Source不在、同GUI呼出しCLI録音880Hz120BPM/Stop再開passed。実GUI PID15040でも所有AudioPathを選んだPlay/Stopと録音内再開/Stopを別々の校正済WASAPI録音で確認、無音RMS0/75・43onset/880Hz120BPM。派生反例各3拒否、範囲外PChannelはDownload/Play前拒否。別GUI PID2012で保存Style/Motif/AudioPath候補復元・取消し、両通常終了0/入力全bytes保持。GUI128/再読込105/CLI56/host24modules原版40hash一致0。全体共通AudioPath/原版動的比較/physical isolation/全core/全40/全八未完了。

原版根拠：work/analysis/help/htm/auditioninganaudiopath.htm、playingmotifs.htm。共有defaultは残課題、今回明示Motif選択のみ。証拠：work/analysis/owned-motif-gui/20261004T144700Z/report.md。次：Implement the original documented Transport default AudioPath shared by component playback, including embedded Segment precedence and explicit standalone Motif selection, preserving existing session ownership and source bytes; validate new current build with relevant native cases and calibrated unattended recording only. Then remaining native per-file runtime folder memory from retained CHM/JAZP evidence, source save/reopen and integration acceptance. Do not replay frozen FreshDls.pro/Sound.dls error5; recovery Browse/dirty/source/configured/deletion GUI, large journals/partial metadata/races/directory ownership and full40/full8 remain.


## 2026-10-05 Transport default

現行150511775Z保存75sources構成/build/install0。Conductor共通default AudioPathを次のSegment/Motif要求へ適用し、埋込Segment優先・私有コピー/元文書保持を11checks/独立RIFF全bytes監査で確認。本体TransportメニューとMotif Transport defaultを接続、現行GUI未実行。新native Motif28/Runtime11出力・元Source不在、共通default経由GetMotifとWASAPI2回880Hz120BPM/Stop再開/3区間RMS0/3派生PCM拒否passed。再生56modules原版40hash一致0。再生中切替/未接続silent互換/GUI/全core/物理隔離/全40/全八未完了。 証拠：work/analysis/transport-default/20261004T151000Z/report.md。次：Validate current Transport menu and Motif default selection in actual GUI with calibrated WASAPI capture and normal exit/separate reload; verify real Segment embedded-path precedence using a conflicting default configuration. Then implement live AudioPath switching and original silent unconnected-PChannel behavior with multi-session ownership, without dropping document bytes. Continue retained per-file Runtime folder memory/native save/reopen and all40/all8; do not repeat frozen OS error5 failures without changed-condition evidence.

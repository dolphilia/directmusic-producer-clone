
# 2026-10-04 Frameworkの原版JAZP既存参照保存

全体未完了。前回mixed-tempo202500Zはsource PCM template解析/陰性対照/実原版JAZP観測と197証拠凍結の進捗あり。最新計画・ソースを確認、AGENTS.mdなし、既存変更/成功・不合格証拠を保持。全40責務/全八受入を縮小しない。

Framework.open_projectはJAZPをDMPJ catalog＋orig bytesへ取り込んでいたが.proへの保存を全面拒否していた。今回原版JAZPのLIST file/nameと未知metadataを区別し、既存catalog参照を保存したnative LIST file/nameへ更新する経路を実装。orig bytesをparseしてproj/pjct/rdir/rfld/fltr/pjpn、file/filh44byte、UNFO/rnam+nnam、node/edwp等を全bytes保持する。出力は実RIFF:JAZPでDMPJへ偽装しない。owned Segment/Style/Band/DLSは従来通りdirty未保存を拒否し、source SaveAs後の参照を同じroot indexへ反映する。原版file entriesと現catalogが一対一に対応しないと新filhを推測せず拒否。

原版metadataのruntime-export directoryは相対pathを含むため、取り込み時にobasというDMPJ専用basis chunkを保持。native出力はその元directoryでだけ保存を許可し、runtimeの未知の位置依存を変更せず保持する。DMPJの中間保存/再読込後もorig/basisが残りnativeへ戻せる。obasはJAZPへ出力しない。新nativeプロジェクト/newfile metadata生成・runtime path移動は残作業として明示する。これは今回の中間実装の制限であり全体受入条件の変更ではない。ファイル書込成功前はコピーrootだけを変更し、write_file_atomic成功後にFramework.root/path/reference/dirty stateを採用する。

本体Save ProjectのfilterへNative Producer project *.proを追加。元のDMPJ保存は維持。原版を開いた際の警告はread-only全面拒否という古い内容から現在の実装範囲へ変更した。現行GUIのnative保存操作/原版Producerでの出力読込は未実行。

work/build/product-snapshot/20261003T202847899Z/build-summary.json保存62sources、構成/compile/install各0、製品EXE 65460699b44757bdd4b8146159828f6a9374f3948e9120f2180b02d35a77eb10、core 07cd42ae2b82d064ea109775cd53ab1127326ce3ffe267d7073930b3800ac4f4。build.log warning/errorコードなし。保存sources/currentworkspace全hash一致。現行全core suite/GUI/audioは未実行。旧版GUI/録音合格を新生成物へ転用しない。

work/acceptance/jazp-save/20261003T203042257Zはcore --jazp-saveで実原版work/producer/samples/QuickStart/QuickStart.pro＋同dirのread-only入力を使用、native exit0/26checks。元project/実Style/実Segmentを試験dirへ複製し、.PRO大文字でも無変更保存が元2904bytesと全一致。既存Segment tempo768→137を編集しdirty native-saveを拒否、Segment renamed.sgp SaveAs後native出力の変更は元file/nameだけ。別Frameworkの再開/edited Segment全bytes復元/再保存が一致、DMPJ bridge→nativeも一致。元project metadata実filename/GUID/FILETIME/node位置/unsupported vssver.scc参照は保持し未知source-control操作は行わない。原版input fileは変更していない。

失敗経路: dirty保存のoutput未生成、別directory native保存でpath/state/outputを保持、新Segmentを追加した場合のnative保存は既存destination bytesを保持しdirtyのまま。DMPJへ保存すれば既存＋新Segment2文書を別Frameworkへ復元できる。これらはprevalidation拒否の証拠で、OS書込拒否/lock下のnative save失敗の新試験ではない。既存汎用atomic save testを今回のnative-specific成功へ転用しない。独立Inspect-JazpSaveは保存sources/生成物/入力/driver/native出力hashを結合しraw RIFFで変更name以外のbytes・橋渡しrefsを照合、passed。

work/acceptance/product-host/20261003T203041698Z製品host smoke exit0/由来点監査passed、原版40hash一致0。JAZP coreプロセスの動的module inventoryは今回未取得で、host inventoryの成功をcoreへ転用しない。Windows DirectMusic/DirectSound/GM.DLS依存は残る。今回プロジェクト保存はOS音声ランタイムを呼ばないが再生全体の不要化を意味しない。

台帳差異: Pattern clipboardのSPC1 copy/paste・fresh Part GUID再配置は既にstyle.cpp/framework.cpp/main.cppにある。古いfeature-mapの一般Clipboard未実装という記述を今の実装範囲へ訂正し、原版clipboard形式/GUI比較は残す。

再現: Build-ProductSnapshot.ps1 → Test-JazpSave.ps1 -BuildSummaryPath <同summary> -Project work/producer/samples/QuickStart/QuickStart.pro → Inspect-JazpSave.mjs <run dir>。Test-ProductHost同summary → Inspect-ProductModules -CaseName host-smoke。GUI filter/原版読込は別試験なので未実行を成功へ埋めない。

次の具体的な一手: 原版Producerで新規/文書追加/.pro SaveAsを観測し、pjct/filh identity・time/type/runtime pathの生成・relocation仕様を定めて新規JAZP作成と新参照保存を実装する。現行native出力の原版/自作GUI読込、native-specific lock失敗のatomicityも確認する。既存Clipboard原版互換/UI、本体未実装designer/全40/全八、同時二音source template/原版再生比較/長期通知制約を継続。

# 2026-10-04 空のネイティブJAZP生成と原版読込み

全体未完了。最新計画/前回jazp-save203300Z/指示を確認し、既存変更を保持。全40責務/全八受入の条件は変更しない。Computer Use skillで原版を操作。失敗後の古いUI indexを再使用せず再観測、原版の既存QuickStartは閉じたり破棄したりしていない。

原版New ProjectをNativeNewという名前、work/analysis/jazp-original/new-20261003T203900Z/NativeNewで作成。original-blank.proは372bytes、RIFF JAZP/LIST projにpjct(WORD16+GUID16+UTF16作成者dolph)、UNFO/rdir(..\RuntimeFiles\)、空pjpn、空rfld、open bookmark/componentがある。実観測のpjctは30bytes。製品は作成者Producerを使い36bytes、bookmark/componentの既定状態を推測生成せず、projの四項目を生成する。原版固有EXE/DLL/OCXを製品生成に呼び出さない。CoCreateGuidで新規identityを作り、orig/obas保持により同一project再保存でidentityを保持。文書参照がある新projectは引き続きnew filh未実装を拒否する。

原版Open Projectでwork/analysis/jazp-original/new-20261003T203900Z/source-blank.proを指定すると、親folder名がsource-blankでないため明示的拒否。警告を保存し通常OKで閉じ、同じbytesをwork/analysis/jazp-original/new-20261003T203900Z/source-blank/source-blank.proに置き読込み。source-blankがproject treeに追加され、エラーダイアログなしをfresh UI tree/screenshotで確認。入力hashはcore生成empty.proと一致し原版で書き換えていない。これは空JAZP読込みの原版比較であり、文書追加/原版再保存/自作GUI/再生受入の成功ではない。製品は現時点でこのfolder名条件を強制していないため、任意filenameでのnative SaveAsは原版再読込に使えないことがある。次にこの条件をUI/Frameworkへ統合する。

work/build/product-snapshot/20261003T204639275Z/build-summary.json保存62sources、構成/compile/install各0。製品EXE bbef89d9c34cd3df5eeeea2bdd4515e486a5e5ca5c1bd5e2a91bc374a9bf96a0、core 325f26131ec14fed00b198ece71cafebcea32b4b3dfc00915cc9b673153e85d2。work/acceptance/jazp-save/20261003T204825072Z core --jazp-save exit0/38checksとInspect-JazpSave独立raw監査passed。空JAZPのGUID長/作成者/rdir/pjpn/rfld、別projectGUID非同一、再保存・別Framework再読込全bytes一致を確認。既存QuickStartの26checksも同生成物で実行。stdoutのscope文字列は旧existing-entry文言が残るが保存sourceのtest本体・38checks/driver/監査が新しい試験範囲を記録している。

新規保存とimported保存それぞれでCreateFileW sharing lockを取得し、実MoveFileEx replaceが拒否される経路を検証。新規はsentinel destination・path空・dirty保持、importedはold JAZP destination・元project path・dirty保持。失敗後DMPJのorigが書込前bytesのままで、未保存metadataを採用しないことも確認。自分で取得したhandleを閉じた後、成功保存・別Frameworkで参照復元を確認。OS policy拒否を迂回した試験ではなく、アプリ所有の共有lockの正常な失敗処理試験。独立監査は書込後raw差分、orig保持、残留temporaryなしを照合。

work/acceptance/product-host/20261003T204825562Z本体host smoke exit0、24modules由来点監査passed、原版40hash一致0。最初の監査呼出しはRunDirectoryという存在しないparameterを指定したため実行前に失敗、正しいRunPathで一度実行し成功。JAZP core process動的一覧/製品GUI/音声/全core suiteは未実行。前版音声証拠は保持するが本版へ転用しない。Windows DirectMusic/DirectSound/GM.DLS再生依存は残る。全40機能と全八受入は未完了。

再現: Build-ProductSnapshot.ps1 → Test-JazpSave.ps1 -BuildSummaryPath <summary> → Node Inspect-JazpSave.mjs <run directory>。Test-ProductHost同summary → Inspect-ProductModules.ps1 -RunPath <host/run.json> -CaseName host-smoke。原版比較はOpen Projectで生成物を同名folder内に置き、入力bytes/hashを保存してproject treeとerror dialog有無を観測。構成・compile・native runtime・本体受入は別判定。

次の具体的な一手: 原版でsource-blankまたはNativeNewへ新Segment/Style追加して保存しfilh44bytesのidentity/time/type/UNFOと実文書を比較。原版folder名条件をnative UI/Framework/testへ統合し、新file metadata生成/保存復元へ進む。runtime relocation、Clipboard原版互換/GUI、未実装designer、同時音source templateと全40/全八も継続。

# 2026-10-04 ネイティブJAZP新規Segment参照

全体未完了。最新計画/指示/直近jazp-empty記録を確認し、既存成果を保持。全40責務と全八受入の範囲は維持。Computer Useで別観測用NativeEntryへSegment1を作成し、文書保存後に明示Save Projectを実行。空project372bytesと追加後1714bytesを別保存。原版filh44はファイルGUID16、最終更新FILETIME8、size DWORD4、文書guid16と実データ一致。サイズ406と更新時刻134355346640421313、末尾16bytesはDMSG/guid一致。文書型GUIDという以前の仮説は棄却。原版GUIDは生成ごとに異なるため同一値を製品へコピーしない。原版LIST node/edwpとLIST openはGUI状態であり製品は推測生成しない。

Frameworkは新規Segmentのみnative LIST file生成を追加。CoCreateGuid独立file identity、実保存ファイルの時刻/サイズ/文書guid、相対name、rnam .sgt、UNAMまたはstemのnnamを保存。新projectとimported/native再読込み後の追加を扱い、既存file metadataは維持。未保存/dirty文書や新Style/Band/DLS、移転は拒否し、成功write後だけroot/path/reference/dirtyを採用する。失敗時に新metadataを採用しない。native folder==stem条件は現時点未統合。GUIには既存Save Project .pro経路があるが本版製品GUI未実行。

構成/compile/install: work/build/product-snapshot/20261003T210944411Z/build-summary.json保存62sources、各0。core work/acceptance/jazp-save/20261003T211217861Z exit0/48checks、独立raw監査passed。新Segment GUID/time/size/names、未保存拒否、別Framework完全復元、再保存全bytes、追加既存metadata保持、既存QuickStart、sharing-lock失敗保持、新Style未対応拒否とDMPJ復元を検証。原版比較: 同run生成FreshSegment.pro/First.sgpを同名folderへhash一致コピーし、原版Open ProjectでFreshSegment/First.sgp treeを確認。First.sgpをdouble clickして原版Tempo120編集画面も確認(source-segment-editor-confirmed)。文書UNAMを持たないためタイトルSegment名空欄。再保存/再生/全機能編集は未実行。

本体 work/acceptance/product-host/20261003T211217487Z smoke exit0、実module点一覧由来監査passed、原版40hash一致0。core動的modules/製品GUI/本版音声/全core suite/全八は未実行。Windows DirectMusic/DirectSound/GM.DLS依存は残る。過去音声成功を本版へ転用しない。

失敗保持: 210516724Z buildは試験コードstyles()誤記によるC2039でcompile失敗、style_documents()へ修正。210709525Z buildは成功したが210906886Z runの48番目でStyle件数期待値2が誤り失敗。QuickStartはStyle2件＋追加1件なので保存前Framework件数との照合へ修正。その版host210906465Z成功は現行へ転用せず、現行hostを別実行。いずれも入力・出力・ログを保持。

再現: Build-ProductSnapshot.ps1 → Test-JazpSave.ps1 -BuildSummaryPath <summary> → Node Inspect-JazpSave.mjs <run>。Test-ProductHost同summary → Inspect-ProductModules.ps1 -RunPath <host/run.json> -CaseName host-smoke。原版Open Projectは生成物と同名folderを使い、hash一致とUI treeを保存。

次の一手: native folder名条件をFramework/Save Project/試験に統合し、本版製品GUIで新規Segment .pro保存・終了後別起動復元を確認。原版Style追加filh/UNFO観測からStyle新参照を実装。runtime移転、Clipboard、同時音声、残designer/全40/全八も継続。

# 2026-10-04 ネイティブproject保存先の名前検証

全体未完了。直近jazp-segment記録、計画、リポジトリ指示（AGENTS該当なし）を確認し既存変更を保持。原版で確認したfolder名と.pro stemの一致条件をFramework save_projectへ統合。大小文字はWindows ordinal case-insensitiveで照合（原版大小文字差異は未観測）。失敗は書込み・project metadata採用前に拒否。GUIのnative filterへ条件表示を追加。.dmpjは任意名を許可し、native openは観測用snapshot名も読める従来挙動を保持。

構成/compile/install: work/build/product-snapshot/20261003T211852898Z/build-summary.json保存62sources、全0、保存source不変。Product a46f97d50ddfd86045cd9b43ac2dd7ae4662bfccd58a62b394d547f495a0dfb3。core work/acceptance/jazp-save/20261003T212028408Z exit0/51checks。独立raw schema4 passed。不一致の既存destination bytes/path/dirty保持、DMPJ任意名、empty/newSegment/追加/既存ref/lock失敗を照合。各native書込み先をfolder一致に変更し、別名.proは比較snapshotとして明示。FreshSegment.proは追加後2文書、first-state.proは初期1文書の比較artifact。古い成功は現行へ転用しない。

本体 work/acceptance/product-host/20261003T212028019Z smoke exit0、点一覧24modules、原版40hash一致0。製品GUI work/acceptance/product-project-gui/20261003T212052530Z exit0。Computer Useのtool返却画面上でNew Project→768clocks/137BPM追加→Created.sgp保存→wrong-name.pro拒否案内→GuiNative/GuiNative.pro保存を観測。実ファイルの独立raw監査 work/analysis/jazp-folder/gui-20261003T212100Z/gui-save-proof.jsonはDMSG208bytesの0/120と768/137、JAZP306bytesのCreated.sgp参照、filh44の文書GUID/時刻/size一致を確認。wrong-name.proなし。GUI点一覧103modulesは由来監査passed/原版40hash一致0。

証拠収集障害: nfArchiveのREPL closureが古いnfStateを捕捉しており、保存したJSON/JPGが同じstartup状態を反復した（JPG SHA256全5c7fbec1d4b34f62575c8f8cfd1d34e8130ca387dc89a84b02ed49760a46f374）。誤った画像をGUI成功証拠へ使わず、失敗captureとして保持。tool返却画像での観測と、独立保存物/正常exit/module証拠を分離した。次回helperはstateを引数に渡しinclude_text:trueを明示する。

再起動 work/acceptance/product-project-gui/20261003T213125601Z PID4388は入力hashとEXEを結合して起動、応答/本体handleは存在するがComputer Useのlist_windows/list_appsへ返らずGUI復元未確認。main.cpp startupのProject limitations modalで待機の可能性はあるが未観測なので原因確定しない。同条件で再起動を繰り返さずユーザーへ表示時のOKのみ依頼済み。再読込み/再保存/終了は未確認、running snapshotはこの時点の状態として保持。OS拒否なし。

現行音声、全core suite、全40/全八は未実行/未完了。Windows DirectMusic/DirectSound/GM.DLSは残る。新Style/Band/DLS native参照、runtime移転、既存filh更新、default UNAM等は残作業。

再現: Build-ProductSnapshot.ps1→Test-JazpSave.ps1 -BuildSummaryPath <summary>→Node Inspect-JazpSave.mjs <run>。Test-ProductHost→Inspect-ProductModules -RunPath <run.json> -CaseName host-smoke。Test-ProductProjectGui同summary、新規文書GUI保存→Capture-ProductGuiModules→Inspect-ProductGuiModules。work/audit-jazp-folder-gui.cjs <gui>は保存物/lifecycle/modulesのみ監査。

次の一手: 再起動GUIの操作可能化（非破壊のproject limitationsを非modal表示へ変更することも検討）と正しいstate captureで保存復元を確認。原版Style追加metadata観測から新Style参照を実装し、本体文書管理を広げる。全範囲と完成条件は維持。

# 2026-10-04 Project制限案内の非モーダル表示

全体未完了。直近jazp-folder147証拠の記録から継続。main.cppのstartupとOpen Projectで毎回表示していた非破壊Project limitations MessageBoxを除き、Framework warningsを再生状態欄へ常時追記。欄はread-only multiline EDIT＋vertical scrollへ変更して長い案内も読めるようにした。Segment/Style/Bandどのモードでも案内を表示。失敗や未保存の確認ダイアログは保持。無人再起動が確認ボタン待ちになる経路をなくす目的。

構成/compile/install work/build/product-snapshot/20261003T213746885Z保存62sources全0、Product 163692e3f8fde0eb5c43f220bd82efe59823d7c756da3d3c9542826a46f08819。host work/acceptance/product-host/20261003T213928279Z smoke exit0/由来監査passed/24modules原版40hash0。GUI work/acceptance/product-project-gui/20261003T213926754Zは起動入力hashを記録してPID14680、Responding true/main handleあり。しかしComputer Use list_windowsは本プロセスを返さず、UI操作/視認/復元/再保存/終了の受入は未実行。main modalを外しても一覧に出ないので旧PID4388の未表示原因をmodalだけと確定しない。同条件の再起動やOS設定変更はしない。GUIプロセスの点一覧39modules由来監査passed/原版40hash0はUI合格を意味しない。

GUI入力は前版のGuiNative.pro/Created.sgpを別作業folderへhash一致copyし、.sgp最終更新時刻も保存。前版の51checks/raw、GUI生成保存物/正常exit0は前版211852898Zの成果であり現行へ転用しない。main以外の保存モデルは今回不変。現行core suite/関連JAZP実行/音声は未実行。Windows DirectMusic/DirectSound/GM.DLS依存、新Style/Band/DLS native参照/移転/全40/全八は未完了。

前版GUI画像保存はclosureの古いstate参照で全同一startupを保存した障害を保持。次回はhelperの引数にその場で得たstateを渡し、include_text:trueでfresh状態を収集する。現行は操作対象が取得できず画像採取なし。launch runningの証拠は記録時点snapshotであり終了結果と推測しない。

再現: Build-ProductSnapshot.ps1→Test-ProductHost.ps1同summary→Inspect-ProductModules.ps1 -RunPath <host/run.json> -CaseName host-smoke。Test-ProductProjectGui.ps1同summary/GuiNative.pro/Created.sgp→Capture-ProductGuiModules→states.json→Inspect-ProductGuiModules。

次の一手: 操作対象が取得できる場合に非modal案内とnative GUI復元/再保存を確認。独立作業として原版Style追加のfilh/UNFOを観測しFramework新Style native保存へ進む。未確認GUIを全体完成に算入せず、全対象/全八の範囲維持。

# 2026-10-04 新規Styleのnative Project保存

全体未完了。前回nonmodal記録から継続。Computer Useの技能で原版の別New Project NativeStyle/New Styleを作成し、Ctrl+Sと明示Save Project NativeStyleで保存。既存Projectへ保存をかけない。work/analysis/jazp-style-original/20261003T214600Z/original-style-proof.jsonはJAZP1710bytes/DMST1792bytes、filh44のファイルGUID16＋実更新FILETIME8＋size4＋root文書GUID16、.sty runtime名、Style1 display名、4/4 ndscをraw照合。編集窓のplacementやruntime foldersはsession metadataとして生成しない。画像はfresh stateを引数で受け取り、複数のhashを確認した。

Framework native_segment_referenceをnative_document_referenceへ一般化しDMSG/DMSTを許可。新Style参照は実保存ファイルのGUID/時刻/size、.sty名、UNAM（なければstem）、styhの拍子説明を生成。未保存/dirty文書は拒否、全参照を仮生成した後のatomic write成功時だけroot/所有metadataを採用。既存参照全metadataは保持。新Band/DLS参照とruntime移転は引き続き拒否する。Project案内もSegment/Style対応へ更新。

構成/compile/install: work/build/product-snapshot/20261003T215310011Z/build-summary.json 保存62sources、不変、各exit0。Product 845efd3bc13ebc7d3018e7a1d57549b4f76b1805d0d3afde1c74adbd30b53d2c。初回work/build/product-snapshot/20261003T215251588Zは誤ってWindows PowerShell5を指定しGetRelativePathなしで構成前にexit1。その空source準備dirと原因を保持し、既存のPowerShell7.6.6で別buildを実行した。OS拒否なし、設定変更なし。

関連実行: work/acceptance/jazp-save/20261003T215542026Z exit0/64checks、raw schema5 passed。新Style137BPM/3/8/NativePattern保存、unsaved/dirty拒否でbytes/dirty保持、Undo、別Framework全bytes復元、同一保存identity保持、第二Style＋Segment追加と既存metadata保持、混在別Framework復元を確認。既存native/Segment/lock/移転拒否も関係する経路のみ実行。未知Band失敗時の既存ファイル保持、DMPJ bridgeで全新文書を保持。raw監査はFirst.stp実時刻/size/GUIDとFirst.sty/First/3/8、追加3entries/別GUIDも独立照合。全core suite/音声は未実行。古い生成物の成功を転用しない。

本体: work/acceptance/product-host/20261003T215541584Z --smoke exit0、点一覧24modules由来監査passed/原版40hash一致0。GUI操作はこの版で未実行。ユーザーが旧警告OKを閉じた回答後もComputer Use一覧は旧自作本体PID4388/14680を返さなかった。旧launcher213125601Z/213926754Zは15分timeout、exitCode null、強制終了なし。これを正常終了やGUI合格に算入しない。同条件起動を繰り返さない。

比較: 現行core生成first-state.proとFirst.stpをsource-open/FreshStyle/FreshStyle.proへhash一致copyし原版でProject展開/First.stpを開いた。work/analysis/jazp-style-original/20261003T214600Z/source-open/open-proof.jsonで入力全bytes不変、fresh tree137.00、画面3/8/NativePatternを記録。root UNAM未生成のため原版がStyle2を自動付名、native nnamのFirstと差あり。default Bandも空で原版New Style Band1とは差あり。これらを同等動作完成とは扱わない。原版比較EXEを製品依存へ混入しない。

残る依存と未完了: Windows DirectMusic/DirectSound/GM.DLS、全40責務/全八受入、native新Band/DLS/移転/export、既存filh更新、文書名/原版既定初期値、現行GUI再起動保存/音声。録音による音声自動確認の既存計画は維持し今回無音文書保存試験へ流用しない。

再現（PowerShell7）: Build-ProductSnapshot.ps1 → Test-JazpSave.ps1 -BuildSummaryPath <summary> → Node Inspect-JazpSave.mjs <run>。同summaryでTest-ProductHost.ps1 → Inspect-ProductModules.ps1 -RunPath <host/run.json> -CaseName host-smoke。原版観測は別New Project/New Style/保存、source比較はprepare-jazp-style-open.cjsとComputer Use Open Project/Style、audit-jazp-style-open.cjs。

次の具体的な一手: 原版New Bandのfile/UNFO/文書GUIDを別Projectで観測し、同じFramework native保存へ追加する。GUI操作対象が取得できたら自作本体のnative Style復元/再保存/正常終了を実行。文書名と既定Band差も残し、全対象/完成条件を縮小しない。

# 2026-10-04 新規Bandのnative Project保存

全体未完了。新Style単位から継続し、既存変更・証拠を保持。Computer Use技能で原版の独立New Project NativeBand/New Bandを作成し、文書Ctrl+Sと明示Save Project NativeBandで保存。work/analysis/jazp-band-original/20261003T220300Z/original-band-proof.jsonはJAZP1684bytes/DMBD1104bytes、filh44（file GUID16＋実FILETIME8＋size4＋root guid16）、Band1.bnd runtime/Band1 display、ndscなしを独立照合。初期372bytesのProjectは保存前の観測で、raw snapshotなし。Original Band Editorはunsupported-operation警告を出したが、OK後16楽器を表示した。警告をOS拒否や全Editor失敗とは扱わない。

実装: 新規Band factoryで非ゼロ・個別GUID16を生成。既存Band loadにGUIDを追加しないため未知chunk/legacy bytesを保持。Frameworkのnative参照生成へDMBDを追加、実ファイルのguid/更新時刻/sizeと.bnd runtime、UNAM（なければstem）を保存。Band ndscを創作しない。未保存/dirty拒否、既存metadata保持、全参照を仮生成しatomic保存成功時だけ所有metadata採用を維持。新DLSは引き続き明示拒否し、DMPJで保持。

構成/compile/install: work/build/product-snapshot/20261003T221222064Z/build-summary.json、保存62sources不変、各成功。Product SHA256 a76eb3f98f9a3a2a202870033a271158cbaaf5751a6ba62cd4318ca69060b8e0、core 5662fde1c96352f58709f8f6d4152326bfee1e178beedd6de0b26f4a01f769c1。関連実行work/acceptance/jazp-save/20261003T221411650Z exit0/150checks。新Band GUID独立、Violin40編集、実file metadata、dirty時既存Project保護、Undo全bytes/identity、別Framework復元と同一再保存、第二Band＋Style＋Segment追加、既存entry保持、Segment内Band snapshot復元を確認。既存Band lossless/編集、独立文書所有/SaveAs、BandTrack時間/所有など今回factoryの影響範囲も実行。新DLS参照は保存拒否でdestination/path/dirtyを保持し、DMPJ全4種復元を確認。独立raw schema6でfilh/guid/time/size/runtime/ndscなし、4混在entriesの独立fileGUIDと最初のmetadata保持を照合。全core suite/音声は未実行。古い版の成功を転用しない。

本体: work/acceptance/product-host/20261003T221411083Z --smoke exit0、24点modulesの由来監査passed、原版40hash一致0。この版のGUI/通常終了/音声/full8は未実行。以前のGUI操作対象未取得は継続課題として保持し、同条件の起動を繰り返さない。原版比較EXEを製品依存にしない。

比較: 現行core first-state.proとFirst.bnpをsource-open/FreshBandへhash一致でcopy。最初は誤ってforward-slashパスを原版file dialogへ入力しfilename拒否、project-open画像は失敗証拠。Windowsパスへ修正した別操作でProject展開とBand Editor表示が成功。原版自身New Bandと同じunsupported-operation警告はOK後解消し、FreshBand/First.bnp/Band1選択とPCh1 Violin一行を画面で確認。work/analysis/jazp-band-original/20261003T220300Z/source-open/open-proof.jsonで入力bytes不変、状態/画像hashと目視範囲を区別した。UIA文字列からViolinが抽出されたとは主張しない。保存や再生は行わない。root UNAMなしのため表示Band1とnative nnam Firstが異なる、source factory空/原版16楽器の差を残す。

残る依存: Windows DirectMusic/DirectSound/GM.DLS。未完了: 全40責務/全八受入、native新DLS・移転・runtime export、既存filh更新、文書名/既定値、本体GUIの現行保存再起動と音声自動受入。既存WASAPI録音自動確認計画を維持し、今回無音保存検証を音声合格に数えない。

再現（PowerShell7）: Build-ProductSnapshot.ps1 → Test-JazpSave.ps1 -BuildSummaryPath <summary> → Node Inspect-JazpSave.mjs <run>。同summaryのTest-ProductHost.ps1 → Inspect-ProductModules.ps1 -RunPath <run.json> -CaseName host-smoke。原版独立New Project/New Bandを保存→audit-jazp-band-original.cjs。prepare-jazp-band-open.cjs <core run>→Computer UseでWindowsパスOpen Project/First.bnp→audit-jazp-band-open.cjs。

次の具体的な一手: 原版の新DLS文書を独立Projectで保存し、dlid/filh/rnam/nnamを照合してFrameworkのnative DLS保存へ追加する。GUI操作対象が取得できれば現行本体でnative混在Project復元・編集・保存・正常終了・別起動を検証する。原版既定値/表示名差と全40/全8を維持する。

# 2026-10-04 DLSのnative Project参照保存

全体未完了。既存Band単位から継続し成果を保持。Computer Useで独立New Project NativeDls/New DLSを作成。New DLS直後に180bytesのDLS Collection1.dlpが生成済み。Ctrl+Sは前面Band文書を対象にしていたため、DLS保存の根拠にはしない。FileのSave Project NativeDlsを明示実行し1508bytesのJAZPを保存。原版監査work/analysis/jazp-dls-original/20261003T222100Z/original-dls-proof.jsonでfilh44（file GUID16＋FILETIME8＋size4＋root dlid16）、.dlp文書名/.dls runtime、ASCII INFO/INAMからUTF16 nnam、ndscなしを照合した。元の原版文書・既存Projectは保持した。

実装: Framework native_document_referenceへDLS形式/root dlidを追加。表示名をINFO/INAMからWindows ACPで変換し、runtimeを.dlsにする。ComponentCatalogは.dls/.dlp両方を読込・保存・Project復元の対象にする。欠落dlidは参照を創作せずnative保存拒否、DMPJ保持は継続。DLS工場や新規作成UIはまだ実装していない。原版空DLS180bytesの完全再現を主張しない。非ASCIIの原版比較は未実行。

構成/コンパイル/install: work/build/product-snapshot/20261003T222840303Z/build-summary.json、保存62sources不変、各exit0。Producer SHA256 ff885314ee33e251315d598defca32630a89753c898f3d8208dd1f41aeb826a4、core SHA256 9d6d6c242d04f40627017242bcaa670c15baf427f6be438faafd539557eb1364。実行: work/acceptance/jazp-save/20261003T223028287Z/run.json exit0、158関連checks。新.dlp参照の実FILETIME/size/dlid、ANSI表示/runtime、opaque奇数paddingを含む別Framework完全復元、同一Project再保存、BandへDLS指定、SegmentへBandコピー、Styleも混在したProjectを別Frameworkで復元しBand/Segment再生依存のbytesを照合。欠落ID拒否は既存destinationとdirty状態を保持する。これは依存解決APIの確認で、実際の再生ではない。

独立監査: raw schema7で同じ生成物/入力に結び付けてDLS参照と混在4entry/既存metadata保持を照合。最初の監査は二重commaの構文エラーで未実行だった。監査だけ修正して同じ試験出力を再監査し成功。core試験を再実行していない。失敗理由は本記録に保持。

本体: work/acceptance/product-host/20261003T223025818Z/run.json smoke exit0、24点module監査passed。原版40hash一致0。現行GUI・通常GUI終了・音声・全core suite・全八受入は未実行。古い版の成功は現行へ転用しない。製品実装は比較用原版EXE/DLL/OCXを必要としないが、全40責務の完成には未達。Windows DirectMusic/DirectSound/GM.DLS依存は宣言したランタイムとして残る。

再現: PowerShell7でBuild-ProductSnapshot.ps1 → Test-JazpSave.ps1 -BuildSummaryPath <summary> → Node Inspect-JazpSave.mjs <run>。同summaryでTest-ProductHost.ps1 → Inspect-ProductModules.ps1 -RunPath <run.json> -CaseName host-smoke。原版独立New Project/New DLS/Save Project→work/audit-jazp-dls-original.cjs。WASAPI録音による無人音声確認計画は維持する。

未完了/次の具体的な一手: 現行source FreshDlsのfirst-state.pro/First.dlpを原版へ別copyで開き、Collection表示と入力不変を確認する。続いて観測済み空DLS構造をもとにsource DLS新規文書工場と本体New DLSを実装し、保存・別Framework/原版再読込まで検証する。本体GUI操作対象が取得できる場合には混在Projectの編集・保存・正常終了・別起動・無人音声受入へ進める。runtime export/移転/既存filh更新/全40/全8を縮小しない。

# 2026-10-04 DLS新規文書工場と本体コマンド

全体未完了。直前DLS native保存単位は実装・証拠更新のあるprogress。現在計画/最新記録/指示（AGENTS検索なし）を確認して継続し既存変更を保持。

実装: DlsDocument::create()は原版観測の空RIFF DLS 180bytesをソースから構築する。CoCreateGuidの個別dlid、colh0、vers1.0/1、空lins、ptbl8/0、空wvpl、INFOのICMT/ICOP/IENG/INAM/ISBJを作る。createだけで生成しloadへGUIDを足さない。未保存dirty、Undo/Redo履歴なし。Framework::new_collectionで所有・Projectdirtyを設定。本体File/New DLS Collectionから既存DLS Editorへ接続し、初回Save DLSは保存先を選ぶ。.dlp/.dlsをOpen/Saveダイアログの対象にし、新規Save既定を.dlpにする。GUIコマンドの実操作は未実行。楽器/Waveの新規追加はまだなく空文書作成まで。全体完成とはしない。

構成/compile/install: work/build/product-snapshot/20261003T223457111Z/build-summary.json、保存62sources不変、各exit0。Producer SHA256 00d2a563e81225759750b6b4baa47778453ed22e688421156d669b4f0c9f3d1a。関連実行 work/acceptance/jazp-save/20261003T223628020Z/run.json exit0/165checks、core SHA256 9f0eb672fa437f74982bccf6ec8c244682035f477a398077b6a0deee378ad90f。factory個別ID、空typed Instruments/Wavesとplayback事前検査、未保存Project拒否とstate保持、初回DLS保存bytes/identity保持、native別Framework復元と完全再保存を追加確認。既存158関連JAZP/Band/Collection復元を同生成物で確認した。独立raw schema7と work/acceptance/jazp-save/20261003T223628020Z/dls-factory-proof.json で原版空DLSとGUID16だけ正規化した全bytes一致。監査の古いfactory未実装limitationsを別監査参照へ直し、同runを再監査（試験再実行なし）。全core suite未実行。

本体: work/acceptance/product-host/20261003T223627617Z/run.json smoke exit0、24点modules由来監査passed/原版40hash0。現行source GUI操作・GUI正常終了・音声・全八受入は未実行。以前のsource GUI操作対象なしは未解消として保持し、同条件の起動再試行はしていない。Windows DirectMusic/DirectSound/GM.DLSは宣言した依存として残る。原版は比較専用、製品依存ではない。

原版比較: source factoryのFactoryDls.pro/Created.dlpをwork/analysis/dls-factory-original/20261003T223700Z/FactoryDlsへhash同一でcopy。Computer UseによりWindows絶対パスOpen Project、FactoryDls展開、Created.dlpをdouble-click。UIAと実画像でCreated.dlp配下DLS Collection1/空Instruments/Waves表示、dialogなしを確認。背面Band EditorのViolinはこのDLS試験の結果ではない。work/analysis/dls-factory-original/20261003T223700Z/open-proof.jsonは入力全bytes不変、現行core run/EXEと画面state/image/原版EXEを結び付ける。原版Save/Playは実行していない。

再現: PowerShell7 Build-ProductSnapshot.ps1 → Test-JazpSave.ps1 -BuildSummaryPath <summary> → Node Inspect-JazpSave.mjs <run> → work/audit-dls-factory.cjs <run>。同summary Test-ProductHost.ps1 → Inspect-ProductModules.ps1 -RunPath <run.json> -CaseName host-smoke。factory保存物を別copyしComputer Useで原版Open Project/Created.dlp → work/audit-dls-factory-open.cjs。既存WASAPI自動音声確認計画を維持。

残作業/次の具体的な一手: 空CollectionへInstrumentとPCM Waveを追加する原版操作を独立Projectで観測し、source typed作成とpool table/region cue整合、Undo/Redo/保存/別Framework復元を実装する。source本体GUI対象が得られる場合はNew DLS→保存→Project→正常終了→別起動を検証する。runtime export/移転/既存filh更新/非ASCII名/新文書の一意な表示名、全40責務/全8は未完了。完了条件を縮小しない。

# 2026-10-04 DLS新規PCM Wave追加とnative保存復元

全体未完了。最新factory単位/計画/指示を確認し既存変更を保持。原版の独立Projectを比較用にcopy。最初のcopyは開いている原版文書と同じGUIDだったため、取り違え回避として別Project/file/root dlidに独立IDを与えたDlsAuthorUniqueを準備（衝突が実際に起きたと断定しない）。旧copyへ編集保存はしていない。既存凍結NativeDls/FactoryDls成果は保持。

順序変更の根拠: 原版Instruments右click Insert InstrumentはWaveなしを拒否する警告を表示。観測を保存してOKで閉じた。従って新Instrumentより先にWave追加を実装する。対象/全体完成条件は縮小しない。Waves右click Insert Waveで独自生成Tone.wav（mono8000Hz16bit、800frames、440Hz正弦PCM）を追加。File Save Project DlsAuthorUniqueを明示し、そのDLS変更のYesだけを選択。Save First.bnp/CtrlSは対象が違うため実行しない。原版after-wave.dlp2030bytesはPCM全bytes/format、Wave GUID16、WSMP root60/options1/no loops、pool cue0、INFO/Toneを保持。wavu6、wavh16、smpl36も原版に存在。脚本Inspect-DlsWaveCreationは独立raw解析。

実装: DlsDocument::add_wave_pcmは空/既存Collectionへ非圧縮8/16bit mono/stereoPCMと新GUID/名前/WSMP/format/data/INFOを追加、ptblへ末尾cueを追加する。既存cueの番号/alias/拡張header/tail、未知pool chunkと奇数padding、既存Waveの全bytesを保持。全検査後一回adoptするのでUndo/Redoは一transaction。空/半端frame、曖昧data/format、不正byte rate、float/format拡張、非ASCII名、未実装sampler/loop/position metadataは拒否し履歴/redoを保持。本体DLS Editor Add PCM Wave...からファイル名stemを使い接続し新Waveを選択。空文書でも追加可能。原版wavu/wavh/smpl初期値は生成せずportable PCMとして扱う。この差を全Producer互換完成と数えない。GUI実操作は未実行。

構成/compile/install: work/build/product-snapshot/20261003T230126799Z/build-summary.json、保存62sources不変、各成功、Producer SHA256 ca8bc6ce541ba58b74a5dbabd2d1485ec52cd9427e3f9a7b229355a6ec8e78b1。最初の225800386Zビルドは構成/compile/install成功、core work/acceptance/dls-wave-creation/20261003T230039078Z は10checks後native Projectのfolderとbasename不一致でexit1。試験出力先をWaveFactory/WaveFactory.proへ修正して別buildを作成した。失敗source/生成物/ログ保持、OS拒否なし、同条件再試行なし。最初のapply_patchはUI enumの一致行なしで全patch拒否、分割して適用した。

実行: work/acceptance/dls-wave-creation/20261003T230339192Z/run.json exit0/17checks、core SHA256 522e8959737047779d5072b16b7e7e44e7408dfe39f5fd760cf291c60a629a3d。新Wave入力PCM保持/root Collection identity不変/独立Wave ID/default設定、単一whole-byte UndoRedo、invalid入力とredo保持、native実保存/別Framework全bytes復元/同一Project完全再保存、既存alias cue/opaque/header/tail/padding保持を検証。独立raw work/acceptance/dls-wave-creation/20261003T230339192Z/wave-creation-proof.json は現行1948bytesと原版2030bytesのfmt/wsmp/data/INFO/ptblを完全一致照合。filh44の実FILETIME/size/root dlid、runtime Created.dls/表示DLS Collection1を照合。GUIDは意図的に別。全coreと無関係な既存165JAZPを再実行していない。旧165の成功を現行へ転用しない。monofixtureのみ原版比較、8bit/stereo/非ASCII/sampler importは追加受入が残る。

本体: work/acceptance/product-host/20261003T230338769Z/run.json --smoke exit0、24点module由来passed/原版40hash一致0。fresh Computer Use list_windowsにも旧source本体は返らないため、同条件起動を繰り返していない。現行source GUI/Add PCM操作/GUI終了・音声/full8は未実行。Windows DirectMusic/DirectSound/GM.DLS依存が残る。原版EXEは観測/比較だけで製品の依存ではない。

原版再読込: 現行WaveFactory.pro/Created.dlpをsource-open/WaveSourceへhash同一copyしてWindowsパスOpen Project。末尾Projectは画面外だったのでscrollで表示し展開/Collection/Waves/Toneを開いた。UIAの画面外indexはcached boundsなしで操作できず、再観測した画面座標へ切替。原版Wave Editorが波形と8000 Hz 16 bit Mono, 800 samplesを表示、dialogなし。work/analysis/dls-author-original/20261003T224300Z/source-open/open-proof.json は入力2files不変と現行run/EXE/UIA/画像/原版EXEを結び付ける。保存/Playは未実行。windowオブジェクトの旧Band titleは実画面タイトルの証拠にせずfresh UIA/screenshotを使った。

再現: PowerShell7 Build-ProductSnapshot.ps1→Test-DlsWaveCreation.ps1 -BuildSummaryPath <summary> -Observation <original dir>→Node scripts/Inspect-DlsWaveCreation.cjs <run> <original dir>。同summary Test-ProductHost.ps1→Inspect-ProductModules.ps1 -RunPath <run.json> -CaseName host-smoke。原版Wave観測は別ProjectからInsert Wave/Save Project/DLS変更Yes。現行保存物を別copyしOpen Project/Collection/Waves/Tone→work/audit-dls-wave-open.cjs。

残作業/次の一手: Waveを持つ独立原版CollectionへInsert Instrumentを実行し、初期locale/region/articulation/nameを観測・保存してsource create_instrumentと本体操作を実装する。観測後の原版文書はこの単位の別copyで扱い、凍結after-wave/source-open入力を上書きしない。Wave Producer metadata/defaultsとsampler import、8bit/stereo/非ASCII、文書名一意性、runtime export/移転/既存filh更新、現行本体GUI/全40/全八は未完了。既存WASAPI録音による無人音声確認計画を維持し、今回は音声を合格としない。

# 2026-10-04 DLS Instrument作成・原版初期値比較・本体GUI保存履歴

全体未完了。計画と最新Wave単位を確認し既存変更/凍結成果を保持。原版Waveを持つ独立ZZInstrumentAuthorを準備。衝突回避用のProject/file/Collection/Wave identity変更は preparation.json に記録した入力準備であり、原版生成identityの観測と混同しない。before.dlp2030bytes SHA256 ced49658d17a1a236d48627aa9117df105a359d17ad6a1a294eb3ed85084b90e。

原版: Insert Instrumentでtree上0,1,0を確認。最初のInstrument Editorは「An unsupported operation was attempted.」の警告後にwindowが消失、保存未実行/入力不変をeditor-failure.jsonへ保持。起動後PID19248の操作対象なしを観測しユーザーが既知「Failed to update ...」のOKを閉じたとの回答後、fresh listからwindow4982572を取得。別条件としてEditorを開かずInsert Instrument→Save Project ZZInstrumentAuthor→変更DLSのYesだけで保存。after-instrument.dlp2260bytes SHA256174586553f50a796417cdd59fbced81918477ec46494cdb101c36bc16c910bed。insh region1/bank1/program0。Region rgnh14/full key0..127/full velocity0..127/options1/group0/layer0、Waveと同じWSMP、wlnk channel1/cue0、dmpr01000100。LIST lar2内art1 20bytes（cb8/count1/connection0000000000050000ffffff7f）。INFOはICMT/ICOP/IENG/ISBJ各空、Instrument DLID/INAMなし。未知dmprとconnectionの意味は推測しない。原版初回locale1/0の一例であり後続の自動割当規則は未確認。original-tree.jsonのscopeはraw parser単体を指し、この原版実操作の有無は保存前後/UI観測で別に証明する。

実装: DlsDocument::create_instrument(bank,program,name,cue)と本体Add Instrument (full range)を追加。mono PCM Wave検証を隔離文書で行い、Instrument/Region/原版で観測した初期articulation/Producer拡張/INFOを一回adopt。名前空ならINAMなし、明示ASCII名ならINAM追加。source固有Instrument DLIDを新規生成。全既存root/Wave/opaque/奇数padding/colh tail/ptbl aliasを保持。重複locale、無効bank/program/cue、不正name、未対応stereo placement、invalidroot/loopを拒否し空Instrumentを残さず履歴/redoを保持。GUIは明示Bank/Program/Cueと生成名を使う（原版自動locale割当との一致は主張しない）。既存create_regionは変更しない。

版別: 最初のwork/build/product-snapshot/20261003T232630067Zはportable defaultsの23checks成功、Wave17/host/raw成功だが原版保存比較は当時未実行。原版保存を得た後に初期値実装を変更したため、それらの成功を現行へ転用しない。現行work/build/product-snapshot/20261003T233543909Z/build-summary.json保存62sourcesはconfigure/build/install各exit0・source不変、Producer SHA256 06d4f4f8e669865f580aac7899326bb2ae0a6ab8fa9ff0ac820019b7a408791d。関連だけ再試験し work/acceptance/dls-instrument-creation/20261003T233735675Z/run.json exit0/25checks、work/acceptance/dls-wave-creation/20261003T233736437Z/run.json exit0/17checks、work/acceptance/product-host/20261003T233737433Z/run.json --smoke exit0。全core/旧JAZP165は再試験していない。

比較: work/acceptance/dls-instrument-creation/20261003T233735675Z/instrument-creation-proof.jsonは構造、Region/sample/独立ID、native Project実FILETIME/size/Collection identity、保存別Framework復元/全Project再保存、Band patch519とowned Collection snapshot、alias cue1/未知chunk/tail保持を独立rawで照合。元のbefore.dlpからbank1/program0/cue0/空名で生成したObservedShape.dlp2284bytesは追加したInstrument DLID24bytesだけを除いてcontainer lengthを再計算すると、原版after2260bytesと全バイト一致。日時/root/WaveIDや未知情報を広く正規化していない。明示名のCreated.dlp2216bytesは別fixtureなので原版2260bytesとの全一致とは扱わない。

本体GUI: work/acceptance/dls-instrument-gui/20261003T233913Z/launch.jsonで現行EXEを一度起動しwindow10750300をfresh listから取得、独立コピーBefore.dlpをOpen。DLS Editor window205260764でAdd Instrument→Save DLS→Undo→Save DLS→Redo→Save DLSを実行。GUI Created.dlp2226bytesはbank0/program0/name Instrument 0, 0と上記defaults。Undone.dlpは実入力1948bytesと全一致、Redone.dlpは実Created.dlpと全一致。work/acceptance/dls-instrument-gui/20261003T233913Z/gui-proof.jsonと保存画像/観測が根拠。UIA menu clickは負のbounds、file name set_valueはcached app stateなしで失敗。再観測した座標へ切替。ファイル名caretと実入力を画面で確認（focused_elementはsearchのまま返り、信頼しない）。原版・製品のOS拒否なし。終了前のユーザー入力検出はfresh stateで再観測した。REPL未定義変数は終了操作後の記録を中断したためfresh list/stateで確認。終了時「Discard unsaved documents?」が出たのでNoで取消、初期Untitled segment/Projectを保持。正常終了/GUI Project保存/再起動は未合格。PID17504は継続作業用に残っている。DLS保存物は独立rawで監査済み。別名保存・全GUI八受入はこの単位では未実行。

依存: host24点、GUI104点（address付き）は同版EXEと原版40PE hash由来を照合しpassed。GUIは一時点のinventoryで連続監視ではない。Windows DirectMusic/DirectSound/GM.DLS依存は残る。原版EXEは観測だけで製品に組み込まない。現行新規Instrumentのdownload/Play/無人音声録音は未実行。過去の聞こえた回答や録音成功を本版へ転用しない。全40責務と全八受入は未完了。

再現: PowerShell7 scripts/Build-ProductSnapshot.ps1 → scripts/Test-DlsInstrumentCreation.ps1 -BuildSummaryPath <summary> -Observation work/analysis/dls-instrument-original/20261003T231831Z → Node scripts/Inspect-DlsInstrumentCreation.cjs <run> work/analysis/dls-instrument-original/20261003T231831Z。同summaryでTest-DlsWaveCreation/Inspect-DlsWaveCreation、Test-ProductHost/Inspect-ProductModulesを実行。GUIは現在版を起動し独立DLS input copyをOpen→Add Instrument→Save→Undo/Save→Redo/Save、work/audit-dls-instrument-gui.cjsで全bytes照合、Capture-ProductGuiModules/Inspect-ProductGuiModulesで由来確認。固定日時/パスは保存された試験証拠の識別子であり、再試験結果は新しいrunへ記録する。

次の具体的な一手: 既存現行GUI PID17504をfresh listで選び、初期Segmentと保存したDLSを新しいnative Projectに保存する。所有Bandへこの新Instrumentを割当しSegmentへコピー、終了後別起動で保存物と依存snapshotを復元して再保存全bytesを照合。新規Instrumentのknown PCMを長さ/loop付きでConductor→DirectMusicへ渡しWASAPI loopback録音でPlay/Stop/再開とGM fallbackなしを自動検証する。人の聴取を待つ工程へ戻さない。原版後続locale/Instrument Editor/articulation編集/stereo、Wave metadata/sampler、他文書/全40/全八は残す。

# 2026-10-04 新規Instrumentを所有Project/Band/Segmentへ接続し無人録音確認

全体未完了。前単位は進捗あり（Instrument作成/原版初期値/GUI保存）。最新計画/記録と現在ソースを確認し既存変更を保持。AGENTS検索なし。前GUI実保存Created.dlp2226bytes SHA256 a3f028f3f78f8a1a5766ff8a8cc2a2e0cf08bd28592ed05f1385a9929bcdd93cを入力に用いた。本単位の現行生成物は別版であり、前版GUI成功を現行GUI成功へ転用しない。

変更: --prepare-authored-dlsは実入力をFrameworkへ所有しtyped set_region_loopsで800frame全長forward loopを一回設定/UndoRedo全bytes確認する。Wave/Instrument metadataは維持。所有Bandへ新規Instrument bank0/program0を割当、Segmentへコピー、native AuthoredDls.proとして保存/別Framework復元/全Project再保存一致を検証。Segment長49152clocksは試験入力としてseghを設定し64個MIDI60/velocity96/duration384/interval768 notesは編集APIを用いる。GUI長編集の成立は主張しない。初期GUIDのみCollection参照を持つSegmentではProjectの所有一覧が必須なので、--audio-lifecycleへnative Projectを開く入口を追加（Segment入力入口も維持）、Segment一件を明示要求する。各Play/再開のsource/runtime DLS snapshotを保存し由来比較を可能にした。Conductorの所有DLS memory load/download/Play/Stop/再開を製品経路で実行する。

音高判定の修正: 新規Waveは8000Hz/800frames=0.1秒。旧Inspect-DlsSamplePitchの0.1秒開始窓は空になり誤った0Hz結果を返し得る。開始をmin(0.1秒,frames/4)、終了min(全frame,0.6秒)、最低0.02秒とし同fixtureの440Hzを測定。DLS lifecycle専用明示--dlsはhash付きowned.dlsとruntime/再開全bytes、Project/Segment実入力、native note/channel/velocity、phase時刻、録音packet/無音/音高を照合。MIDI60はこのDLSのroot60/PCM440Hz、GM C4=約261.63Hzを反例に使用。通常GM/Motifの判定は既存profileを保持。Prepare-AuthoredDlsLifecycle.ps1/Test-AuthoredDlsAudio.ps1でbuild/source/exe/input/fixture/recorderをhashで結ぶ。

失敗と是正: 最初のpatchは最後の一致行欠落で全拒否、分割し実変更。最初の235514932Zはconfigure/build/install成功、準備235719078Z成功・host235852651Z成功。録音235827981Zはrecorder成功、player exit1/play前「GUID-only collection requires an owned project entry」、runtime call0。Segment単体入口に所有catalogがなく失敗したので、Project入口へ変更して新build/新fixtureで試験した。OS拒否/迂回/同条件再試行なし。raw fixture auditorの初回はBand全byte同一を誤って要求し失敗。Framework::assign_bandは所有DLSをGUIDのみ参照へコピーする既存契約を確認したため、refh flags19→3/owned.dls file削除だけを明示変換した後のBand全bytes一致を要求して修正。製品側の未知差分を無条件正規化していない。存在しないsegment.h/Windows literalglob検索も失敗したがファイル変更/実行影響なし。

現行構成/compile/install: work/build/product-snapshot/20261003T235955845Z/build-summary.json 保存62sources不変/各exit0、Producer SHA256 285e86ecb0fedc15d1dce43b418401e5349460a51a186cb270fb79bb0cf408b9。実行 work/acceptance/authored-dls-lifecycle/20261004T000229628Z/run.json exit0/input不変、fixture-proof独立rawpassed（Region WSMP20→36のloop追加だけ、他DLS全bytes、Band CollectionID/patch/Segment GUID-only copy、native3entriesのsize/FILETIME照合）。work/acceptance/product-host/20261004T000231212Z/run.json --smoke exit0/23点由来passed。前Instrument25/Wave17/旧165/fullcoreを再実行していない。変更は本体の統合試験入口/記録と解析なので関連native準備/再生/hostで検証し、前版全core成功は主張しない。

現行音声: work/acceptance/authored-dls-audio/20261004T000247960Z/run.json player/capture各exit0、16秒WASAPI default render endpoint loopback録音48000Hz stereo float32、1600packets。work/acceptance/authored-dls-audio/20261004T000247960Z/lifecycle-audio-proof.json passed、source/runtime/restart DLS全bytes一致、Project/Segment snapshot一致、phase時刻・native note検証passed。first/restart RMS約0.02387/0.02328、440Hz energy0.00976/0.00887、GM音高energy約0.00000428/0.00001412。baseline/Stop hold/final Stop RMS0、peak0.04646、max gap3frames、timestamp errors0。55点再生module由来と原版40hashを照合passed。録音はデジタルendpointの出力を示す。実スピーカー/GUI Play/原版音の同時比較/テンポ変更はこの単位の合格範囲ではない。

判定器対照: work/acceptance/authored-dls-audio-controls/20261004T000600Z/negative-tests.jsonは派生録音コピーのみ。無変更exit0/passed、Stop中に音を入れる/再開音を消す/再開をGM C4音高へ変える3例はexit1/拒否、元native API記録は成功のまま。実製品失敗や別の実録音とは数えない。これによりAPI成功/コンパイル成功だけで音声を合格としない自動工程を実行確認した。元録音/fixtureを上書きしない。

残る依存/未完了: Windows DirectMusic/DirectSound/録音WASAPI/GM.DLS依存、全40責務/全八受入は残る。原版固有EXE/DLL/OCXを製品へ取り込まない。既存原版Insert Instrument観測は前単位を参照し、本単位で新たな原版動的観測は実行していない。現行GUI/native Project開閉/別プロセス再保存、GUI Band割当/Play、テンポ変更での新規DLS音声は未実行。前GUI233543909Z PID17504は未保存初期Segment/Projectを保持している（今回再操作/生存確認していないため現在の生存は未確認）。現行235955845ZのGUIを成功としない。

再現: Build-ProductSnapshot.ps1→Prepare-AuthoredDlsLifecycle.ps1 -BuildSummaryPath <summary> -InputCollection <GUI saved DLS>→Node Inspect-AuthoredDlsLifecycle.cjs <prep/run.json>→Test-AuthoredDlsAudio.ps1 -BuildSummaryPath <same summary> -PreparationRun <prep/run.json>。録音器は保存build work/build/audio-capture/20261003T102354462Z/build-summary.jsonと一致するproducer_loopback.exe、ソース2点も検証。Node Test-AudioLifecycleAuditor.mjs <audio> <new controls dir>、同summary Test-ProductHost/Inspect-ProductModules。結果は毎回新しいrunで固定する。

次の具体的な一手: 現行build235955845Zの本体GUIへAuthoredDls/AuthoredDls.proを開き、Segment/Band/新Instrument/loop依存を確認して別保存/通常終了→別起動復元/再保存全bytesを一巡する。旧GUIはfresh listで確認し、未保存初期Segment/Projectを必要なら独立保存して保持する。現行GUI Play/Stop/再開も録音へ結び付ける。次に原版と比較するInstrument/Region articulation編集、文書のloop/tempo経路と全機能台帳の未完了を進める。計画の全体対象/受入条件を縮小しない。

## 2026-10-04 同一パス文書Save後のnative filh更新

FrameworkのSegment/Style/Band/DLS Save成功は、imported/factory native metadataがある既存所有entryについてProject dirtyを立て、内部DMPJのmupdにfile chunk index(uint32 little-endian)を記録する。journalの準備は文書書込み前、確定は成功後。文書Save失敗はProject/journal/履歴を更新しない。Project Saveは保存済みentryだけのFILETIME/sizeを実ファイルから読み、filh 16..27を書換える。file/document GUID、未知tail/pad/その他metadataを保持し、document identity mismatchや欠けたfilhは拒否する。native Project書込み失敗はpendingとorigを保持する。DMPJ保存・別Framework再読込でもpendingを保持し、native成功後にjournalを空にする。native JAZPへ内部journalを出力しない。未編集native全bytes保持は継続する。表示ndsc/runtime-export等の動的更新は本変更の範囲外で未実装として残す。

版・根拠・191checks・独立raw・反例・未実行範囲: work/analysis/project-metadata-save/20261004T010000Z/report.md。原版44-byte header配置を使用したが、本単位の原版GUI保存比較は未実行。原版と同じdirty通知とは主張しない。全体未完了。


## 2026-10-04 保存Styleのnative拍子説明同期

現行063439196Z保存67sources構成/build/install0。Frameworkは保存Styleのnative Project ndsc拍子説明をfilh更新と同じpending journalで再計算。3/8→5/4、Project write失敗/bridge pending保持、GUID/未知chunk/padding不変、別Framework復元/完全再保存を関連197checksと独立rawで確認。本体smoke0/24modules由来passed/原版40hash0。旧JAZP監査器は過去filh変更未対応で失敗を保持、対応済みProjectMetadata監査passed。現行GUI/音声/原版動的比較/全core/full40/full8未完了。 証拠：work/analysis/style-project-description/20261004T064000Z/report.md。順序変更：Envelope別GUI復元は残件で保持し、既存ndsc未更新を先に修正して文書管理を進める。次：Launch current063439196Z GUI once with independent copy of native StyleDescription Project and Meter.stp; verify5/4, edit meter and Save Style→Save Project, normal exit/separate reopen and exact ndsc/file metadata. Then continue native runtime-export/relocation and remaining40 responsibilities. Keep preceding060504851Z Envelope GUI separate-reload/Slow earlyStop/replay pending; its audio/GUI success does not prove new build. Preserve old saved GUIs; no original unsupported-editor retries or listening questions.


## 2026-10-04 Style Unicode文書名とnative表示同期

現行064438272Z保存67sources構成/build/install0。Style Unicode名UNAMをtyped文書/Framework/本体root Style編集欄へ接続し、native nnamを文書Save後のProject Saveで更新。11新checksを含む関連208checks、UNAMだけ/Project nnam+filhだけの独立全bytes比較、UndoRedo/無効・重複名拒否/別Framework復元・再保存、ndsc/filh回帰passed。本体smoke0/24modules原版40hash0。前063439196Z GUI PID18804応答ありだがsky操作対象0、GUI試験未実行・強制終了なし。現行GUI/音声/原版動的比較/全core/full40/full8未完了。 証拠：work/analysis/style-document-name/20261004T064800Z/report.md。次：Keep old063439196Z PID18804/launcher65966 until authoritative terminal status or user-visible window recovery; no same-condition relaunch. User front-window question pending. If recoverable, verify its native Style5/4 Save/Project Save/exit/reopen against that build only. Current064438272Z name GUI unexecuted: launch via an established targetable context once available, root Style Set Style Name Unicode/Save/Project nnam/UndoRedo/reload. Advance runtime export/relocation and remaining40; preserve Envelope060504851Z pending reload/replay, no listening or original unsupported-editor retries.


## 2026-10-04 Projectと所有文書の別folderコピー

現行065340245Z保存67sources構成/build/install0。Frameworkと本体File Copy Projectで保存済みProject/所有4文書を新folderへcopy、bytes/日時/identity保持、native別Framework復元・完全再保存、DMPJ内native基準dir更新を実装。既存先/dirty/外部変更拒否を含む関連223checks、独立copy/name/ndsc/filh監査passed。本体smoke0/24modules原版40hash0。旧063439196Z GUI launcher15分timeout・PID18804応答あり、終了/GUI未確認。現行GUI/音声/runtime export/原版Copy比較/全core/full40/full8未完了。 証拠：work/analysis/project-copy/20261004T065800Z/report.md。次：Verify copy with real filename-only nested Style/DLS dependencies, failure during staging and copied playback ownership; add same-build native preparation/audio input to move beyond empty4documents. GUI Copy menu and Unicode name/ndsc lifecycle remain unexecuted: preserve old PID18804, terminal launcher timeout is not process exit; use existing pending user front-window answer if received, no same-condition launch or listening/original unsupported-editor repeats. Runtime-format export, native runtime path metadata/session/source-control behavior and remaining40/all8 remain required.


## 2026-10-04 nested filename依存copyと途中失敗

現行071122340Z保存67sources構成/build/install0。Copy Project公開直前に元Project/全依存bytes・日時を再照合。filename-only Segment→サブfolder Style→さらにnested DLSの再帰copyを元folder不在で別Framework復元・コピー内path限定・native完全再保存まで検証。staging中Project書込み失敗の元保持/公開先なし/残留stageなしを確認。関連231checks・独立nested/raw copy・host smoke passed、24modules原版40hash0。現行音声/GUI/全core/全40/全八未完了。 証拠：work/analysis/nested-project-copy/20261004T071400Z/report.md。次：Implement runtime export from authoritative original help/format contracts: read existing extracted help or extract the retained CHM once, observe reference/export options without repeating failed Instrument editor/startup warnings, specify Segment/Style/Band/DLS runtime filenames, reference rewriting and unknown chunk policy, then add source-preserving Framework export and product menu with same-build native/runtime validation. Original Copy dynamic semantics, GUI copy/native name/metadata reopen remain unexecuted; preserve old PID18804 pending front-window answer.


## 2026-10-04 runtime四形式の一括書出し

現行072633758Z保存67sources構成/build/install0。本体Runtime Save All Files/Framework export_runtimeで4形式(.sgt/.sty/.bnd/.dls)と入れ子の参照名変換、観測済み編集専用チャンク除去、元所有モデル/履歴/Project保持、一時folder検証後publishを実装。関連245checksと独立runtime/nested/copy解析・host smoke passed、24modules原版40hash0。root AudioPathは設定損失を避け未対応拒否/後始末確認。現行DirectMusic書出物Load/音声/GUI/原版動的export比較/全core/全40/全八未完了。 証拠：work/analysis/runtime-export/20261004T073000Z/report.md、docs/analysis/runtime-export-contract.md。次：First integrate exported real Envelope PCM/Instrument Project into a same-build native/runtime catalog fixture and calibrated WASAPI Play/early Stop/restart, with source folder unavailable and exported input hashes bound. Then implement original root AudioPath to runtime AudioPath track transformation using retained BGDawn sample/header contracts. Verify DMRF date valid flags, output filename collisions, dirty rejection, unknown runtime metadata and compressed data. Runtime Save As/per-component default names/folders/rdir/rfld/native runtime metadata and update-existing-folder transaction remain required, not reduced away. GUI and original dynamic export compare remain unexecuted; preserve pending old18804 GUI state.


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

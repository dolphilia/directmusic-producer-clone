# Segment Command文書接続

全体は未完了。CommandStripMgr相当の文書モデルを `command.*` と `SegmentDocument` に接続した。原版UI/COM ABI、StyleのPattern選択、GUI編集は未実装・未検証として残す。標準生成物のOS CommandParamは同版で検証した。

固定SDK `dmusicf.h` のDMUS_IO_COMMAND、`dmusici.h` のDMUS_COMMANDT_TYPES、`dmplugin.h` のCommandTrack CLSIDが形式の根拠。`work/analysis/command-track/saved-reference.json` に入力/SDKのハッシュを記録した。保存済み原版QuickStartのcmndはstride12、時刻0/21504、小節0/7、拍0、type0/1、groove87/0、range0、repeat0xe4、末尾0x6f。これは静的保存観測であり原版の動的編集結果ではない。未知repeatの意味を推測しない。

cmndはDWORD strideと固定長records。typed readerはstride11以上と完全なrecord境界を検証し、未知フィールド値をそのまま読む。編集対象は時刻、小節WORD、拍BYTE、type/groove/range/repeat。新規入力はtime非負、type0..5、groove/range0..100、repeat0..5に制限する。既存編集は各未知フィールドが元recordと同値であれば保持を許可し、未知値への新規変更を拒否する。実QuickStartのrepeat0xe4とtail0x6fを保持してGroove87→60だけ変更する経路を検証した。groove範囲の原版動的比較は未完了。既存ファイルを読んだだけで未知値を正規化しない。

Segment編集は選択group-maskとCommand型内indexを使う。timeはSegment長未満。新規またはtime変更時の小節と拍は選択Timelineから0-basedで導出する。同じtimeで他fieldを編集すると保存元の小節/拍を保持し、未解決Style参照があってもその座標を推測で置換しない。保存既存座標の修復や拍子変更時の全Command再配置は未実装。変更は既存recordのstrideと11byte以降を保持し、time変更時だけraw-clock順へ移動、同時刻は既存同時刻recordsの後。削除は一recordだけを除去。追加は現在strideのゼロ初期化record、新規trackはstride12・選択group・max位置+1。既存trkh、未知siblings/padding、兄弟track、root metadataを保持する。payloadの欠落・曖昧、record欠損、無効値、index外、position overflowは文書commit前に拒否。保存/dirty/UndoRedoは全RIFF snapshotを所有する。

現行 `work/build/product-snapshot/20261003T064734634Z/build-summary.json` は保存52sources/3targets。構成・compile・install各exit0、warning/error0。Producer SHA `af83f36d21022106f4abd48a59b888f8f9126d56cd31e457e6ff7559f3aead5a`、core SHA `a25a816e0b5d3f183b8c6b1f29c3334acfbf695e8868fd9c97c45dd213bf1437`。製品コンパイル成功と全体受入は別判定。

`work/acceptance/command-track/20261003T064845191Z/run.json` は関連28件exit0。拡張stride16/未知field/tail/header/兄弟保持、Timeline座標、移動・削除・失敗原子性、全bytes UndoRedo、新track、別Framework復元、実QuickStart読取と1byteだけのGroove変更を確認。独立Inspect-CommandTrack監査はソース/EXE/driver/input/保存物/project/runtime/provenanceをhashで結合する。Sequence関連回帰 `sequence-crud/20261003T064929086Z/run.json` 26件と独立proofも成功。Command試験を通常full suiteへ追加したが、現行full suiteは未実行。

標準生成playable.sgpはSequence/Band/Tempo/Commandの4tracks。`product-group/20261003T064900882Z/run.json` と独立監査は12group/6Tempo/6CommandParam、全bytes runtime snapshot、Play/Stop成功。CommandParamのtype/groove/range/repeat全4値が保存元と一致。57ロードmoduleの原版40hash一致0。StyleのPattern選択・音声・GUIを確認したことにはならない。host-only `product-host/20261003T064928842Z/run.json` もexit0。

失敗版064553474Zは構成成功後、core_tests.cpp329で既存Chunk名unknownとBytes名が衝突しC2374/C2086/C2371、compile失敗。BytesをimportedPayloadへ改名し、現行064734634Zを新規snapshotから構築した。失敗版の生成物を成功版へ転用しない。既存Sequence shadow warningはlocal loop名変更で修正し、関連26件で回帰確認。前回063639317Z/21件とdriver生成parse error等の履歴は前回unit-recordおよび今回previous-recordsに保持。

再現は許可された通常Windows環境で次を実行する。

```powershell
./scripts/Build-ProductSnapshot.ps1
./scripts/Test-CommandTrack.ps1 -BuildSummaryPath <summary>
./scripts/Test-ProductHost.ps1 -BuildSummaryPath <summary>
./scripts/Test-GroupRuntime.ps1 -BuildSummaryPath <summary> -Segment <command-run>/core/command-track/playable.sgp
node scripts/Inspect-GroupPlayback.mjs <runtime-run>/run.json
./scripts/Inspect-ProductModules.ps1 -RunPath <runtime-run>/run.json -CaseName group-playback-api
node scripts/Inspect-CommandTrack.mjs <command-run>/run.json <runtime-run>/run.json
```

次はmainのCommand一覧・group/type-index・入力・履歴・保存/reloadへ公開し、所有StyleのPattern選択へ接続する。OS CommandParamは上記標準入力で確認済み。現在のSequence GUI削除確認は前版044136915Zの窓で承認待ち、現行GUI成功へ転用しない。原版窓非公開/停止拒否は同条件再試行なし。対象全40責務/全八受入を維持する。


## 2026-10-03 本体Command編集窓（065935795Z）

mainのEdit Commandsから独立した同期編集窓を開き、メインをdisableしてFramework文書indexを安定させる。group/Command型内indexの選択をコピー検証後にcommit、文書/group/indexごとの選択保持、一覧・全field・追加/変更/削除確認・UndoRedo・Framework SaveAsを接続。時刻変更の結果indexを低レベルrecord insertionから返し、失敗・無変更は出力indexを更新しない。raw選択の3件を追加しCommand31件成功。

保存54sources/3targets build065935795Z構成/compile/install各exit0、warning/error0。Producer 5dfd6fd2f725c9ffe9ea9205e4986846f2c896447f6f5b9f11c9e8be92d8770f、core 4e1d5252a3e0ee0a8bda7d4714e9956bdaef4fa62f3ca724b77983246618043a。Command070101193Z/独立proof、host070100306Z成功。標準native入力runtime070130269Zは12group/6Tempo/6CommandParam・全bytes snapshot/PlayStop、56modules原版40hash一致0。現行fullsuite/Sequence回帰は未実行、前版26件を転用しない。

GUI070200Zの27capturesはowned.sgp読込・編集窓、Groove50→65、Modified表示、Undo50/SavedとRedo65/Modified画面、edited.sgp SaveAsまで。Inspect-CommandEditorGui.mjsは保存54source/EXE/captures/input/保存物を結合し、offset728の1byte50→65以外の全bytes不変を監査。accessibility更新遅延があるためUndoRedoのin-memory bytes成功は主張せず画像を保持。GUI保存edited.sgpの同版runtime070810734Zも12group/6Tempo/6CommandParam/PlayStop、56module原版40hash一致0。現在GUI process provenance/audioは未確認。

操作障害を保持：観測helperのdefault引数にundefined窓/外側state参照が入りcapture前失敗、globalThis.ceUiへ明示状態保持に改めて再観測。File OpenアクセシビリティクリックがNew Projectの破棄確認を開いた。No indexはcached unavailable、Escapeでは閉じず、fresh modal後Alt+nでNo取消。破棄なし。Fileメニューは画面座標へ変更。forward slash絶対パスはWindows file dialogが形式エラー、OK後backslash形式へ修正し読込成功。同条件失敗の無差別再試行なし。

現在main593004/editor3542138は同版edited.sgp保存済みで開いたまま。次はgroup/型内index/時刻移動/追加・保存/別起動復元/正常閉鎖をGUIで検証し、所有Style Pattern選択へ進む。旧044136915Z Sequence削除承認待ちは別版の独立項目、未回答を承認扱いしない。原版非公開窓/停止拒否は再試行なし。全40責務/全八受入未達、goal active。


## 2026-10-03 Command選択保持・時刻移動（071214854Z）

CommandEditorContextを本体が文書ごとに所有し、group-mask/型内index別にtrackとイベント選択を保持。窓を閉じて開き直しても選択を復元し、project切替時はcontextをclearする。負のimported時刻の表示をunsigned wrapからsignedへ修正した（負時刻編集の受入は未確認）。

保存54sources/3targets build071214854Zは構成・compile・install各exit0、warning/error0。Producer SHA a1475a97deb6ef1cb2af3b7dc60af7508cbbb39e8804b2ab887ab938a4998237、core SHA af36eec996c614dba09d55105ccd04d9276786fcbb663f0a21410527af176eb7。Command run071633049Z31件、host071633924Z成功。標準native入力runtime072211802Zは12group/6Tempo/6CommandParam、whole snapshot/PlayStop、56module原版40hash一致0、独立監査成功。fullsuite/Sequence回帰/実音声/Style Pattern/原版動的比較は今回未実行。

GUI071700Z34captures：所有入力85164beb1c08f84d50ae076a47b38b74464cba82b1cab01d2b59b87a78f7935c、最初のCommand0→4608 clocks、並び替え後Command2/measure2/beat3へ選択追従。group2は空/ChangeとDelete disabled、group1へ戻すとCommand2復元。窓を閉じて開き直しても同じ選択。moved.sgp SHA cd8367e0fd4e781ee6ff4387ab86184277a263d1861330bb50d89533d597a7da。Inspect-CommandSelectionGui.mjsは保存版source/EXE/capture hashとRIFFを独立照合し、選択recordの順序/time/measure/beat以外の全bytesを保持したことを確認。別launchの本体で両イベントとfieldを表示復元。最初のGUI processは開いたまま、通常終了後再起動・GUI再保存・HWND/PID binding/provenance未確認。選択自体のprocess間永続化は要求していない。

再生失敗を保持：product-group/072127640ZのGUI保存moved入力はexit1、snapshotExact true、CommandParamが3072でexpected type1/groove80/range4/repeat2に対しactual type0/groove62/range0/repeat0。GetParam S_OK、StopEx/Unload/CloseDown cleanup0。試験のstarted/stopped欄はfalse（検証完了前失敗）でPlaySegmentEx S_OKだけを再生受入成功にしない。原因未確定。初期時刻0のCommandを残した対照入力は同版072211802Zで成功した。初期Commandの欠落、Windows runtimeの補間/パラメータ意味、元イベントの取り込みを次に切り分け、観測なしに値を正規化/期待値緩和しない。

ツール失敗：画像保存のdata_urlは未定義で捕捉失敗、返却urlへ修正して再観測。Openダイアログelement167がcached unavailable、再観測したfilename caretへtype_textし成功。Command native auditor初回は第2引数runtime証拠未指定でTypeError、対照runtime/独立監査後に全引数指定して成功。製品失敗とは別。

再現：Build-ProductSnapshot.ps1後、Test-CommandTrack.ps1とTest-ProductHost.ps1にBuildSummaryPathを指定。Test-GroupRuntime.ps1へ同版native playable.sgp、Inspect-GroupPlayback.mjsとInspect-ProductModules.ps1 -CaseName group-playback-api、Inspect-CommandTrack.mjsへnative runとruntime runを指定。GUI保存物はInspect-CommandSelectionGui.mjs product-ui/071700Zで監査。失敗moved入力を対照成功に置換しない。

次はCommand runtimeの初期イベント欠落/補間を診断し、本体再生契約を実装する。所有Style Pattern経路とGUI追加/削除/copy/paste/通常終了/再保存、文書間状態分離、他40責務/全八を継続。旧Sequence削除action-time承認待ちは保持、原版非公開窓/停止拒否の同条件再試行なし。OS DirectMusic/DirectSound/GM.DLS依存は宣言のまま、全機能で原版依存が解消したとは主張しない。


### 2026-10-03 Command初期イベント移動後の本体再生修正（074230941Z）

旧071214854Z GUI moved.sgp（SHA cd8367e0fd4e781ee6ff4387ab86184277a263d1861330bb50d89533d597a7da）は3072 Fill80/range4/repeat2と4608 Groove50の昇順で時刻0なし。旧072127640Z失敗は保持。旧GUI成功を現行版へ転用しない。ユーザーが旧coreエラー窓をOKで閉じた回答は受領。原版非公開窓/停止拒否の同条件再試行なし。

診断版073128156ZにSDK CommandParam2（8bytes/GUID28f97ef7-9538-11d2-97a9-00c04fa36e58）と--command-observeを追加。7実行/5対照入力、保存ソース・EXE・driver・入力・runtime全bytesを独立照合。無anchor昇順2イベントで3072に合成groove62、4608に相対時刻-1536のFill80を取得。逆順2/3、単一、時刻0anchorありは全実イベント境界一致。Param2.timeはqueryに対する相対値（絶対=queryTime+eventTime）。Param1は境界直前lookaheadとFill後beatでtype変化があり任意時刻のraw saved recordとは扱わない。合成groove62は文書へ追加しない。Windows内部原因や原版Producer動的同値は未確定。work/analysis/command-runtime/20261003T073400Z/observation-proof.json。公開資料Microsoft DMUS_IO_COMMAND https://learn.microsoft.com/en-us/previous-versions/ms807908(v=msdn.10) / DMUS_COMMAND_PARAM https://learn.microsoft.com/en-us/previous-versions/ms807532(v=msdn.10)、Param2 ABIは凍結SDK dmusici.h（saved-reference.json hash照合）。

Conductor::playへprepare_command_playbackを接続。Stop前に検証・コピーし、時刻0なし/2件以上/厳密正時刻昇順cmndだけ、再生用コピーの全strideレコードを逆順にする。保存文書、未知tail/trkh/他track/chunk/paddingは保持。zero anchor/単一/逆順は不変。重複/任意未整列は補正せず動的意味未確認。拡張stride16は静的保持のみ、runtime動的確認は12。

現行保存54sources/3targets build074230941Z構成/compile/install各exit0、warning/error0。Producer910848bytes SHA 98b7433fbb8b95330408b627a9fdd4cb3a6b78d45a19afff491d96c421b719ea、core1180160bytes SHA fc83344503a386372f99699656c06c89984e29621f57897f5dc969716c690e5b。Command074356471Z37件（元31+6）、host074357358Z成功。独立native監査は文書APIによる0→4608移動late.sgpの全bytesも確認。

runtime4入力074414429Z旧GUI moved、074414851Z同版playable、074415229Z同版late、074415600Z逆順3。全12group/6Tempo、Command順に6/6/6/9、PlayStop/cleanup成功。独立raw-byte span oracleで期待再生コピー全bytes一致。sourceSnapshotExactはfalse/true/false/true、wholeSnapshotExact全true（期待再生コピーとの一致）。各56module原版40hash0、宣言Windows DirectMusic/DirectSound/GM.DLS。現行full suite/Sequence回帰/GUI/通常終了/音声/所有Style Pattern/原版動的比較/全40全八は未完了。旧音声回答を転用しない。

ツール経過：python短名PATHなしで編集は未実行、Node fsに切替。074100379Zは最終試験追加前の中間保存ビルドで実行結果採用なし。診断ループは古いLASTEXITCODE=1でreverse成功後に打切り、残り3を別実行した。記録用exec初回は外側template変数未定義で呼出前失敗（ファイル変更なし）。製品不具合と区別。

再現：Build-ProductSnapshot.ps1、Test-CommandTrack.ps1/Test-ProductHost.ps1へBuildSummaryPath。Test-GroupRuntime.ps1へ旧moved/同版playable/late/逆順3を個別指定、各runへInspect-GroupPlayback.mjsとInspect-ProductModules.ps1 -CaseName group-playback-api。Inspect-CommandTrack.mjsはnativeとplayable runtime run.jsonの2引数。Inspect-CommandRuntimeObservations.mjsはinputs.json。unit-record.jsonに証拠hashを保存。

次手：現行GUIで移動入力Open/PlayStop/正常終了/別launch全byte再保存、その後所有Style Pattern/Groove選択とCommand接続。meter変更のmeasure/beat再整列、同時刻/未整列/拡張stride runtime、GUI CRUD/copy-paste/文書状態分離は継続。旧Sequence削除action-time承認待ちは独立保持。全対象と全体受入条件を維持。

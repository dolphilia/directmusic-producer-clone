# Segment Boundary基本設定

候補083819483で基本4境界Immediate/Grid/Beat/Barline、Start at next marker、Quick Response/After Prepare Timeの本体編集・保存・別起動復元を限定確認した。原版CHM・静止図・保存Microsoft SDKと4つの原版native sampleを根拠に、segh DWORD20の既知bitだけを独立変更する。未知bit、拡張tail/odd padding、文字情報、GUID、tracksを保持し、Align/Queue/Default/End/競合boundaryは基本選択へ正規化せず拒否する。Align/Invalidationと追加境界は無効で未受入、新規文書の既定値も変更していない。

0838は292保存/作業ソース・4出力一致。feature base0639から9変更/283同一、直前0808からProperties配置とtest到着待ちの2変更。新model144を含むsequence697と他4modeが合格。全7関連はTimeline Range.sgpのWindows5 after67を凍結しているためfalse。出力先変更・rename・代替test・同条件再実行はしていない。

作者12356（1280×800、96DPI）で4境界、Markerのon/off、Timing両方、最終Beat+Marker+AfterPrepareを操作した。Tree focusの二Undo/二Redoを含む18保存は、Segment1のDWORD20だけが全期待値に一致し、InfoΩ/Codex/2026/CRLF/Version1.2.3.4、Start1920、Infinite3072..6912、Tempo120、二C6音、Band1とGUIDを全byte保持。二peerは全byte、ProjectはFILETIME以外を保持した。4tabの順/逆巡回、通常Tab/逆Tabのdisabled skipを確認し、Properties3再作成のGDIは91→91。下段の実矩形はtab bottom204内に収まる。作者通常exit0。

別newprefs/PID15280も同じ表示設定で4tabと全設定を復元し、再保存/通常exit0。全3SGP全byteとProject FILETIME以外を保持。初期Tree白は2回のread-only frameでも残り、UIAで実在するSegmentを開くと復帰した。全描画修復は未受入。即時UIAのfocus/enable欠落・遅れ、Timing UIA要素183利用不能と実画面入力への変更もrawのまま保持した。

native4は0730のstate-poll比較、0750のcurrent-time通知比較、0808のterminal未到着、0838の二terminal unique条件がそれぞれ不合格。0838のImmediate/Grid/Beatは合格、Barlineはowned SEGABORT4=4688とSEGEND1=4689が両方届き、expected4689の-32/+64内だがunique時刻条件でcheck14失敗。旧runを遡及合格にせず、候補090907740で全owned terminalを非空/全valid/各々同じ幅へ照合するtest-only修正を保存build中。良い通知だけを選ばず、無効時刻も拒否する。StopEx100ms、通知到着待ち500ms、product scheduling/defaultとPCM閾値は不変。新native4/継承runtime/新本体境界停止は未達。

親Infoの新240秒PCMはUTC/QPC drift8.5643ms>5msで不合格。録音しない同条件240秒照合も8.5674msで失敗し、条件変化の根拠はない。Boundaryでは新録音・二probe・Windows時計変更を行っていない。必要PCM・原版同入力動的比較・全geometry・全40責務/全8受入は未達、6working/2blocked/0pass、boundedFinished=false/fullAcceptance=falseを維持する。

一次入口は`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v4f-metadata.json`、進行中候補は`unit-record-v5a.json`。作者GUI証拠は`author-v2-gui-proof-v1.json`、別復元は`reload-v2-proof-v1.json`、四つ目のnative失敗は`native-dual-terminal-failure-v1.json`。計画と継続プロンプトから新候補の未達条件を続行する。


## 2026-11-10 現行再開点（090907740 native4・本体境界停止・保存復元後）

test-only全owned terminal個別照合で未阻害6mode/1168検査、新model144/native4/継承runtime128が合格。四境界のexpected/terminalは1886/1886、2008/2008、2396/2396、4705/[4704,4705]、StopEx全0ms、到着待ち0/15/0/15ms。同じ-32,+64幅/500ms/100ms、旧四native失敗は不合格のまま。作者15800本体PlayFromStart1920とStopAtBoundary S_OK、停止4558安定/再生ボタン復帰、保存通常exit0。別newprefs20908で4tab/全設定復元再保存/通常exit0、全3SGP全byte・ProjectFILETIMEのみ。292saved-working/4outputs保持、新PCMなし。

Marker解除では保存1400/STATIC IsWindowVisible=falseでも文字が複数画像に残るため表示不合格をfallback-label-ghost-proof-v1に保全、設定を1001400へ戻して保存済み。次はvisibility変化時だけoverlap子windowを再描画し毎回show/hideを抑える限定候補。全7関連はTimeline OS5 after67でfalse、clock8.5674ms>5ms不変で録音/二probeなし。原版dynamic/全geometry/全40全8/fullAcceptance=false、goal active。一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v5e-routing-restore.json`。


### 093428615 Boundary fallback label描画build中

0909のnative4/6mode1168・本体境界停止/作者15800・別復元20908両通常0を保全。Marker off時に実STATIC非表示でも画像に文字残像が残る証拠を根拠に、Boundary全controlの毎回show/hideからfallbackだけを除き、実visibility変化時だけPropertiesと全子windowを同期再描画する一source修正。Win32 RedrawWindow一次docsのINVALIDATE/ERASE/ALLCHILDREN/UPDATENOWに沿う。model/native/font/geometry/scheduling/通知時刻閾値は不変。候補093428615/build99558、次は292saved-working4outputs→未阻害6/native4→新GUI三off/on/履歴/4tab/GDI/保存通常終了/別復元/境界停止。clock不変で新録音なし/Timeline OS5再試行なし/全7関連false/全40全8/fullAcceptance=false、goal active。一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v6a.json`。


### 正式原版Boundaryの静的resource追加照合

正式SegmentDesigner.ocx（SHA541e5dd87c62cb0b449c571aba4e7ac863b093877ded7793eea89e4c6877c1bb）からRT_DIALOG275/1033を実行せず抽出。標準DLGTEMPLATE18controls、250×110DLU、font8point MS Shell Dlg、全1176bytesを消費して解読。初回decoderのMS Sans Serif予測assert失敗は原文とともに保全し、実際のShell Dlg文字列へdecoderだけ訂正。cloneの明示MS Sans Serif8との字体宣言差を残し、実行時font/厳密pixelsは未受入。&Start/ &Quick/After &Prepareのnative mnemonicsが原版にあり、clonecaptionには未実装。描画0934の検証後、AltS/Q/Pを次の独立実装・本体保存復元へ進める。原版の再起動/COM登録/OS変更なし。一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/original-resource-audit-v1.json`、fullAcceptance=false。


## 2026-11-10 現行再開点（095233098 parent背景・正式Boundary mnemonic build中）

0934は292saved-working/4outputs、未阻害6/1168/native4合格。ただし最初のMarker解除でconditional show/hide+ALLCHILDREN再描画後もhidden STATIC文字残像を確認し不合格。二/三cycle、履歴、GDI再作成、Play、別復元へ進まず、元1001400保存・作者11564通常exit0・全native保持で保全。次095233098は浮動Properties ownerのWS_CLIPCHILDRENを外してGroupBox下の背景消去を可能にし、正式resource由来の&Start/&Quick/After &Prepare等captionを追加する一source候補。font/geometry/model/native/通知閾値は不変。Win32 parent clipping一次資料と原版RT_DIALOG275/18controls/ShellDlg8を根拠にし、全Properties/原版runtime字体の受入は未達。build57043→未阻害6/native4→新GUI AltS三off/on/AltQ/P/履歴/4tab/GDI/本体境界停止/保存通常終了→別newprefs復元。旧paint/native失敗/clock8.5674ms/TimelineOS5凍結を維持、全7関連/newPCM/originaldynamic/全40全8/fullAcceptance=false、goal active。一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v7a.json`。


### 次の独立依存：Invalidationの正式原版静的mapping確定

現在0952 build/本体検証を優先して継続。待機中に正式SegmentDesigner.ocxのRT_DIALOG275 control IDsとBN_CLICKED message-map10件を結び、共通thunk0042C3F8→handler0042C1E5を静的解読した。Invalidation1216は0x8000、1217は0x10000000をOR、1218は0。三radioのcaptionとID範囲減算/分岐/即値命令をwholemodulehash固定で照合。9combo jump tableは0/800/1000/2000/11000/12000/810000/800000/100。Align to SegmentをbareALIGNと推測しない根拠も得た。全原文/命令抜粋/解読scriptを保全し、原版実行/COM変更なし。現在候補保存検証後、Invalidationの独立atomic mask0x10008000/三値と不正両bit拒否・全未知bytes/history/native readback/本体radio保存通常終了/別復元へ進める。音声的cutoff/secondary regeneration/原版dynamicは新PCMなしで合格にせず未達。一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/original-boundary-flags-audit-v1.json`、補足advanced-boundary-audit-v2.json。全40全8/fullAcceptance=false。


## 2026-11-10 再開点（095233098残像修正・本体操作・別復元保存後）

作者2536/root30411152:child-focus AltS三off/on全状態で残像なし、AltQ/P/no-op/Timing-Marker UndoRedo、全4tab両方向と3Properties再作成GDI91→91。19native保存stageは宣言DWORD20のみ、他2SGP全byte/ProjectFILETIMEのみ保持。本体PlayFromStart1920/StopAtBoundary S_OK、停止5256安定・120入力/再生復帰、最終保存通常0。別newprefs7756/root11600760、1280×800/96DPIで4tab全設定復元・再保存・通常0、292saved-working/4outputs照合。窓self-focus AltSの無反応は子focus結果で格上げせず未達、全Properties系列/原版fontpixel/newPCMは未達。

次は原版RT_DIALOG275とhandler42C1E5の静的mappingに基づくInvalidation三値atomic mask0x10008000とmouse-tab focus修正。bit8000/10000000/0、両bit不正import拒否・未知bytes/history/native読戻し/本体radio保存通常終了/別復元を進める。時計8.5674ms>5ms、TimelineRangeOS5 after67凍結、全7関連/newPCM/原版dynamic/全40全8/fullAcceptance=false。一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v7e-paint-routing-restore.json`。


## 2026-11-10 継続中：Invalidation103512652Z

0952の両通常終了後、7sourceのみ変更してbuild56769開始。独立getter/enum/atomic mask10008000、両bit importの三setter拒否、三radio3654..3656と原版mnemonic、Segment TCN_SELCHANGE後の実active field focusを実装。モデルでは4header×全9遷移/opaquepadding/history/no-op/invalidenum/両bit/advanced保持、nativeは三値GetDefaultResolutionとsource/解放を追加。heard cutoff/secondary再生成は検査対象外・未達保持。現在候補build→未阻害6/native→新本体操作保存復元へ。全7関連/新PCM/原版dynamic/全40全8/fullAcceptance=false。入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v8a.json`。


## 2026-11-10 継続入口（103512652 Invalidation保存復元後）

Invalidationは正式原版の有効三値と原版mnemonicを実装、210モデル検査追加。未阻害6mode/1387、Boundary144+Invalidation210=354モデル、nativeBoundary4とInvalidation3readback、継承runtime計137が合格。作者20624/root72155184、1280×800/96DPIでmouseBoundary直後Markerfocus→AltS off/on残像なし、AltD/N/2、CtrlS子focus保持保存、同値no-op/一UndoRedo/矢印/Tab両方向/全4page往復、Properties再作成とGDI91→92<=8。本体PlayFromStart1920/StopAtBoundary S_OK、5021cursor安定・120入力/再生復帰、最終11001400を保存通常exit0。15native stageは宣言DWORD20のみ、他2SGP全byte/ProjectFILETIMEのみ。別newprefs19884/root7603062で4page全設定復元・再保存・通常0、292saved-working/4outputs保持。

toolbar保存後のfocusはmainへ移る。空timelineF11一入力は再focusせず、既知文脈制約として記録して繰り返さず、mouseInfo→Boundaryで入力へ復帰。以降CtrlSは子focusを保ち保存成立。原版refreshは不正Invalidation両bit時1217を優先、cloneは無選択/三編集拒否でraw保持する差を明記し、その完全同等性は未達。音声的cutoff/secondary再生成/newPCM/全7関連/原版dynamic/全Properties字体pixel/全40全8は未達、fullAcceptance=false/boundedFinished=false。

次の独立単位：正式原版refresh42BEF6+handler42C1E5の9Boundary値、Align/Queue時Marker強制解除、Align時validstart表示（Grid→Beat→Bar→Timeの優先、bit無しTime）と編集連動。一次static：original-boundary-sync-audit-v1.json/next-boundary-coupling-audit-v2.json。全OS/clock同条件再試行凍結。入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v8d-invalidation-restore.json`。


## 2026-11-10 継続中：Extended Boundary111617767Z

1035両通常終了の後、7source限定の次候補build75077開始。正式originalEnableWindow helperはMFC42 ordinal2642の現行DLL codeからUSER32!EnableWindow IAT呼出しまで追跡。最初の名前export照合は空でassert失敗、v1失敗とv2 ordinal観測を保全、静的code法v3で解決。原版実行/COM/OS変更なし。9Boundary/Align4とMarker無効/解除連動、Align入出時validstartの保持/解除、元のDEFAULT/複合import拒否、9件dropdown180、有効inputへF11/タブfocusを実装。モデルは9選択trail/3Align×4条件/旧拡張header/opaquepadding/UndoRedo/no-op/不正/連動の検査を追加、nativeは9+4×3=21読戻しを追加。古い4選択native停止の全通知/閾値は変更なし。

次：保存build→未阻害6/全新native→新PID9選択/4Align/連動/履歴/4pages/本体操作保存通常終了/別newprefs復元。音声的位相/Queue時刻/cutoff・secondary再生成は新PCM不在の未達保持。時計8.5674ms/TimelineOS5/原版起動同条件再試行なし、全7関連/newPCM/originaldynamic/全40全8/fullAcceptance=false。入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v9a.json`。


## 2026-11-10 継続中（1116選択検査失敗→112858885 test-only順序修正）

1116のsequence-crudはcheck5で新選択保持predicateが不合格、他5mode/543とnativeBoundary4/Invalidation3/Extended21は同候補で合格。旧run20261110T112550845Z/全stdoutstderr/292sources4outputsをextended-selection-order-failure-v1.jsonへ保全し、本体は起動せず。保存sourceのrestore_history→import→Track::loadがselected=falseを作る既存経路を確認。具体的conjunct値は旧log未出力のため原因はsource根拠の推論と明記し、履歴を跨ぐTempo選択は未達保持。

新112858885は同じ選択predicateをUndoRedo前へ移す1testheaderのみ。製品変更なし、predicate削除/数値幅/旧結果格上げなし。build84778→未阻害6/native新規全実行→author-v7 GUI（protocolv10/v11候補IDはこの新候補へ版付けして更新が必要）/native26/保存通常終了/別newprefs復元へ。clock/Timeline/原版条件不変の再試行なし、全40全8/fullAcceptance=false。入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v10a.json`。


## 2026-11-10 継続入口（112858885 Extended Boundary保存復元後）

1116選択check5失敗を残し、1128は同predicate順序だけのtest-only修正。製品源は1116と同一、292saved-working/4outputs、未阻害6/1986、model890(basic136+Invalidation210+Extended544)、nativeBasic4全terminal/horizon幅不変・Invalidation3・Extended21読戻し/継承runtime200が合格。作者15080/root18416608/1280×800/96DPIで9dropdown全表示/9選択保存、Align/QueueのMarker解除無効化・Align4のみ有効、4条件選択、同値no-opの一UndoRedo、基本Beatへ戻すvalidstart解除とMarker再有効化、全4page両方向/focus、最終Align下の再作成まで二再作成GDI91→92→92。basicBeat+Marker11001400で本体PlayFromStart1920/BoundaryStop S_OK、3777cursor安定・120入力/再生復帰。最終AlignSegmentBarline18810400をその後保存・通常0、26native stageは宣言DWORD20以外全SGP保持/ProjectFILETIMEのみ。別newprefs15768/root184746248で4page全設定/focusCombo復元・再保存・通常0、復元3stage全保持。GUIのUIA Tree228一度キャッシュ不一致は保全し、最新画像座標でTreefocus後に履歴操作、同UIA再試行なし。

音声的Align位相/Queue時刻/advancedStop/cutoff/secondary再生成/新PCM/原版dynamic/字体pixel/全Properties/全7関連/全40全8は未達、fullAcceptance=false/boundedFinished=false。時計8.5674ms/TimelineRangeOS5/原版同条件再試行は凍結、旧失敗/音声を格上げしない。次は原版RT_DIALOG8MS Shell Dlgに対し現Segment専用8MS Sans Serifの差を現ホストalias/一次資料から照合して限定font候補へ。ClockTimeLoop拡張は保存SDKに定義不足のため原版codec調査を先行しoffset推測をしない。入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v10e-extended-restore.json`。

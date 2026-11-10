# Style参照と拍子取得の暫定契約

更新：2026-10-03（日本時間）。StyleDesigner/StyleRefStrip/TimeSigの全互換実装ではない。原版ファイルの静的観測、製品の読込・Timeline接続、合成入力の試験を区別する。

## 観測した入力

`work/analysis/style-inputs/20261002T152849031Z/observation.json` は観測器のソースとSHA、3入力のコピー・SHA、RIFF位置・サイズ・選択したヘッダー値を保持する。以前の151100Z観測は旧観測器のまま保存する。

| 入力 | SHA-256 | 観測 |
| --- | --- | --- |
| heartland.sgp | cfff2d948051dc7dbdaca2009982086e50cfd00e046b9717ad96cb31c6fbec05 | StyleTrackのsttr/strf、stmp=0、DMRFのGUID・name=Heartland・file=Heartlnd.stp |
| Heartlnd.stp | 00600efd7ce8cb7133b327115fe961f4ff825d2d1f56840b2c86206eea17b0a5 | DMST、styh12バイト、4/4、grids4、112 BPM。root GUIDが参照GUIDに一致 |
| DemoStyle.stp | f3d78a46bf86ab596e42d1c314f85e63a1421c201c3c729adbf21df565c5c8b6 | DMST、styh12バイト、4/4、grids2、87 BPM |

この観測は原版Producerの動的な拍子照会・通知・再生結果を示さない。SDKの識別子・構造は既存の保存済み公開DirectMusicヘッダーを根拠とする。既存資料の位置・由来は playback-contract.md と time-signature-strip-manager.md を参照する。

## 実装した契約

`src/producer/style.*` はDMST全体を所有し、未知Pattern/Band/metadataと奇数paddingを保持する。styhの先頭4バイトをbeats、denominator、little-endian WORD gridsとして読む。観測した12バイト形式ではdouble tempoはoffset4、SDKの16バイト以上の形式ではoffset8。整数拍数・非ゼロgrids・2の冪denominator（1～128）・有限正tempoを要求する。未対応の値を既定値で埋めない。GUIDがあれば16バイト・一意であることを要求する。失敗した読込は既存文書を保持する。

StyleTrack CLSID d2ac288d-b39b-11d1-8704-00600893b1bd とStyle CLSID d2ac288a-b39b-11d1-8704-00600893b1bd をバイト照合する。32バイトtrkhのgroupsはoffset20。sttr/strfのstmpは4バイト時計位置、DMRF/refhの有効ビットに従ってguid/file/nameを読む。ファイル参照は文書ディレクトリ内の指定した相対名に限り、物理パスも包含を確認する。GUID指定時はファイル内GUIDが一致しなければ失敗する。GUIDのみの参照はproject catalogが未実装なので拒否する。フォルダー全走査・拡張子推測・原版COM登録へのフォールバックは行わない。

SegmentDocumentは解決済みStyleのバイトを文書ごとに保持する。依存物・拍子遷移をすべて検証してから採用し、失敗した再解決は旧snapshotを保持する。解決だけではSegmentのバイト・dirty・Undo履歴を変更しない。TempoのUndo/RedoはStyle contextを保持する。

Timelineは明示TimeSigがある場合にそれを優先する。この優先順位は製品の暫定契約で、原版での混在条件は未確認。明示TimeSigがなくStyle参照がある場合、全参照が解決してからgroup1のStyle meterを取り込む。group1の初期参照はtime0、後続は前の拍子に対して小節境界にある一意の位置を要求する。未解決・重複・小節途中の拍子遷移では座標を推測しない。他groupの全表示、Patternごとの拍子、編集通知と混在条件は残作業。

Frameworkは開く際に文書ディレクトリから解決する。失敗してもSegment全体と警告を保持する。別ディレクトリへのSave Asは新ディレクトリの参照解決を成功させてから書き込み、失敗した場合は保存先ファイル・旧文書パス・旧contextを保持する。Styleを自動コピーする導入機能は未実装。同じパスへのlossless保存は未解決でも可能。

GUIは拍子を取得できる場合にstatusへ初期meterを表示し、Tempo一覧に小節/拍/tickを表示する。取得できない場合は時計位置/BPMと未解決状態を表示する。時刻線はraw clocksのpixel変換のみで描画する。拍子入力欄は編集用の入力値で、現在meterの表示ではない。Style-backed文書への明示拍子編集・Style/Pattern編集は拒否・未実装のまま管理する。

## 検証と残作業

152902807Zの保存44ファイルから3ターゲットを構成・コンパイル・installし、152951369Zのnative core95件が成功。合成3/4・5/8遷移、旧/SDK形式、未知chunk/padding保持、欠損・GUID不一致・traversal・GUID-only拒否、失敗時の状態保持、Framework open/Save Asを含む。原版Heartlandをsource readerで解決し、保存された依存コピーのSHAが上記入力と一致した。Tempo編集比較は57leaf一致・tetrのみ変更。

同じEXE c8fc13b1…のGUI証拠は `work/acceptance/product-ui/20261002T153100Z/states.json`。3/4 grids2、5396 clocksの3:2:20表示、欠損参照の座標非表示、Close後window不在を観測。GUI終了コード・QuickStart GUI再生・音声は未実行/未確認。

次は保持済みStyleのバイトをConductor/Windows DirectMusic loaderへ明示的に渡し、QuickStart再生の入力とmodule inventoryを固定する。現在の再生経路はファイルをOS loaderが改めて読むため、Style文書snapshotと実再生依存物の一致はまだ保証していない。その後、Style/Pattern文書管理・編集・原版比較とCOM通知を拡張する。40 PEの全責務と全体8条件は維持する。

## 追記：保持済みStyleのOS再生接続

154730195Z/154812445ZではConductorが呼出側のStyle contextを検証し、GUID+memoryとしてWindows loaderへ登録する。Style runtime meter/tempoとsource readerを照合し、Segmentの拍子も照会する。元Styleが4/4のまま同じGUIDのメモリコピーだけ3/4 grids2へ変更するとruntime Segmentも3/4 grids2となることを確認した。827leaf一致、styhだけ変更。identity-proof.jsonに元入力とコピー・変更コピー・比較器のSHAを結び付けた。

filename-valid+memory併記の初回154241666ZはSetObject0x88781182で失敗し、履歴を保持した。filename-only再生mappingとGUID-only catalogは未実装として拒否する。source contextと再生Style meterの接続はこのGUID付きQuickStart経路で成立したが、全ファイルアクセスtrace、Pattern拍子、通知、全Style編集は未完了。次はFramework Style factory/catalogと参照編集の所有関係へ進む。

## 2026-10-03: 独立Style文書とFramework catalog

160341839ZでStyle専用factoryを追加した。全DMST/未知leaf/GUIDを所有し、新規Styleは4/4 grids4 tempo120と生成GUIDを持つ。tempo編集は有限1..1000 BPM、styh12はoffset4/SDK16はoffset8を書換える。全RIFF Undo/Redoと保存bytes基準dirty、atomic file saveを用いる。Pattern/Band/meterのUI編集はまだない。

FrameworkはStyle/Segmentを別に所有し、Style変更後に所有catalogのbytesから各Segment参照を再評価する。GUID-onlyはcatalogの一意GUIDのみで解決し、同GUID重複は同bytesでも拒否。filenameはcatalog path照合→制約付きdisk読込。GUIDがある場合はrootGUID一致を要求。ProjectはStyle先行の二段階読込、Style/Segment参照index保持、dirtyStyle保存拒否、移動後path更新を行う。一般COM通知ABIではなく自作Framework内部の更新である。

work/acceptance/product/20261002T160437078Z/run.jsonのcore118件はfactory/tempo/Undo/Redo/保存/独立再読込/順序反転/GUID-only/曖昧GUID/移動/壊れたStyleの保持を確認した。GUI evidence work/acceptance/product-ui/20261002T160600Z/states.jsonは原版Heartland作業コピー112→132のUndo/Redo、保存/別process復元/再保存を確認。同style-edit-diff.jsonはstyhのみの変更、827leaf payload一致。SHA e84d12b0017b09d309f5030e208755cb1cfc2a8ce16998f63c9ec4897e023320が復元後再保存で一致した。原版ProducerによるこのStyle保存物の相互読込/動的比較は未実行。

今回OS Style playbackの入力同一性はwork/acceptance/product/20261002T160437078Z/style-playback/identity-proof.json。GUI版も同EXE 80832b62fe4715dc7cc9bb825afb59f9ed7a2d0acf4ba1c03b300b1cfac209ebだが、GUIの編集後132BPM Styleでの再生は未実行。過去の原版・候補・最小音符・聴取結果をここへ転用しない。次はStyle拍子/Patternヘッダー編集と参照Timeline更新、filename-only runtime identityを実装する。

## 2026-10-03: Style拍子・Pattern groove編集

162531064Z/162621204ZでStyle set_meterとPattern一覧/groove編集を追加。grooveはptnh offset4/5、0..100かつ下限<=上限。10-byte旧Pattern/16-byte拡張Patternを保持し、他payloadは変更しない。Patternの拍子overrideはStyle default変更から独立であり、この編集では書き換えない。長さ/notes/Part/variation/Band編集は未完成。Frameworkは全既知contextを先に解決し、不成立な拍子遷移ならStyle/Segmentを一緒に保持する。current native136件、GUIの原版20Pattern読込/3/4/範囲5..50/Undo/Redo/保存/別process復元を確認。evidenceはwork/acceptance/product-ui/20261002T162800Z/states.json、826leaf一致・styh/選択ptnhのみ変更。原版Producer動的保存比較と変更したPatternのOS選択は未確認。

SDK記述の訂正: 凍結dmusicf.hのpack(2)でDMUS_IO_STYLEは12bytes/tempo offset4、DMUS_IO_PATTERNは16bytes。以前SDK16-byte Styleと記したものは合成の代替layout検証であり、このSDK形式ではない。実装の16-byte/offset8許容は維持しているが原版由来を証明していない。SDK注釈のdenominator0=256thも明示未対応。

新しい静的原版観測はwork/analysis/style-inputs/20261002T162608903Z/observation.json。次はfilename-only runtime identityと編集後所有snapshot再生、その後Pattern Part/notes/長さの原版観測と実装を進める。
# 2026-10-03 追加：再生用identityの写像

現行164907492Z／run165035676Zではfilename-only Style参照とGUIDなしDMSTを所有snapshotから再生できる。保存文書は元のfilename-only参照を維持し、再生用DMSGだけにGUIDを設定してfilename/fullpath valid bitsを解除する。DMSTのroot GUIDは既存identityを優先し、ない場合だけ再生用DMSTコピーへ生成する。同じsource pathの同一bytesには同一GUIDを共有し、同path/同GUIDの矛盾するbytesと古いcontextを拒否する。GUID+memoryのOS Loader descriptorにfilename flagsを付けない。

`scripts/Inspect-StylePlaybackMapping.mjs`とrun内`style-playback/mapping-proof.json`で、元保存bytesの不変・runtime参照とStyle GUID一致・許可したleaf以外の保持を照合した。所有Styleを3/4 grids2/tempo132/Pattern1 groove5..50へ編集し、project移動・新Framework再読込後の再生と、root GUID除去コピーの再生を検証した。Pattern grooveの音楽的選択と現行GUI/聴取は未確認。詳細と初回非同期開始判定失敗はproduct-host.md末尾に記録する。

# 2026-10-03 追加：選択Patternの拍子・長さ

170048529Zではset_pattern_layoutをStyleDocument/Framework/本体へ接続した。ptnh meter先頭4bytes・length offset8/9だけを変更し、rhtmを1小節1DWORDへresizeする。既存DWORDは保持、追加分0、短縮分はUndoで復元。独立Partの拍子/長さ/音符/variationとPartRefは変更しない。rhythm bit意味・原版編集時のresize方針は未確定で動的比較を要する。mtfsの既知playStart/loop boundsを無効にする変更、欠落/サイズ不一致rhtmはatomic rejection。未知tail/flags/groove/paddingを保持する。

観測165724213ZはHeartland77Part/20Pattern/2468notesとDemoStyle24Part/4Pattern/611notesを静的解析した。SDKのPart meter/length独立性と照合したが、原版変更保存の観測ではない。run170138478Zはnative161件とlayout編集Styleの移動/reload・再生成功。GUI170200ZはPattern1 5/8 grids3 measures3、Undo/Redo/save/別process復元を観測し、ptnh/rhtm以外826leaf不変。証拠・生成物SHA・残作業はproduct-host.md末尾とproduct-state.json。Part/note/Band編集・Pattern選択・音声・全体八受入は未完了。

## Part/PartRef/音符（171748908Z）

`parts` はprth150bytes以上の既知fieldsを読み、GUID重複を拒否する。`part_references` はPatternのLIST:pref/prfc GUIDを一意なPartへ結び、旧22byte prfcではPChannel未記載、28byteではoffset24を読む。`part_notes` はnote先頭DWORDのrecord stride>=22、残りsizeの正確な倍数を検査する。offset0 signed gridStart、4 variation、8 signed duration、12 signed timeOffset、14 musicValue WORD、16 velocity、21 playMode、>=23bytesなら22 flagsを読む。Partの拍子・gridStartとMUSIC_TIME clocksを区別し、musicValueはMIDI pitchと混同しない。未知tailは保持し、表示/edit APIで意味を捏造しない。

選択Part noteのduration>0とvelocity1..127だけを編集し、record offset8..11/16以外は全bytes不変。共有Partの全PatternRefへ同じ編集が届く。Part自動複製・音符追加削除・grid/musicValue/variation編集・Bandは未実装。Frameworkは既知contextを先行検証して採用、Undo/Redoは全Style保存bytes、project再読込は新Frameworkで検証した。誤ったindex/範囲は採用なし、切れたrecord/曖昧GUID/未解決refは例外で原bytesとdirtyを保持する。

run171841130Zは177checksと編集576/88の所有Style移動後OS再生成功、mapping-proofはstyh/選択ptnh/rhtm/選択note2fieldsだけを許容する。GUI171900Zは原版copyの3036→768/77→92とUndoRedo/save/別process復元を目視確認、保存全bytesはこの2fields以外不変（827leaf一致）。OSによるPattern選択/音符の音声変化、原版エディタの動的同操作比較は未確認。版・入力・生成物・証拠SHAと再現手順はproduct-host.md末尾。

## 音符CRUDと6fields（173648628Z）

gridStart/duration/timeOffset/variation/musicValue/velocityを既知位置に書込み、offset17..末尾のrandomization/inversion/playMode/flags/tailを保持する。variationは0..FFFFFFFF、musicValueはWORD、gridStartはsigned grid、offsetはsigned int16 clocks。duration>0とvelocity1..127を検査する。意味のある和声音程・variation選択・境界位置へ自動変換しない。

insertは指定positionの前にrecordを挿入し、templateなら全opaque strideを複製、newならzero初期化して既知fieldsを設定する。既存note size DWORD/strideは保持、欠落noteは24bytes。deleteは一recordだけを除き、empty noteはsize DWORDを保持する。既存record順は保持、自動sortなし。GUICloneは選択音符の直後、empty PartのAddはposition0、共有Partの全参照へ反映する。原版の操作時のsort/Part複製規則は未観測。

run173732667Zは196checks、GUI173800Zは原4音+clone Note2（grid2/offset-7/music17408/variationFFFFFFFF/duration768/velocity92）の保存と別process復元を確認。leaf比較とancestor長の増分を含むexpected全bytes一致、827leaf不変。初回closureの古い画像は除外し、第三processの直接serialize画像/文字をstates.jsonへ保存した。旧GUI171900Zのlabel画像もevidence-limitations.jsonにより受入証拠から除外する。削除/changeボタンの今回GUI操作は未実行、native文書/Framework CRUDは合格。保存5音Styleを入力とした別run174855031ZのOSメモリ再生は成功したが、特定Pattern/追加音/record順と実音の意味は未確認。



## 2026-10-03 Band楽器割当の文書化と本体接続

現行180653686Zは保存46ソースの構成・ビルド・installとnative225件が成功、警告なし。BandDocument/Style内Band/Frameworkにpatch・bank・percussion bit31・PChannel・pan・volume編集とUndo/Redo/save/reloadを実装。GUI patch29→5/UndoRedo/save/別process復元は175953941Zの証拠。保存物は827leaf/変更外全bytes保持、現行OS memory Play/Stop成功。現行GUI/音色/DLS/独立Band文書/全八受入は未完了。

静的原版観測は Heartland 5Band/80割当、DemoStyle 2Band/12割当。両入力のlbinにDLS DMRFなし。SDK dmusicf.hのDMUS_IO_INSTRUMENT offsets0/24/28/32/33を使用し、未知flags・assignPatch・noteRanges・transpose・priority・pitchbend・末尾・padding・DMRFを保持する。patchはprogram/LSB/MSB各7bitとサンプルで観測したpercussion bit31を許容。bit31の詳細動的仕様は未比較。最初の入力検証がbit31を拒否したため180653686Zで修正し、専用native checkを追加。履歴175707510Z/175953941Zは別版として保持する。

構成/コンパイル/install: work/build/product-snapshot/20261002T180653686Z/build-summary.json。現行EXE 001b916bbb7b082050542a58daf7b6381090dfb6be3fe729c706e33ea20052fc、run work/acceptance/product/20261002T180749821Z/run.json、native225件。headless24/初期Style57実ロードは原版40hash一致0。Style mapping-proofは保存46source/workspace/EXE/入力を照合。初期Stylecapture後のfilename casesとGUI modulesは未捕捉。既存比較DLL試験は再実行していない。

GUI: work/acceptance/product-ui/20261002T180100Z/states.json の6直接capture（unique6）でpatch29→5/Undo29/Redo5/save/別process復元。GUI版EXE 0c1fdcffaa8a8d4b380d4893e62e2e68b09870d85141bc6919d4acc853ebbbd3、current GUI試験に転用しない。band.stp SHA62ad6358083e255bbb95e70248992c34e5b88839d6b8840ff490e1327b93fe28は first bins patchとvalidity bitsのみ変更、全ファイルexpected一致、827leaf保持。pan35/volume120/PChannel0はGUIで未変更、native別fixtureでは変更確認。両window消失を確認、exit code未取得。fileName UIA set_valueはcached state unavailableで未入力、画像から焦点再観測後に入力。classic EDITでCtrl+Aは選択されず295となったが、適用前にShift+Homeで5へ修正、保存差分に295はない。

再生: GUI保存物をruntime-input/Heartlnd.stpへ同SHAで配置し、現行memory Style load/Play/Stopおよび追加filename/missingGUID/project移動ケースが成功。実際に選択されたBand/音色/聴取は未確認。旧ユーザー聴取を現行へ転用しない。

独立DMBDモデルは保存/再読込までnative試験済みだが、独立Bandのfactory・Framework所有・project・GUIは未接続。DLSはdescriptor raw保持/露出のみ、音源取得/download/編集は未実装。BandTrack時刻編集・原版動的比較・全40責務/八受入は残る。次は既存BandDocumentを独立文書のfactory/Framework/project/UIへ接続してから、DLS所有参照と再生downloadを実装する。監査: work/acceptance/product-ui/20261002T180100Z/unit-audit.json。


## 2026-10-03 独立Band文書・プロジェクト・本体UI

GUI証拠の注意：中間captureのaccessibility treeは一操作前の値を返したため値の判定から除外した。画像でpan0/Undo64/Redo0/保存後dirty解除を確認し、state-0.jpg〜state-5.jpgとscreenshot-review.jsonに保存した。最初/別process読込後のtreeは画像と一致する。

現行181723070Zは保存46ソースの構成・ビルド・installとnative248件が成功、警告なし。独立Bandのfactory/Framework所有/project/UIとGM割当追加を接続。現行GUIで打楽器patch/PChannel/volumeを保持したpan64→0・UndoRedo・保存・別process project復元を確認、変更はpanの1byteのみ。DLS・独立Band再生接続・原版動的比較・現行聴取・全八受入は未完了。

BandEditorは限定typed factoryとしてDMBDを生成し、FrameworkがSegment/Styleとは別にBandを所有する。.bnp/.bndを読込/保存、case-insensitive同一pathの再読込は同じ所有者を返す。Save Asの文書間衝突を拒否し、失敗時は旧ファイル/所有者を保持する。project読込は全体を次ownerに構築してから採用。Band-onlyと混在project、相対path、移動後reload、重複/missing参照、未知chunk/paddingを保持する。新規GM割当は44byte bins、default GM flags0x1163、percussion bit31を許容、PChannel重複を拒否する。DLS参照を暗黙生成しない。

本体はNew Band/Open/Save Document、文書切替、Add GM Instrument/Set、UndoRedoを接続。BandモードでSegment編集/Playを無効にし、未接続の再生を成功扱いしない。独立Bandには現時点でSegmentへの割当UIはない。Add/New GUIボタンは未実行、native factory/addは検証済み。

構成・コンパイル・installは work/build/product-snapshot/20261002T181723070Z/build-summary.json、EXE SHA218239fabd9e5811681ad4975b1e72429b8a34e2e81440e4739693b2f9fac85e。work/acceptance/product/20261002T181808726Z/run.json のhost/core/Style APIはexit0、native248件（前回225に独立文書23件追加）。保存46source/workspace/EXE/入力一致を監査。既存原版DLL比較は再実行していない。headless24/初期GUID Style57は原版40hash一致0。filename追加case/GUI module inventoryは未捕捉。Style memory/filename/missingGUID/編集後project移動の再生回帰は成功、独立Bandの音色/再生とは別。

GUIは work/acceptance/product-ui/20261002T181900Z/states.json の6直接capture（unique6）。native生成drums.bnp/Band-only projectを入力とし、patch0x80000010・PChannel9・volume100を保持、pan64→0/Undo64/Redo0/save/別process reload0を確認。保存物SHA5ee180ffddf077ddfc70c21571b2673ef343c367d0de760e16a6d26a78a1ff94、88byte全体expected一致、変更offset76の1byteのみ。projectは入力SHAを保持。両試験window消失、exit code未取得。原版で編集したBandとの動的比較ではない。監査 work/acceptance/product-ui/20261002T181900Z/unit-audit.json。

残る依存は宣言済みWindows DirectMusic/DirectSound/GM.DLS。限定captureでは原版固有PE一致0だが全GUI/全機能の原版非依存受入は未完了。DLS raw descriptor保持のみ、collection編集/所有解決/download、独立Band→Segment再生接続、BandTrack時刻編集、原版動的比較と全40責務/八受入は残る。次は所有BandをSegment/BandTrack runtime snapshotへ接続し、GM音色のAPI/実聴取を版別に検証する。その後DLS参照/所有音源管理を拡張する。旧144958360Zの聴取確認を現行へ転用しない。


## 2026-10-03 空Styleへの所有Band作成

全体未完了。StyleDocument.add_band_gm_instrument(optional Band index, patch, PChannel, pan, volume)を追加した。nulloptは新しいDMBD Bandと最初のGM楽器を一回の履歴操作で作る。既存Band indexは同Bandへ楽器を追加。BandDocumentのGM validation/重複PChannel拒否を利用し、失敗時は元Styleを変更しない。Frameworkはcopy/apply_style_editにより所有Styleの変更と参照文脈更新を一つのtransactionへ接続した。本体のAdd GM Instrumentを空Styleにも表示/有効化し、選択中楽器のBandへ追加、選択がなくBandが存在する場合は先頭Bandへ追加、Bandがない場合に新規作成する。別の新規Bandを選ぶGUI、Band名/GUIDの作成は今回実装していない。原版との同等性は未確認。

最終work/build/product-snapshot/20261003T143452471Z/build-summary.jsonは保存59sources/3targets、構成0/build0/install0。EXE a8424fd8d8e334a3b9efa0ff1b8959bd43f9a1199f801cd550e3f381a7f8e5ed、core 7e0b775e5e218a7c903671a7fd35742f33e7a4b4731a55485e48f967f5a462a1。work/acceptance/style-band-creation/20261003T144009668Z/run.jsonの対象native14項目exit0。空StyleにGM48/PChannel5/pan35/volume120を生成、既存BandへGM0/PChannel0/pan64/volume100を追加、invalid patch/pan/indexとduplicate channelの原子的拒否、全bytes Undo/Redo、Style保存復元、Framework所有snapshot、Segment文書を作らないStyle-only project復元を確認。既存Segmentの依存cache更新を直接検証した試験ではない。core全suiteは未実行。

Inspect-StyleBandCreation.mjsは製品モデルと独立したraw RIFF解析で、元の全Style childbytes/JUNK奇数padding0xb7保持、追加DMBD/lbil/lbin/bins44bytes、flags0x1163/patch/channel/pan/volume/残りzero、既存楽器の全bytes保持を確認。saved.stpは二楽器版と全一致、owned.stpは一楽器版と全一致、projectはowned.stpだけを参照しSegmentファイルなし。保存sources/workspace/exe/driverをhash照合。work/acceptance/style-band-creation/20261003T144009668Z/style-band-proof.json。

GUIwork/acceptance/product-project-gui/20261003T144214163Z PID19220で空Style-only projectを開き、Add GM Instrument一回でBand 1/PChannel0を作成、一回のUndoでBand欄が空/dirty解除、Redoで復元、File > Save DocumentによりGM0/PChannel0/pan64/volume100を保存、通常終了exit0。Inspect-StyleBandGui.mjsは元入力hash/保存bytes（nativeの期待Bandの四値をdefaultへ置換）/project不変/全画像hash/安定UIA/同版sources/EXE/core/host/modulePIDを照合した。GUI別process再読込・GUI既存Band追加・再生/録音は未実行。GUI直後UIAは旧値を返す場合があり別の安定観測を保存。Save Documentのelement-index操作でDiscard unsaved documents?を開いたため、破棄は実行せずキャンセル。最初のNo element操作はcached app stateでunavailable、再観測後の画面座標でNo、再取得した保存メニューの座標でSaveに成功。すべての途中状態を保持し、誤操作を保存成功と扱わない。

現行host exit0、host module監査とGUI45modules監査は原版40hash一致0。点観測であり全40責務の完成を意味しない。Windows DirectMusic/DirectSound/GM.DLS依存は残る。以前141622797ZのMotif録音/GUI再生は以前の生成物の証拠として保持し、新版の音響成功へ転用しない。人の聴取は不要、次回の新規録音もソース製WASAPI loopbackとAPI照合を使う。

原版work/analysis/original-style-band/20261003T143300Z/observation.jsonは既存process/window447416902のEXEhash/画像/全UIAを保持。Project Propertiesを閉じ、File > NewがCreate New Filesを開くこと、BandとStyleが別の文書種別として列挙され、Use Default Namesがcheckedであることを観測。Cancelで戻った。新規文書の作成/保存/原版再launch/registry/security変更なし。原版Style default/Band生成/Motif設定/単独再生/Clipboardは未確認。この列挙だけから製品の原子的Band+楽器作成が原版と同じとは推定しない。

再現：Build-ProductSnapshot.ps1、Test-StyleBandCreation.ps1 -BuildSummaryPath <summary>、Node scripts/Inspect-StyleBandCreation.mjs <run.json>、Test-ProductHost.ps1とInspect-ProductModules.ps1。GUI入力はnative original.stpをowned.stpとして別dirへcopy、native project.dmpjを同dirへcopyしてTest-ProductProjectGui.ps1で開き、Computer Useで上記Add/Undo/Redo/Save/通常終了、Capture-ProductGuiModules.ps1。終了後Inspect-StyleBandGui.mjs <GUI dir> <native run.json> <host run.json>とInspect-ProductGuiModules.ps1。通常の承認済みWindows対話環境を使用。

次の具体的な一手は新しく作ったStyle Bandに結び付いたPart/Motifの入力を保存し、現行EXEで生成音符と新規loopback録音を確認する。同時に参照Segmentの所有Style cache更新/historyを直接検証する。原版の新規Style/Band defaultsを作業用projectで観測して差を仕様化する。Motif custom DLS/指定時刻/secondary/実tempo、原版Clipboard、JAZP、全40責務/全八受入は未完了。全体条件を縮小しない。凍結記録：work/analysis/style-band-creation/20261003T144200Z/unit-record.json。


## 2026-10-04 製品で新規作成したMotifへの明示Band割り当て

全体未完了。前回のBand作成保存を進め、空Styleから製品APIでGM48/PChannel5/pan64/volume100のBand、Authored Motif、専用Part、C5の4音を生成する対象試験を追加した。Style GUID/Part GUIDは作成時に新規生成。元のStyle/Pattern/Band sampleコピーは使わない。参照Segmentを別作成して所有Style cacheの更新、Band一回Undo/Redo、Segment bytes/dirty不変、保存別Framework復元を検証。CLI再生はStyleだけを開き、その参照Segmentをロードしない。

最初のwork/build/product-snapshot/20261003T145358240Zではnative11項目が通り、work/acceptance/product-notes/20261003T145550277ZのStyle-only GetMotif APIはC5x6・自然終了exit0。だがwork/acceptance/audio-loopback/20261003T145621134Zの新規WASAPI録音はAPI成功のまま全16秒無音（RMS0/peak0/onset null）で検査失敗。録音器/player正常終了でpacket最大gap2。Style rootのBandとMotif内Bandは別であり、所有Style BandだけでGetMotifが発音するという仮定は成り立たなかった。保存SDK dmusicf.h lines420..428はMotif pttnにoptional DMBD Bandを定義。新規Style styh12はOS GetTimeSignature/GetTempo比較が成功しており、この失敗をheader不一致と推定しない。

StyleDocument.assign_motif_band(patternIndex,bandIndex)を追加し、所有Style Bandを選択MotifのDMBDへ明示copyする。既存一つは置換、同bytes/invalid index/非Motifは変更なし、複数既存Bandは拒否、他のPattern/Part/chunksを保持。一回のUndo/Redo。Framework.assign_style_motif_bandが所有文書と参照cacheへ接続。本体PatternメニューにAssign Selected Style Band to Motifを追加し、選択MotifとBand instrument一覧で選んだStyle Bandを使用する。割り当てはsnapshotのcopyであり、後のroot Band編集をMotifへ自動伝播しない。暗黙fallback/無人試験限定のruntimeデータ修正は加えない。GUI操作と原版同等性は今回は未確認。

最終work/build/product-snapshot/20261003T145909887Z/build-summary.jsonは保存59sources/3targets、構成0/build0/install0。EXE 5b56b22cc64028b851274ffcf54084d65601090df6a83209af9cfab696a1cdd0、core 8f238b53d72999e8682be5c95db80d3313f728211821ff696006ef0360ac2f7a。work/acceptance/authored-style-band/20261003T150036596Z/run.json対象14項目exit0。明示copy/no-op/invalid index/一回UndoRedoと依存cacheを追加検査。各fixtureは作成ごとにGUIDが異なるため旧/新の全bytes同一とは主張しない。新版内ではunassigned.stpからfinal Heartlnd.stpへの変更が選択Motifへ追加したDMBDだけであることを独立監査した。native全suite/新GUI試験は未実行。

work/acceptance/audio-loopback/20261003T150057747Zの新規default render endpoint WASAPI loopbackは既存ソース製録音器102354462Z、48kHz/2ch/float32/16秒、player/capture exit0。Style-only GetMotifでC5x6、768clock間隔/duration384/velocity96/PChannel5/group1、自然終了。onset 3.2秒、active RMS 0.021547966736025284、baseline/tail RMS 0/0、peak 0.12410677969455719、最大packet gap 2frames。入力mtfs[1,0,768,2304,1]からpreloop3+repeat2+tail1の有限6音。Inspect-AuthoredStyleBand.mjsはraw rootBand bins44・flags0x1163/patch48/channel5/pan64/volume100/他zero、Motif内Band全bytes一致、name/type/settings/Part GUID binding/4音、空Style→Band→Motif非変更chunk保持、runtime Style/input.stp全一致、input.sgp/runtime.sgp不存在、GetMotif一回/Load DMSGなし、同保存sources/workspace/build/exe/core/driver/入力hashを照合。work/acceptance/audio-auditor-controls/20261003T150200Z-authored対照4件は未変更合格、無音/誤音程/背景音混入を拒否し、API成功は保持した派生copy。過去の無音を成功へ置換せず保持する。

現行host/録音module監査は各原版40hash一致0の点観測。Windows DirectMusic/DirectSound/GM.DLS依存は残り、全40責務完成を意味しない。人の聴取/ステレオミキサー/マイク/OS設定変更は不要。デジタル録音の結果を物理スピーカーやGUI Playの成功へ転用しない。前回143452471ZのGUI成功は旧生成物の証拠として保持。

再現：Build-ProductSnapshot.ps1、Test-AuthoredStyleBand.ps1 -BuildSummaryPath <summary>。作成されたcore/Heartlnd.stpへTest-LoopbackAudio.ps1を同summary/recorder102354462Z/-Profile motif-standalone/-MotifName 'Authored Motif'で実行。Node scripts/Inspect-AuthoredStyleBand.mjs <native run.json> <audio dir>。Test-LoopbackAuditor.mjs <audio dir> <新対照dir>。Test-ProductHost.ps1/Inspect-ProductModules.ps1を同summaryへ指定。旧失敗再試行は行わず明示assignmentという条件変更後だけ新録音。通常の承認済みWindows環境を使用。

次の具体的な一手は同版本体GUIで未割り当てStyleのMotifを選び、Band選択/新メニュー/UndoRedo/保存/別起動復元を実操作し、保存bytesと明示copyを検証する。次にMotif Bandの再割り当て/既存copyの編集UIとcustom DLSを進める。原版新Style/Band/Motif defaults比較、Motif指定時刻/secondary/実tempo、Clipboard/JAZP/全40責務/全八受入は未完了。全体条件を縮小しない。凍結記録：work/analysis/authored-style-band/20261003T150300Z/unit-record.json。

## 2026-10-04 通常Patternの発音とStyle Bandコピー

root StyleにDLSを割り当ててもBandTrackのない通常Style Segmentではnotes生成/DLS検索成功と録音全無音が同時に成立した（024213050Z）。Band downloadとpatch選択は別の役割である。Microsoft仕様: https://learn.microsoft.com/en-us/previous-versions/ms808954(v=msdn.10)。Framework::assign_style_bandは選択root BandをSegmentの時刻へ明示コピーし、Collectionを所有GUIDで接続、元Styleを保持する。GUI Copy Band at clocks source一覧に各Style root Bandを列挙する。独立Bandコピーは保持。自動的なroot Band切替や原版との動的同等性は主張しない。

現行024829130Zの通常Pattern4音/Part PChannel5、Sequenceなし、明示Band copy、Tempo0:120/3072:180、Groove50を独立rawで検査。025034077Z録音のPlay/replay各16onsetsは0.5→約0.333秒、所有DLS440Hz、停止3区間RMS0。無音/停止中音/誤テンポの対照拒否。GUI選択実行は未確認。再現と全体未完了範囲はwork/analysis/normal-style-dls/20261004T023800Z/report.mdに記録。


## 2026-10-04 通常StyleのGUIコピー・履歴・終了と音声不合格の分離

最新記録はwork/analysis/normal-style-gui/20261004T030000Z/report.mdとunit-record.json。現行024829130Z保存62sources変更なし。GUI PID10044でStyle root Bandコピー/Save/Undo Save/Redo Saveは現行core全bytes一致、Project/Style/DLS不変。GUI104と再生後128module/address由来passed/原版40hash一致0。File Exitはnative process handleでexit0、強制終了なし。別起動復元は未実行。

無人GUI録音は合格基準を維持して不合格を記録した。031121507Zは22秒曲のStopが自然終了に遅れtiming不合格。別128.6667秒入力（segh length4bytesのみ）031715101Zは両早期Stop/440Hz/120→180BPM/停止無音passedだがPlay前17〜18秒に音が混入しbaseline不合格。原因未特定、同条件再試行なし。CLI同版025034077Zの26秒passedは別範囲で保持。音声APIや保存成功でGUI音声合格を代用しない。

次は別プロセスの同版GUI Project復元/再保存、録音開始UTC/QPC対照とbaseline混入原因の切り分けを実装する。既存原版警告OK回答は確認済みで再要求しない。Windows DirectMusic/DirectSound/GM.DLS/WASAPI依存は保持。残る40責務/全八受入/原版動的比較/ndscを継続し、全体未完了。


## 2026-10-04 通常Style別起動復元と録音時刻対応

work/analysis/normal-style-reload/20261004T033300Z/report.md・unit-record.jsonを最新記録とする。現行製品024829130Zソース62点は変更なし。別PID8628/window32771656で前保存Projectの120→180 BPM/Band0/0/所有DLS bank2 program7/Region loop800を復元、Segment/DLS再保存全bytes一致、GUI104由来passed/原版40hash0。前10044 exit0と別起動を関連付けた。8628は保存済み停止中で保持、今回通常終了は未実行。

録音器032918787Z configure/compile0、開始終了UTC/QPC schema2を追加し、GUI操作を実録音起点へ対応させる。6秒033129067Zと現行GUI停止16秒034521901Zの時刻/packet由来passed、PCM測定RMS0/peak0、4時刻改変拒否。Date.parseのsub-ms欠落はBigInt小数復元で修正し同録音のみ再解析。GUI5phase音声は未実行、前不合格2録音を保持し原因解明/成功を推測しない。Microsoft API資料は本単位reportのリンクに記録。追加録音APIはWindows8以降、製品条件を独断で変えない。

次は保持中8628で別LongNormalGui Projectを開き、新録音器/時刻対応/事前無音確認によるGUI5phase音声を実施。その後articulation/native ndsc/原版比較、全40責務/全八受入を継続する。全体未完了。


## 2026-10-04 通常Style GUI無人音声受入

2026-10-04校正済み通常Style GUI音声：work/analysis/normal-style-gui-audio/20261004T035000Z。現行024829130Z/PID8628、録音器032918787Z、180秒5phase、440Hz・120→180BPM両再生合格（64/72音）、3無音RMS0、自然終了前Stop/再開/finalStop、128module原版40hash0。4派生反例拒否。製品62sources不変、configure/build/installは同版既存0。物理スピーカー/GUI MIDI callback/fullcore/full40/full8未確認。次はDLS articulation/native ndsc/原版比較。 詳細・再現手順は [work/analysis/normal-style-gui-audio/20261004T035000Z/report.md](../../work/analysis/normal-style-gui-audio/20261004T035000Z/report.md)。原版依存・全体対象と八受入条件は維持する。

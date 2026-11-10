# 本体と編集モジュールの接続調査

更新日：2026-10-02。工程1は作業中。以下の RVA は [modules.csv](modules.csv) のハッシュで識別したバイナリに対する値。

## 確認できた経路

| モジュールと RVA | 静的に確認した内容 |
| --- | --- |
| DMUSProd `0x2432e` | 初期化経路の途中で `0x18c93` を呼ぶ。初期化関数全体の名前はまだ確定していない |
| DMUSProd `0x18c93` | `HKLM\Software\Microsoft\DMUSProducer\Components` を開き、サブキーを列挙する |
| DMUSProd `0x18de1`、`0x18e02` | 列挙した文字列を GUID 化し、`CoCreateInstance` を呼ぶ。生成要求の IID は RVA `0x3d70` にあるが、名称は未確定 |
| SegmentDesigner `0x1248b` | 引数の GUID を文字列化し、`HKLM\Software\Microsoft\DMUSProducer\StripEditors\{GUID}` を構築する |
| SegmentDesigner `0x12555`〜`0x125a7` | 値名 `StripManager` を読み取り、文字列から GUID を得る。失敗時は別の既定 GUID を返す経路がある |
| TempoStripMgr `0xc2bf`、`0xc2ff` | 登録用の経路で編集クラスの GUID とトラックの GUID を別々に文字列化する |
| TempoStripMgr `0xc38d`〜`0xc3cb` | `StripEditors` 配下へ対応を記録する処理。登録処理は実行していない |

TempoStripMgr の編集クラスは `{C6ED2EA5-F1D3-11D1-88CB-00C04FBF8D15}`。その登録リソースは `REGISTRY/102/1033`、RVA `0x14628` で、Apartment の in-process COM クラスとして記述されている。

対応付けに使うトラック側 GUID は RVA `0x1e28` の `{D2AC2885-B39B-11D1-8704-00600893B1BD}`。編集クラスの GUID とは異なる。インポート一覧だけでは、このレジストリを経由する対応関係は現れない。

この時点では「本体の Components 列挙」「SegmentDesigner の StripEditors 解決」「TempoStripMgr の登録と生成」を個別に確認している。これらをつなぐ初期化順序すべてを動的に観測したわけではない。

## Framework との接続

本体の RTTI は `CFramework` と `IDMUSProdFramework` / `IDMUSProdFramework8` の存在を示す。`CFramework` の先頭仮想関数テーブルは RVA `0x36fc`、QueryInterface は RVA `0x19e08`。

GUID `{3B8D0E01-46B1-11D0-89AC-00A0C9054129}` は本体 RVA `0x3de0`、TempoStripMgr RVA `0x2634`、SegmentDesigner RVA `0x4d4c` に同じバイト列で存在する。本体の QueryInterface は `0x19e73` でこの GUID と比較し、先頭インターフェースを返す。RTTI と合わせて Framework の接続 IID と判断できるが、Framework の全メソッド宣言は未復元である。

TempoStripMgr の仮想関数スロット8、RVA `0x5bd9` はプロパティ設定処理と判断できる。プロパティ値2、`VARIANT` の `VT_UNKNOWN` を受ける分岐は、渡されたオブジェクトにこの Framework IID を要求し、結果を `CTempoMgr + 0x34` に保存する。ここが未設定のまま Load を呼ぶと異常終了することが動的試験で分かった。

## 未完了の確認

- 本体のアプリ固有初期化の入口と終了処理の対応。
- Components の IID 名と、各コンポーネントの初期化呼び出し順序。
- 文書を開く処理から SegmentDesigner とトラック編集部へ至る経路。
- SegmentDesigner の複数の生成経路の差と、本体上での呼び出し順序の動的観測。
- Timeline の本体上での生成順序と、描画・プロパティページを含む所有権の境界。
- 原版のインストーラーが実際に作るレジストリと配置。

## 生成から読込までの追加確認

SegmentDesigner の一般的な生成経路では、`0x182d8` で StripEditors を解決し、`0x182f2` で編集オブジェクトを生成する。その後、プロパティ3（トラックヘッダー）と4（フラグ）の設定・取得を経て、`0x18450` でプロパティ2へ Framework を設定する。ランタイムトラックの生成は `0x18483`、編集オブジェクトのプロパティ1への設定は `0x184dc`。保存ストリームがある場合、`0x18537` の QueryInterface と `0x18557` の呼び出しを通じて IPersistStream::Load に進む。

別の経路（`0x15ec3` から）は、既に存在するランタイムトラックを先に設定する。上記の順序をすべての生成経路に共通する規則とはしない。現時点で必要と確認できた条件は、Load の前に Framework とランタイムトラックが接続されていることである。Timeline を接続しない限定試験でも初期ストリームの読込・再保存とランタイムへの反映は成功した。後続では原版 Timeline の接続も確認したが、編集画面の描画・操作は未検証。

| 接続先 | スロットと RVA | 復元した呼び出し |
| --- | --- | --- |
| TempoStripMgr | スロット8、`0x5bd9` | `HRESULT SetProperty(DWORD, VARIANT)`。VARIANT は値渡し。x86 の戻り命令は `ret 0x18` |
| Framework | スロット3、`0x19f77` | `HRESULT FindComponent(REFCLSID, IUnknown**)`。該当コンポーネントがない場合は `E_FAIL` |
| Framework | スロット19、`0x17728` | `HRESULT AllocMemoryStream(DWORD, GUID, IStream**)`。GUID は値渡し、`ret 0x1c` |
| CMemStream の PersistInfo | スロット4、`0x29c09` | 24 bytes の情報を返す。先頭 DWORD、続く GUID、末尾ポインターの配置を確認 |

Framework のストリーム生成は `0x29df0` へ委譲され、CreateStreamOnHGlobal のストリームを CMemStream で包む（コンストラクター `0x299b3`）。TempoStripMgr はストリームへ IID `{A8AE1161-99FD-11D0-89AC-00A0C9054129}` を要求し、PersistInfo のスロット4から保存形式 GUID を取得する。試験用ホストは、この使用経路と参照管理を実装している。コンポーネント一覧は空として `E_FAIL` を返し、未復元のスロットが呼ばれたらプロセスを失敗終了させる。

## SDK 宣言との照合

公開ミラー [edgeforce/directx8](https://github.com/edgeforce/directx8/tree/e6fa10f5f92bf523c15fdb068206ce25c8d2121d/include) のコミットを固定して `dmplugin.h`、`dmusici.h`、`dmusicf.h` を取得した。URL と SHA-256 は [sdk-reference-sources.json](sdk-reference-sources.json) に記録している。Microsoft が現在配布していることや、Producer 固有ヘッダーの入手を意味しない。

- `dmplugin.h`：`F96029A1-4282-11D2-8717-00600893B1BD` は **IDirectMusicTrack**。Track8 の IID とは異なる。GetParam / SetParam はスロット7 / 8。
- `dmusici.h`：テンポ取得・設定に使う GUID_TempoParam は `D2AC28A5-B39B-11D1-8704-00600893B1BD`。
- `dmusicf.h`：DMUS_TEMPO_PARAM は MUSIC_TIME と double。8-byte packing で16 bytes、double はオフセット8。

この宣言でランタイムの90 BPM設定・取得と、原版 Load 後の120 BPM取得を確認した。原版本体の Framework を実行した試験ではなく、復元した限定サービスと実際の COM ランタイムを接続した試験である。

## 原版 Timeline の接続確認

Timeline の CLSID は `{DB838A7C-B4F5-11D0-A97F-00A0C922E6EB}`、接続 IID は `{22B5869D-523E-11D2-8913-00C04FBF8D15}`。`tests/native/reference_timeline.h` で実際に呼んだメソッドは次のとおり。RVA は Timeline.dll（SHA-256 `bebc8149e31f3b4b0b74bffbf482acc38ba8b5b28a1468c9741e71961c6db176`）内。

| スロット | 復元した役割 | RVA |
| --- | --- | --- |
| 3 | InsertStripMgr(IUnknown*, DWORD groupBits) | `0xe1e2` |
| 10 | SetTimelineProperty(DWORD, VARIANT by value) | `0x1409d` |
| 11 | GetTimelineProperty(DWORD, VARIANT*) | `0xe8da` |
| 13 | ClocksToMeasureBeat(DWORD groups, DWORD index, LONG time, LONG* measure, LONG* beat) | `0xbdd1` |
| 15 | MeasureBeatToClocks(DWORD groups, DWORD index, LONG measure, LONG beat, LONG* time) | `0xbf65` |
| 31 | RemoveStripMgr(IUnknown*) | `0x104ac` |
| 33 | GetParam(REFGUID, DWORD groups, DWORD index, LONG time, LONG* next, void*) | `0xb487` |
| 37 | EnumStrip(DWORD index, IUnknown**) | `0xbb60` |
| 40 | AddToNotifyList(IUnknown* manager, REFGUID, DWORD groups) | `0x105d6` |
| 41 | RemoveFromNotifyList(IUnknown* manager, REFGUID, DWORD groups) | `0x106fd` |
| 42 | NotifyStripMgrs(REFGUID, DWORD groups, void*) | `0xe467` |

登録処理は StripMgr のプロパティ0へ Timeline を設定する。TempoStripMgr は Timeline に strip と通知を登録する。削除時はプロパティ0を切断し、strip と通知を解除する。これらの内部呼び出しは静的に追跡した。`20261002T005936693Z` では登録後の GetParam 委譲、strip 取得と管理オブジェクト同一性、変更・削除後の取得、登録解除、両 DLL のアンロード可能状態を動的に確認した。

位置の変換には原版 TimeSigStripMgr を追加した。生成直後の拍子モデルは空で、4/4の `tims` を読み込ませた後に変換が成功した。Timeline のプロパティ1に長さ30720を設定し、テンポ時刻4708を小節1・拍2・tick100へ変換している。`20261002T014530015Z` と代替版との比較で、非0位置の変更・1小節移動と3 DLLの解放まで確認した。複数 strip の中からテンポ strip を見つける際には、管理オブジェクトの IUnknown 同一性を照合する。

その後、`20261002T020601617Z` で境界・複数選択の編集と拍子変更通知を追加した。Timeline のスロット42を介し、拍子変更時には絶対時刻を再配置し、位置更新通知では絶対時刻を維持する原版の動作を観測した。代替版との比較で712レコード・55ファイルが一致している。

スロット40/41の登録・解除は代替版に実装済みで、原版内部の通常経路では登録対象ポインターを AddRef せず保持する。参照数が0になるだけでは解除漏れを証明できないため、追加プローブで6 GUIDの登録の存在・重複なし・切断後の不在を確認した。追加試験は一度起動拒否されたが、同じハッシュの後続実行で成功。原版 `20261002T021855247Z` と代替版 `20261002T021919993Z` の比較は750レコード・55ファイルで差分0となった。

イベント追加の接続も確認した。Timeline のスロット7は ClocksToPosition(LONG, LONG*)、14は PositionToMeasureBeat(DWORD groups, DWORD index, LONG x, LONG* measure, LONG* beat)、5は SetMarker(DWORD marker, DWORD unit, LONG time)、6は GetMarker(DWORD marker, DWORD unit, LONG*)。戻り値はいずれも HRESULT。表示倍率はスロット10のプロパティ8、VT_R8で設定する（RVA `0x14313`）。0.125を明示した試験で時刻900はX=113となり、追加処理はスロット14・15を使って拍の先頭768へ変換する。保存・同期・解放まで原版と代替版の結果が一致した。

追加処理はプロパティページの表示も要求する。Timeline のスロット18は RVA `0xc198` にあり、Framework 未接続の場合は E_FAIL となる。現在の限定環境でデータ処理が一致しても、この UI 経路の互換性は証明できない。次は残る通知処理、コピー・Undo、プロパティページへの依存を調べ、原版 UI 上の試験へ範囲を広げる。

## コピー用データと Undo 名の境界

Timeline のスロット43（RVA 0xbba9）はIUnknown**へTimelineDataObjectを生成して返す。TempoStripMgrのCopyはその境界を取得し、拍の先頭を基準とする24バイトレコードを追加する。データオブジェクトのスロット3/5/6/7/8と所有権は [TempoStripMgr仕様](tempo-strip-manager.md) に記録した。コピー用データを呼出し側が渡す経路では、システムクリップボードを介さず比較できる。

Undo用の操作名はStripMgr::GetParamへ専用GUIDを渡してBSTRとして取得する。追加・変更・移動・削除でラベルが変わり、Copyは保持、CutはDeleteのラベルとなる。Pasteは Paste Tempo(s) を返す。本体側の履歴保存・復元処理への接続は未確認。

## 貼付け時の TimeStripMgr 接続

Timeline.dll 内の CLSID 884F3F04-BFE0-11D0-BBDB-00A0C922E6EB は TimeStripMgr。StripMgr IID で生成し、プロパティ0へ Timeline を直接設定する。SetProperty（RVA 0x17dfe）は TimeStrip を生成・登録し、Timeline 内の時刻ストリップへの参照を設定する。切断は同じプロパティに VT_UNKNOWN / null を渡してから Release する。通常の編集トラックとして InsertStripMgr へ渡す経路は今回使用していない。

この接続がない限定環境では SetMarker(0,0,time) が S_OK でもカーソルは0のままだった。接続後は900と30000の設定・読戻しが一致し、その位置を使う貼付け結果も原版と代替版で一致した。戻り値だけで位置設定を判定しない。貼付けモードは Timeline スロット44 GetPasteMode(DWORD*)（0xbc5d）、45 SetPasteMode(DWORD)（0xbc98）で取得・設定する。

TimelineDataObject のスロット6はストリームを複製・巻戻しするだけでなく、形式を利用可能リストから消費済みリストへ移す（0x16064～0x16114）。観測用に一度取得したデータをそのまま Paste へ渡すと CanPaste が失敗する。プローブでは観測用と実行用を別々に作り、この状態変化もログで検証する。


## Timeline の表示位置と OLE ホスト

Timeline は標準 IOleObject と IOleInPlaceObject を公開する。非表示の限定OLEコンテナーへDoVerb(OLEIVERB_INPLACEACTIVATE)で接続し、子HWNDの生成・終了まで確認した。ホストは tests/native/hidden_ole_site.h、単独プローブは timeline_window_probe.cpp。これは原版 DMUSProd.exe を実行した結果ではない。

SetTimelinePropertyのプロパティ9はVT_I4の横スクロール位置（ピクセル）。分岐RVA 0x14343から0x10422、0x1025fを通り、実スクロールバーの範囲と位置を使う。GetTimelineProperty(9)はVT_I4で読み戻せる。表示開始時刻はGetMarker(3,0,LONG*)で取得する。20261002T035622533Zでは、倍率0.125で設定192→読戻し192→開始1536、設定500→読戻し500→開始4000、0への復帰を観測した。

描画比較では20ケースすべてで位置を読み戻し、横スクロール8ケースでは開始1536または1600を検査する。OLEホストの切断後もstrip・通知の解除と原版TimelineのDllCanUnloadNow=S_OKを確認した。旧ATLのウィンドウ生成に必要なDEP互換条件は [試験環境](reference-environment.md) に記録した。

## Timeline からプロパティシートへの接続

SetTimelinePropertyのプロパティ3（VT_UNKNOWN、分岐RVA 0x14218）はFrameworkを保持する。Timelineスロット18（RVA 0xc198）はFrameworkへシートIID 3095F6E0-C160-11D0-89AE-00A0C9054129を問い合わせ、シートのスロット14がS_OKを返す場合にページ管理と編集対象を接続する。それ以外はS_FALSEで戻る。

ページ管理と編集対象へそれぞれQueryInterfaceし、シートのスロット3へページ管理を設定する。設定が失敗しなければページ管理のスロット6へ編集対象を渡す。取得したインターフェースは最後にReleaseする。TempoMgr::ShowPropertiesはこのTimeline呼出しの失敗を戻り値に反映しないため、表示成功はページの生成と接続状態で判定する必要がある。

原版Timelineと限定Framework・シートを使う動的比較で、非表示・表示・再表示・設定失敗と参照解放を確認した。ページ管理側の仕様は [TempoStripMgr仕様](tempo-strip-manager.md) に記録した。実際の本体のシートウィンドウと編集コントロールへの接続は未確認。

## 右ドロップメニューのウィンドウ境界

TempoStripMgrの0x971aはTimelineのGetStripProperty（スロット30、property1）へVT_I4を渡してHDCを受け取る。WindowFromDCで所有HWNDを得てReleaseDCする。右DropはそのHWNDへTrackPopupMenu(TPM_RIGHTBUTTON)を呼び、DestroyMenuの後で当該ウィンドウのキューをPeekMessageA(PM_REMOVE)・TranslateMessage・DispatchMessageAで排出する。HDCをHWNDと解釈した初回候補の差異を修正した。

現行試験は実メニューの項目・灰色化と閉鎖結果を比較している。Move/Copy/Cancelのコマンドをどのホスト／インターフェースへ配送するかは未確定で、内部の保留効果へ値を書き込む経路を直接改変して代用していない。次はTimelineのコマンド配送とstripのSetStripPropertyを追う。Windows実OLE中止の限定環境・失敗履歴は [試験環境](reference-environment.md)、6449件比較は [比較結果](tempo-comparison.md) を参照。

## Components 未登録による本体初期化の中断

DMUSProd RVA 0x18c93 は HKLM の Software\Microsoft\DMUSProducer\Components を KEY_READ で開き、失敗するとリソース0xee76のエラーを表示して false を返す。Default デスクトップでの起動試験はこの表示と一致した。キーが存在する場合は GUID 名のサブキーと既定表示名を列挙し、DWORD Skip=1 の項目を除外する。RVA 0x18e02 の CoCreateInstance が IID 9F3ED901-46B7-11D0-89AC-00A0C9054129 を要求する。

SegmentDesigner の登録ヘルパー0x1f9beは、Components\{DFCE860B-A6FA-11D1-8881-00C04FBF8D15} の既定値を Segment Designer、Skip を0に設定する。HKCR の COM 登録と HKLM の探索用登録の両方が必要である。現環境では登録未実施。再抽出用スクリプト `scripts/Inspect-ProducerRegistration.mjs` と [producer-registration.json](producer-registration.json)、動的結果は [tempo-integration.md](tempo-integration.md) を参照。

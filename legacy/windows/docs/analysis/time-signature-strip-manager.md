# TimeSigStripMgrの接続・保存仕様

対象は5.3.0.900、131584 bytes、SHA-256 `898258cfbf1b17bef0054695d25331c530e2ee2846c5d45073a8fd5670484bb7`。`src/time_signature` に保存・時刻問合せ・接続情報・サービス参照管理とTimeline接続の限定した互換ソースがある。保存17ソースからビルドした候補630bで、原版と322観測・27入出力ファイル、および試験用Timelineの接続・解除160観測が一致した。実Timeline、Style拍子データの取込み、UI・編集・同期・本体置換は未確認である。

`Inspect-TimeSignatureAbi.mjs` が元DLLのハッシュ、4つの仮想関数表39スロット、13識別子と接続情報の32命令列をバイト照合する。抽出結果は `work/analysis/pe/app__TimeSigStripMgr.dll/time-signature-abi.json`（schema5、SDK参照を照合）。StripManager表はRVA24a4、IPersistStream表2480、編集表20b4、Strip表210c。クラス識別子は8c6005d2-abda-11d2-b0d9-00105a26620b、拍子パラメーターはd2ac28a4-b39b-11d1-8704-00600893b1bd。取得値の配置はLONG time、BYTE beats、BYTE denominator、WORD gridsの8 bytes。

## 動的に確認した範囲

原版の境界追加実行は `work/reference/time-signature/20261002T095342635Z`。プローブSHA-256 `2a2afa3cb0b157fd24007c47d60dc73faf4b6a5dd547ca43aca9a42848acfc4d`、保存11ソース、終了コード0。261件の観測、196件の時刻問合せ、13入力、14保存を保持し、managerの最後のRelease=0とDllCanUnloadNow=S_OKまで確認した。最初の176件／7ソースの実行 `20261002T094657214Z` も別履歴として保持する。

`Summarize-TimeSignatureProbe.mjs` のschema2は両観測版を検査する。入力を期待バイト列と照合し、全保存のチャンク境界・レコード・期待イベント、14時刻の問合せと保存の整合性、HRESULT、プロパティ値、Dirty、解放を検査する。run.json・ログ・保存ソース・入力・出力・検査器のハッシュを保持する。新しいメタデータ観測版が成功したとは推定せず、未対応版の集計を拒否する。

| 操作 | 観測 |
| --- | --- |
| 生成直後のGetParam | 0x88781161、next=0、8 byte出力値を変更しない |
| 拍子GUIDの対応判定 | S_OK。未知GUIDはS_FALSE |
| GetParamの未知GUID／null出力 | E_INVALIDARG／E_POINTER |
| 拍子GUIDのSetParam／null入力 | E_INVALIDARG／E_POINTER。拍子追加APIとは扱わない |
| GetClassIDのnull／Load・Saveのnull | E_POINTER／E_INVALIDARG |
| IsDirty、GetSizeMax | S_FALSE、E_NOTIMPL |
| 接続先未設定のプロパティ0〜2 | VT_UNKNOWN/null、E_FAIL |
| プロパティ4／5、VT_BYREF | 56／0を返す |
| プロパティ6 | VT_I4、0x30038 |

単一4/4、3/8、5/4のtimsを読み込むと、時刻0以降は該当拍子を返す。時刻−1では先頭拍子とnext=1。先頭が時刻3072の入力では、時刻0に同じ拍子を返しnext=3072、時刻3072ではnext=0になる。GetParamの返すtimeは今回の全成功ケースで0だった。一般的なDirectMusicランタイムのtimeと同じ意味だとは推定しない。

時刻0の4/4、3072の3/4、7680の5/8を順に読むと、3071では4/4・next=1、3072では3/4・next=4608、7680以降では5/8・next=0。内部は小節位置へ変換する経路を持つが、全入力・異常値・並び順の仕様は未確定。これらの入力は保存時に元の時刻へ戻った。

追加した5ケースでは、先頭1500の3/8は保存時に1152へ丸められた。後続4100と8000は、それまでの拍子で小節位置に変換され、保存時3072と7680になる。同じ小節に2レコードがある場合は後の3/4が置き換える。順序逆転した6144の4/4→0の3/4は、保存時0の3/4→4608の4/4になる。負の先頭−3072は保存に残り、今回の問合せ範囲ではその4/4を返した。単純な時刻順ソートや全時刻の0以上への制限では再現できない。これらは今回の固定入力の観測であり、任意の異常値の仕様ではない。

beats・denominator・gridsが全て0の入力は4・2・2へ置換された。LIST:TIMSで包んだレコードサイズ12の入力も読み込め、末尾4 byteを保存に残さず、サイズ8で保存した。空のtimsをLoadすると以前のマップが空になった。

保存はLIST:TIMS。空モデルは12 bytesで子timsを出さず、非空モデルはtims子チャンクにDWORD recordSize=8と8 byteレコードを並べる。1イベント32 bytes、2イベント40 bytes、3イベント48 bytes。全保存のハッシュとデコード済みイベントはsummary.jsonへ保持した。未初期化領域の正規化は行っていない。

再検査：

```powershell
cmake -S tests/native/time_signature -B work/build/time-signature -G 'Visual Studio 17 2022' -A Win32
cmake --build work/build/time-signature --config Release
./scripts/Run-TimeSignatureProbe.ps1
node scripts/Summarize-TimeSignatureProbe.mjs work/reference/time-signature/20261002T095342635Z
```

## 静的に確認した接続情報と追加ソース

| 経路 | 原版の根拠・追加した動作 |
| --- | --- |
| GetStripMgrProperty(3) | RVA67e7〜6816。VT_BYREFの32 byteヘッダーへ時刻トラックCLSID、position=0、group bits、chunk=0、list=TIMSを返す |
| SetStripMgrProperty(3) | RVA76c6〜76cc。入力ヘッダーのgroup bitsだけを取り込む。他の4フィールドは取り込まない |
| property4／5 | VT_BYREF・非nullを要求。4はRVA769cで0x30038のマスク、5はRVA767cでマスクなし。初期値56／0、group bitsは1 |
| 借用IUnknownパラメーター | GUID f9a03440…。RVA66be〜66c4のSetはAddRefせず保存。Getは65ea〜65fcで出力へ参照を追加。manager基底位置を考慮した破棄経路74c2〜7574ではこの借用スロットを解放しない |
| 操作名パラメーター | GUID178633a6…。以前の抽出名stripNameParamをundoLabelParamへ訂正。RVA660f〜663fはstripのresource IDからBSTRを返す。初期resource IDは8e75で0。編集後の操作名は未実装 |

上表の経路を互換ソースへ追加した。初期操作名は空BSTRを返す。後続の322件の比較で、トラック情報・フラグ・借用参照と初期操作名の動的観測も確認した。借用ポインターは呼出し側が寿命を維持する必要がある。サービス接続property1・2と、試験用Timelineで比較したproperty0の接続・通知登録／解除も後述の通り追加した。UI・編集・通知の配送・ランタイム同期は未実装であり、本体対応とはしない。

## ロード拒否と保存ソースからのビルド

候補4d2fc233…は同じ11ソース・プローブによる `work/candidate/time-signature/20261002T095342972Z` でLoadLibraryExWが0x800711c7（Windows error4551）を返し、終了1。候補の動作比較の結果ではなく、DLLロード拒否として保持する。

接続情報・借用参照の観測プローブを追加してビルドしたが、その新規EXE `ce2089eb799cd42d2bc5681e3166826362bea11bfa738020afea6bd1f17416ad` も最初の実行でOSのアプリケーション制御に拒否された。`work/reference/time-signature/20261002T105324536Z/run.json` はexitCode=null、launchErrorに拒否、probe.jsonlは空である。追加のトラック情報・参照所有権・操作名の動的観測には数えない。用語を訂正した後続ビルドも実行していない。

共通CMakeにTimeSigのcore、DLLと独立プローブを追加した。`scripts/Build-TimeSignatureSnapshot.ps1` は13ファイルを新しいsourcesディレクトリへハッシュ照合してコピーし、その保存物だけから構成・ビルドする。成功記録 `work/build/time-signature-snapshot/20261002T110056098Z/build-summary.json` はconfigure/buildとも終了0、全保存ソース不変、3生成物のサイズとSHA-256を保持する。生成DLLは49ddc5b83506961643e914f7c15fdd6cd8db8184129c9140d7cc08b8f078b498。この生成物の実行・ロード・登録・本体置換は行っておらず、ビルド成功を互換性の成功へ拡大しない。

```powershell
./scripts/Build-TimeSignatureSnapshot.ps1
node scripts/Inspect-TimeSignatureAbi.mjs
node scripts/Summarize-TimeSignatureProbe.mjs work/reference/time-signature/20261002T095342635Z
```

残る判定は同じ入力の候補動作比較、接続情報と借用参照の動的確認、Timeline・UI・拍子編集・選択・コピー・通知・ランタイム同期、本体置換と音声出力である。全体再構築の完了条件は変更しない。

上記はその時点の履歴である。後続で候補動作・接続情報・借用参照を確認した範囲は次節に記す。以前拒否されたDLL／EXEのハッシュと拒否記録は保持し、新しい生成物の成功で過去の拒否を取り消さない。

共通構成のクリーンビルドは `scripts/Build-RecoverySnapshot.ps1` でも確認した。`work/build/recovery-snapshot/20261002T110246069Z` の保存47ファイルだけから、Tempo・TimeSigのcore／DLLと各比較プローブの全10ターゲットを構成・ビルドし、全ソースの不変と全生成物のサイズ・ハッシュを再照合した。原版モジュールはビルド入力に含まないが、生成プログラムの動作時には原版への依存が残る。今回の生成物は実行していない。

`node --test tests/time_signature_summary.test.mjs` は6件成功。旧／拡張の原版観測を受け入れ、入力改変、問合せ不整合、保存コンテナ破損、保存ソース改変とOS拒否の実行を成功扱いしないことを確認する。試験用コピーと結果は `work/reference/time-signature/summary-checks/e08ec163-f064-4d56-838f-dfa5cc264da8` に保持し、元の観測ファイルを改変していない。

## Framework・再生トラックの参照管理と初回候補比較

2026-10-02、property1・2の参照管理を追加した。SetはVT_UNKNOWNだけを受け付け、以前のサービスをReleaseしてから、新しい入力へQueryInterfaceする。property1はIID_DirectMusicTrack（RVA2194、f96029a1…）、property2はIID_IDMUSProdFramework（RVA29e4、3b8d0e01…）を要求する。property1はQI失敗を無視してS_OK、property2はQIのHRESULTを返す。null入力は以前のサービスを解放してS_OK。Getは保存したサービスからcanonical IUnknownをQIで返す。破棄時には両サービスをReleaseする。

根拠はSet property1のRVA7711〜773c、property2の76db〜7705、Getの6819〜6843、破棄の7503〜752b。base+0x34はFramework、base+0x50はruntime。借用ポインターbase+0x54と混同しない。サービスQI、旧サービスRelease、QI失敗を無視する分岐、canonical IUnknownのQI、破棄時Releaseの8命令列を追加照合した。

プローブに、canonical IUnknownと要求サービスのポインターが異なる2つのViewを持つfixtureを追加した。接続、Get／呼出し側Release、同じ入力の再設定、未対応QIへの置換、再接続、null切断、manager破棄まで22段階を観測する。fixture自体はmanager破棄後まで生存させた。保存試験中はfixtureを接続せず、IUnknown以外のFramework／runtime呼出しを模倣していない。実DirectMusicトラックへの同期を確認した試験ではない。

保存13ソースからの構成・ビルドは `work/build/time-signature-snapshot/20261002T113150810Z/build-summary.json`。構成／ビルド終了0、保存ソース不変、プローブSHA-256 `83eda4a34e8ad15f306940bc0a510be12be06e2e3f78e233aa42d47ac30bba07`、候補DLL SHA-256 `3ac571e20d9e4fb8f052c7009975fd6211f3f0819f5a8842297bf770ace6c74a`。RunスクリプトへBuildSummaryPath照合を追加し、保存ソースと現ソース、プローブと候補生成物のハッシュを確認してから実行した。

原版 `work/reference/time-signature/20261002T113228581Z`、候補 `work/candidate/time-signature/20261002T113247424Z` は同一プローブ・同一12実行ソース・同じビルド記録でともに終了0。比較 `work/comparison/time-signature-dll/20261002T113511741Z/comparison.json` は、322観測ログ全体と13入力・14保存の27ファイルが正規化なしで一致した。196時刻問合せ、トラックヘッダー・フラグ、初期操作名、借用参照とサービス所有権、manager Release=0、DllCanUnloadNow=S_OKを含む。

`Summarize-TimeSignatureProbe.mjs` schema3は旧176件／261件に加え、新しい322件のプロファイルを扱う。全メタデータ、canonical IUnknownの同一性、各段階のQI・AddRef・Release回数、借用参照と所有参照の破棄結果、ビルドの由来を検査する。`Compare-TimeSignatureDll.mjs` は原版集計が成功したこと、両実行の同一ソース・ビルド・プローブ、27ファイルの集合と完全一致、ログ完全一致を要求し、比較器の保存コピーとハッシュも残す。

再実行：

```powershell
./scripts/Build-TimeSignatureSnapshot.ps1
# 生成されたディレクトリのビルド記録を両実行へ渡す。
./scripts/Run-TimeSignatureProbe.ps1 -BuildDirectory work/build/time-signature-snapshot/20261002T113150810Z/build -BuildSummaryPath work/build/time-signature-snapshot/20261002T113150810Z/build-summary.json
./scripts/Run-TimeSignatureProbe.ps1 -BuildDirectory work/build/time-signature-snapshot/20261002T113150810Z/build -BuildSummaryPath work/build/time-signature-snapshot/20261002T113150810Z/build-summary.json -ModulePath work/build/time-signature-snapshot/20261002T113150810Z/build/Release/TimeSigStripMgr.dll -Candidate
node scripts/Compare-TimeSignatureDll.mjs work/reference/time-signature/20261002T113228581Z work/candidate/time-signature/20261002T113247424Z
node --test tests/time_signature_summary.test.mjs
```

集計器の試験は10件成功。新プロファイルを受け入れ、canonical IUnknownが異なる結果、所有参照の解放欠落、借用参照の残存、ビルド記録ハッシュの改変も拒否した。試験用コピーと結果は `work/reference/time-signature/summary-checks/ad5e8cb4-4765-4b51-85ad-96c7fbbab52b` に保持した。共通構成も新しい保存47ファイルから全10ターゲットをビルドした（`work/build/recovery-snapshot/20261002T113554670Z`）。この共通生成物は実行しておらず、動作比較に使用した生成物は上述の13ソース版である。

この時点ではproperty0のTimeline接続とStripの生成・挿入・通知登録／解除を次の調査対象とした。その後の観測と実装を次節に記す。実Framework・DirectMusicトラックへの保存同期、拍子のUI編集、選択、コピー、通知、本体置換・再生とProducer全体の再構築は未完了である。

## Timeline接続・切断とStrip基本プロパティ

2026-10-02、property0の接続・切断を復元した。根拠はRVA7752〜7868、Timeline getter6845〜685d、Strip getter9593〜970e。原版は旧接続に対してmanagerのページ対象を解除し、拍子・Style・位置更新の順で通知を解除し、stripのページ対象とstripを除去してTimelineをReleaseする。新しい入力のTimeline QIが失敗するとE_FAILとなり、古い接続は残さない。成功時はstrip挿入、位置更新・Style・拍子の順に通知を登録し、Style管理対象を検索する。挿入・通知登録のHRESULTを無視してS_OKを返すことを、失敗を返すfixtureで動的に確認した。

Timeline getterは保存したインターフェースを直接AddRefする。property1・2のcanonical IUnknown QIとは異なる。Strip property0はグループ名付きのTimeSig、1はVT_BOOL値1、6・8・9はVT_INT値20、7・10はVT_BOOL値0、12はmanagerのcanonical IUnknown。今回のグループ1と0x42では名前は `1: TimeSig` と `2, 7: TimeSig` だった。未知propertyはE_FAIL、null出力はE_POINTER。

`tests/native/time_signature_connection_probe.cpp` は、確認したTimelineスロットだけを持つfixtureで11接続ケースを実行する。未知スロットを呼ばれた場合は失敗して終了する。同じ入力の再設定、置換失敗、挿入と通知の失敗、グループ変更、切断の反復を含む160観測を保存し、20プロパティ・通知登録15回／解除15回・最終fixture参照数1・保持strip数0・manager Release=0・DllCanUnloadNow=S_OKを比較する。本体や実Timelineを起動する試験ではない。

| 証拠 | 保存先 |
| --- | --- |
| 保存17ソースのビルド、4生成物 | `work/build/time-signature-snapshot/20261002T114801406Z/build-summary.json` |
| 同じ最終ソース・プローブの原版接続観測 | `work/reference/time-signature-connection/20261002T114827904Z` |
| 候補の接続観測 | `work/candidate/time-signature-connection/20261002T114834059Z` |
| 接続160観測の完全一致 | `work/comparison/time-signature-connection/20261002T115011578Z/comparison.json` |
| 既存322観測の原版／候補 | `work/reference/time-signature/20261002T114853726Z`、`work/candidate/time-signature/20261002T114900001Z` |
| 既存322観測・27ファイルの回帰比較 | `work/comparison/time-signature-dll/20261002T115011874Z/comparison.json` |
| 接続比較器7テスト | `work/reference/time-signature-connection/checks/7a674de7-6ea8-421e-8946-04a0c71000e7/results.json` |
| 共通48ファイル、全11ターゲットのビルド | `work/build/recovery-snapshot/20261002T115108613Z/build-summary.json` |

候補DLL SHA-256は `630b4a9bc10bb84404290807c7715645d7c9ea288765797fea16a1e930c20cc2`、接続プローブは `52362a65a5509b9b68deb98ce0d4df18cf702b7ee318b192aa7204277afd5c71`。接続・既存比較とも正規化はなく、ログ全体と対象ファイルをバイト単位で比較した。初回観測 `20261002T114551814Z` と、不足ヘッダーによるビルド失敗 `20261002T114457660Z` も履歴として保持する。共通ビルド生成物は実行しておらず、今回の動作比較は17ソースの独立ビルドを使用した。

`Compare-TimeSignatureConnection.mjs` は160観測の順序・識別子・戻り値・参照数・プロパティを検査し、原版識別・保存ソース・ビルド・実プローブのハッシュを照合してから候補と比較する。7テストで正常観測を受け入れ、通知解除欠落、登録順序変更、失敗挿入時の戻り値変更、参照残存、manager同一性変更、ビルド記録改変を拒否した。

```powershell
node scripts/Compare-TimeSignatureConnection.mjs work/reference/time-signature-connection/20261002T114827904Z work/candidate/time-signature-connection/20261002T114834059Z
node scripts/Compare-TimeSignatureDll.mjs work/reference/time-signature/20261002T114853726Z work/candidate/time-signature/20261002T114900001Z
node --test tests/time_signature_connection.test.mjs
# 現ソースから新しい保存ビルドを作り、その戻り先をBuildDirectory/BuildSummaryPathへ渡す。
./scripts/Build-TimeSignatureSnapshot.ps1
./scripts/Run-TimeSignatureProbe.ps1 -Connection -BuildDirectory work/build/time-signature-snapshot/20261002T114801406Z/build -BuildSummaryPath work/build/time-signature-snapshot/20261002T114801406Z/build-summary.json
```

Style管理対象の検索に失敗する経路だけを今回比較した。成功すると原版RVA7205はStyle拍子データを取り込み、保持している拍子項目へ反映する。この経路は候補で未実装であり、実Timelineを使う次の調査で復元する。ページ管理のデータ編集、描画、通知配送、ランタイム同期も未実装。接続したままのmanager破棄は原版と比較しておらず、候補の破棄処理は残接続を解除する。今回の比較は明示的に切断した正常寿命だけを証明する。次は実TimelineとStyle管理対象を接続して観測し、本体接続の前提を確立する。全体再構築の完了条件は維持する。

## Style識別子の訂正と実Timelineプローブの制約

SDK参照 `work/analysis/sources/dmusici.h`（SHA-256 `4801df72813ff1aa5a7609c0efb78b888b2f064c962051eb900198cad974a796`）のGUID宣言とインターフェース表を照合した。`d2ac28a1` は `GUID_IDirectMusicStyle`、Tempoは `d2ac28a5` である。以前「テンポ管理対象」「テンポ位置データ」と記した部分をStyle管理対象・Style拍子データへ訂正した。保存済みの旧ソース・実行ログ・比較結果は改変していない。GUIDの値と通知の順序は変えず、現行ソースの名前とコメントを訂正した。ABI抽出器schema5はSDKのハッシュと `IDirectMusicStyle::GetTimeSignature` のslot11も照合する。

静的解析では原版RVA7205がTimeline slot35でStyle管理対象を取得し、対象のGetParam slot4からStyleを時刻0と相対nextに従って列挙する。Styleのslot11から8バイトの拍子を取得し、Timeline slot13で変換した小節位置へ挿入する。取り込んだStyleはReleaseする。Style管理対象が存在する場合、拍子管理DLLへの直接GetParamはE_INVALIDARGとなる。項目offset28の意味、通知・Loadによる再取り込み、UI編集とランタイム同期は引き続き調査対象である。

新しい `tests/native/time_signature_style_probe.cpp` は原版Timelineと合成Style管理対象を接続し、4/4・3/4・5/8の3拍子、既存項目なし・先頭項目あり・後方項目ありの3ケースを観測する。最初の実行 `work/reference/time-signature-style/20261002T115807543Z` は終了0だったが、合成Style管理対象に拍子パラメーターの応答がなく、Timelineの小節・クロック変換が失敗する不完全な条件だった。保存した全クロックが同じ負値になった結果は変換失敗時の不定値と判断し、保存互換性の基準から除外する。Style取得と参照解放の観測だけを保持し、候補の成功比較とは扱わない。

合成Style管理対象に拍子パラメーターの応答を追加し、保存19ソース・5ターゲットのビルド `work/build/time-signature-snapshot/20261002T115913422Z/build-summary.json` は成功した。しかし修正したプローブSHA-256 `911b7a23311eeddb8a20e1ffc8f38b51e518c3cab95cfb7c039dc11e9ef089a5` の実行はWindowsのアプリケーション制御ポリシーで拒否された。`work/reference/time-signature-style/20261002T120234201Z/run.json` にlaunchErrorとexitCode=nullを保持し、probe.jsonlは空である。修正後の実Timeline保存結果・原版との候補比較は未確認。拒否されたプローブの実行を別方式で試みたり、Windows設定を変更したりはしていない。

名称訂正後の現行19ソース・5ターゲットは `work/build/time-signature-snapshot/20261002T120513711Z/build-summary.json`、共通49ソース・12ターゲットは `work/build/recovery-snapshot/20261002T120616172Z/build-summary.json` で構成・ビルド成功を確認した。これらの生成物は実行していない。共通ビルド出力台帳にStyleプローブを追加し、Tempoの実行ソース保存にも接続・Styleプローブを含めて、全CMake入力を保存するよう修正した。接続比較器7件と既存検証器10件の計17テストが成功した。結果は `work/reference/time-signature-connection/checks/311b837c-5a31-4b45-8370-19e45e509fa8` と `work/reference/time-signature/summary-checks/bb2c40a3-2b27-41d9-9935-04c302e1c2a4`。旧候補630bの322／160観測の成功を、今回の未実行生成物の成功へ転用しない。

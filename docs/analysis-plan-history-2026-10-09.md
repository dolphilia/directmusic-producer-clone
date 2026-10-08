# DirectMusic Producer 解析と再構築の実施計画

改訂：2026-10-09（日本時間、0735限定採用を維持。1713 Send既定Envのbuild/関連13試験を保存し、ユーザー依頼で中断）。対象：DirectX 9用 DirectMusic Producer 5.3.0.900。全体未完了、`fullAcceptance=false`。

## 目標と現行の判断

Producer本体と必要なProducer固有機能をソースからビルドし、原版固有のEXE・DLL・OCXに依存せず、下記「全体完成時の受入試験」8項目をすべて満たす。原版と同じソース・同じバイナリ・同数のDLLの復元は要求しない。機能・文書互換性と必要な外部ABIの責務を維持し、対象範囲を独断で縮小しない。

本体を先行実装した方針は継続する。修正済みQ0と既存実装を保護し、SDK障害が解除されたら未検証変更を新候補で検証して、固定候補の主要経路へ統合する。障害中は進行中単位の独立した不足を減らす。未検証機能の追加だけを連続させず、受入不足・未対応責務・原版依存の減少で優先順位を評価する。限定DLL・大量プローブ・compile成功・一つの経路の受入だけで全体完成とはしない。

Waves記録範囲のソース/fixture準備は [2026-10-07継続記録](analysis/q3-waves-reverb-2026-10-07.md)。未完単位の実行と受入はcurrentから判断する。最新の再開判断は [2026-10-07引き継ぎレビュー](analysis/plan-review-2026-10-07.md)。方針の根拠は [2026-10-05レビュー](analysis/plan-review-2026-10-05.md)、以前の全文は [履歴](analysis-plan-history-2026-10-05.md)、さらに以前の方針は [2026-10-02レビュー](analysis/plan-review-2026-10-02.md) に保存。履歴の「次」は現在の指示ではない。

## 現在の基準点

| 項目 | 確認済みの範囲 | 未完了・現在の判定 |
| --- | --- | --- |
| 候補と作業ソース | 最新限定採用は073500338Z・218保存ソース。最新試験171355889Zは224保存/作業ソースと4生成物一致、configure/build/install各0 | 1713未採用。本体GUI/二通常終了/現Q1は未実行。1056 Farm/Bandと旧Q1の証拠は候補別に保持 |
| Send既定Envの専用検証 | 1713関連native13合格、authoring57/runtime89。実global DMO取得/共有Stop/再開/Flush、独立SDKの32768-byte impulseと完全一致 | ユーザー依頼で中断。GUI挿入/既定宛先/native保存/別process復元/二通常終了/WASAPI/原版比較は未達。native91/driver164の全一巡合格ではない |
| native原始一巡 | 1056登録91は53合格/37既知障害/三Band失敗1。0735の90原始一巡は履歴 | 通常coreと各native保存拒否は条件変更なし、個別凍結 |
| driver原始一巡 | 1056の159原始一巡は24合格/23障害/112未実行。新登録5を同候補で限定実行。登録数164、併記計29限定合格/23障害/112未実行 | 0735のFarm10/Q1対照2補足は履歴へ分離。原始一巡と追加5・既存2改訂の証拠を分け、全driver合格にしない |
| 現本体/native | Farm author8660がScript/Segment/Project保存後通常exit0、別PID15508で復元/初期化し通常exit0。23入力は保存コピーと終了後hash一致 | Scriptのactive NAME10/CATEGORY5、owner10を独立監査。case/collision/ANSI/config/graph/full KEEP等は残る |
| 現音声 | 同じreloadPIDで六種SFX音源/音高、Night発音中Stop/無音/再開、private close無音を固定WASAPIで限定確認 | 初回120秒録音の時間外操作失敗を保持。Night/Bird音源/100BPM/一般混合/Predawn/Dawn/End未確認 |
| 現Q1統合 | 0735で新規native Project六形式10文書・五形式編集/history保存・author22256/reload21004二通常exit0・Style/Transport録音・6+11対照が限定合格 | 29 settled観測と12最終入力を結合監査。全責務・原版比較・Q2は未達。030413 Q1/Sendは版付き履歴 |
| 原版依存と全体 | 宣言OS依存と全40責務を保持。受入8は6作業中/2障害/0合格、fullAcceptance=false | 原版approval、Q2独立Windows、全責務・同最終構成の全8は未達 |

限定採用候補は[0735 build](../work/build/product-snapshot/20261008T073500338Z/build-summary.json)。最新の限定完了への入口は[現Q1報告](analysis/q1-current-0735-2026-10-08.md)、[結合監査](../work/analysis/q1-current-0735/20261008T084556994Z/scenario-proof.json)、[採用記録](../work/analysis/q1-current-0735/20261008T084556994Z/adoption.json)。[Farm参照報告](analysis/q3-farm-descriptor-2026-10-08.md)も同候補の限定成立として保持。Legacy AASY/PERはOS Style Save E_NOTIMPL、複数Bandは返却Band ID/NAME無効・Save E_NOTIMPLで未完。[公開API限界](analysis/q3-style-player-multiple-band-2026-10-08.md)を保持する。1056の[Farm音声単位](analysis/q3-farm-background-audio-2026-10-08.md)は二通常exit0・23入力不変とnative/main先頭音の限定PCM照合、独立入力との13実ノート一致まで進んだ。明示22050Hz/Echoを確認したが、全PCM源・Night・100BPM・混合・main発音中Bird停止は未達。同条件失敗や旧Q1を転用せず、追加診断と二回の遅延Stopでも不足は減らず、未完として保存し、Send既定Reverb宛先へ優先順位を移した。1713で既定EnvのSource実装と関連13試験を限定確認したが、本体/native二終了/endpoint PCMは未達として中断する。旧[Q1](analysis/q1-current-candidate-2026-10-08.md)と[Send](analysis/q3-audiopath-send-2026-10-08.md)、Farm183/Waves183/Style210は候補別履歴として保持する。

## 記録の入口と更新規則

1. 本書は目標・受入条件・現行queueだけを管理する。時系列の作業報告や毎回の「次」を追記しない。本文はおおむね250行以内を維持し、古い判断は履歴へ移す。
2. `docs/analysis/product-state.json` schema2の`current`が現在の候補と最新単位への入口。旧schema1フィールドと移行前JSONを履歴として保持。現行判定には必ず候補ID・対象機能・証拠・実行時点を添える。
3. `feature-map.csv` は40 PEの責務の入口として保持し、ソース・契約・証拠で機能単位の対応先と残責務を更新する。`implementation-status.*` は派生表示とし、固定文言による古い状態への上書きを防ぐ。既存の状態表やmanifestを旧Q0指示に従って作り直さない。
4. `product-host.md` と `work/analysis/<unit>/` は版別履歴。開始時に全履歴を読み直さず、状態から参照された最新単位と今回の機能契約だけを読む。内容が矛盾したらソース・保存生成物・一次runを照合する。
5. `docs/analysis/acceptance-status.json` と `regression-manifest.json` を正本とし、全8と登録native91/driver164を管理する。1056原始native91は53合格/37既知障害/失敗1、原始driver159は24合格/23障害/112未実行。追加5の同候補限定実行を別証拠で結び付ける。0735原始90/159と12補足、旧030413/210/160/0749等は版付き履歴に保持し、新候補へ転用しない。actual native保存/二終了/実PCMは各一次証拠で別判定する。
6. 同じ作業木を引き継ぎ、未コミット・未追跡ファイルとGit対象外の`work/`証拠を保持する。別checkout/ホストへ移す場合は、入力・録音・保存ソース・生成物・一次run・承認記録の保全とhash照合を行う。追跡文書だけでは移送・再現の完了にならない。

受入状態は「未着手・作業中・検証待ち・合格・失敗・障害あり」を使い、部分合格には対象を必須とする。最低限、候補/生成物hash、機能ID、入力/依存hash、コマンド、環境、結果、証拠パス、残差を記録する。古い成功は履歴として保持し、新候補の実行済み結果へ転用しない。

## 次に実施する作業と終了条件

ユーザーの計画メンテナンス依頼に従い、1713保存候補のbuild/installと関連13試験を終えた境界で中断。[Send既定Envの最新報告](analysis/q3-send-environmental-reverb-2026-10-09.md)とcurrent.independentInProgressUnitに修正・失敗履歴・再開条件を保存した。再開時は保存/作業ソースと実process状態を照合し、準備済みprotocolで本体Add Env/Send既定宛先/−600/UndoRedo/native保存/通常終了/別process復元/通常終了、固定WASAPIのPlay/発音中Stop/停止後無音/再開へ進む。ソース変更があれば新候補のbuild/関連回帰が先行する。0735限定採用は維持し、1713の13合格を全91/164や全体8へ転用しない。Style Legacy/複数Band、1056 Farm後続音/100BPM/混合/発音中停止、各Windows5保存、原版approval、Q2の別障害を保持する。以下のSDK条件分岐は再発時にも適用し、同条件拒否を再試行しない。

| 再開条件 | 次の具体的な作業 | 判定・終了条件 |
| --- | --- | --- |
| SDK拒否条件が変わらず、必要な環境判断も未回答 | 進行中[Waves Reverb](../work/analysis/q3-waves-reverb/20261007T102349728Z/unit-record.json)の記録済み範囲で、default効果追加/既存履歴/本体接続/型付きreadonly取得と必要fixture・回帰を準備する。製品不整合の独立した修正を続ける | 旧SDK拒否時の条件分岐を履歴として保持。現183は既定Wavesのbuild/native/main/実DSP/PCMを限定達成。原版比較は未確認。Send factoryやcustom parameter編集へ無制限に広げない |
| 宣言SDKの探索拒否条件の変更と、必要な実行承認が証拠で確認できた | その時点の作業木を新候補へ保存してbuild/install。Sequence/DLS/Script依存/Farm/AudioPath Send/FileOutput等の関連回帰を実行し、まずTempo本体表示/native保存/通常終了/別PID復元/通常終了を完結させる | 208保存ソースや旧EXEを現在の作業木ソースの実行結果にしない。新規拒否は記録し、別々の既知Windows5は各解除条件が満たされるまで凍結 |
| 新候補の関連回帰とTempoが成立 | Farm・Send・Wavesの未達終了条件を順に検証/修正し、機能群の節目で全登録群とQ1代表経路を一巡する | 各単位のnative/二プロセス正常終了/必要音声/原版比較を結ぶ。比較障害やruntime未実装が残る単位は未完を維持し、独立した次単位へ進む |
| Q2用の独立Windowsが用意された | 原版実行物・Producer COM登録・原版探索先なしで、ソースと宣言依存からbuild/install/Q1を実行 | Q2未用意の間は障害を維持。Q1成功や通常環境のロードhash一致0で代用しない |

未検証ソースが蓄積しているため、Wavesの記録済み範囲を超える追加は受入不足を減らす根拠と依存関係を先に示す。連続3単位の基準で優先順位を見直し、検証再開を先送りするための細分化を避ける。この条件分岐が再開順序であり、以下のQ0〜Q4は工程IDである。

| 順序 | 作業 | この単位の終了条件 |
| --- | --- | --- |
| Q0 | 修正済み契約を保護し、新候補の回帰へ反映 | DWORD許可値/16以上/4予約値/不正入力時のbytes・dirty・UndoRedo不変とSequence試験を保護。新候補の関連回帰と節目の全登録群を実行し、通常coreの既知拒否・真の失敗・未実行を区別。assert削除、旧16制限、manifestの作り直しをしない |
| Q1 | 修正後の候補を固定し、本体の代表経路を一巡 | 同じinstallから新規native project→Segment/Style/Band/DLS/AudioPath作成/読込→編集/UndoRedo→保存→通常終了→別プロセス再読込→Play/Stop/再開/テンポ音声を実行。Transport共通defaultと競合するembedded Segment優先を実GUI・再生で確認。各段階の証拠を同一シナリオへ結ぶ |
| Q2 | 原版なしの製品構成を早期に実証 | 原版実行物・Producer COM登録・原版探索先のない独立環境で、ソース/宣言依存だけのbuild/installとQ1代表経路を実行。OS/ランタイム/音源の由来を記録。環境を用意できない場合は障害として残し、Q3を進める |
| Q3 | 未着手責務を順に本体へ実装し、原版互換を確認 | 下記群を機能単位に分け、原版観測→契約→実装→本体保存/再起動→比較を完結させる。1群の限定成立を全対象完成とせず、残責務を保持。各群の節目で固定候補の回帰・統合を行う |
| Q4 | 全範囲の残差解消・配布・最終受入 | 40 PEの全責務が実装/代替/宣言依存へ対応し、未解決の機能不足・原版依存を解消。全登録試験と全体8受入を最終候補の同じ構成で一巡し、再現手順と証拠を提示 |

Q0 DWORD許可値/予約値/不正入力文書不変を維持。旧153候補Sequence26/Timeline93は合格、新160候補Sequence26/Timeline93合格。通常core等の個別保存拒否は凍結。旧PCMは履歴として保持し、新候補へ転用しない。原版比較/Q2/全40/全8未完。

Q2は独立Windows環境未用意で障害。原版の起動警告・window未取得の旧障害と、旧観測のProject操作可能/Script種別なし/Add-Ins空を履歴として保持し、現在のGUI状態と断定しない。原版StylePlayerの`Computer Use app approval timed out`は別障害。未解消拒否の同条件再試行や迂回を行わず、SDK・RIFF監査・録音だけで原版互換合格にしない。

Q1の代表経路は全体受入の予行であり、対象機能の縮小ではない。現在の不具合修正と統合確認を終えるまでは、live AudioPath切替や追加Recovery UI等の細部を無条件の次作業にしない。ただし、当該経路のデータ破壊・誤再生・所有権不良を防ぐ修正は先行する。

## Q3の残責務と暫定順序

40 PEは [modules.csv](analysis/modules.csv)、機能への対応は [feature-map.csv](analysis/feature-map.csv) で保持する。以下は優先順の仮説であり、依存調査の根拠を残して変更できる。各群の最初に原版help・登録情報・既存サンプルから必要接続を確認する。

進行中の限定範囲と残責務：

- **Tempo**：描画修正は現候補で関連Timeline93/Sequence26、本体label分離/native保存/別process復元/二正常exit0まで限定合格。保存bytes不変とProject FILETIMEだけの差を独立監査。Q1音声・原版Typography/全責務は未完。
- **Farm**：0735候補の正確なUnicode NAME/CATEGORY/class参照は関連回帰200/32、本体Script/Project保存・別PID復元・二通常exit0、六種SFXと発音中Stop/無音/再開/private close無音まで限定合格。原版19データと20alias fixture、23最終入力は不変。旧183のGUID/file、160無音失敗、176context失敗を保持。case/collision/ANSI、identity-less/cycles/config/graph/追加class/full KEEP、APFarm/Wave loop/end、Night/Bird音源・音高・100BPM/混入・Predawn/Dawn/End・原版比較は残る。
- **Send**：218実class/4B減衰/private宛先先行/PChannel0 global取得と追加/音量UIを採用。authoring43/runtime49 exit0、承認済みnative Project保存、別PID exact復元、4通常exit0、full native監査1/9を確認。dry-zero本体PCMは限定合格、dry-minus600 packet失敗、Waves前Stop残音不合格を保持。録音用Waves DMOのStop修正は030413で関連8/native89/driver152、承認済みnative保存/full監査/controls1/9・exact別PID復元を実行。現四条件の各二停止窓endpoint/taps RMS0・packet/clock/全結合合格。採用五process通常exit0。dry-zero比1.0、dry-minus600約0.5066、Waves前約0.5066、Waves後約0.08711。旧218証拠と二手順失敗を保持。外部Other寿命/既定Reverb/原版比較は残る。
- **Waves Reverb**：default追加/UI/型付き取得をcompile、文書19合格。固定200ms確認を準備時間に対応した有界待機へ修正し、実DSP取得/再読込/二buffer/Stopを含むruntime29合格。183本体追加/UndoRedo/native保存/別PID復元/四通常exit0、発音中defaults取得と2route drywet PCMを限定合格。原版/customは未確認。宣言OS dsdmo.dllとProducer原版依存を区別する。
- **その他**：StyleLibrary AASY/default ChordMap/複数Band、FileOutput未接続group/多重session/legacy ABI、Timeline/OLE/clipboard、配布・補助機能・外部ABI・原版比較を引き続き全40対象のqueueに保持する。

| 群 | 主な対象 | まず閉じる経路 |
| --- | --- | --- |
| A 既存本体の互換残差 | native Project、Tempo/TimeSig/Timeline、Sequence/MIDI、StyleRef/Band/Command | Q1で出た保存/再読込/編集差分、任意MIDI残責務（限定import成立、非対応形式を保持）、Runtime Save As standalone記憶/dialog初期値/path表現の残差（owned6種記憶とper-file folder保存復元は限定成立）。独自DMPJだけの成功をnative Project互換にしない |
| B 和声と生成 | Chord、ChordMapDesigner/Ref/Strip、SignPost | 原版最小文書→節点/イベント/参照編集→保存/別起動→Style/Segment再生への効果 |
| C 時間付きイベント | Marker、Lyric、Mute、SegmentRef | 共通Timelineに時刻/選択/CRUD/UndoRedoを接続し、文書復元と通知・再生効果を確認 |
| D メディアと音源 | Wave、DLS残責務、ADSREnvelope/RegionKeyboard/PanVol | Wave参照/位置/trim、音源・音色・演奏UIの必要操作を本体へ接続し、録音で確認 |
| E 拡張実行と参照 | ScriptDesigner/Strip、Container、ToolGraph、Param | 参照/埋込/alias、変数/routine、tool順序・parameterの契約と本体編集/保存/実行 |
| F 再生・出力の残責務 | Conductor、AudioPathDesigner、FileOutputDMO | live切替、未接続PChannelのsilent、多重session、全component audition、ファイル音声出力を原版契約と比較 |
| G 配布・補助機能 | Uninst、MFC42/MSVCRT/MSFLXGRD、Farm/StylePlayer | 必要な機能の実装/代替、使用ライブラリの由来・導入/解除・補助アプリとの対応を確定 |

既存DLS/envelope等の実装を台帳の「未着手」に従って作り直さない。コードと記録を照合し、実装済み部分と不足部分に分割する。OSランタイムを許可された外部依存として使うことと、Producer固有原版の責務を残すことを区別する。不要と判断した対象も勝手に台帳から削除せず、根拠付きの代替対応を残し、対象範囲そのものの縮小はユーザー判断にする。

## 各作業を全体完成へ結び付ける規則

- 作業開始時に「対象機能ID・閉じる受入の不足・変更範囲・契約根拠・必要試験・終了条件」を短く記録する。契約調査は次の実装に必要な疑問へ絞り、根拠なしの同一探索を繰り返さない。
- 通常は一つの機能単位を実装から本体受入まで進める。関連する記録更新は同じ単位に含める。UI/環境の障害は具体的に残し、独立した実装を妨げない。
- 一つの単位で新たに見つかった細部はqueueに登録し、終了条件を無制限に広げない。終了したら次の優先責務へ進む。全体の対象から外した扱いにはしない。
- 連続する3単位で同じ群だけを細分化し、受入の不足・未対応責務・原版依存のいずれも減っていなければ優先順位を再評価する。試験件数・画像数・コード行数だけを進捗にしない。
- main.cppやFrameworkへの追加は責務境界を明確にし、必要な部分を分離する。大規模な美化リファクタリングを機能受入より先にしない。
- 原則として契約→実装/修正→関連回帰→本体操作→native保存→正常終了→別process復元→必要音声→比較まで進める。非適用の段階は理由を記録する。状態整理・契約調査・判定器作成だけでは単位完了にしない。
- 作業終了・中断時は変更、候補、各終了条件の達否、実行結果、残責務・原版依存、障害解除条件、次の一手を単位へ保存する。完了単位とソース変更済み/開始のみの未完単位を`current`で分ける。Git対象外の証拠は主要hash・再現コマンド・保全先を追跡文書へ要約する。

## ビルド・回帰・互換性の検証方針

製品構成は `PRODUCER_BUILD_REFERENCE_TOOLS=OFF` のWin32 presetを使用する。比較用原版と製品の配置を分け、原版への暗黙fallbackを許さない。構成・compile・install・native・GUI・audio・全体受入を別判定にする。保存ソースからのビルド、入力、生成物、実行環境の同一性を記録する。

変更ごとは影響する単体/統合試験を実行する。通常coreや専用群を無条件に毎回すべてやり直す必要はない。ただしQ0、機能群の節目、Q4では固定候補で登録済み回帰を一巡する。manifestには試験ID、runner/専用引数、入力の生成/取得、原版要否、環境、対象責務、合格条件を記録する。必要入力がない試験は未実行/障害であり合格にしない。

同一生成物の成功証拠は、入力・依存・操作・判定器を含む適用条件が一致すると確認した場合に再利用できる。新しい生成物は新しい候補であり、旧版試験を現行の実行済み結果と表示しない。最終受入中に修正した場合は候補を更新し、影響する証拠と全体の統合一巡を更新する。保存入力の改変や判定器変更も版と影響を記録する。

原版比較では同じ入力・操作・依存版を使用し、構造/イベント/参照/設定/出力の意味と、説明可能な非決定的差分を比較する。独立RIFF監査は重要だが、原版観測の代わりではない。未観測の期待値を実装から逆算して仕様と呼ばない。未知chunkは保持し、保持だけを編集互換の完成にしない。

WASAPI自動録音を基本とし、人の聴取を待機条件にしない。[音声自動化契約](analysis/audio-automation-contract.md) に従い、製品/録音器/入力/音源/endpoint/操作時刻/packet/PCM/解析器を結合する。必要な発音中Stop・停止後無音・再開・テンポ・音高・混入を確認し、API成功だけで合格にしない。失敗録音・操作時刻不備・判定器誤指定を保存し、閾値を緩めて合格にしない。GUIとCLIは別経路として記録する。

原版依存の最終確認はhash一致0だけに頼らず、配置・COM・動的/遅延ロード・実ロード元・許可依存の由来を調べる。Q2で原版を配置しない独立環境を使い、通常環境の登録削除・セキュリティ設定変更を前提にしない。OS DirectMusic/DirectSound/GM.DLS等の依存は版・取得/導入条件を明示する。

## 全体完成時の受入試験

工程6の完了判定では、次の試験を同じ構成で一巡させる。対象の OS、ビルドツール、外部ランタイム、試験データと実行手順を記録し、必要なファイルを事前配置しただけの結果と、再現手順から構築した結果を区別する。

| 試験 | 入力と操作 | 合格条件と残す証拠 |
| --- | --- | --- |
| クリーンビルド | 新しい作業ディレクトリへソースと宣言した依存物だけを用意し、記載したコマンドを実行する | 過去の生成物や手修正済みバイナリなしで全対象をビルドできる。ツールの版、コマンド、ビルドログを保存する |
| 起動と終了 | 再構築した本体を起動し、新規プロジェクトを作成して終了する | 必要な編集画面が開き、正常終了する。生成された文書、ログ、使用した構成を保存する |
| 原版データの読込 | 形式ごとに選んだ原版のサンプルと、原版で作成した最小文書を開く | 宣言した対象機能のイベント・設定を欠落させず読み込む。入力のハッシュと比較結果を残す |
| 編集と保存 | イベントの追加・変更・削除、選択、コピー・貼付け、Undo・Redo を対象機能に応じて実行し保存する | 文書状態と保存内容が期待する操作結果に一致する。操作手順と操作前後のデータを残す |
| 終了後の再読込 | 保存後にアプリを終了し、再起動して文書を開く | メモリ上の状態に依存せず、編集結果が復元される。再保存時の差分は意味と理由を説明できる |
| 再生と停止 | 既知の音源と短い試験曲を使い、テンポ変更を含む文書を再生・停止する | WASAPIループバックWAVを自動解析し、入力に対応する発音・Stop後の減衰と無音・再開後の発音・テンポに対応する発音間隔を確認する。API成功だけでは合格とせず、音源・出力先・録音と判定・版を記録する。未対応の判定項目は未確認のまま残す |
| 繰り返しと異常入力 | 複数文書の開閉、空文書、境界値、破損入力を試す | 文書間の状態混入や正常文書の破壊がなく、失敗を扱える。原版との違いと残る制約を記録する |
| 原版依存の解消 | 実行時に必要な EXE・DLL・OCX と、その由来を照合する | Producer 固有の原版モジュールを必要としない。OS・外部ランタイムへの依存は導入手順とともに一覧化する |

各機能単位ではこの表の該当責務を検証し、限定ハーネスでは確認できない表示・操作・音声出力を本体試験へ持ち越したと明記する。該当範囲の部分合格は全体8項目の合格ではない。

## 障害と既存承認の扱い

OS拒否、SDK探索アクセス拒否、sandbox制限、共有違反、GUI観測エラー、製品assert、互換性不一致を区別する。2026-10-05レビューで確認した旧候補core6件目の失敗は実行済み試験の期待値不整合であり、現在は修正保護対象。SDK拒否原因を証拠なしに特定のsandbox/OS設定と断定しない。GUI観測エラーも原因の証拠なしにOS拒否や製品不具合と断定しない。

通常core atomic Chordmap Project、Script Reference.spp、Container埋込、ContainerProject.pro、Trigger.pro、RouteBand.bnpの各Windows5と、原版StylePlayer approval timeoutは個別の一次記録・解除条件を保持する。SDKが使えるだけでこれらが解除された扱いにはしない。失敗候補・ログ・入力・録音を保持し、拒否条件変更の証拠なしに同条件の起動・保存を再試行しない。

許可ゲートやOS拒否を別loader・別UI手段・登録削除・セキュリティ設定変更で迂回しない。既存のユーザー指示・承認を照合し、本当に新判断が必要な場合だけ対象・正確なエラー・試行済み内容・必要変更・具体的な判断を示す。GUIは利用するcomputer-useの復旧手順に従い、観測したPID/EXE/windowへ操作する。新規起動前に既存processを確認し、過去の画面・PID・古い変数を現在の証拠へ転用しない。

## 全体完了の判定

全対象の責務に未実装・未解決の互換不足・原版依存がなく、最終候補で上記8受入が合格したときだけ完了とする。中間経路の成功で `fullAcceptance` をtrueにしない。第三者がソース・宣言依存からbuild/install/実行/検証を再現できる手順と証拠を提示する。

次のタスクには [2026-10-07再開プロンプト](analysis/continuation-prompt-2026-10-07.md) を使う。[10月5日版](analysis/continuation-prompt-2026-10-05.md)は履歴として保持する。今回の計画レビューは製品実装・受入とは別であり、全体完成に数えない。

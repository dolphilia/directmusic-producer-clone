# DirectMusic Producer 解析と再構築の実施計画

現行入口（2026-11-10、休止解除済み）：063900475はSegment Info文字/VersionとTab操作の候補。親055654512のUnicode/CRLF/4Version/拒否/4桁上限/空Subject/owner切替/二UndoRedo/18native/作者8940normal0を保全。新候補はProperties cpp一source変更291同一、feature base041453112から5変更287同一、292saved-working/4outputs一致、関連7/1125（Info56model/native4starts）。作者20808で全8欄を視覚順にTab/逆順、Informationから離脱、Subject Tab確定/Undo、Information Enter改行/CtrlEnter/Undo、CtrlTab/逆巡回を確認。末尾と先頭間にはnativeページTabが入り、事前手順の省略を旧原文/実観測付きで訂正、製品/音声閾値変更なし。GDI三作成90→91、7native段階全期待バイト、Project FILETIMEのみ。作者20808/別newprefs復元14356はnormalexit0。別復元でUnicode/CRLF/Version1.2.3.4/Start1920/Infinite3072..6912/Tempo120を保持して再保存。 [Info記録](analysis/ui-segment-info-2026-11-10.md)。新member固定240秒録音はrecorder0・六実入力・Stop25.005/25.011秒・停止5225安定。ただしUTC/QPC drift8.5643msが既定5msを超え、positive監査が時刻gateでexit1、PCM19項目/負例7/fourclockは未成立。WAV626394e11e690d77fcd94d0a761e0cb84aff2c9d0e683988b6659cb49e6d3391と全時刻/packet/入力/監査sourceを保全。negativeは実行せず、同条件再録音凍結/閾値不変/旧PCM昇格なし。二つの無入力25秒試験25.010/25.009合格だけでは240秒のclock整合を保証しなかった。音声受入newPcm=false/boundedFinished=falseで保持。実runtimeでは二primary要求1920/二private/二Stopを確認し、録音後/通常終了後の全4nativeファイルとsource/output完全保持。 Infoの新PCMはclock不合格のまま保全し、条件が変わらない再録音をしない。独立作業としてSegment Boundary一次auditを継続し、根拠のあるdefault-resolution保存モデル/GUI/関連/本体保存通常終了/別復元/必要音声へ進む。Boundary原版静止図/CHM/SDK/4native sample auditはwork/analysis/ui-segment-boundary/20261110T064400000Z/primary-audit-v1.json。原版動的/全geometry/全40全8は未達、6working2blocked0pass/fullAcceptance=false。既知OS50native30driverや正式原版起動を同条件再試行しない。






























表示環境の現行条件：ユーザーが主ディスプレイと解像度を変更した。4K/200%は以前の条件で、変更後のOS倍率は未測定。入力前に返却windowと最新画像で座標を再取得し、厳密比較では双方のDPI/font/theme/client寸法を記録する。

登録検証追補：224147同候補一巡：native100＝51合格2990検査/48既知OS凍結/匿名Band1保留。driver172＝18合格/30凍結/124未実行。153入力（7ディレクトリを含む）・18 producing run・291保存/作業ソース・4出力を285検査で照合。新規失敗なし、全登録合格ではない。 [最新一巡](analysis/priority-regression-224147-2026-10-12.md)。fullAcceptance=false。

追加比較：151135・画像7/8の追加比較：正式原版と候補で初期Band選択/Down→Chord/Home→Band/Cancelが一致。候補空OKは異なる手順による単体確認。4トラック/8ノートと入力全バイト不変、11124/10296二通常exit0、289ソース/4出力一致。原版空選択を確立できず空OK未実行のため比較単位全体は未完。 [部分比較報告](analysis/ui-add-tracks-comparison-2026-10-12.md)。全体fullAcceptance=false。

改訂：2026-10-12（日本時間）。現行進行中単位：20261012T004853599Z Current Tempoの停止中Segment編集を実装。正式原版で132.25の保存/UndoRedo、spin上133・下131、bar2から既存time0 markerの編集を観測しnative7合格。新候補configure/build/install0、291保存/作業ソース・4出力一致、関連4件/439検査合格。segment-creationは既知Mixer.sgp Windows5のため未実行・障害維持。候補本体/native二process/音声/追加原版契約は未検証。ユーザー指示で休止。 [休止記録](analysis/ui-current-tempo-pause-2026-10-12.md)。fullAcceptance=false。

次単位：ユーザーが再開を指示するまで作業を進めない。再開後は004853の保存/作業291ソース・4出力・入力を照合し、現在の低解像度と最新windowを再観測。正式原版のEscape/focus-loss/不正値契約の未確認を保持し、004853本体を専用candidate Projectで初起動→132.25入力/Enter/UndoRedo/spin/文書所有権→native保存→通常終了→別process復元/通常終了→固定WASAPIテンポ/発音中Stop/無音/replay比較へ進む。保存済み235833空CC・224147一巡/CC15/16を新候補の実行証拠へ転用しない。49native/30driverの凍結、SSP04親/6640破棄回答待ち、独立Style015244/Q2/全40/全8を保持。

Sequenceの現行試験は[作業報告](analysis/ui-sequence-basic-2026-10-11.md)を参照。140539のLength履歴は[旧報告](analysis/ui-segment-length-2026-10-10.md)、130456の同入力原版比較は[最新報告](analysis/ui-segment-properties-2026-10-11.md)に分離する。Band131728等は候補別証拠として保持し、全操作・厳密外観は未達。

## 目標と現行の判断

Producer本体と必要なProducer固有機能をソースからビルドし、原版固有のEXE・DLL・OCXに依存せず、下記「全体完成時の受入試験」8項目をすべて満たす。原版と同じソース・同じバイナリ・同数のDLLの復元は要求しない。機能・文書互換性と必要な外部ABIの責務を維持し、対象範囲を独断で縮小しない。

原版の画面構成、配置、字体、色、アイコン、メニュー、キーボード・マウス操作、編集手順を可能な限り再現する。同じ編集結果に加え、原版に慣れた人が同じ入口と手順で操作できることを評価する。環境差による変更や再現できない部分は理由・影響・代替操作と根拠を記録し、未解決の差分・必要な比較未確認を完成扱いにしない。新規の全面刷新を目標にせず、既存の文書・再生実装を共通UIへ接続する。

本体を先行実装した方針は継続する。修正済みQ0と既存実装を保護し、ユーザーの2026-10-10追加指示により、ツールバー→Segment→Sequenceの原版操作経路を優先する。原版の画面・操作基準と共通UIを各段階で照合し、機能/外観/操作の不足を同時に解消する。未完Style等の証拠と残責務は保持するが、独立した旧音声検証を新優先経路の全着手の前提にはしない。限定DLL・大量プローブ・compile成功・一つの経路の受入だけで全体完成とはしない。

最新の優先順は [2026-10-10指定経路](analysis/priority-segment-sequence-2026-10-10.md)。全体方針は [2026-10-09計画改訂](analysis/plan-review-2026-10-09.md)、実行指示は [刷新した継続プロンプト](analysis/continuation-prompt-2026-10-10.md)。改訂前全文は [10月9日改訂前の履歴](analysis-plan-history-2026-10-09.md) に保存。SDK障害時の判断は [10月7日レビュー](analysis/plan-review-2026-10-07.md)、本体先行の根拠は [10月5日レビュー](analysis/plan-review-2026-10-05.md) と [10月2日レビュー](analysis/plan-review-2026-10-02.md) を必要な範囲で参照する。履歴の「次」は現在の指示ではない。

## 原版の優先参照先

今後の外観・操作・文書・再生比較は、ユーザーが正式にインストールした C:/Program Files (x86)/Microsoft DirectMusic Producer/DMUSProd.exe を優先する。5.3.0.900/x86、EXE SHA256 fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011。[参照先の確認と方針](analysis/original-reference-2026-10-10.md)に39ファイル/35PE・登録・processの一次証拠を保存した。35PEは既存コピーとhash一致するが、登録・探索先・設定を含む実行環境の同等性は別判定。全40責務は維持する。

再開時はインストール版のEXE/hash/PID/開始時刻/返却window/文書状態を新たに確認し、現行ディスプレイ設定を再観測し、同じ入力・状態でmain frame/メニュー/Project/Timeline/Properties/Transportの基準を観測する。旧コピーの観測は版付き履歴とし、新参照先を既定のコピー起動へ置き換えない。ユーザー文書を保護し、編集比較は検証用コピーで行う。

インストール先・登録・対象が変わったため、旧原版app approval timeoutやScript/Add-Ins不足を新対象に無条件適用せず、動的比較は新条件の検証待ちとする。個別Windows5/Clipboard5/凍結37件はケース別解除条件を維持する。新対象でも許可ゲートの迂回はしない。

現在のホストは原版インストール済み・一部COM登録あり。今後の製品runは環境/登録/探索先/実ロード元を記録し、旧インストール前runと分ける。製品の原版暗黙fallbackを確認し、Q2は原版PE・Producer COM登録・探索先のない別Windowsで行う。比較用原版の削除/登録解除で代用しない。原版参照先の変更だけで製品受入を昇格させない。実装の現在地は下記の最新試験とcurrentを参照する。

旧コピーはユーザー承認により実験・解析・分析・改造へ自由に利用できる。改造前のhash/バックアップと改造差分を保持し、改造後の動作を未改造原版の仕様根拠にしない。正式インストール版はオリジナルの動作確認用とし、インストールファイルを保護する。

プログラムからの起動APIは対象window未取得のまま[履歴](../work/analysis/original-installed-reference/20261010T025959223Z/startup-check-v1.json)へ保持。その後の手動起動済み正式原版では、本体撮影、Help/Contents操作、CHM閲覧を[限定確認](../work/analysis/original-manual-session/20261010T031624075Z/session-check.json)した。CHMは586 HTML/252 GIFを抽出・索引化済み。040727でPlay Cursorの表示追従/固定を部分比較し、045647で停止中viewportを静的比較した。同条件の全編集/再生比較と起動APIの成功は未確認であり、一般的な操作成功や全体互換へ拡張しない。

## ユーザー指定の優先実装

[ツールバーとSegmentとSequenceの詳細計画](analysis/priority-segment-sequence-2026-10-10.md)を、以下の一般的なQ3順序より優先する。画像1〜47の要求と受入確認、依存順SSP01〜SSP08、関連回帰/保存復元/音声/外観/操作の条件を明記した。[索引](analysis/priority-segment-sequence.json)とUI台帳の`UIxx.priority-NN`を照合する。

優先順は、3ツールバーとView/MIDI Options→New/Segment Length/Add New Tracks/Band/Tempo→Sequence基本と楽器→ノート端/移動/スナップ/velocity→Band pan/volume→PBend/Expression等CC→複数パート/ドラム/DLS→同候補の連続シナリオ。Segment Properties→LengthもSegment段階に含む。45番はTimeSigの誤撮影として除外し、44→46→47を採用する。

`screenshot/`はGitから除外し、原本はローカルに保持する。移送時はGit対象外`work/`とともに別途保全し、索引のhashを照合する。計画追加だけで製品実装済み/受入済みへ昇格させない。Standardの限定実装と検証は別の一次単位へ記録し、詳細計画と継続プロンプトは残る実装の入口とする。

## 現在の基準点

| 項目 | 確認済みの範囲 | 未完了・現在の判定 |
| --- | --- | --- |
| 候補と作業ソース | 限定採用0735。最新20261011T224147887Z：291保存/作業ソース・4出力一致。空DLS flags/native/DEFAULTGM有効PCMを限定確認 | 非空DLS既存契約保持。厳密外観/音色・全SSP/全体未完。224147全一巡を実施、障害/未実行は残る |
| Send既定Envの専用検証 | 1713関連native13合格、authoring57/runtime89。実global DMO取得/共有Stop/再開/Flush、独立SDKの32768-byte impulseと完全一致 | ユーザー指示で再開。GUI挿入/既定宛先/−600/UndoRedo/native保存/別process復元/二通常exit0成立。WASAPI9/10、初回onset1.013秒>固定1秒で全体不合格。原版本体approval障害。native91/driver164全一巡は未達 |
| native原始一巡 | 最新224147：100モード51合格2990検査/48OS凍結/匿名Band1保留 | 新規失敗なし。凍結/未実行は未達 |
| driver原始一巡 | 最新224147：172 driver18合格/30凍結/124未実行 | 18 producing run同候補照合。全登録合格ではない |
| 現本体/native | Farm author8660がScript/Segment/Project保存後通常exit0、別PID15508で復元/初期化し通常exit0。23入力は保存コピーと終了後hash一致 | Scriptのactive NAME10/CATEGORY5、owner10を独立監査。case/collision/ANSI/config/graph/full KEEP等は残る |
| 現音声 | 同じreloadPIDで六種SFX音源/音高、Night発音中Stop/無音/再開、private close無音を固定WASAPIで限定確認 | 初回120秒録音の時間外操作失敗を保持。Night/Bird音源/100BPM/一般混合/Predawn/Dawn/End未確認 |
| 現Q1統合 | 0735で新規native Project六形式10文書・五形式編集/history保存・author22256/reload21004二通常exit0・Style/Transport録音・6+11対照が限定合格 | 29 settled観測と12最終入力を結合監査。全責務・原版比較・Q2は未達。030413 Q1/Sendは版付き履歴 |
| 原版依存と全体 | 宣言OS依存と全40責務を保持。受入8は6作業中/2障害/0合格、fullAcceptance=false | 原版approval、Q2独立Windows、全責務・同最終構成の全8は未達 |
| 外観・操作互換 | 旧Tempo限定ホストで画像66枚等の一致記録あり。現在の本体には編集・保存・再生の限定操作証拠がある | 原版本体全画面の外観・操作手順比較は未完。旧原版Timelineを使用した一致を自作本体全体へ転用しない。比較台帳15群を設定し、全40対象を対応付け |

限定採用候補は[0735 build](../work/build/product-snapshot/20261008T073500338Z/build-summary.json)。最新の限定完了への入口は[現Q1報告](analysis/q1-current-0735-2026-10-08.md)、[結合監査](../work/analysis/q1-current-0735/20261008T084556994Z/scenario-proof.json)、[採用記録](../work/analysis/q1-current-0735/20261008T084556994Z/adoption.json)。[Farm参照報告](analysis/q3-farm-descriptor-2026-10-08.md)も同候補の限定成立として保持。Legacy AASY/PERはOS Style Save E_NOTIMPL、複数Bandは返却Band ID/NAME無効・Save E_NOTIMPLで未完。[公開API限界](analysis/q3-style-player-multiple-band-2026-10-08.md)を保持する。1056の[Farm音声単位](analysis/q3-farm-background-audio-2026-10-08.md)は二通常exit0・23入力不変とnative/main先頭音の限定PCM照合、独立入力との13実ノート一致まで進んだ。明示22050Hz/Echoを確認したが、全PCM源・Night・100BPM・混合・main発音中Bird停止は未達。同条件失敗や旧Q1を転用せず、追加診断と二回の遅延Stopでも不足は減らず、未完として保存し、Send既定Reverb宛先へ優先順位を移した。1713の既定Envは再開後に本体/native二通常終了まで限定確認した。固定WASAPIで両経路の音高・能動Stop/無音/replayは成立したが初回onsetが1秒を約13ms超過し、音声全体不合格を保持する。原版本体の画面取得はapp approval timeoutで障害あり。同条件再試行をせず、Help由来の基準とProject Treeの共通UI単位へ進む。[再開後の一次記録](../work/analysis/q3-send-environmental-reverb/20261008T154611123Z/main-20261008T180824898Z/unit-record.json)に候補/操作/native/PCM/依存/失敗を保存。旧[Q1](analysis/q1-current-candidate-2026-10-08.md)と[Send](analysis/q3-audiopath-send-2026-10-08.md)、Farm183/Waves183/Style210は候補別履歴として保持する。

## 記録の入口と更新規則

1. 本書は目標・受入条件・現行queueだけを管理する。時系列の作業報告や毎回の「次」を追記しない。本文はおおむね250行以内を維持し、古い判断は履歴へ移す。
2. `docs/analysis/product-state.json` schema2の`current`が現在の候補と最新単位への入口。旧schema1フィールドと移行前JSONを履歴として保持。現行判定には必ず候補ID・対象機能・証拠・実行時点を添える。
3. `feature-map.csv` は40 PEの責務の入口として保持し、ソース・契約・証拠で機能単位の対応先と残責務を更新する。`implementation-status.*` は派生表示とし、固定文言による古い状態への上書きを防ぐ。既存の状態表やmanifestを旧Q0指示に従って作り直さない。
4. `product-host.md` と `work/analysis/<unit>/` は版別履歴。開始時に全履歴を読み直さず、状態から参照された最新単位と今回の機能契約だけを読む。内容が矛盾したらソース・保存生成物・一次runを照合する。
5. `docs/analysis/acceptance-status.json` と `regression-manifest.json` を正本とし、全8と現登録native100/driver172を管理する。Waves persistenceを追加し、0127関連19は15合格/4凍結。2137の原始92一巡は歴史的構成の結果として別管理する。共通Properties単位でcomponent-nameを追加。1056原始native91は53合格/37既知障害/失敗1、原始driver159は24合格/23障害/112未実行。新登録を旧一巡へ加算しない。追加5の同候補限定実行を別証拠で結び付ける。0735原始90/159と12補足、旧030413/210/160/0749等は版付き履歴に保持し、新候補へ転用しない。actual native保存/二終了/実PCMは各一次証拠で別判定する。
6. 同じ作業木を引き継ぎ、未コミット・未追跡ファイルとGit対象外の`work/`証拠を保持する。別checkout/ホストへ移す場合は、入力・録音・保存ソース・生成物・一次run・承認記録の保全とhash照合を行う。追跡文書だけでは移送・再現の完了にならない。
7. 外観・操作の正本は [ui-compatibility.json](analysis/ui-compatibility.json)、記録・判定の規則は [UI互換契約](analysis/ui-compatibility.md)。機能の現行判定は`current`・受入JSON・最新単位から参照し、UI台帳へ古い機能状態を複製しない。全40対象を画面・操作群へ対応付け、UIが非適用の責務も理由と根拠を残す。

受入状態は「未着手・作業中・検証待ち・合格・失敗・障害あり」を使い、部分合格には対象を必須とする。最低限、候補/生成物hash、機能ID、入力/依存hash、コマンド、環境、結果、証拠パス、残差を記録する。古い成功は履歴として保持し、新候補の実行済み結果へ転用しない。

## 外観と操作互換の基準

UI台帳では画面・機能ごとに、原版基準、対象候補、機能証拠の入口、外観状態、操作状態、本体保存・別起動復元・必要音声の証拠、差分、障害、次に閉じる不足を管理する。比較基準の設定や文書更新だけで製品の状態を昇格させない。全画面の列挙は再開後に原版help・登録情報・既存観測と照合し、15群を具体的な画面・操作へ分割する。

| 評価軸 | 主な比較対象 | 終了条件 |
| --- | --- | --- |
| 機能・文書互換 | イベント・設定・参照・通知・保存・復元・再生・外部ABI | 該当する既存8受入、最新単位と原版比較が成立し、残責務がない |
| 外観 | 主画面、文書切替、編集領域、タイムライン、プロパティ、メニュー、字体・色・アイコン、選択/dirty/再生/無効/エラー表示 | 同じ入力・状態で原版と比較し、各要素の差分が解決済みか、根拠付きの環境差として説明できる |
| 操作互換 | 操作入口・手順、キー、マウス、選択範囲、ドラッグ、右メニュー、フォーカス、clipboard、UndoRedo、dialog初期値/取消し | 原版で確認した入口・手順・状態遷移が本体で成立し、操作結果と保存/復元が対応する |

比較はOS、DPI、拡大率、フォント、テーマ、ウィンドウ・編集領域寸法、入力hash、選択・カーソル・スクロール状態を記録して行う。動的表示やOS描画差は根拠を先に定義して説明し、配置や操作差を隠す画像除外・無根拠の許容幅を使わない。静止画像だけでキー/ドラッグの挙動を確定せず、操作列と前後状態も残す。未観測の項目は比較未確認、許可障害は障害ありとして保持する。

全40対象について機能・外観・操作の該当範囲を判定する。ランタイム/ABI等でUIが非適用の場合は責務ごとに根拠を明記する。画像数、試験数、コード量を完成率にせず、未解決責務・比較未確認・本体未検証・UI差分が減ったかで進捗を評価する。UI互換は既存8受入に対する横断条件であり、第9の試験や対象縮小として扱わない。

## 次に実施する作業と終了条件

製品実装はユーザー指示で再開済み。[2015単位](../work/analysis/ui-tree-enter/20261008T201444998Z/unit-record.json)では選択component Enterの不整合を修正し、矢印/Tab、Tempo6→7のUndoRedo/native保存、別processで7/7.5の復元、三通常exit0を確認。native7と固定WASAPI10は合格し、二経路の能動Stop/無音/replay・録音中closeまで結んだ。旧1922の時計ずれ/時間外close、1713 onset超過、原版比較障害を保存し、成功を転用しない。旧候補が混在した台帳入口は[訂正記録](../work/analysis/ui-tree-enter/20261008T201444998Z/metadata-correction.json)へ保全して修正。0735限定採用と旧対象の原版approval履歴、Q2/個別Windows5障害を維持する。

再開時の前処理として、新しい原版インストール/登録/探索先と候補の実ロード元を記録し、旧runとの環境差を固定する。製品PCMと原版音声観測は逐次実行する。

| 順序と条件 | 次の具体的な作業 | 判定・終了条件 |
| --- | --- | --- |
| 1 指定経路 SSP01〜SSP02 | 140539 Lengthと131728 Bandの限定成立を保全。Length末尾データ切詰め/全Properties、Band原版/外観比較、SSP01全MIDI/録音/外観と015244 Style未完を保持 | [詳細計画](analysis/priority-segment-sequence-2026-10-10.md)の番号別条件で機能/外観/操作を検証。旧候補の成立を新候補へ転用せず、必要な残件を各段階で閉じる |
| 2 指定経路 SSP01〜SSP08 | Band live-gesture-followup/main-repeatの固定300秒プロトコル・19条件を読み、新しい検証コピーと作者processで初回発音、pan/volume対照、再生中Undo/Redo、停止/再開と最終無音を同じ基準で再検証する。入力時刻を前半へ配置し、native保存/別process復元/二通常exitを確認。旧1.15秒失敗、Count-In15/16（操作窓80.906秒>80秒）、Echo停止中/実機/Advanced/録音、SSP04全gesture/原版比較、Style015244/Q2/OS凍結を保持。 | [ベロシティ再検証単位](../work/analysis/ui-velocity-revalidation/20261011T090428025Z/unit-record-v1.json)の既存4段階300秒PCM12/12を保持。全SSP04 gestureの完了とはせず、Band19条件/300秒を同基準で確認 |
| 3 保持する未完と他機能群 | 015244 Styleの固定300秒PCM/同候補一巡を未完として保持。指定経路の次にQ3 A〜Gの残責務を継続。直接依存する不具合は先に修正 | 旧Styleのbuild/関連15/保存復元は適用条件一致時のみ活用。録音不成立/原版未比較/凍結を保存。新候補へ旧結果を転用せず、全40対象を維持 |
| 4 全範囲と最終受入 | 全40責務とUI台帳の残差を解消し、最終候補の全登録群・全8受入・配布再現を実行 | 同じ最終構成で該当する機能/外観/操作がすべて合格し、必要比較未確認・原版依存・未実行がない |
| 並行 Q2環境が用意された | 原版実行物・Producer COM登録・原版探索先なしで、ソース/宣言依存からbuild/install/Q1を実行 | Q2未用意は障害を維持。通常環境のロードhash一致0で代用しない。UI資産・フォント等の由来も確認 |
| SDK探索拒否が再発 | 一次エラーと宣言SDK/実行環境/既存承認を照合し、解除条件を特定する。障害に依存しないUI基準整理・独立した修正を進める | MSB4184と拒否先Microsoft SDKsを記録。同条件再試行なし。宣言SDK固定での既存成功や完了済みTempo/Wavesを開始前の状態へ戻さない |

新しい細部の追加は、どの受入不足・責務・UI差分を減らすかと依存関係を示して選ぶ。連続3単位の基準で優先順位を見直し、指定経路やUI再現を先送りする細分化を避ける。実障害では未達条件を保持して独立した指定経路の作業へ進む。番号付き画像・ユーザー報告と正式原版/CHMを各段階で照合する。上表が再開順序であり、以下のQ0〜Q4は工程IDである。

| 順序 | 作業 | この単位の終了条件 |
| --- | --- | --- |
| Q0 | 修正済み契約を保護し、新候補の回帰へ反映 | DWORD許可値/16以上/4予約値/不正入力時のbytes・dirty・UndoRedo不変とSequence試験を保護。新候補の関連回帰と節目の全登録群を実行し、通常coreの既知拒否・真の失敗・未実行を区別。assert削除、旧16制限、manifestの作り直しをしない |
| Q1 | 修正後の候補を固定し、本体の代表経路を一巡 | 同じinstallから新規native project→Segment/Style/Band/DLS/AudioPath作成/読込→編集/UndoRedo→保存→通常終了→別プロセス再読込→Play/Stop/再開/テンポ音声を実行。Transport共通defaultとembedded Segment優先を実GUI・再生で確認。原版の画面状態・操作列との比較を同一シナリオへ結ぶ。比較未確認は限定成立に留める |
| Q2 | 原版なしの製品構成を早期に実証 | 原版実行物・Producer COM登録・原版探索先のない独立環境で、ソース/宣言依存だけのbuild/installとQ1代表経路を実行。OS/ランタイム/音源の由来を記録。環境を用意できない場合は障害として残し、Q3を進める |
| Q3 | 機能と共通UIを並行して仕上げる | 下記群を機能・画面単位に分け、原版観測→契約→実装→本体操作/保存/別起動→機能/外観/操作比較を完結させる。1群の限定成立を全対象完成とせず、残責務を保持。各群の節目で固定候補の回帰・統合を行う |
| Q4 | 全範囲の残差解消・配布・最終受入 | 40 PEの全責務が実装/代替/宣言依存へ対応し、未解決の機能不足・UI差分・原版依存を解消。全登録試験とUI横断条件を含む全体8受入を最終候補の同じ構成で一巡し、再現手順と証拠を提示 |

Q0 DWORD許可値/予約値/不正入力文書不変を維持。旧153候補Sequence26/Timeline93は合格、新160候補Sequence26/Timeline93合格。通常core等の個別保存拒否は凍結。旧PCMは履歴として保持し、新候補へ転用しない。原版比較/Q2/全40/全8未完。

Q2は独立Windows環境未用意で障害。原版の起動警告・window未取得の旧障害と、旧観測のProject操作可能/Script種別なし/Add-Ins空を履歴として保持し、現在のGUI状態と断定しない。原版StylePlayerの`Computer Use app approval timed out`は旧対象の別障害として保持する。正式インストール版は手動起動後の本体撮影/Help操作を限定確認済み、編集/再生の比較は未確認。個別拒否はケース別の解除条件を確認し、許可ゲートの迂回やSDK・RIFF監査・録音だけによる原版互換合格は行わない。

Q1の代表経路は全体受入の予行であり、対象機能の縮小ではない。現在の不具合修正と統合確認を終えるまでは、live AudioPath切替や追加Recovery UI等の細部を無条件の次作業にしない。ただし、当該経路のデータ破壊・誤再生・所有権不良を防ぐ修正は先行する。

## Q3の残責務と暫定順序

40 PEは [modules.csv](analysis/modules.csv)、機能への対応は [feature-map.csv](analysis/feature-map.csv) で保持する。以下は優先順の仮説であり、依存調査の根拠を残して変更できる。各群の最初に原版help・登録情報・既存サンプルから必要接続を確認する。

A〜Fの各群はUI台帳の該当画面・操作へ対応付け、機能結果と原版の操作入口/表示状態を同じ単位で確認する。Gの補助アプリ・コントロールも必要UIの再現を扱い、CRT/ABI等の非適用は根拠を記録する。機能の全実装後まで外観・操作の比較を延期しない。

進行中の限定範囲と残責務：

- **Tempo**：描画修正は現候補で関連Timeline93/Sequence26、本体label分離/native保存/別process復元/二正常exit0まで限定合格。保存bytes不変とProject FILETIMEだけの差を独立監査。Q1音声・原版Typography/全責務は未完。
- **Farm**：0735候補の正確なUnicode NAME/CATEGORY/class参照は関連回帰200/32、本体Script/Project保存・別PID復元・二通常exit0、六種SFXと発音中Stop/無音/再開/private close無音まで限定合格。原版19データと20alias fixture、23最終入力は不変。旧183のGUID/file、160無音失敗、176context失敗を保持。case/collision/ANSI、identity-less/cycles/config/graph/追加class/full KEEP、APFarm/Wave loop/end、Night/Bird音源・音高・100BPM/混入・Predawn/Dawn/End・原版比較は残る。
- **Send**：218実class/4B減衰/private宛先先行/PChannel0 global取得と追加/音量UIを採用。authoring43/runtime49 exit0、承認済みnative Project保存、別PID exact復元、4通常exit0、full native監査1/9を確認。dry-zero本体PCMは限定合格、dry-minus600 packet失敗、Waves前Stop残音不合格を保持。録音用Waves DMOのStop修正は030413で関連8/native89/driver152、承認済みnative保存/full監査/controls1/9・exact別PID復元を実行。現四条件の各二停止窓endpoint/taps RMS0・packet/clock/全結合合格。採用五process通常exit0。dry-zero比1.0、dry-minus600約0.5066、Waves前約0.5066、Waves後約0.08711。旧218証拠と二手順失敗を保持。外部Other寿命/既定Reverb/原版比較は残る。
- **Waves Reverb**：0127で4値編集/cancel/invalid/UndoRedo/native3/別process復元を限定確認。0239の通常Stop/初回runtime準備修正は関連33合格/4凍結、runtime49、本体の標準/custom値復元6/二通常exit0、8音/120180/発音中Stop/無音/replay/設定差の固定WASAPIと故障対照5が限定成立。新候補へ0127編集保存成功は転用しない。原版Effects List/property page/InstanceName/software flag/配置の再現・動的比較、宣言OS DSPと原版等価は未完。
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

- 作業開始時に「対象機能ID/UI台帳ID・閉じる機能/外観/操作の不足・変更範囲・原版基準/契約根拠・必要試験・終了条件」を短く記録する。契約調査は次の実装に必要な疑問へ絞り、根拠なしの同一探索を繰り返さない。
- 通常は一つの機能単位を実装から本体受入まで進める。関連する記録更新は同じ単位に含める。UI/環境の障害は具体的に残し、独立した実装を妨げない。
- 一つの単位で新たに見つかった細部はqueueに登録し、終了条件を無制限に広げない。終了したら次の優先責務へ進む。全体の対象から外した扱いにはしない。
- 連続する3単位で同じ群だけを細分化し、受入不足・未対応責務・UI差分/比較未確認・原版依存のいずれも減っていなければ優先順位を再評価する。試験件数・画像数・コード行数だけを進捗にしない。
- main.cppやFrameworkへの追加は責務境界を明確にし、必要な部分を分離する。大規模な美化リファクタリングを機能受入より先にしない。
- 原則として原版基準/契約→実装/修正→関連回帰→本体操作と外観確認→native保存→正常終了→別process復元→必要音声→機能/外観/操作比較まで進める。非適用は責務と理由を記録する。比較障害は未完のまま残し、独立した次単位を進める。状態整理・契約調査・判定器作成だけでは単位完了にしない。
- 作業終了・中断時は変更、候補、各終了条件の達否、実行結果、残責務・原版依存、障害解除条件、次の一手を単位へ保存する。完了単位とソース変更済み/開始のみの未完単位を`current`で分ける。Git対象外の証拠は主要hash・再現コマンド・保全先を追跡文書へ要約する。

## ビルド・回帰・互換性の検証方針

製品構成は `PRODUCER_BUILD_REFERENCE_TOOLS=OFF` のWin32 presetを使用する。比較用原版と製品の配置を分け、原版への暗黙fallbackを許さない。構成・compile・install・native・GUI・audio・全体受入を別判定にする。保存ソースからのビルド、入力、生成物、実行環境の同一性を記録する。

変更ごとは影響する単体/統合試験を実行する。通常coreや専用群を無条件に毎回すべてやり直す必要はない。ただしQ0、機能群の節目、Q4では固定候補で登録済み回帰を一巡する。manifestには試験ID、runner/専用引数、入力の生成/取得、原版要否、環境、対象責務、合格条件を記録する。必要入力がない試験は未実行/障害であり合格にしない。

同一生成物の成功証拠は、入力・依存・操作・判定器を含む適用条件が一致すると確認した場合に再利用できる。新しい生成物は新しい候補であり、旧版試験を現行の実行済み結果と表示しない。最終受入中に修正した場合は候補を更新し、影響する証拠と全体の統合一巡を更新する。保存入力の改変や判定器変更も版と影響を記録する。

原版比較では同じ入力・操作・依存版を使用し、構造/イベント/参照/設定/出力の意味と、説明可能な非決定的差分を比較する。独立RIFF監査は重要だが、原版観測の代わりではない。未観測の期待値を実装から逆算して仕様と呼ばない。未知chunkは保持し、保持だけを編集互換の完成にしない。

UIの比較は[UI互換契約](analysis/ui-compatibility.md)に従う。旧Tempoの画像一致は原版Timelineを含む限定構成の履歴であり、自作共通UIの合格ではない。共通UIを変更した候補では、影響する文書・選択・履歴・保存復元・再生操作を本体で再確認する。実装内部の診断情報は通常の製品操作へ混在させず、原版の画面・手順に対応する入口を設ける。

WASAPI自動録音を基本とし、人の聴取を待機条件にしない。[音声自動化契約](analysis/audio-automation-contract.md) に従い、製品/録音器/入力/音源/endpoint/操作時刻/packet/PCM/解析器を結合する。必要な発音中Stop・停止後無音・再開・テンポ・音高・混入を確認し、API成功だけで合格にしない。失敗録音・操作時刻不備・判定器誤指定を保存し、閾値を緩めて合格にしない。GUIとCLIは別経路として記録する。

原版依存の最終確認はhash一致0だけに頼らず、配置・COM・動的/遅延ロード・実ロード元・許可依存の由来を調べる。Q2で原版を配置しない独立環境を使い、通常環境の登録削除・セキュリティ設定変更を前提にしない。OS DirectMusic/DirectSound/GM.DLS等の依存は版・取得/導入条件を明示する。

## 全体完成時の受入試験

工程6の完了判定では、次の試験を同じ構成で一巡させる。対象の OS、ビルドツール、外部ランタイム、試験データと実行手順を記録し、必要なファイルを事前配置しただけの結果と、再現手順から構築した結果を区別する。

| 試験 | 入力と操作 | 合格条件と残す証拠 |
| --- | --- | --- |
| クリーンビルド | 新しい作業ディレクトリへソースと宣言した依存物だけを用意し、記載したコマンドを実行する | 過去の生成物や手修正済みバイナリなしで全対象をビルドできる。ツールの版、コマンド、ビルドログを保存する |
| 起動と終了 | 再構築した本体を起動し、新規プロジェクトを作成して終了する | 必要画面・メニュー・文書管理の構成と起動/終了の手順を原版と比較し、正常終了する。生成文書・画面・操作列・構成を保存する |
| 原版データの読込 | 形式ごとに原版のサンプルと最小文書を開く | イベント・設定を欠落させず読み込み、対応する編集画面・プロパティ表示も原版と比較する。入力hashと比較結果を残す |
| 編集と保存 | 追加・変更・削除、選択、clipboard、UndoRedoを原版の入口・キー/マウス操作で実行し保存する | 文書状態・保存内容・選択/dirty/無効表示が一致し、メニュー、ドラッグ、右クリック、フォーカス、dialog初期値/取消しの該当操作が成立する。操作列・前後画面・データを残す |
| 終了後の再読込 | 保存後に終了し、別processで文書を開く | 編集結果が復元され、原版で確認した表示・文書切替・選択/フォーカス等の復元方針と一致する。原版が保持しない状態を独自に復元する要求にはしない。再保存差分を説明する |
| 再生と停止 | 原版に対応するTransport入口で既知音源・短い曲を再生/停止/再開し、テンポを変更する | WASAPIで発音・発音中Stop/減衰/無音・再開・テンポ間隔を確認し、操作入口と再生/停止/無効表示も原版と比較する。API成功だけで合格とせず、音源・endpoint・録音・操作時刻・画面・判定・版を残す |
| 繰り返しと異常入力 | 複数文書の開閉、空文書、境界値、破損入力、取消しを試す | 状態混入・文書破壊がなく、原版で確認したエラー/確認/取消し・フォーカス遷移を扱える。原版との差と制約を残す |
| 原版依存の解消 | 実行時に必要な EXE・DLL・OCX と、その由来を照合する | Producer 固有の原版モジュールを必要としない。OS・外部ランタイムへの依存は導入手順とともに一覧化する |

各機能単位ではこの表の該当責務を検証し、限定ハーネスでは確認できない表示・操作・音声出力を本体試験へ持ち越したと明記する。該当範囲の部分合格は全体8項目の合格ではない。

UI互換の横断条件は`acceptance-status.json.uiCompatibility`とUI台帳に結ぶ。全40対象の必要画面・操作を同じ最終候補で確認し、未解決UI差分・必要比較未確認が残る間は該当受入を合格にしない。クリーンビルド/原版依存では、UI資産・フォント・コントロールの取得と由来も宣言する。

## 障害と既存承認の扱い

OS拒否、SDK探索アクセス拒否、sandbox制限、共有違反、GUI観測エラー、製品assert、互換性不一致を区別する。2026-10-05レビューで確認した旧候補core6件目の失敗は実行済み試験の期待値不整合であり、現在は修正保護対象。SDK拒否原因を証拠なしに特定のsandbox/OS設定と断定しない。GUI観測エラーも原因の証拠なしにOS拒否や製品不具合と断定しない。

通常core atomic Chordmap Project、Script Reference.spp、Container埋込、ContainerProject.pro、Trigger.pro、RouteBand.bnpの各Windows5と、旧対象の原版StylePlayer approval timeoutは個別の一次記録・解除条件を保持する。SDKが使えるだけでこれらが解除された扱いにはしない。失敗候補・ログ・入力・録音を保持し、拒否条件変更の証拠なしに同条件の起動・保存を再試行しない。

許可ゲートやOS拒否を別loader・別UI手段・登録削除・セキュリティ設定変更で迂回しない。既存のユーザー指示・承認を照合し、本当に新判断が必要な場合だけ対象・正確なエラー・試行済み内容・必要変更・具体的な判断を示す。GUIは利用するcomputer-useの復旧手順に従い、観測したPID/EXE/windowへ操作する。新規起動前に既存processを確認し、過去の画面・PID・古い変数を現在の証拠へ転用しない。

## 全体完了の判定

全40対象の責務に未実装・未解決の機能/文書/外観/操作互換不足・必要比較未確認・原版依存がなく、同じ最終候補でUI横断条件を含む上記8受入が合格したときだけ完了とする。責務ごとのUI非適用・環境差は根拠を確認し、機能や操作の未実装を例外へ移さない。中間経路の成功で`fullAcceptance`をtrueにしない。第三者がソース・宣言依存からbuild/install/実行/検証を再現できる手順と証拠を提示する。

継続実装には [2026-10-10継続プロンプト](analysis/continuation-prompt-2026-10-10.md) を使う。[10月7日版](analysis/continuation-prompt-2026-10-07.md)と[10月5日版](analysis/continuation-prompt-2026-10-05.md)は履歴として保持する。計画改訂と再開記録は製品受入とは別であり、候補・全8状態を維持する。停止記録は履歴へ保存した。

現行のWave項目loopは [1825報告](analysis/ui-wave-item-loop-2026-10-10.md) と `current.waveItemLoopIntegration` を入口とする。1734/1446等の失敗と1306/1208の限定成立は版付き履歴へ保持する。旧対象の原版比較障害は履歴として引き継ぎ、正式インストール版の比較は検証待ちとする。1915の入口限定成立とdirectchild残差を保持し、TimeSigの限定成立はcurrent.timesigPropertiesIntegrationに保存済み。015244 Style開始待ちの固定PCM・同候補一巡は未完として保持。当時の優先作業はSSP03/04の初回発音失敗と原版/全操作残件を保持して画像25〜28のSSP05 Band mixerへ進むことであり（現行queueは上表）、旧Styleを完了扱いにしたり、独立した未完を全着手の前提にしたりしない。



## SSP07の限定成立とSSP08の入口

220927限定成立：276保存/作業ソース・4出力一致、関連13/952。本体Add New Partで空2/10パート保存、DLS/ドラムノートと複数roll表示、native11+14、作者2932/別復元22852二通常exit0。固定PCM18/18、原始native98=51合格2903検査/46OS凍結/既知Band1保留、driver170=20合格/28OS凍結/122未実行。原版互換/厳密外観/全操作は未達。 [複数パート報告](analysis/ui-sequence-parts-2026-10-11.md)と `work/analysis/ui-sequence-parts/20261010T215317400Z/unit-record-v2.json` を照合する。SSP06旧PCM14/16とStop2時刻不備は保持し、今回の18/18へ転用しない。SSP01〜07・独立Style015244・46native/28driver OS凍結・Q2の残責務を保持してSSP08へ進む。当時の次作業はSSP08開始記録→同候補で新規Segmentからの連続本体シナリオだった。現在のSSP08限定結果は優先詳細計画を参照し、現行queueへ戻る。SSP01〜07残差を列挙し、再現可能な実装不整合を修正、native保存・二通常終了・別process復元・固定PCMへ接続。原版比較/厳密外観は未確認で保持。 限定採用0735、全体6作業中/2障害/0合格、fullAcceptance=false。


直前の限定検証完了: [ベロシティ再検証単位](../work/analysis/ui-velocity-revalidation/20261011T090428025Z/unit-record-v1.json)。080641の追加ソース変更なし、既存4段階300秒PCM12/12・Undo80・保存/別復元/二通常exit0。旧1.215秒/1.15秒・baseline1.13秒と失敗した録音手順を保持。次はBand固定300秒・19条件。

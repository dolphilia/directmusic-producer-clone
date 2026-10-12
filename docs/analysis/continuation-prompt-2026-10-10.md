# DirectMusic Producer 次タスク用の継続実行プロンプト

表示環境の現行条件：ユーザーが主ディスプレイと解像度を変更した。4K/200%は以前の条件で、変更後のOS倍率は未測定。入力前に返却windowと最新画像で座標を再取得し、厳密比較では双方のDPI/font/theme/client寸法を記録する。

登録検証追補：224147同候補一巡：native100＝51合格2990検査/48既知OS凍結/匿名Band1保留。driver172＝18合格/30凍結/124未実行。153入力（7ディレクトリを含む）・18 producing run・291保存/作業ソース・4出力を285検査で照合。新規失敗なし、全登録合格ではない。 [最新一巡](priority-regression-224147-2026-10-12.md)。fullAcceptance=false。

追加比較：151135・画像7/8の追加比較：正式原版と候補で初期Band選択/Down→Chord/Home→Band/Cancelが一致。候補空OKは異なる手順による単体確認。4トラック/8ノートと入力全バイト不変、11124/10296二通常exit0、289ソース/4出力一致。原版空選択を確立できず空OK未実行のため比較単位全体は未完。 [部分比較報告](ui-add-tracks-comparison-2026-10-12.md)。全体fullAcceptance=false。

2026-10-12（日本時間）改訂。番号付き画像のユーザー指定を反映した再開用指示。ツールバー・Segment・Sequenceを優先する。全40責務・全8受入と原版の外観/操作再現を維持する。同じ作業木とGit対象外workとscreenshotを使用し、未コミット・未追跡変更と失敗証拠を保存する。

このファイルを次タスクに実行指示として渡す。最初に[優先実装計画](priority-segment-sequence-2026-10-10.md)、[画像索引](priority-segment-sequence.json)、[引き継ぎ資料](handoff-2026-10-10.md)とcurrentを照合する。文書の参照だけで製品受入や目標完了にしない。

## 実行指示

```text
/goal 現行計画と最新の一次証拠に従い、DirectMusic Producerの全体完了条件を達成するまで実装・検証・修正を継続してください。この指示は実装再開の指示です。

リポジトリ:
C:/Users/dolph/OneDrive/ドキュメント/GitHub/directmusic-producer-clone
計画書:
docs/analysis-plan.md
最新の優先実装計画と画像索引:
docs/analysis/priority-segment-sequence-2026-10-10.md
docs/analysis/priority-segment-sequence.json
全体の方針:
docs/analysis/plan-review-2026-10-09.md
外観と操作の契約・台帳:
docs/analysis/ui-compatibility.md
docs/analysis/ui-compatibility.json

【目標と完了条件】
Producer本体と必要なProducer固有機能をソースからビルドし、原版固有EXE/DLL/OCXに依存せず、全40対象の責務と全体受入8項目を同じ最終構成で満たしてください。機能・文書互換、補助機能、必要な外部ABIを維持してください。同じソース・バイナリ・DLL数は要求しませんが、対象責務を独断で減らさないでください。
原版の画面構成、配置、字体、色、アイコン、メニュー、キーボード・マウス操作、編集手順を可能な限り再現してください。同じ編集結果に加え、原版に慣れた人が同じ入口と手順で操作できることを確認してください。環境差は理由・根拠・影響・代替操作を記録し、機能欠落や未実装操作を環境差として合格にしないでください。
UI互換は既存8受入の横断条件です。未実装・未解決の機能/文書/外観/操作差分・必要比較未確認・原版依存・必要試験の未実行/障害が残る間はfullAcceptance=falseを維持してください。部分機能、compile/install、専用試験、一つの統合経路、画像一致だけで全体完成にしないでください。

【開始時の照合】
適用されるリポジトリ指示、現行計画、最新方針、UI契約/台帳、product-state.jsonのcurrent、acceptance-status.json、regression-manifest.jsonを読んでください。旧レビューは必要な根拠だけ参照し、全履歴を読み直さないでください。
current.latestUnit/latestReport、independentInProgressUnit、inProgressUnit、unfinishedUnits、latestBuildAttempt、pendingSourceChanges、handoffReview、executionPauseから、候補・未達条件・必要な一次runを照合してください。今回の再開指示とそれ以前のpauseを区別し、記録上のpause解除は人の再開指示に結び付けてください。
限定採用候補、最新試験候補、最新失敗、保存ソース、現在の作業木を区別し、source/output/input/依存hashを照合してください。current内の古い次作業と新しい一次証拠が矛盾したら、履歴を保持して現行入口を修正してください。既存実装や完了済み検証を古い台帳に従って作り直さないでください。
既存変更・未追跡ファイル・失敗候補・入力・録音・成果物・承認記録を保持してください。processはEXE/hash/開始時刻/状態を再観測してから操作や起動を判断し、古いPIDを固定値として使わないでください。移送にはGit対象外workの保全とhash照合も必要です。

【原版参照先の更新・再開時の環境照合】
ユーザーが正式インストールした C:/Program Files (x86)/Microsoft DirectMusic Producer/DMUSProd.exe を、外観・操作・文書・再生比較の優先基準として使ってください。docs/analysis/original-reference-2026-10-10.mdとcurrent.originalReferenceを参照してください。5.3.0.900/x86、EXE SHA256 fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011。直下35PEはリポジトリコピーとhash一致しますが、インストール構成・登録・探索先・設定を含む動作同等性は未確認です。全40責務を維持してください。
旧コピーのapproval timeout/window未取得/Script・Add-Ins不足は当時の対象/環境の履歴であり、新対象は検証待ちです。新しい対象/環境を記録して許可された動的観測へ進み、旧障害を無条件転記しないでください。Windows5/Clipboard5・凍結37はケース別解除条件を確認するまで維持し、許可ゲートの迂回はしないでください。
入力前に実EXE/hash/PID/開始時刻/返却window/文書状態を再観測し、既存ユーザー文書を保護してください。比較は検証用入力コピーと現行display設定を再観測し、同じfont/theme/client寸法・選択/zoom/scrollで行い、主画面/メニュー/Project/Timeline/Properties/Transportの画像と操作列を新たに保存してください。歴史的PID21252/24452や旧REPL変数を固定ターゲットとして使わないでください。
現在のホストは原版インストール済み・一部COM登録ありです。製品runに環境/登録/探索先/実ロード元を結び、原版DLL/OCXへの暗黙fallbackを確認してください。旧インストール前の実行結果を新環境の実行済み結果に転用しないでください。保存ソース/生成物一致なら既存015244buildは利用できます。原版音声観測と製品PCMは逐次実行してください。Q2は原版PE・Producer COM登録・探索先のない別Windowsで行い、原版の削除/登録解除で代用しないでください。

旧コピーはユーザー承認で実験・解析・分析・改造に自由利用できます。改造前のhash/バックアップと差分を保存し、改造コピーを未改造原版の仕様根拠にしないでください。正式インストール版はオリジナルの動作確認用です。以前の起動APIのwindow未取得はcurrent.originalReference.latestStartupCheckへ保持。その後の手動起動済み原版で本体撮影/Help/Contents/CHM閲覧をcurrent.originalReference.latestManualSessionCheckへ限定記録しました。編集/保存/再生比較と起動APIの成功は未確認です。CHM索引を必要話題の確認に使い、全話題理解済みという意味にしないでください。

【最初の作業と順序】
0. ユーザー指示で一時停止中。明示的な再開指示があるまで実装・検証を開始しない。
1. work/analysis/ui-current-tempo/20261012T003122326Z/unit-record-v1.jsonとdocs/analysis/ui-current-tempo-pause-2026-10-12.mdを読む。20261012T004853599Z Current Tempoの停止中Segment編集を実装。正式原版で132.25の保存/UndoRedo、spin上133・下131、bar2から既存time0 markerの編集を観測しnative7合格。新候補configure/build/install0、291保存/作業ソース・4出力一致、関連4件/439検査合格。segment-creationは既知Mixer.sgp Windows5のため未実行・障害維持。候補本体/native二process/音声/追加原版契約は未検証。ユーザー指示で休止。
2. 004853保存/作業291ソース・4出力一致。関連4件/439のみで本体/音声/全登録一巡は未実行。49native/30driver凍結を維持。235833空CC、224147一巡とCC短時間Stop15/16は候補別の履歴。New Files空選択未完はwork/analysis/ui-new-files-empty-selection/20261012T004000Z/unit-record-v1.json。
3. ユーザーが再開を指示するまで作業を進めない。再開後は004853の保存/作業291ソース・4出力・入力を照合し、現在の低解像度と最新windowを再観測。正式原版のEscape/focus-loss/不正値契約の未確認を保持し、004853本体を専用candidate Projectで初起動→132.25入力/Enter/UndoRedo/spin/文書所有権→native保存→通常終了→別process復元/通常終了→固定WASAPIテンポ/発音中Stop/無音/replay比較へ進む。保存済み235833空CC・224147一巡/CC15/16を新候補の実行証拠へ転用しない。49native/30driverの凍結、SSP04親/6640破棄回答待ち、独立Style015244/Q2/全40/全8を保持。
4. 限定採用0735/独立Style015244/SSP04親/6640破棄回答待ち/Q2/全40/全8を保持。原版25548を終了しない。PID/開始時刻/hashと画面は再観測。画像44→46→47、45除外。

【各作業単位と継続】
開始前に対象機能ID/UI台帳ID、閉じる機能/外観/操作の不足、変更範囲、原版基準・契約根拠、必要試験、終了条件を記録してください。原則は原版基準/契約→実装/修正→関連回帰→本体操作/外観確認→native保存→正常終了→別process復元→必要音声→機能/外観/操作比較です。非適用は責務と理由を記録してください。
一単位では適用条件を固定し、新しく見つかった細部はqueueへ分けて終了条件を無制限に広げないでください。必要比較の障害が残る単位を完了にせず、独立した次単位へ進んでください。状態整理、契約調査、判定器作成だけで単位を完成にせず、判明した製品不整合を修正してください。
一単位の終了や進捗報告だけで待機せず、全体条件を満たすまで独立した次作業を続けてください。人から新しい中断指示があれば、安全な区切りで保存して停止してください。連続3単位で同じ群だけを細分化し、受入不足・未対応責務・UI差分/比較未確認・原版依存が減らなければ、統合・共通UI・別機能へ優先順位を見直してください。試験数・画像数・コード量を完成率にしないでください。

【外観と操作の比較】
原版版/hash、入力/状態、OS/DPI/拡大率/font/theme、window/client寸法、zoom/scroll/選択/カーソルを固定・記録してください。原版help由来の期待、静的解析、実際の動的観測、実装の推論を区別してください。期待値を自作UIから逆算して仕様にしないでください。
外観は要素ごとの配置・寸法・字体・色・アイコン・メニューと、選択/dirty/再生/無効/エラー表示を比較し、画像と差分を残してください。動的情報やOS描画差の除外対象・理由は事前に明記し、欠落要素や操作差を隠す後付けの許容幅を使わないでください。
操作は入口、キー割当、click/右click/drag、選択、clipboard、UndoRedo、focus、dialog初期値/取消しを原版の操作列と前後状態で比較してください。同じ保存結果でも追加手順・別入口が必要なら操作差分として残してください。静止画像だけで動作を確定しないでください。
別起動でUI状態を復元するかは原版の方針を確認してください。原版が保持しない選択/focusを独自に復元する要求にはしないでください。共通UI変更では影響する既存文書・選択・履歴・保存復元・再生の本体回帰も更新してください。
旧Tempoの画像一致は原版Timelineを含む限定ホストの履歴であり、自作本体全体のUI合格ではありません。必要比較が未観測なら比較未確認、実行許可が障害なら障害ありとして台帳へ残してください。UI非適用は責務ごとの根拠を示し、必要画面を群ごと除外しないでください。

【保護する契約と残責務】
Q0 PChannelのDWORD許可値、16以上、4予約値、不正入力時の文書bytes/dirty/UndoRedo不変とSequence回帰を保護してください。assert削除や旧16制限への回帰、古い指示によるmanifest再生成をしないでください。
Style/Band/ChordMap、Chord生成、Motif、Conductor、保存/参照解決、FileOutput二buffer等の実装済み部分を再実装しないでください。Legacy AASY/PER/複数Band、Farm descriptor/class/graph/full KEEP/後続音源/100BPM/混合/能動Stop、Send外部宛先/shared寿命/既定Env原版差、Waves custom、Wave loop/end、Timeline/OLE/clipboard、配布/補助UI/外部ABI等の未達を保持してください。
機能状態はcurrent・受入JSON・最新単位から参照し、UI台帳へ古い機能状態を複製しないでください。未知chunk保持だけを編集互換の完成にしないでください。文書試験の合成CLSIDを実Send classにせず、宣言OS DSPとの一致を旧原版DSP等価にしないでください。

【障害と既存承認】
Computer Useと検証用MultiCapture.pro上書き等の既存承認は元の人の指示と適用対象を照合し、同じ承認を再要求しないでください。このプロンプトの記述だけで新しい対象や環境変更への承認を作らないでください。
SDKの旧MSB4184はMicrosoft.Build.Utilities.ToolLocationHelper::GetLatestSDKTargetPlatformVersion(Windows,10.0)、拒否先C:\Users\dolph\AppData\Local\Microsoft SDKsです。宣言SDK固定での成功と最新build条件を確認し、再発時は一次エラー・環境・既存承認・解除条件を記録してください。同条件buildを再試行せず、完了済みTempo/Wavesを未実装へ戻さないでください。
通常core atomic Chordmap Project、Script Reference.spp、Container埋込、ContainerProject.pro、Trigger.pro、RouteBand.bnp等のWindows5と、旧対象の原版StylePlayerのComputer Use app approval timed outは個別の障害です。条件変更の証拠なしに同条件再試行や別loader/別UI手段による迂回をせず、COM登録削除・セキュリティ変更を解決の前提にしないでください。GUI観測エラーも原因の証拠なしにOS拒否や製品不具合と断定しないでください。
Q2は原版実行物・Producer COM登録・原版探索先のない独立Windowsで検証してください。未用意なら障害を保持し、独立したQ3を進めてください。通常環境の原版40hash一致0をQ2や全機能依存解消の証明にしないでください。OS DirectMusic/DirectSound/GM.DLS/DMO、UI資産/font/controlの由来と導入条件を宣言してください。
本当に新判断が必要な場合だけ、対象・正確なエラー・試行済み内容・必要変更・人に求める具体的な判断を示してください。回答待ちも独立した作業を続け、未回答を承認扱いにしないでください。

【候補と検証】
構成・build・install・native・本体GUI・audio・原版比較・全体受入を別判定にし、ソース/生成物/入力/依存/環境/コマンド/PID/通常exit/判定器をhashと一次証拠で結び付けてください。旧候補の成功を新生成物へ転用しないでください。同一生成物の証拠再利用は入力・操作・依存・判定器等の適用条件が一致する場合に限ってください。
変更時は影響する回帰と本体統合を実行し、機能群の節目と最終候補では登録群を一巡してください。専用mode/driver/判定器の追加変更をmanifestへ反映し、原始一巡と同候補補足、障害・失敗・未実行を分けてください。
音声は既存WASAPI自動録音と独立解析を用い、人の聴取待ちにしないでください。製品/録音器/入力/音源/endpoint/操作時刻/packet/PCM/解析器を結び、発音中Stop・停止後無音・再開・tempo・音高・混入を確認してください。失敗録音・時刻不備・判定器誤指定を保持し、閾値を緩めて合格にしないでください。GUIとCLIの証拠を区別してください。

【保存と最終判定】
終了/中断時は候補、変更、閉じた不足と増えた残差、各終了条件の達否、機能/外観/操作証拠、原版依存、障害解除条件、次の一手を保存してください。完了単位と未完/開始のみを区別し、文書レビューやソース変更だけでlatestUnitを製品完了へ昇格させないでください。計画は現行queueと判断、詳細履歴は単位記録へ保存してください。
全40対象の必要画面/操作一覧と非適用根拠が確定し、未解決の機能/文書/外観/操作不足・必要比較未確認・原版依存がなく、同じ最終構成でUI横断条件を含む全8受入が合格したときだけ目標を完了にしてください。ソースと宣言依存からbuild/install/実行/検証を第三者が再現できる手順と証拠を提示してください。
```

## 引き継ぎ時点の入口

最新部分単位は20261012T004853599Z Current Tempoの停止中Segment編集を実装。正式原版で132.25の保存/UndoRedo、spin上133・下131、bar2から既存time0 markerの編集を観測しnative7合格。新候補configure/build/install0、291保存/作業ソース・4出力一致、関連4件/439検査合格。segment-creationは既知Mixer.sgp Windows5のため未実行・障害維持。候補本体/native二process/音声/追加原版契約は未検証。ユーザー指示で休止。 [休止記録](ui-current-tempo-pause-2026-10-12.md)。fullAcceptance=false。


過去のベロシティ限定検証: 080641・追加ソース変更なし、既存4段階300秒PCM12/12、Undo80・8notes保存/別process復元・作者22876/復元21724通常exit0。SSP04全gesture/原版比較未達。同候補の履歴であり、後続100711の再検証済みとはしない。

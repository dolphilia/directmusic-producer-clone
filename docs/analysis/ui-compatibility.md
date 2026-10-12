# DirectMusic Producer 外観と操作互換の契約

表示環境の現行条件：ユーザーが主ディスプレイと解像度を変更した。4K/200%は以前の条件で、変更後のOS倍率は未測定。入力前に返却windowと最新画像で座標を再取得し、厳密比較では双方のDPI/font/theme/client寸法を記録する。

登録検証追補：224147同候補一巡：native100＝51合格2990検査/48既知OS凍結/匿名Band1保留。driver172＝18合格/30凍結/124未実行。153入力（7ディレクトリを含む）・18 producing run・291保存/作業ソース・4出力を285検査で照合。新規失敗なし、全登録合格ではない。 [最新一巡](priority-regression-224147-2026-10-12.md)。fullAcceptance=false。

追加比較：151135・画像7/8の追加比較：正式原版と候補で初期Band選択/Down→Chord/Home→Band/Cancelが一致。候補空OKは異なる手順による単体確認。4トラック/8ノートと入力全バイト不変、11124/10296二通常exit0、289ソース/4出力一致。原版空選択を確立できず空OK未実行のため比較単位全体は未完。 [部分比較報告](ui-add-tracks-comparison-2026-10-12.md)。全体fullAcceptance=false。

改訂：2026-10-10（日本時間）。原版5.3.0.900の外観と操作感を可能な限り再現し、機能・文書互換と一緒に受け入れるための基準。対象は全40責務。再開後の製品実装を進め、2026-10-10のユーザー依頼で次タスクへの引き継ぎを準備した。外観・操作の全体比較は未完了。

最新限定調査：20261012T004853599Z Current Tempoの停止中Segment編集を実装。正式原版で132.25の保存/UndoRedo、spin上133・下131、bar2から既存time0 markerの編集を観測しnative7合格。新候補configure/build/install0、291保存/作業ソース・4出力一致、関連4件/439検査合格。segment-creationは既知Mixer.sgp Windows5のため未実行・障害維持。候補本体/native二process/音声/追加原版契約は未検証。ユーザー指示で休止。 [休止記録](ui-current-tempo-pause-2026-10-12.md)。fullAcceptance=false。 次：ユーザーが再開を指示するまで作業を進めない。再開後は004853の保存/作業291ソース・4出力・入力を照合し、現在の低解像度と最新windowを再観測。正式原版のEscape/focus-loss/不正値契約の未確認を保持し、004853本体を専用candidate Projectで初起動→132.25入力/Enter/UndoRedo/spin/文書所有権→native保存→通常終了→別process復元/通常終了→固定WASAPIテンポ/発音中Stop/無音/replay比較へ進む。保存済み235833空CC・224147一巡/CC15/16を新候補の実行証拠へ転用しない。49native/30driverの凍結、SSP04親/6640破棄回答待ち、独立Style015244/Q2/全40/全8を保持。

## 正本と対象範囲

[ui-compatibility.json](ui-compatibility.json)を比較状態の正本とする。15群に[feature-map.csv](feature-map.csv)の全40対象を対応付けた。群は画面の完成数ではない。再開後、原版help・既存観測・ソースを照合し、必要な画面と操作を各群の`cases`へ追加する。列挙が終わるまで`inventoryComplete=false`を維持する。

機能の状態は[product-state.json](product-state.json)の`current`、[acceptance-status.json](acceptance-status.json)、最新単位の一次証拠から読む。UI台帳の`functionalTracking`はその入口を示し、古い実装状態を複製しない。専用試験の合格は本体操作や原版UI比較の合格にしない。

| 台帳ID | 画面・責務群 | 最初に確定する基準 |
| --- | --- | --- |
| UI01 | 本体・文書管理・共通コントロール | 主画面の領域、文書切替、メニュー、確認/取消し、dirty表示 |
| UI02 | Timeline・Tempo・拍子・Sequence・MIDI | 時刻軸、zoom/scroll、選択、プロパティ、キー/マウス、clipboard/drag |
| UI03 | Segment・時刻付き参照 | trackの配置・切替・設定と参照編集の入口/手順 |
| UI04 | Style・Pattern・Motif | 所有/共有の表示、編集・複製・auditionとプロパティ |
| UI05 | Band楽器割当 | 音色/PChannelの表示・変更・選択・履歴 |
| UI06 | Chord・ChordMap・SignPost | 節点/イベントの表示、選択、接続、編集手順 |
| UI07 | DLS・Envelope・RegionKeyboard・PanVol | 波形/鍵盤/Region/包絡の表示と範囲・音量・panの操作 |
| UI08 | AudioPath・Send・効果 | 経路/宛先/効果順序・既定値の表示、挿入・編集・取消し |
| UI09 | Command・Lyric・Marker・Mute・Param・Wave | stripの描画、時刻/範囲選択、追加/変更/削除と履歴 |
| UI10 | Script文書・時刻起動 | 参照/変数/routine、実行・エラー表示と編集操作 |
| UI11 | Container | 参照・埋込・aliasの表示と追加/削除/保存操作 |
| UI12 | ToolGraph | toolの順序・接続・parameter表示と編集操作 |
| UI13 | Transport・音声ファイル出力 | Play/Stop/再開/tempo/録音の入口、状態表示と操作可能状態 |
| UI14 | Farm・StylePlayer | 補助アプリの必要機能、選択・再生・停止の画面/手順 |
| ROLE01 | 導入解除・CRT | 必要な導入/解除画面の有無と、UI非適用の責務を根拠付きで確定 |

旧Tempoの[画像66枚等の比較](tempo-comparison.md)は原版Timelineを含む限定ホストの履歴。現在の自作本体全体の外観・操作合格ではない。[0735 Q1](q1-current-0735-2026-10-08.md)の本体保存・再起動・音声も、原版全画面の比較とは別に保持する。

## 優先する原版の比較環境

ユーザー指定の C:/Program Files (x86)/Microsoft DirectMusic Producer/DMUSProd.exe を優先参照先とする。[参照先更新](original-reference-2026-10-10.md)のファイル/登録/環境証拠を各ケースのreference/environmentへ結び、同じ入力・4K/200%・表示/文書状態で新たに動的観測する。旧コピーのhelp/画像/操作証拠は当時の適用条件で保持し、新しい比較実行へ転用しない。

旧コピーのapp approval timeoutを新対象の失敗とせず、インストール版は検証待ちとして記録する。新対象の実際の許可/起動/表示/操作結果を保存し、個別Windows5/Clipboard5の解除条件や許可ゲートを迂回しない。原版のインストール/登録/探索先と製品の実ロード元を区別し、Q2は原版なしの別Windowsで検証する。基準変更だけで外観/操作の合格やinventoryCompleteへ昇格させない。

## 比較ケースの記録

各ケースに次を記録する。空欄を合格にせず、未観測・障害・非適用を区別する。

- `id`、所属群、対象責務、対象画面と閉じる不足。
- `reference`：原版版/hash、入力hash、help該当箇所、画像/操作観測の証拠と適用範囲。help由来の期待と実際の動的観測を区別する。
- `environment`：OS、DPI/拡大率、font/theme、window/client寸法、zoom、scroll、文書/選択/カーソル状態。
- `candidate`：保存ソース/生成物hash、実processのEXE/PID/開始時刻、通常exit、入力・依存の同一性。
- `steps`：原版と本体の操作入口、キー/マウス入力、前後状態、取消し/失敗時の状態。追加手順がある場合は差分にする。
- `functionalEvidence`：本体操作、native保存、別process復元、必要音声、既存8受入との対応。
- `appearance`と`interaction`：状態、比較証拠、残差、障害、次の一手。全体状態と限定成立を別々に記録する。

台帳の初期15群は外観/操作とも未着手。既存証拠の入口を設定したことは、比較実行や製品合格ではない。原版approval障害は保存済み観測・helpの利用まで妨げるものではないが、動的比較未確認を解消したことにもならない。

## 判定規則

機能・外観・操作を別々に判定し、該当する三軸が成立した範囲だけを完了にする。状態は未着手・作業中・検証待ち・合格・失敗・障害ありを使う。限定成立には機能・候補・入力・操作・環境を必須とし、部分成立を群全体の合格へ昇格させない。

外観は主画面/編集領域の構成、位置と大きさ、字体・色・アイコン、メニュー、選択/dirty/再生/無効/エラー表示を同じ入力・状態で比較する。画像比較だけでなく、要素ごとの対応と差分を残す。OS文字描画、DPI、時刻/識別子などの差は対象・理由・確認方法を先に定め、配置や欠落要素を隠す除外・後付けの許容幅にしない。

操作は入口、手順、キー割当、クリック/右クリック/drag、選択範囲、clipboard、UndoRedo、focus、dialog初期値と取消しを原版の観測に従って比較する。同じ保存結果でも余分なdialogや別の操作入口が必要なら、操作互換の差分として扱う。静止画像から動作を推定して合格にしない。

別起動で選択/focus等を復元するかは原版の方針を確認する。原版が保存しないUI状態の独自復元を受入要件にしない。共通UI変更時は、影響する既存文書・選択・履歴・保存復元・再生を新候補の本体で回帰する。

UI非適用は画面群ごとの一括除外ではなく、責務/ケースごとに理由と根拠を記録する。CRT等の内部ABIと導入解除の必要画面を分けて評価する。やむを得ない環境差には原版との差、実際の原因、利用者への影響、代替操作、未解決責務の有無を残す。機能欠落や操作変更を環境差へ移して合格にしない。対象範囲を減らす判断は既存のユーザー承認に従う。

## 既存8受入との対応

UI互換は第9の受入試験を追加するのではなく、[既存8項目](../analysis-plan.md)の横断条件にする。

| 受入ID | UIで確認する内容 |
| --- | --- |
| clean-build | UI資産・font・controlの取得/配置/版が宣言依存から再現できる |
| startup-shutdown | 主画面/メニュー/文書管理と起動/終了・取消しの入口と手順 |
| original-data-load | 読込後の文書・track・プロパティ・選択可能状態の表示 |
| edit-save | 原版のキー/マウス/メニューによる編集と、選択/dirty/無効/確認/取消し表示 |
| separate-process-reload | 保存値の表示、文書切替と原版で確認したUI状態の復元方針 |
| play-stop-tempo-audio | Transportの操作入口/手順と再生/停止/無効表示、同processの音声 |
| repeat-invalid-input | 文書間状態分離、確認/エラー/取消し、focus遷移と文書保護 |
| original-independence | UI資産・font・controlの由来と、原版固有PEなしの画面/操作成立 |

全40対象の画面/操作一覧が確定し、同じ最終候補で該当三軸が合格し、非適用・環境差の根拠も確認されたときに横断条件を合格にする。必要な原版比較が未確認、未実行、障害ありなら`fullAcceptance=false`を維持する。

## 優先順と進捗判断

最新候補の未達本体統合を先に検証し、主画面/文書管理、Timeline/プロパティ、Transportの基準を確定する。共通UIとQ3各機能を小さな単位で接続し、全責務と独立Windowsの最終受入へ進む。統合が実障害で止まった場合は、未達条件を保持して独立したUI基準整理・実装を進める。

各単位の開始/終了で、閉じた不足、増えた残差、必要比較未確認、本体未検証、障害を記録する。群数・画像数・試験数・コード量を完成率にしない。連続3単位で不足が減らなければ、共通UI・別機能・統合へ優先順位を見直す。

引き継ぎ時点のUI13.style-responsive-startは[015244報告](ui-style-responsive-start-2026-10-10.md)を参照。本体操作/native保存/別process復元と二通常exit0は限定成立、固定PCM・同候補一巡・原版比較は未達。[次タスクへの引き継ぎ](handoff-2026-10-10.md)から再開する。

## 番号付き画像による優先ケース

2026-10-10のユーザー指定は[詳細計画](priority-segment-sequence-2026-10-10.md)と[画像/要件索引](priority-segment-sequence.json)に固定した。画像1〜47を全件確認し、45番のTimeSig誤撮影を除いた46要求をUI台帳のUIxx.priority-NNへ追加する。ツールバー→Segment→Sequence編集/音色/Band→CC→複数パート/ドラム/DLSを優先し、機能/外観/操作と保存復元/必要音声を各段階で結ぶ。

baselineKind=userScreenshotAndReportedBehaviorとし、画像は静的状態、動的手順はユーザー報告として区別する。製品/原版でのエージェントによる再現・比較は未実施であり、新規ケースは比較未確認、completed=false。既存ケースの限定成立や障害は保持し、新根拠を旧候補の合格へ転用しない。全40の一覧確定を意味せずinventoryComplete=falseを維持する。

画像はscreenshot/に原本を保ちGit対象外。移送はwork/とともに別途保全し索引のhashを照合する。200%を維持し、crop物理座標/DPI/論理座標を区別する。静止画のみでhover、ドラッグ中カーソル、スナップ、音声、DLS選択成功を判定しない。44→46→47がSegment Properties経路であり45を代用しない。

## Transport速度変更の限定実行

画像2は[074753報告](ui-transport-tempo-2026-10-10.md)により試聴倍率と部分Transportの機能/操作を限定確認した。UI13.priority-02へ一次証拠を結び、原版の既定50%/ダイアログ/Cancelの限定観測と、未実施の同入力再生比較を分ける。UIA値が古い場面では新しい画面表示を使用した。MIDI録音/Options/Performance/seek/境界停止/外観差分を残し、SSP01・UI13全体を合格にしない。

## 現在位置と境界停止の限定実行

画像2は[083342報告](ui-transport-position-2026-10-10.md)により現在位置/先頭再生・即時/境界停止を限定確認した。主画面の180秒PCMはCLIと別に音高/停止後無音を確認した。保存/別process復元とカーソルによる文書/history不変が成立。旧074753の残差のうち今回閉じた範囲はUI13.priority-02の最新証拠で管理する。原版同入力比較、Ctrl指定の本体実測、他境界/Motif、録音/Options/Performance/全外観は未達で、UI13全体は合格にしない。

SSP02画像9〜11は[131728 Band統合](ui-segment-band-2026-10-10.md)で空白位置Insert/Properties/埋込MDI子画面・所有履歴・native保存・二正常終了・別process復元が限定成立した。原版同入力/厳密外観比較は未確認でUI05全体を完成にしない。123249の空白hit不具合は修正し、旧証拠は保持。画像44→46→47のLengthは後述の限定成立へ進んだ。16初期行を内部PChannel上限にせず、SSP03/05の有効化・楽器・pan/volumeを残す。

SSP02画像44・46・47は[140539 Length統合](ui-segment-length-2026-10-10.md)でファイルProperties/Length、20→24小節/40→48秒、Cancel/未変更OK/UndoRedo/native保存・二通常終了・別process復元が限定成立した。258ソース/4出力一致、関連7モード553確認。末尾切詰め・全Propertiesタブ・clocktime実再生と原版同入力/厳密外観操作比較は未達でUI03/SSP02を完成にしない。SSP03の進捗は次段落へ記録する。一物理Sequence track=一part/PChannel/group、同groupの複数trackをまとめて表示するCHM契約を保護する。

SSP03画像13〜16・21〜24は[Sequence報告](ui-sequence-basic-2026-10-11.md)を参照。162022は264ソース/4出力一致、関連13合格755検査、8ノート/Organ 1保存・作者17360/復元1668の二通常exit0が限定成立。固定300秒PCMは初回1.15秒で1秒基準に不合格（後続0.505/0.430秒）。音を出さないAudioPath/楽器準備も実行済み。原始native98一巡は51合格2746検査/46OS障害/既知Band失敗1保留、driver170一巡は20合格/28障害/122未実行。原版/厳密外観・全体受入は未達。 UI02/05/SSP03全体を完成へ昇格させず、SSP04の指定編集操作へ進む。

SSP04画像17〜20の実行記録：[ノート編集報告](ui-sequence-edit-2026-10-11.md)。170536限定成立:266sources/4outputs;13related822checks;端/中央/velocity/Grid/None/Selected-Part数値/UndoRedo/Cancel;8notes/Organ1保存復元/二通常exit0。初回PCM1.215s不合格、音量差成立。原版/厳密外観/全操作未達。 端別カーソル/配置/色/字体の厳密比較、保持中キー/取消し/全Snap/AllParts等を残し、SSP05画像25〜28へ進む。


## SSP05候補201607の履歴

201607限定成立：268保存/作業ソース・4出力一致、関連13モード878検査合格。再生中の音量・パンのUndo/Redoを含む固定PCM18/19項目が成立し、初回発音1.15秒のみ1秒基準に不合格。作者3356/別復元21780の保存・二通常exit0、1 binsのみ変更/他30 leaf保持を確認。明示PAN/VOLUMEの連続previewと取消し復元はソース接続済みだが、ドラッグ途中/取消しの本体・PCM確認、継承値、原版/厳密外観/全操作は未達。 [Band報告](ui-band-mixer-2026-10-11.md)と `work/analysis/ui-band-mixer/20261010T175036931Z/unit-record-v5.json` を照合する。SSP05の初回発音1秒基準、途中preview/取消し、継承値、原版/厳密外観、全98/170一巡を未達で保持する。同群の細分化を区切り、独立したSSP06画像29〜37のCC/PBend/Expressionを、CHM・既存Sequenceモデルとの照合→開始記録→実装→新候補関連/本体/native保存・別process復元/固定PCMへ進める。SSP01〜04・独立Style015244・46native/28driver OS凍結を保持する。 限定採用0735、最新全一巡162022、46native/28driver OS凍結、全体6作業中/2障害/0合格とfullAcceptance=falseを維持する。

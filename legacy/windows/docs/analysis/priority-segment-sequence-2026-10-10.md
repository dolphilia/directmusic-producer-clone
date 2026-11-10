# ツールバーとSegmentとSequenceの優先実装計画

表示環境の現行条件：1280×800、96 DPI（100%）。062231復元本体の環境と6control hitを測定済み。4K/200%は履歴。厳密比較では双方のDPI/font/theme/client寸法を記録する。

登録検証追補：224147同候補一巡：native100＝51合格2990検査/48既知OS凍結/匿名Band1保留。driver172＝18合格/30凍結/124未実行。153入力（7ディレクトリを含む）・18 producing run・291保存/作業ソース・4出力を285検査で照合。新規失敗なし、全登録合格ではない。 [最新一巡](priority-regression-224147-2026-10-12.md)。fullAcceptance=false。

追加比較：151135・画像7/8の追加比較：正式原版と候補で初期Band選択/Down→Chord/Home→Band/Cancelが一致。候補空OKは異なる手順による単体確認。4トラック/8ノートと入力全バイト不変、11124/10296二通常exit0、289ソース/4出力一致。原版空選択を確立できず空OK未実行のため比較単位全体は未完。 [部分比較報告](ui-add-tracks-comparison-2026-10-12.md)。全体fullAcceptance=false。

2026-10-10（日本時間）。ユーザー指定の操作説明と `screenshot/1.png`〜`47.png` を、既存の[全体計画](../analysis-plan.md)へ追加する詳細計画。以下の順序を既存の一般的なQ3順序より優先する。計画追加自体は製品完了の記録ではない。その後のStandard基本統合・速度変更・現在位置再生と停止は末尾の状態と別の一次単位へ記録し、SSP01全体は未完とする。

## 目的と適用範囲

原版に慣れた人が、同じ入口・配置・キー・マウス操作でSegmentを作り、Sequenceのノート・音色・CC・複数パートを編集し、保存・再読込・再生できる状態を先に完成させる。ツールバー、Segment、Sequenceを最優先とし、その操作に必要なBandとDLSを同じ流れへ接続する。全40責務と全体8受入は維持し、Style等の残責務を削除しない。

スクリーンショット番号はユーザーの番号を維持する。本文中の「Segmentトラックの楽器ボタン」は画像上の **Sequenceパートのヘッダー**として扱う。Sequence/Segmentの綴り揺れは統一する。**45番はTimeSig Track Propertiesの誤撮影であり、Segment Propertiesの受入基準に使わない。44→46→47が正しい経路**である。

## 根拠と比較条件

- 47枚すべてを確認した。画像の番号・相対パス・SHA256・寸法・採否と、各要件のUI台帳IDは[画像と要件の索引](priority-segment-sequence.json)で固定する。46枚を優先要件へ対応付け、45番は除外理由付きで保持する。
- 静止画像は配置・ラベル・色・有効/無効・選択の根拠。キー操作、ドラッグ、スナップ、発音、操作後の移動などはユーザーによる観測報告であり、エージェントが動的比較済みという意味ではない。実装時に操作列・前後状態・必要音声を確認し、根拠が異なるものを混同しない。
- 動作確認の優先原版は `C:/Program Files (x86)/Microsoft DirectMusic Producer/DMUSProd.exe`、5.3.0.900。旧コピーは実験・解析・改造用。正式インストール版のファイルを改造しない。[原版参照方針](original-reference-2026-10-10.md)に従い、編集比較は検証用コピーで行う。
- 手動起動済みの正式原版では、本体撮影・Help/Contents操作・CHM閲覧を限定確認済み。プログラムによる起動APIの失敗と、未実施の編集/再生比較は別に保持する。CHMは586 HTML/252 GIFを抽出・索引化済みで、必要な話題と図を読んで仕様の疑問を解決する。全文を理解・比較済みとはしない。
- 現行測定は1280×800/96 DPI（100%）。4K/200%は旧条件。OS/DPI、フォント、テーマ、原版/製品のDPI認識、クライアント寸法、zoom/scroll、選択・カーソル状態を記録する。切り抜き画像の物理ピクセルを無条件に論理座標へ転用しない。画像にない全画面配置やhoverカーソルの形は追加観測で補う。
- `screenshot/` はGit対象外。画像は移動・改変・削除せず、別checkout/ホストへの引き継ぎでは `work/` とともに別途保全し、索引のhashと照合する。画像がない環境で比較を実行済みにしない。

## 既存実装を活用する範囲

開始時に保存ソース・現在の作業木・候補・一次実行証拠を照合する。ソースにAPIがあることと、原版の画面から操作できること、現在の生成物で検証済みであることを分ける。

| 入口 | 確認した基盤 | 今回閉じる不足 |
| --- | --- | --- |
| `src/producer/sequence.h` / `sequence.cpp` | ノートの読取/挿入/変更/削除、範囲clipboard、既存MIDI/curve/未知データ保持 | パート別の原版ピアノロール、選択・カーソル・ジェスチャー、CC曲線の実編集。保持だけを編集対応としない |
| `src/producer/band.h` / `band.cpp` | 楽器のpatch/PChannel/pan/volume、DLS割当、Bandイベント、履歴 | Sequence楽器ボタンとの連動、埋込Bandの子編集画面、16行とpan/volumeビューの状態遷移 |
| `src/producer/segment_timeline_window.cpp` | 時間軸・拍子/テンポ表示、文書選択/履歴、スクロール。Sequenceは現在、固定行のノート矩形描画 | トラック追加と順序、パート別開閉、鍵盤/音高/ノート高さ、複数ピアノロールとCCの同時表示 |
| Framework / main / Conductor / Properties / DLS | 既存の文書管理、保存、再生、個別Properties・DLS編集の限定実装と証拠 | 共通コマンドへ接続し、原版の入口・dialog・toolbar状態にそろえる。既存機能を再実装しない |

限定採用0735、Standard基本統合の最新試験064441、未完の015244独立Style音声/同候補一巡を保全する。Style単位を完了へ昇格させず、今回の優先経路と独立した未完として保持する。旧015244のPCM完結を今回の全着手の前提にはしない。共有Transport等に直接影響する欠陥だけは依存関係を示して先に直す。新生成物へ旧候補の結果を転用しない。

## 実装順序と各段階の終了条件

順序はSSP01→SSP02→SSP03→SSP04→SSP05→SSP06→SSP07→SSP08。段階内は既存実装と依存に応じて分割できるが、画像番号の操作を抜かさない。外観と操作の比較は各段階で行い、最終段階まで延期しない。

| ID | 優先範囲と画像番号 | 実装・接続する内容 | 段階の終了条件 |
| --- | --- | --- | --- |
| SSP01 | ツールバー・View・MIDI設定（1–4） | Standard、Transport Controls、Transport Optionsを原版の並び・アイコン・区切り・有効状態で実装。Viewから各バーを独立表示/非表示。保存/新規/子ウィンドウ操作、再生/録音/速度、エフェクト/メトロノーム/MIDI設定を共通コマンドへ接続 | メニューとボタンが同じ処理を行う。文書なし/停止/再生/録音時の状態を比較。MIDI/Performance Optionsの選択・適用・取消しを検証し、速度倍率は文書テンポを変更しない。関連する再生/録音/設定の実効性を確認 |
| SSP02 | Segment作成・トラック・長さ（5–12、44、46、47） | 3入口のNew→Create New File(s)→Segment Length→TimeSig 4/4→Add New Tracks。Band/Tempo追加、Band挿入/Properties/子編集画面。Segment Properties→Lengthを接続 | 空選択で進む経路、Bandあり経路を両方確認。Tempo追加時120、各イベント位置/名前、長さ変更の保存/UndoRedo/別process復元。Create New File(s)のCancelは文書を作らない。確定後のSegment LengthのCancelは既定長20小節の作成済みSegmentを残してAdd New Tracksへ進み、そのCancelはTimeSigだけを残す。既存文書を保護する。Track種別は全体の残責務を保持 |
| SSP03 | Sequence基本・最小発音経路（13–16、21–24） | パート1の開閉、鍵盤、active/inactive色、挿入カーソル、赤い時刻バー、ノート挿入/選択。Insert New Instrument→Piano 1→GM/GS→Organ 1を同じBandモデルへ接続 | 開閉ボタン/ダブルクリック、右メニュー/Insertの両入口が成立。青/赤ノート、クリック試聴の音高/音色/velocity、楽器ラベル・bank/program・Band状態が一致。名前変更だけで音色変更としない |
| SSP04 | ノートの編集操作（17–20） | 上端velocity、右端duration、左端start、中央/下端moveのhit領域とカーソル。Snap None/Grid/Beat/Barline、矢印移動、挿入後位置、Edit Velocity | 各edge/bodyとスナップ設定を組み合わせて確認。右端の長さは非スナップ、左端/移動/クリック位置は設定に従う。None時の左右キー不動・上下可、velocity変更ごとの試聴、対象範囲別一括編集、UndoRedo/取消し/native復元が成立 |
| SSP05 | Bandとpan/volume（25–28） | PCh1とOrgan 1の連動、リスト/図の選択同期、選択赤と有効な濃字の区別、ドラッグによるpan/volume | Sequence→Band→Sequenceで設定が一致。位置に対応する値と表示、再生効果、選択解除後の有効状態・位置・保存復元を確認。例のL2/102を同条件で再現できる |
| SSP06 | CCとPBend曲線（29–37） | Add New CC Track、種別選択、PB/Expr要約、個別開閉、Draw Curve/Insert、ドラッグ中プレビュー、曲線作成/編集 | PBendとCC11を同時表示し個別編集。符号/中心値/範囲、時刻・PChannel・曲線形状をnative監査/復元で確認。曲線を無条件に単発CCへ平坦化しない。PB音高変化とExpressionの音量効果を既存PCM手順で確認 |
| SSP07 | 複数パート・ドラム・DLS（38–43） | Add Sequence Part(s)→Add New PartのPChannel/使用状態/Create New Part/Close。複数ピアノロールとCC、Part10ドラム名/キット、Choose DLS Instrument | パート1・2・10を併置し、選択/スクロール/UndoRedo/挿入先が混線しない。ドラムの音名・キット・実発音、DLS collection/bank/program/参照解決/試聴/保存復元を確認。空collectionとCancelも保護 |
| SSP08 | 一連の操作と共通UI回帰（全採用番号） | 下記の連続シナリオを同一候補の本体で実行。必要な全登録回帰と原版比較を結合し、各段階の残差を回収 | native保存→通常終了→別process復元→再生/停止/再開、機能・外観・操作比較が同じ候補で成立。未比較・未実行・障害は残す。全体8/全40をこの経路だけで合格にしない |

SSP01のMIDI機器や出力先、DLS、エフェクトには環境依存がある。実在する機器/音源を列挙し、Noneや未接続の無効状態も確認する。スクリーンショットの機器名・20小節・サンプルレート等のセッション値を、無条件に工場出荷時の既定値としない。録音・全MIDI機能・全エフェクトが未完なら、バーを描画しただけの成立と分けて残す。

## 番号ごとの要求と受入確認

画像リンクはローカルの原本を参照する。ここでの「確認」は今後行う受入確認であり、製品での実施済み判定ではない。詳細な状態・証拠は[UI台帳](ui-compatibility.json)の `UIxx.priority-NN` と各単位へ保存する。

| 番号 | 画像 | 要求と受入確認 | 段階 |
| --- | --- | --- | --- |
| 1 | [1.png](../../screenshot/1.png) | Standard Toolbar。New/Open/Save等と、ツリー/子ウィンドウの表示・整列操作を原版配置で提供。Viewのチェックと表示が同期し、対象文書の有無で有効状態が変わる | SSP01 |
| 2 | [2.png](../../screenshot/2.png) | Transport Controls。先頭/現在位置からの再生、停止、録音、Tempo表示と速度倍率。停止/再生時のアイコン切替、文書テンポと試聴倍率の区別、実際の再生/録音を確認 | SSP01 |
| 3 | [3.png](../../screenshot/3.png) | Transport Options。AudioPath/エフェクト、メトロノーム、MIDI等のボタンと状態。MIDIアイコンから4番へ。実機能への接続と利用不可時の表示を確認 | SSP01 |
| 4 | [4.png](../../screenshot/4.png) | MIDI/Performance Options。MIDI入力/Echo/PChannels、voices/sample rate/default synth、latency、GM/Custom DLSのDownload/Unload等の欄と有効条件を再現。OK/Cancel・設定反映と再起動時保持の原版方針を確認 | SSP01 |
| 5 | [5.png](../../screenshot/5.png) | File New、Standard New、ツリー右クリックNewから同じCreate New File(s)。種類一覧、名前、Use Default Names、OK/Cancel。Segment選択が6→7へ進む。他種別の既存機能を失わない | SSP02 |
| 6 | [6.png](../../screenshot/6.png) | Segment Length。小節数/Clocktime、換算表示、Extension bars、pick-up表示、OK/Cancel。画像は20小節/40秒。OK後はTimeSig 4/4だけのSegmentと7番。数値境界と取消し時の作成状態を確認 | SSP02 |
| 7 | [7.png](../../screenshot/7.png) | Add New Tracksの種類一覧と選択。空選択でも進める。Band選択で8番へ。既存の17種の責務を残し、初期優先のBand/Tempo/Sequenceと他種別の進捗を区別 | SSP02 |
| 8 | [8.png](../../screenshot/8.png) | TimeSigとBandの2行、ラベル、時間軸と現在位置を表示。選んでいないTempo/Sequence等を作成済み行として紛れ込ませない | SSP02 |
| 9 | [9.png](../../screenshot/9.png) | Band空白位置の右メニューInsertとInsertキー。挿入可能/不可の状態とクリック位置の時刻を一致させる。Add/Delete TracksやProperties等の表示も記録 | SSP02 |
| 10 | [10.png](../../screenshot/10.png) | Band挿入後のBand Properties。Band名・位置を変更できる。画像のBar1/Beat1/Tick -1、Belongs Toを照合し、負tickを無根拠に0へ正規化しない。閉じる/変更/履歴/保存を確認 | SSP02 |
| 11 | [11.png](../../screenshot/11.png) | Bandイベントのダブルクリックで子編集画面。16 PCh行とInstrument等の列、pan/volumeビュー。初期は未有効。開閉で別Bandが増殖せず、埋込Bandの所有先が維持される | SSP02 |
| 12 | [12.png](../../screenshot/12.png) | Add New TracksでTempo追加。初期120.00、Tempo行・TimeSig・Bandを表示。既存Tempo編集/履歴/native機能へ接続 | SSP02 |
| 13 | [13.png](../../screenshot/13.png) | Sequence追加。グループを含む `1: 1 (1): Sequence` の表示と右端の開閉ボタン。パート名/PChannel表記を区別し、開閉してデータを変更しない | SSP03 |
| 14 | [14.png](../../screenshot/14.png) | 右端ボタン/ダブルクリックで鍵盤とピアノロール。activeは淡黄、inactiveは淡白。半透明の挿入位置、上下左右移動と赤い時刻バーの追従。フォーカス/音高/時刻を別状態として管理 | SSP03 |
| 15 | [15.png](../../screenshot/15.png) | ピアノロールの右メニューInsert Note/Insertキーで位置へ入力。Delete、Quantize、Edit Velocity、Snap To、Views、Zoom、Part/Track操作など画像内の隣接項目も台帳に残す | SSP03 |
| 16 | [16.png](../../screenshot/16.png) | ノートは青、選択は赤。クリックでその音高・楽器・velocityを試聴。選択の変更だけで音符内容/dirty/historyを変えない | SSP03 |
| 17 | [17.png](../../screenshot/17.png) | 上辺は上下矢印、矩形の高さがvelocity。ドラッグ変更ごとに変更後velocityで試聴。右メニュー/Insertで入力後、挿入位置は右へ1グリッド。velocity範囲と最小高さも確認 | SSP04 |
| 18 | [18.png](../../screenshot/18.png) | 右端は左右矢印で長さを連続変更しグリッドに依存しない。左端は開始位置を変更し基本Gridへスナップ。左右端の意味、最小duration、範囲外操作、取消しを確認 | SSP04 |
| 19 | [19.png](../../screenshot/19.png) | Snap ToのNone/Grid/Beat/Barline、既定Grid。画像のCtrl+E/G/T/Bも対象。Noneではクリック/左端/中央moveは連続時刻、左右キーで挿入位置は動かず上下は可。Gridでは位置がスナップ。中央/下辺は十字矢印で時刻/音高移動 | SSP04 |
| 20 | [20.png](../../screenshot/20.png) | Edit Velocity。Selected Notes/Part/All Parts In Sequence、Offset Start/End、%/Linear、Limit Range Min/Max、OK/Cancel。画像の実際の調整方式を再現し、単一の絶対値入力だけに置き換えない。範囲/丸め/選択対象の意味はHelpと動的観測で確認 | SSP04 |
| 21 | [21.png](../../screenshot/21.png) | Sequenceヘッダーの楽器アイコンメニュー。Insert New Instrument、Open Band Editor/Locate Bandの有効条件を再現。対象時刻/パートのBandへ接続 | SSP03 |
| 22 | [22.png](../../screenshot/22.png) | Insert New Instrument後、左下の楽器ボタンがPiano 1として有効になる。Band/PChannelのデータも更新し、二重挿入/UndoRedoを確認 | SSP03 |
| 23 | [23.png](../../screenshot/23.png) | 楽器ボタンからGM/GSの階層メニューとOther DLS。Organ→Organ 1を選べる。表示名とbank/program/variationの対応を保持 | SSP03 |
| 24 | [24.png](../../screenshot/24.png) | Organ 1のラベルと実音色へ変化。クリック試聴・Segment再生・保存復元後にも同じ割当が有効 | SSP03 |
| 25 | [25.png](../../screenshot/25.png) | 同じBandを開くとPCh1/Organ 1が有効表示。右のpan/volumeは未選択の状態。楽器の有効化と図の選択/編集状態を混同しない | SSP05 |
| 26 | [26.png](../../screenshot/26.png) | 左PCh1または右の1を選ぶと相互に選択が同期し、右の1は赤。文書やパート切替でも誤ったPChを編集しない | SSP05 |
| 27 | [27.png](../../screenshot/27.png) | 図の1をドラッグしてpan/volume。画像例はPan L2、Volume 102。座標→値→ラベルと再生を一致させ、左右端/上下端・履歴/取消しも確認 | SSP05 |
| 28 | [28.png](../../screenshot/28.png) | 選択解除後も1の濃いラベルと位置は保持。有効/未有効と選択/非選択を別管理。値を保存し再読込できる | SSP05 |
| 29 | [29.png](../../screenshot/29.png) | Sequence下のCCバー右クリック→Add New CC Track...。対象パートに追加し、CCなし時のDraw Curve等の無効状態も一致 | SSP06 |
| 30 | [30.png](../../screenshot/30.png) | Add New CC Trackのコンボ/OK/Cancel。取消しでレーンやデータを追加しない | SSP06 |
| 31 | [31.png](../../screenshot/31.png) | PBend、MonoAT、PolyAT、RPN、NRPN、各CC等の種別一覧を照合し、まずPBendを追加。種別ごとの値域/parameterを同一の7bit CC扱いにしない | SSP06 |
| 32 | [32.png](../../screenshot/32.png) | 閉じたCCバーにPB要約を表示。開閉とレーンデータを独立させる | SSP06 |
| 33 | [33.png](../../screenshot/33.png) | 右端ボタン/ダブルクリックでPBendレーン展開。中心線、時間軸、赤い位置バーがピアノロールと一致 | SSP06 |
| 34 | [34.png](../../screenshot/34.png) | PBend右メニューDraw CurveまたはInsertで描画モード。通常選択との切替、focus、Esc/取消しの原版方針を確認 | SSP06 |
| 35 | [35.png](../../screenshot/35.png) | クリック/ドラッグ中に破線のプレビュー。画像にhoverカーソルが写っていない点を保持し、カーソル形状は追加観測で確定。途中の描画で文書を多重commitしない | SSP06 |
| 36 | [36.png](../../screenshot/36.png) | ドラッグ完了で青いPBend曲線。原版の曲線形・始終値・時間範囲を保持し、履歴/native保存/再読込と実際のpitch bendを確認 | SSP06 |
| 37 | [37.png](../../screenshot/37.png) | PBendとCC11 Expressionを同時追加・展開・編集。レーン固有の範囲、選択、図形、再生効果とパート所有を保持 | SSP06 |
| 38 | [38.png](../../screenshot/38.png) | Add Sequence Part(s)→Add New Part。PChannel 2、unused等の状態、名前、Create New Part/Close。ダイアログを閉じるまで複数追加できるかを確認 | SSP07 |
| 39 | [39.png](../../screenshot/39.png) | パート1の下に `1: 2 (2): Sequence`。PChannel表示と内部値の対応、既存ノート/CC/楽器の所有を保持 | SSP07 |
| 40 | [40.png](../../screenshot/40.png) | 複数パートのピアノロールを同時展開・編集。CCも併置可能。画像でCCが閉じていることを同時表示の禁止と解釈しない。各パートの高さ/スクロール/active状態を管理 | SSP07 |
| 41 | [41.png](../../screenshot/41.png) | Part10の既定ドラム割当とGMドラム音名付き鍵盤。表示10と内部PChannel、打楽器フラグの対応を照合。音名・ノート番号・実発音を一致させる | SSP07 |
| 42 | [42.png](../../screenshot/42.png) | Part10はStandard/Room/Power/Electronic/TR-808/Jazz/Brush/Orchestra/SFX等のドラムキットとOther DLS。通常GM楽器一覧との切替、bank/program/percussion属性を保持 | SSP07 |
| 43 | [43.png](../../screenshot/43.png) | Choose DLS Instrument。DLS Collections、Instruments、Bank Hi/Lo、Patch、OK/Cancel。画像はcollectionあり/instrument一覧空であり、選択成功の証拠にはしない。実データ入りfixtureで選択/試聴/参照保存復元まで確認 | SSP07 |
| 44 | [44.png](../../screenshot/44.png) | ツリーのSegment→右メニューPropertiesで46番へ。ファイル/内部componentの選択先と既存F11共通Propertiesの対応を確認 | SSP02 |
| 45 | [45.png](../../screenshot/45.png) | **除外。TimeSig Track Propertiesの誤撮影。Segment Propertiesの期待値に使わない**。元画像と番号は保持 | 対象外 |
| 46 | [46.png](../../screenshot/46.png) | Segment Properties。Segment/Loop/Info/Boundaryタブ、Name、Lengthボタン、Start Time/Track Group等の配置。今回の経路はLength→47番。既存タブ機能/残責務も失わない | SSP02 |
| 47 | [47.png](../../screenshot/47.png) | LengthボタンでSegment Lengthを再表示し長さを変更。6番とdialogを共用し、既存文書の拍子/テンポで換算する（新規の4/4・120既定値と区別）。短縮時の範囲外ノート/CC/ループ/再生終端の原版方針、Cancel/UndoRedo/native復元を確認 | SSP02 |

## データと操作の実装方針

共通の文書モデルを正本とし、ツリー/各toolbar/右メニュー/キーは同じコマンドへ接続する。SegmentのトラックとSequenceのパート、CCレーン、Bandイベント、表示の開閉状態を区別する。描画座標・スクロール量・DPIを一貫してhit判定と時刻/音高へ変換し、ラベル領域や他パートへの誤挿入を防ぐ。

ノートの上端/右端/左端/bodyを分け、重なりや極短ノートの優先hit規則を原版観測に合わせる。挿入カーソル、選択ノート、再生位置を別管理し、必要な追従だけを接続する。ドラッグは一操作としてUndoでき、途中プレビューとcommitを区別する。Cancel/不正入力ではbytes・dirty・UndoRedoを変えない。選択/表示開閉だけで文書を変更しない。CC/楽器/長さの変更も所有文書の履歴へ統合する。

画面の16行やPart10の慣例を、内部PChannel全体の16制限へ戻す理由にしない。Q0のDWORD許可値、16以上、4予約値、不正入力時不変を保護する。UIの1起算表示、MIDI channel、PChannel、track groupを明示的に変換し、別パートのデータを一つの配列へ混ぜない。

GM/GSは表示名だけでなくbank/program/打楽器属性、DLSはcollection identity/参照/bank/programを保持する。Sequenceヘッダーと埋込Band編集は同じ割当を更新し、元の外部文書を無断で書き換えない。未知chunk/拡張record、既存CC/curve、他トラックと既存入力を保護する。原版DLL/OCXや改造コピーを製品の実装依存へ追加しない。

## 連続シナリオと検証

1. Viewで3バーを表示/非表示し、3つのNew入口から同じダイアログへ進む。空選択/取消しを確認後、新しい検証用ProjectとSegmentを作る。TimeSig 4/4、Band、Tempo 120を用意する。
2. Bandイベントを位置指定して挿入し、Properties→Band子ウィンドウ→Segmentへ戻る。Sequenceパート1を追加・展開する。Insert/右メニューでノートを入力し、色・位置移動・クリック試聴を確認する。
3. Insert New Instrument→Piano 1→Organ 1。上端/右端/左端/bodyを各Snap設定で操作し、Edit Velocityの各対象範囲を試す。UndoRedoと取消しで内容・dirty・履歴を確認する。
4. BandでPCh1と図の1を選びpan/volumeを変更、選択解除。Sequenceへ戻り、同じ楽器・値で試聴/再生されることを確認する。
5. PBendとExpressionを追加・展開し、Draw CurveとInsertの両入口で曲線を作る。パート2/10を追加し、複数ピアノロール/CCを併置して編集先の分離、ドラムキット、実データ入りDLS選択を確認する。
6. SegmentツリーのProperties→Lengthで長さを変更。native Project/Segmentと必要なBand/DLS依存を保存し通常終了、別processで再読込する。ノート・曲線・PChannel・楽器・pan/volume・長さを独立RIFF監査で照合し、再生/発音中Stop/無音/再開を自動録音で判定する。

各段階では変更に対応する意味のある回帰を選ぶ。Sequence/Timeline/Band/DLS/Framework/Conductorの関連試験を再利用し、スナップの時刻変換、ノートジェスチャー、パート分離、CC曲線値域、長さ変更の境界等の不足だけを追加する。計画変更だけでは製品buildや全回帰を実行しない。機能群の節目と最終候補では現行manifestの全登録群を実行し、凍結/失敗/未実行を合格へ混ぜない。

本体試験では画像と操作ログを対で残し、メニュー/キーの同じ結果、フォーカス、active/inactive、有効/無効、保存/復元と必要音声を同じ候補に結ぶ。外観と操作の結果は別判定とする。音色はbank/program/DLS割当の監査と再生証拠を合わせ、音声スペクトルだけでGM名を確定しない。PBend/Expression/velocity/pan/volumeは既存[音声契約](audio-automation-contract.md)に従う対照入力とPCM解析で効果を確認し、人の聴取待ちにしない。

原版で未観測の細部は、該当するCHM話題と必要な限定操作で補い、ユーザー説明と食い違う場合は根拠・差分・採用理由を記録する。必要な比較が止まっても独立した次段階は進められるが、その段階の比較未確認は残す。画面の描画成功やモデル単体試験だけで段階全体の完了にしない。

## 状態の更新と次の一手

SSP08終了時点の220927同一候補で、連続シナリオを限定確認した。276保存/作業ソース・4出力一致、関連13/952、原始native98は51合格2903検査/46OS凍結/既知Band1保留、driver170は20合格/28OS凍結/122未実行。新規Project/Segmentからノート編集、Band pan/volume、PBend/Expression、parts2/10、DLS、TR-808、Length20→24小節へ進み、独立native監査11+15+16+22、作者19856/復元7748の二通常exit0、Project/Segment/DLSの再保存bytes一致を確認した。50%固定PCMは16/16で発音・DLS440Hz・発音中Stop・無音・再開が成立。通常速度の2録音はStop操作時刻が不成立で不合格のまま保持する。

過去のSSP08入口は[SSP08単位記録v4](../../work/analysis/ui-priority-scenario/20261010T225536878Z/unit-record-v4.json)と[SSP07複数パート報告](ui-sequence-parts-2026-10-11.md)。245本体操作、別の音声8操作、全録音・失敗判定・修正前判定器を保存している。50%は独立条件であり、通常速度の失敗を置き換えない。79ロードmoduleの原版40hash一致0は点観測で、Q2成立を意味しない。

現行部分入口は20261012T004853599Z Current Tempoの停止中Segment編集を実装。正式原版で132.25の保存/UndoRedo、spin上133・下131、bar2から既存time0 markerの編集を観測しnative7合格。新候補configure/build/install0、291保存/作業ソース・4出力一致、関連4件/439検査合格。segment-creationは既知Mixer.sgp Windows5のため未実行・障害維持。候補本体/native二process/音声/追加原版契約は未検証。ユーザー指示で休止。 [休止記録](ui-current-tempo-pause-2026-10-12.md)。fullAcceptance=false。 次：ユーザーが再開を指示するまで作業を進めない。再開後は004853の保存/作業291ソース・4出力・入力を照合し、現在の低解像度と最新windowを再観測。正式原版のEscape/focus-loss/不正値契約の未確認を保持し、004853本体を専用candidate Projectで初起動→132.25入力/Enter/UndoRedo/spin/文書所有権→native保存→通常終了→別process復元/通常終了→固定WASAPIテンポ/発音中Stop/無音/replay比較へ進む。保存済み235833空CC・224147一巡/CC15/16を新候補の実行証拠へ転用しない。49native/30driverの凍結、SSP04親/6640破棄回答待ち、独立Style015244/Q2/全40/全8を保持。 画像44→46→47を採用、45を除外。

各段階の残件は画像索引とUI台帳へ保持する。SSP01はMIDI入力/録音/Echo/PChannels、Download/Unload、Play CursorのElapsed表示/同操作原版比較/全外観、Transport List/Transition/編集BPM、全AudioPath/多ポート、配置/アイコン/字体/ドッキング。SSP02は末尾切詰め/全Properties/新Project衝突/Pattern/Wave、Band同論理時刻/Cycle/clipboard/全PChannel。SSP03〜07は全gesture、preview途中/取消し、全CC種類と形状、空CC lane、原版psql互換、全ドラム/DLS選択履歴などが未完である。旧候補の初回発音1秒超過、SSP06 Stop時刻失敗を新候補の成功へ転用しない。

46native/28driver OS凍結は解除条件が変わるまで同条件再試行しない。限定採用0735、独立Style015244固定PCM/同候補一巡、Q2別Windowsを保全し、全40・全8は6作業中/2障害/0合格、fullAcceptance=falseを維持する。旧本文は[改訂前計画](../../work/analysis/ui-priority-scenario/20261010T225536878Z/progress-before-v4/priority-segment-sequence-2026-10-10.md)に保持。[継続プロンプト](continuation-prompt-2026-10-10.md)とcurrentが再開入口となる。

## Play Cursor後の優先順位の見直し

Metronome→Count-In→Play Cursor→Viewport→MIDI Input→基本MIDI Echo→初回私有試聴→Sequenceベロシティ→Band固定PCM→New Files→Add Tracks→保存キー→Segment Propertiesの限定作業を保存済み。画像18・19のSequenceノート左右端・中央移動とGrid／Noneを、専用コピーの同じ入力で原版と比較する。確認した差だけ修正し、UndoRedo/native保存/別process復元/通常終了を固定する。発音はPCMを取得した条件だけ評価する。Properties残欄、Add Tracks初期focus/原版空選択、旧Band停止失敗・Count-In15/16・Echo/SSP04全操作/SSP08・Style015244/Q2/OS凍結を保持。

054337で画像(3)・(4)のMIDI Input列挙/None/未接続設定保持/本体保存復元を限定確認した。実機0件で実受信成功とはしない。045647の表示/Fit/Zoom/横追従、040727のPlay Cursor、031359のCount-In失敗PCMは各単位の履歴を保全する。原版のEcho/Advanced/Downloadは有効であり、未実装を機器なしという環境差へ置き換えない。


080641・追加ソース変更なし。既存4段階ベロシティ300秒PCM12/12、初回80=0.495秒/96=0.480秒/上端preview=0.505秒/40=0.485秒、音量比96/80=1.437・40/80=0.248、最終無音成立。Undo80・8notes保存復元・作者22876/別21724の二通常exit0、独立監査20/20。SSP04全gesture/原版外観操作、Band300秒等は未達。 288ソース/4出力一致。同候補の直前一巡native100=53合格2965検査/46OS凍結/既知Band1保留、driver172=20合格/28OS凍結/124未実行を参照（今回再実行なし）。限定採用0735、独立Style015244、全体6作業中/2障害/0合格、fullAcceptance=falseを維持。画像16・17は既存ベロシティ4段階PCMを再検証した。20はダイアログ開閉のみ。SSP04全gestureとミキサー固定300秒・19条件は未検証。

## 2026-11-09 再開の現行入口

休止解除済み、goal active。050149同一候補で停止中SegmentのCurrent Tempo入力/Enter/文書UndoRedo/spin/原版Escape保留とfocus-loss確定/1..1000正規化/空欄/Peer追加直後の表示と編集所有権を確認。関連5件496検査、native14状態+Peer5状態、作者14160/別12000の二通常exit0、別起動保存復元成立。132.25/120 BPMの実PCMは各17発音・音程・発音中Stop・無音・replay成立、間隔中央値0.455/0.500秒、負例5件拒否。全登録native100は50合格2830検査/49既知OS凍結/匿名Band1保留、新実行失敗なし。driver172は一覧参照のみで新一巡ではない。

SSP01を継続。現環境の原版/CHMからTransport Listの対象と停止中/編集対象切替/再生の契約を限定観測し、同じ選択・配置・Current Tempo連動を実装→保存build/関連回帰→本体/保存/二通常終了/別復元/必要PCMへ進む。Style/live/secondary・録音/Transition・厳密外観・全40/全8/Q2・既知OS凍結を保持。

一次入口：`work/analysis/ui-current-tempo-resume/20261109T035725Z/unit-record-v3.json`、報告：`docs/analysis/ui-current-tempo-resume-2026-11-09.md`。全体fullAcceptance=false。旧休止指示・旧PIDは履歴。

現行の継続作業：Current Tempo050149の限定機能/保存復元/二通常exit0/132.25対120 PCM/全native100一巡を保存済み。SSP01の独立作業としてCurrent Tempo→速度倍率の順序とpaired Play/Stop glyphの識別を修正中。054537初回保存buildの型宣言誤りを保全し、修正版保存build→関連回帰→本体glyph/順序/View再配置/Tempo/保存/二process/必要PCM。Transport Listの動的原版比較は同じPID1984の起動待ち、再起動せず保留。 一次入口：`work/analysis/ui-transport-list/20261109T054141550Z/unit-record-v1.json`。全体fullAcceptance=false。

継続作業の訂正（055400）：055400保存build→関連回帰→paired glyph/Tempo左・Modifier右/View再配置/1000.00と編集UndoRedo→native保存/二通常exit0/別復元→2つのPlay入口とImmediateStop固定PCM。原版PID1984は通常exit0、Listが編集対象と別である限定契約を保存。List実装/録音/Transition/厳密外観/全40/全8は未達。 一次入口：`work/analysis/ui-transport-list/20261109T054141550Z/unit-record-v2.json`。全体fullAcceptance=false。

現行の継続入口（055400検証後）：055400でCurrent Tempo→Modifierの順序、FromStart/PlayとImmediateStop/BoundaryStopのglyph識別、Standard非表示/再表示後の入力・CtrlS、最大1000.00、UndoRedo/spinを確認。関連5/496、8Seed+8Peer native状態、291保存/作業ソース・4出力一致。作者6156/別2868の二通常exit0、別起動復元/再保存成立。2つのPlay入口の固定132.25 PCMは17発音/音程/Stop/無音/replay合格。 SSP01 Transport Listを実装する。編集対象とprimary再生選択を分け、停止中の選択はTempo表示へ反映し、再生中は現在primaryのTempoを維持する。編集窓の再アクティブ化でprimary選択を更新。Segmentの限定登録/選択/文書owner/FromStartとPlay/保存二process/必要PCMを閉じ、録音/Transition/他component/厳密外観は残す。既知OS凍結は条件変更なしで再試行しない。 一次入口：`work/analysis/ui-transport-list/20261109T054141550Z/unit-record-v3.json`。全体fullAcceptance=false。

現行の継続入口（Transport List実装）：062231 Transport ListのSegment限定実装を保存build中。構文検査exit0。編集文書とprimary選択の分離、selectedPeerのTempo125編集owner、編集再アクティブ化、View、native2文書保存/二通常終了/別復元、再生中132.25保持→次Play120の固定PCM、全native100節目へ進む。055400の完了証拠を転用せず保持、全40/全8/fullAppearanceは未達。 一次入口：`work/analysis/ui-transport-primary/20261109T062110913Z/unit-record-v1.json`。全体fullAcceptance=false。

現行入口（062231本体/音声後）：062231はconfigure/build/install0、関連5件496検査、7Seed+7Peer native状態と17notes保持、2process保存復元/二通常exit0。選択Peerと編集Seedの分離、Peer125 focus-loss確定→Seed再primary、PeerUndo120、View hide/showを確認。再生中Seed132.25維持→次PlayPeer120の固定PCM合格、中央値0.455/0.500秒、負例6件拒否。全native100節目を実行中、全体fullAcceptance=false。 062231同候補native100一巡の終了/全入力と保存ソース・4出力を照合して状態/UI台帳へ記録し、Transport Listの名前fallback/他component登録・全外観等の独立次作業を継続。既知49native/30driver凍結や匿名Band失敗の同条件再試行、全40/全8の昇格はしない。 一次入口：`work/analysis/ui-transport-primary/20261109T062110913Z/unit-record-v2.json`。

## 2026-11-09 現行再開点（062231検証後）

休止解除済み、goal active。062231のSegment限定Transport Listで、編集Seed132.25とprimaryPeer120の分離、選択先Peer125のfocus-loss確定/Undo120、編集再アクティブ化/View/空Project無効状態を確認。関連5/496、7Seed+7Peer native状態・17notes保持、作者8932/復元9436の二通常exit0と保存復元成立。固定PCMは再生中Seed132.25保持→次PlayPeer120、17発音/音程/activeStop/無音、中央値0.455/0.500秒、負例6件拒否。全native100は50合格2830検査/49既知OS凍結/匿名Band1保留。30入力（7directory）・291ソース・4出力一致、新実行失敗なし。driver172は旧一覧参照のみ。

SSP01継続。実画面で確認したUntitled SegmentのTransport List名の空表示を、ツリー/編集対象と同じfallbackへ修正し、保存後ファイル名/文書ownerの保持を限定検証する。続いてTransport Listの他component/開閉lifecycle、録音/Transition、Style/live/secondary、全外観の独立未達へ進む。全40/全8/Q2/Style015244と既知OS凍結・匿名Band失敗を保持し、条件が変わらない再試行はしない。

一次入口：`work/analysis/ui-transport-primary/20261109T062110913Z/unit-record-v3.json`。全体fullAcceptance=false。旧休止指示・旧候補/PIDは履歴。

## 2026-11-09 現行再開点（070706原版差分）

070706の名前fallback試行は関連5件472検査、作者8276/復元19412の二通常exit0、6native状態・17notes・全Segment bytes、Projectの改名/FILETIME以外保持が成立。現行1280×800/96DPI・6control中心hit一致。原版19404も通常exit0。ただし原版の保存済み無名Segmentはcomponent空表示であり、SaveAsは別GUID追加・元ファイル/Project参照維持。候補のnative名fallback/パス移動とは異なり、原版互換単位は未完。

SSP01継続。070706は検証済み試行として保全し、保存済み無名SegmentのTransport Listを原版同様の空componentへ戻す。fallbackは未保存Untitledだけへ限定し新候補の起動/保存/通常終了/別復元で確認。その後、原版File SaveAsの別文書追加lifecycleを、既存Frameworkパス移動APIとは別のUI契約として検討する。全component/録音/Transition/厳密外観/全40/全8は未達。既知49native/30driver凍結・匿名Bandの同条件再試行なし。

一次入口：`work/analysis/ui-transport-labels/20261109T070235697Z/unit-record-v2.json`。全体fullAcceptance=false、goal active。

現行継続入口（072804構築中）：072804は原版で確認した保存済み無名Segmentの空component表示へ修正し保存build中。未保存Untitledのみ生成名を補い、起動→AName空名/Peer名前/独立Tempo選択→native保存/通常終了/別復元/再保存・全bytesを確認。SaveAs別文書追加の原版差は独立残件。全体fullAcceptance=false。 一次入口：`work/analysis/ui-transport-label-parity/20261109T072748612Z/unit-record-v1.json`。

## 2026-11-09 現行再開点（072804検証後）

072804で保存済み無名SegmentのTransport Listを原版同様の空component表示へ戻した。生成名fallbackは未保存Untitledだけ。関連4件213検査、作者/復元の二通常exit0、AName132.25/Peer120の独立選択、native全bytes/17notes/Project FILETIME以外保持・291ソース/4出力一致を限定確認。新PCMは表示変更だけのため不要、過去PCM結果は転用しない。

SSP01→Segmentの文書lifecycleを継続。原版File SaveAsは別GUIDの文書をProjectへ追加して元文書を保つため、既存Framework save_segmentパス移動APIを保ち、別のUI用コピー保存を実装・関連回帰・本体操作・native保存/通常終了/別復元で検証する。072804の限定名表示は確認済み。全component/録音/Transition/厳密外観/全40/全8/Q2/Style015244は未達、既知49native/30driver凍結・匿名Bandの同条件再試行なし。

一次入口：`work/analysis/ui-transport-label-parity/20261109T072748612Z/unit-record-v2.json`。全体fullAcceptance=false、goal active。

## 2026-11-09 現行再開点（074348 SaveAsコピー構築中）

074348はFile SaveAsの別GUID/別owned Segment追加を実装しbuild中。common-openにsource履歴/unknownbytes/衝突/保存失敗保全/Project復元のAPI検査を追加、manifest dispatcher hash更新。構文v1 C3538を保全してv2通過。次：保存build→関連回帰→AName editor/Peer primaryからZNameコピー、3文書所有権/Undo保存、通常終了/別復元native照合、必要音声判定・機能群一巡。072804の名前表示/旧候補音声を転用せず全40/全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-saveas-copy/20261109T074237460Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（074348 native成功/音声ロード失敗）

074348のFile SaveAsは別GUIDの新文書を追加し、元AName132.25/Peer120を保ち、コピーのみ137→Undo132.25→Redo137/保存を確認。関連5件493検査、作者10564/復元6724の二通常exit0、7native状態・17notes・source/Peer全bytes・copyのGUID/宣言Tempo以外・3文書Projectを照合。ただし最初PlayがLoad current DMSG 0x88781184で失敗し音声未達。録音120秒/時計4負例は保全し、同じ入力の再生は再試行しない。

074348の音声失敗から継続。無音声の条件変更診断でSegment生成/UNAM追加版/空UNFO除去版はS_OK。保存bytesを維持し、private loader copyから空の任意UNFOのみ除く修正を新候補で検証する。SDKヘッダー欠落の診断build失敗も保全。新候補の関連回帰/3文書GUI保存/二通常終了/別復元/137 PCMと負例/全native100節目へ進む。原版のdisabledTempo/dirty/layout厳密一致、他component/全外観/全40/全8/Q2/Style015244/49native・30driver凍結は未達のまま。

一次入口：`work/analysis/ui-segment-saveas-copy/20261109T074237460Z/unit-record-v2.json`。全体fullAcceptance=false、goal active。

## 2026-11-09 現行再開点（081250 private空UNFO構築中）

081250は074348の新コピー音声ロード失敗に対し、空の任意UNFOをprivate loader bytesだけから除く変更で構文検査通過・保存build中。失敗時と同じ3文書native入力のbytesを保ち、新候補で関連回帰→3文書本体選択/保存/通常終了→別復元/再保存→137固定PCMと負例→通常終了/全native100節目を検証する。FileSaveAs操作の074348証拠は候補を分け、全40/全8/fullAppearance/fullAcceptance=false・49native/30driver凍結/匿名Band保留を保持。

一次入口：`work/analysis/ui-segment-empty-unfo/20261109T081231083Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（081250検証後・起動New Filesへ）

081250は保存済み空UNFOをnativeに残し、再生用コピーからだけ省く修正を検証。関連5件497検査、作者19008/復元7736の二通常exit0、12native状態・各17notes・全Segment bytesとProject FILETIME以外を保持。137 BPMの2Play/発音中14秒Stop/無音PCM合格、発音間隔中央値0.440/0.440秒、負例5拒否、時計4負例。全native100は50合格2855検査/49既知OS凍結/匿名Band1保留。30入力（7directory）・292ソース・4出力一致。

SSP01→SSP02の起動/New Files lifecycleへ継続。原版引数なし起動PID3984を同じPIDで初期化待ちし、NoProjectのactive editor/primaryとNew/Cancelを観測。候補の自動Untitled Segment追加と空状態dirty/close/新規1文書の操作を照合し、根拠に従って修正・保存/通常終了/別復元へ進む。原版のSaveAs後Tempo disabled/dirty/全layout、他component/Recording/Transition/全40/全8/Q2/Style015244は未達。既知49native/30driver凍結・匿名Bandの同条件再試行なし、全体fullAcceptance=false。

一次入口：`work/analysis/ui-segment-empty-unfo/20261109T081231083Z/unit-record-v2.json`。goal active。

## 2026-11-09 現行再開点（084341 起動/New Files構築中）

084341は原版NoProject/NewCancelの一次比較から、自動Segment追加を止め、起動直後の空workspaceだけにUI所有pristine判定を導入してbuild中。原版3984/旧候補081250 baseline11032は通常exit0。次：関連回帰→空起動/NewCancel/無確認終了→別作者で最初Segment1を1文書だけ作成20小節/40秒TimeSigのみ、dirty保護/native保存/通常終了/別復元・再保存。segment-creationの同条件Mixer.sgp失敗は実行しない。旧原版専用.pro自動更新の記録を保全、以後original作業先と固定証拠を分ける。全40/全8/fullAcceptance=false。

一次入口：`work/analysis/ui-startup-newfiles/20261109T083128897Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（084341 起動/New Files検証後）

084341は自動Segment追加を止め、原版同様に空起動/NewCancel/無確認終了を実現。関連4件238検査、空5500/作者8652/復元15300の三通常exit0。最初Segment1を1件だけ作成し、20小節/40秒・TimeSig4/4のみ、作成後の破棄確認とNo保持、native保存/別復元/再保存を確認。292ソース/4出力一致、Segment全bytesとProject FILETIME以外保持。音のない作成内容とframe変更のため新PCMは不要、過去PCMを転用しない。

SSP01→SSP02継続。空workspaceのlegacy編集フォームを隠し、原版のdisabled CurrentTempo120表示へ合わせる独立作業へ進む。NewCancel/View再配置/新規文書のdirty保護/保存/通常終了/別復元を確認する。AddTracksは実UIでBand初期選択をSpace/CtrlSpaceで解除できずCancelしたため空選択OK未達。厳密外観・全component/Recording/Transition/全40/全8/Q2/Style015244、既知49native/30driver凍結・匿名Bandを保持し同条件再試行しない。全体fullAcceptance=false。

一次入口：`work/analysis/ui-startup-newfiles/20261109T083128897Z/unit-record-v2.json`。goal active。

## 2026-11-09 現行再開点（091437 空frame構築中）

091437は空workspaceのlegacy編集欄を非表示にしCOLOR_APPWORKSPACEで描画、targetなしCurrentTempoはdisabledのまま原版120表示へ変更してbuild中。次：関連4件→空起動/NewCancel/View/treeの再配置/無確認通常終了→別作者で最初Segment1作成後に編集欄復帰/dirty保護/保存/通常終了→別復元/再保存。084341の検証は候補を分けて保全。全40/全8/fullAppearance/fullAcceptance=false、49native/30driver凍結と匿名Bandの同条件再試行なし。

一次入口：`work/analysis/ui-empty-frame/20261109T091347255Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（091437 空frame検証後）

091437は空workspaceの編集欄103個を隠し、作業領域をシステムgray、targetなしCurrentTempoをdisabled120へ修正。関連4件238検査（2run）、空18412/作者3780/復元17648の三通常exit0。NewCancel/Tree/View Transport再配置を保持し、明示作成した空ProjectとSegmentの両方で破棄確認/No保持を確認。最初Segment1の20小節・TimeSig4/4のみ、native保存/別復元/再保存でSegment全bytesとProject FILETIME以外一致。292ソース/4出力一致、音を含まないframe変更で新PCMは不要。

SSP01→SSP02継続。保存Projectの初期editor/primary、最後のSegment編集窓Close/Reopenの原版契約を追加比較する。原版PID15872は専用StartupProof作業コピーで起動し、レジストリ更新失敗通知がSkyに列挙されず手動OK待ち。同じPIDを保ち重複起動/登録再変更/通知の偽handle操作をしない。原版比較が止まる間も独立した製品側作業を進める。全40/全8/fullAppearance/fullAcceptance=false、49native/30driver凍結/匿名Bandの同条件再試行なし。

一次入口：`work/analysis/ui-empty-frame/20261109T091347255Z/unit-record-v2.json`。goal active。

## 2026-11-09 現行再開点（091437 Tree New/Length Cancel検証中）

091437同一候補で独立したTree右クリックNew→Segment Lengthの30小節入力→Cancel→既定20小節/TimeSigのみ保持とAddTracks続行→native保存/通常終了/別復元を本体検証中。新ソース変更なし。関連4/238は同候補の既存2runを参照し新実行ではない。原版15872の起動通知はツール列挙されず手動OK待ち、同じPIDを維持。全40/全8/fullAppearance/fullAcceptance=false、既知49native/30driver凍結・匿名Band同条件再試行なし。

一次入口：`work/analysis/ui-segment-length-cancel/20261109T093620584Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（091437 Tree New/Length Cancel検証後）

091437同一候補でTree右クリックNew→最初Segment1のLength未確定30→Cancel→AddTracks→Cancelを本体確認。作成済み20小節/TimeSig4/4のみ・空UndoRedoが残り、作者20272/別復元20904は二通常exit0。native全Segment bytes/Project FILETIME以外と292ソース/4出力一致。ソース変更/新build/新PCMなし。関連4/238は同じ候補の既存2run参照で新実行ではない。原版契約は110746時の正式原版観測を参照し、新しい同入力比較とはしない。

SSP01の原版見た目/操作を継続。保存済み正式原版のメニューバー先頭File/Edit/View/Add-Insを照合し、候補の先頭Transport AudioPathとFile mnemonic欠落を限定修正する。実機の原版15872は起動通知がSkyへ列挙されず手動OK待ちで同じPIDを維持。独立したメニュー修正/保存build/関連/本体/保存/通常終了/別復元へ進む。全40/全8/fullAppearance/fullAcceptance=false、49native/30driver凍結と匿名Bandの同条件再試行なし。

一次入口：`work/analysis/ui-segment-length-cancel/20261109T093620584Z/unit-record-v2.json`。goal active。

## 2026-11-09 現行再開点（095232 frame menu構築中）

095232はFile/Edit/View/Add-Insの先頭順序とFile mnemonicを修正し、既存AudioPathメニューをView内へ移してbuild中。次：関連→空起動AltF/NewCancel/View下AudioPath/通常終了→専用保存文書のmenu操作/Cancel/保存/通常終了→別復元/再保存。原版15872はユーザーの通知OK後に編集窓開閉/primary保持/NoTempo空欄/再Open AddTracksを観測して通常exit0。native作業出力は固定保存済み。この新契約は次の独立修正へ残し、現menu候補の厳密NoTempo/全外観合格へ転用しない。全40/全8/fullAcceptance=false、49native/30driver凍結・匿名Bandの同条件再試行なし。

一次入口：`work/analysis/ui-frame-menu/20261109T095015925Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（100901 menu key修正構築中）

095232メニュー試行は関連4/238・File先頭/動的View AudioPath/空通常exit0だがAltF未達を保全。100901はframe system key/F10をIsDialogMessage前へ渡す修正でbuild中。既存AltShift再生キーは先に処理し保持。次：関連4件→空AltF/NewCancel/F10/View下AudioPath/通常終了→専用TimeSig-only保存文書のCtrlN/Cancel・2再生キーの無ノートdispatch/runtime・保存/通常終了→別復元/再保存。原版15872は通知OK後の編集窓開閉/primary保持/NoTempo空欄/再Open AddTracksを固定保存し通常exit0。NoTempo120誤表示/初期primary/最後Closeのgray/再Open promptは次の独立修正、全40/全8/fullAppearance/fullAcceptance=false。既知凍結49native/30driver・匿名Band同条件再試行なし。

一次入口：`work/analysis/ui-frame-menu/20261109T095015925Z/unit-record-v3.json`。goal active。

## 2026-11-09 現行再開点（100901 menu key検証後）

100901はFile/Edit/View/Add-Insの先頭順序とView内AudioPathを実現し、Alt/F10をnative frameへ渡す修正を検証。関連4件238検査、空16236/作者11488/復元20764の三通常exit0。AltF→N/NewCancel、F10→Down、AltV→A→Enter、マウスNewCancel、MDI/Edit focusからのFile保存が成立。既存2再生キーは無ノートSegmentでLoad/Play/Stop各2回成功・停止後IsPlaying false。native初期/作者/復元のSegment全bytesと3Project FILETIME以外、292ソース/4出力一致。新PCMなし、音声受入への転用なし。

SSP01 CurrentTempo表示を継続。原版15872の固定比較ではprimaryなしはdisabled120、TimeSig-only primaryありはdisabled空欄、最後の編集窓Close後もprimary/Play/空欄保持。091437の全noTempo120fallbackを、primaryなしだけ120へ狭める独立修正へ進み、numeric BPMとの切替/保存/通常終了/別復元を検証する。初期primary登録/最後Closeのgray/TimeSig-only再Open promptは別残件。全40/全8/fullAppearance/fullAcceptance=false、49native/30driver凍結と匿名Band同条件再試行なし。原版15872は通常exit0、通知手動対応待ちは解消済み。

一次入口：`work/analysis/ui-frame-menu/20261109T095015925Z/unit-record-v4.json`。goal active。

## 2026-11-09 現行再開点（103640 primary-NoTempo表示構築中）

103640はprimaryなしの120表示を保持し、Tempoなしprimaryありはdisabled空欄へ狭めるtoolbar表示修正でbuild中。Main/選択/keyboard/runtime/編集モデル変更なし。次：関連4→空起動120/NewCancel/通常終了→TimeSig-only Segment1空欄→GUI InsertFilesでSeed132.25/17notes追加/独立選択/numeric切替/View/NoTempo編集窓Close→2文書native保存/通常終了→別復元/再保存/通常終了・全bytes照合。原版15872の空欄/Close primary保持を参照し、初期登録/gray/reopen promptは別未達。音声は表示だけで新PCM不要・過去結果転用なし。全40/全8/fullAppearance/fullAcceptance=false、49native/30driver凍結・匿名Band同条件再試行なし。

一次入口：`work/analysis/ui-tempo-target-display/20261109T103529762Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（103640 NoTempo表示検証後）

103640はprimaryなしの無効120、Tempoなしprimaryの無効空欄、Tempoあり132.25の編集可能表示を原版の契約に合わせた。関連4件238検査。空2676/作者6452/復元21216は通常exit0。primaryだけ切替えても編集文書/空履歴を保持し、Save Fileは編集中Seedだけを書いた。Transport再表示とNoTempo編集窓Close後も空欄を保持。2文書native保存/別復元/再保存を通し、両Segment全bytesとSeed17音符、Projectの既存参照を保持。native100は50合格2855検査、既知OS49件と匿名Band1件はPIDなし。入力30（7directory）/292ソース/4出力一致。表示修正なので新PCMなし・過去音声合格の転用なし。

SSP01→SSP02のエディター開閉を継続。原版15872の固定証拠では、Project初期はprimary未登録、TimeSig-only編集窓を開くとAddNewTracks、最後Close後はgray workspaceのままprimary/Play/空欄を保持し、再OpenでもAddNewTracks。次は最後Closeでlegacy formを出さずgrayを保持する独立修正を優先し、関連回帰/本体/保存/通常終了/別復元まで検証する。初期primary登録とTimeSig-only reopen promptは別未達として扱う。全40/全8/fullAppearance/fullAcceptance=false、49native/30driver凍結と匿名Bandの同条件再試行なし。正式原版15872通常exit0・通知手動対応待ちは解消済み。

一次入口：`work/analysis/ui-tempo-target-display/20261109T103529762Z/unit-record-v2.json`。goal active。

## 2026-11-09 現行再開点（110449 Segment workspace構築中）

110449は最後のSegment編集窓Close後のgray workspaceを実装してbuild中。native MDI clientが見える間はlegacyフォーム/Tab対象を隠す。個別CloseとWindow CloseAllで文書/primaryを保ち、Project置換・終了のreleaseは従来どおりclient非表示。次：関連回帰→本体CtrlF4/再Open/マウスClose/複数WindowCloseAll/primaryとtempo保持/保存→通常終了→別復元/再保存/通常終了・native/ソース全照合。初期primary登録とTimeSig-only再Open AddTracksは別未達。全40/全8/fullAcceptance=false、49native/30driver凍結・匿名Band同条件再試行なし。

一次入口：`work/analysis/ui-segment-workspace-close/20261109T110310557Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（111021 workspaceレビュー補正版構築中）

111021はSegment最後Close後のgray workspace修正と、File NewStyle/NewBandからlegacy表示へ戻すレビュー補正を含む保存build中。110449は本体/保存/音声未実行のまま保全。次：関連4回帰→空起動120/NewCancel/最初Segment Close gray/StyleとBandフォーム復帰/own smoke文書のみ破棄して通常終了→専用2参照ProjectでCtrlF4/再Open/CloseAll/primaryとtempo保持/native保存/通常終了→別復元/再保存/通常終了・全bytes/292ソース4出力照合。初期primary登録とTimeSig-only再Open AddTracksは別残件、全40/全8/fullAcceptance=false。49native/30driver凍結と匿名Band同条件再試行なし。

一次入口：`work/analysis/ui-segment-workspace-close/20261109T110310557Z/unit-record-v2.json`。goal active。

## 2026-11-09 現行再開点（111021 lastClose gray検証後）

111021は最後のSegment/埋め込みBand窓CloseやWindow Close Allで、文書・primary・Tempoを保ちgray workspaceへ戻す修正。native MDI表示中のlegacyフォーム103件とTab対象を隠す。File NewStyle/NewBandは従来フォームへ復帰。関連4件238検査、空19288/作者11068/復元16244の三通常exit0。CtrlF4/再Open/マウスClose/複数CloseAll/Bandだけ最後Close/NewCancel/Tab/既存CtrlF11/View/窓を閉じた後の保存先を確認。両Segmentは初期/作者/復元の全bytes一致、NoTempo20小節TimeSig-only/Seed132.25と17音符を保持。2参照ProjectはFILETIME以外同一、292ソース/4出力一致。表示・focusのみの変更で新PCM不要、過去音声受入は転用しない。

SSP01の初期primary登録を継続。正式原版15872の固定契約ではProjectを読んだだけではprimaryなし/Play無効/Tempo120で、Segmentを開いてから登録され、最後Close後は登録を保持する。候補111021はProject初期に全owned Segmentを自動登録しlegacyフォームを出す差が残る。次はowned文書と登録済みtransportのUI状態を分け、明示Segment活性化/SaveAsコピー/Close後保持/Project置換を照合して実装→保存build/関連回帰→本体/保存/通常終了/別復元/必要PCMへ進む。TimeSig-only再Open AddTracks prompt・他component登録・全外観/全40/全8/Q2/Style015244は未達。fullAcceptance=false、49native/30driver凍結・匿名Band同条件再試行なし。

一次入口：`work/analysis/ui-segment-workspace-close/20261109T110310557Z/unit-record-v3.json`。goal active。

## 2026-11-09 現行再開点（114031 registration構築中）

114031はProject所有文書とTransport登録を分離するmainだけの修正でbuild中。読込直後はgray/登録なし/list・Play・SaveFile無効/Tempo120、明示Segment活性化で登録、Close後保持、SaveAs新コピー登録、Project置換でreset。次：関連4→空/firstcreation→専用2文書初期無効/1→2登録/Close→FileSaveAsでSeedCopy120・UndoRedo・3native保存→Project置換reset→通常終了→別復元3文書/初期無登録/再登録/再保存→新PCM132.25→copy120/Stop/quiet/負例6とclock4→通常終了・全照合。全40/全8/fullAcceptance=false、TimeSig-only再OpenAddTracks/他component/fullAppearance等は別未達、49native/30driver凍結・匿名Band同条件再試行なし。

一次入口：`work/analysis/ui-transport-registration/20261109T113809853Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（114031関連回帰の新規OS拒否）

114031関連回帰は3合格168検査、component-nameは67検査後Names.proのatomic replaceでWindows5・PID19492 exit1。新しいnative凍結を追加し計50件、driver30件は不変。同条件再試行/出力移動での迂回なし、過去70検査合格の転用なし。本体は未実行。次は独立に宣言済みの初期未登録/明示登録/Close/SaveAs copy120/Project置換/保存通常終了/別復元/新PCM132.25→120とcontrolsを進め、component-name障害とは分けて記録する。関連4合格238とはしない。全40/全8/fullAcceptance=false。

一次証拠：`work/analysis/ui-transport-registration/20261109T113809853Z/component-name-os-refusal-v1.json`。goal active。

## 2026-11-09 現行再開点（114031登録/本体/native/音声検証後、関連1障害）

114031はProjectの所有catalogとsession内Transport登録を分離し、初期はgray/登録なし/list・Play・SaveFile無効/Tempo120、明示Segment活性化後だけ登録、Close後は保持する。SaveAsコピーは新GUID/3件目として登録し元Seed編集窓を保持、コピー120の確定/コピー自身のUndo132.25・Redo120/保存、Project置換resetを確認。空13628/作者6576/復元16816は通常exit0。両既存Segment全bytes、コピーの新GUID+Tempo120以外の全bytes・17音符、2→3参照Projectの既存情報とFILETIME以外を保持し、音声後/終了後もnative4ファイル不変。新PCMは132.25再生中のコピー選択でも132.25を保ち、次Playコピー120、両17発音/音程/activeStop/quietが成立。中央値0.455/0.500秒、負例6とclock4拒否。292ソース/4出力一致。関連回帰は3合格168検査、component-nameは67検査後Names.pro置換Windows5で新規凍結。全4合格238/単位終了条件完了とはしない。

SSP02 TimeSig-only Segmentのopen/reopen AddNewTracks promptを継続。原版15872の固定証拠ではTimeSigのみのSegmentを開くたびにAddNewTracksを出し、Cancel後も文書/primary/grayCloseを保持する。114031ではpromptなしが残る。次は編集窓を新規openした時だけ条件判定し、NewFiles既存の初回promptと二重化しない独立修正→保存build→凍結50を再試行しない関連3回帰→本体/Cancel/再Open/既存窓再活性化/保存/通常終了/別復元・全bytesまで進む。component-name OS拒否の解消・他component登録・全外観/Recording/Transition/全40/全8/Q2/Style015244は未達。native凍結50/driver30、匿名Band同条件再試行なし、fullAcceptance=false。

一次入口：`work/analysis/ui-transport-registration/20261109T113809853Z/unit-record-v3.json`。goal active。

## 2026-11-09 現行再開点（122735 TimeSig-only reopen prompt構築中）

122735はTimeSig-only編集窓のopen/reopen AddNewTracksを実装してbuild中。新しい編集窓だけを判定し既存窓再活性化は通知なし、NewFiles初回は専用notificationで既存prompt二重化を防ぐ。全native trklがTimeSig1本だけかを調べ、選択groupで他Trackを見落とさない。次：関連3/168（component-nameの新OS拒否は凍結、再試行なし）→空/初回作成1prompt/Cancel/再活性化noPrompt/Close再Openprompt→専用3native文書でTimeSig-only promptCancelと4Track非対象noPrompt/保存/通常終了→別復元/再保存/通常終了・全bytesと292ソース4出力。音楽変更なし新PCM不要・過去音声転用なし。native凍結50/driver30と匿名Band、全40/全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-reopen-tracks/20261109T122451800Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（122735 reopen prompt検証後）

122735はTimeSig-only Segmentの編集窓open/reopen時にAdd New Tracksを表示し、既存窓の再活性化では出さない。NewFiles初回はLength Cancel後の通知が1回だけで、Cancel一度で編集へ戻る。関連3件168検査、空11332/作者11348/復元20460は三通常exit0。3文書の保存/別process復元/再保存を確認し、全Segment bytes・20小節TimeSig-only・Seed132.25/Copy120と各17音符・3distinctGUIDを保持。3ProjectはFILETIME以外同一、292ソース/4出力一致。原版15872固定証拠のopen/reopen/Cancelと一致。音楽変更なし、新PCMなし・過去音声受入転用なし。

SSP02のAdd New Tracks→Band/Tempo追加・UndoRedo・120既定値・保存復元を独立に継続する。現候補の保存ソースを保ち、既存実装/画像7〜12と原版の限定比較証拠を先に照合し、未検証の操作から進める。空選択OKは既知CtrlSpace/Space不成立の同条件再試行をせず未比較として保持。前114031のcomponent-name関連終了条件はWindows5で未達のまま、今回の専用GUIで代替合格にしない。全40/全8/fullAppearance/fullAcceptance=false、native凍結50/driver30・匿名Band同条件再試行なし。

一次入口：`work/analysis/ui-segment-reopen-tracks/20261109T122451800Z/unit-record-v2.json`。goal active。

## 2026-11-09 現行再開点（122735 Band/Tempo追加検証準備）

122735同一候補の追加検証：専用3参照ProjectでTimeSig-only→Band追加/空欄/UndoRedo→Tempo追加120/UndoRedo/Cancel/再Open noPrompt→native保存/通常終了→別process復元/再保存/通常終了。新ソース/build/PCMなし。関連既存3/168は参照、新timeline-rangeだけ実行。segment-creation/component-nameのOS拒否は代替合格にせず凍結50/driver30、全40/全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-add-band-tempo/20261109T125633600Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（122735 Band/Tempo履歴検証後）

122735同一保存候補でBand/Tempo追加の本体履歴を検証。Band追加322bytes、Undo234bytesは元入力と完全一致、Redo同一。Tempoはcursor1920でも時刻0に120で追加し402bytes、UndoはBandのみ322bytes・disabled空欄、Redoは同一402bytes・有効120。Cancel/最後Close gray/再Open noPromptを挟んでもUndoRedoを保持。作者16276/復元6752は二通常exit0、別processでは3tracks120・空UndoRedo、再保存全Segment bytes同一。他2文書は各17音符/Tempo/GUID全bytes不変。3Projectは宣言済Segment1 size234→402とFILETIME以外不変、292ソース/4出力一致。新build/ソース/PCMなし。関連は新timeline-range117、既存同候補3/168参照で再実行ではない。

SSP02の未実装Cycle Bandsを継続する。正式CHM bandtrack.htmは同じbeatに重なったBand名のitalic表示と右メニューCycle Bandsを規定するが、候補は常に無効。実装前に物理/論理時刻・同一beat判定・描画/hit/選択・Propertiesの重複許可を照合し、表示/選択だけの操作として文書bytes/dirty/historyを保護する。専用重複Band入力・保存build・未凍結関連・本体cycle/Properties/子editor所有先・保存通常終了/別復元へ進む。原版同入力動的/全外観比較・他残責務は未達のまま。component-name/segment-creationの既知OS拒否は代替合格にせず50native/30driver凍結、全40/全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-add-band-tempo/20261109T125633600Z/unit-record-v2.json`。goal active。

## 2026-11-09 現行再開点（131516 physical-beat Band cycle構築中）

131516 Cycle Bands候補を構築中。Timelineだけで、異なる論理時刻/同じStart Time拍のBandを斜体1表示にまとめ、現在選択を描画/hit/子editorで共有、Cycleで次選択へ巡回する。同一論理時刻重複のcore拒否は別残件。専用4inactiveBand入力→関連4/285→本体cycle3周/Properties/子editor所有先/単独無効/dirty履歴不変→保存通常終了→別復元再保存/通常終了・全bytes292sources4outputs。新PCM不要。50native/30driver凍結・匿名Band同条件再試行なし、全40/全8/fullAcceptance=false。

一次入口：`work/analysis/ui-band-cycle/20261109T131212131Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（131516 physical-beat Band cycle検証後）

131516は異なる論理時刻/同じStart Time拍のBandを斜体でまとめ、Cycle Bandsで選択を巡回するTimelineだけの修正。作者C→A→B→Cを2巡/復元1巡、選択を右クリックしても維持。Propertiesはphysical−1と各BelongsTo1/2/3拍、C/Aと復元Aの子editorは各16inactive行を表示。Solo/空白はCycle無効、dirtyなし/UndoRedo無効。関連4/285、作者3548/復元1156は二通常exit0。初期/作者/復元の全Segment bytes、4Band名前/GUID/時刻/無効flags・2control文書各17音符を保持し、3ProjectはFILETIME以外同一。292ソース/4出力一致、前候補と291ソース同一。新PCMなし。

同じ131516の専用文書で、Edit→Escape直後のCtrlF4が一度無効だった操作差を独立診断する。子Band CtrlF4とmouse Closeは成立しており、原因は未断定。最新window/PID/表示/実focusを再観測し、main system-key routingとTranslateMDISysAccelの順序を照合。再現した場合だけ修正し保存build/未凍結関連/本体/保存/通常終了/別復元を進める。その後の現行native100節目は50OS凍結/匿名Band1をPIDなしで保持し、49実行だけを集計する。完全同一論理時刻Band重複・同拍別physical tick・原版同入力比較/全外観/全40/全8/Q2/Style015244は未達。fullAcceptance=false、50native/30driver凍結同条件再試行なし。

一次入口：`work/analysis/ui-band-cycle/20261109T131212131Z/unit-record-v2.json`。goal active。

## 2026-11-09 現行再開点（131516 CtrlF4診断準備）

131516同一候補のCtrlF4差を診断中。専用コピーで通常canvas→EditEscapeと、Properties/子Band開閉後の条件を分け、actualfocus/MDI数/修飾キーをread-only測定する。未再現なら過去失敗を解消扱いにせず残し、再現した場合だけmessage routingを修正して新保存build/関連/本体/保存/通常終了/別復元。音楽変更なし。次のnative100節目は50OS凍結+匿名Band1をPIDなし、49実行。全40/全8/fullAcceptance=false。

一次入口：`work/analysis/ui-mdi-close-key/20261109T133909247Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（131516 CtrlF4/menu状態診断後）

131516同一候補でCtrlF4の条件差を診断。EditをクリックしてEscape一回では、canvas実focus・修飾キー全upでもGUI_INMENUMODE=4が残りCtrlF4一回は編集窓を残す。追加Escapeでflags0となり、同じfocusのCtrlF4がgrayへ正常Close。別processでも4→0と通常Closeを確認し、既知menu状態での無効キーは再試行しなかった。Properties/子Band開閉と独立に条件を確認した。ソース不具合の断定/修正なし、原版同条件比較は未達。作者3172/復元4536は二通常exit0、全Segment bytes・3Project FILETIME以外・292ソース/4出力不変。新build/PCM/関連実行なし、同候補4/285参照。

131516候補のnative100登録節目を実行する。50既知OS凍結と匿名Band1はPIDなし、残49実行の期待2785検査を別集計し、driver172の新実行はしない。入力/出力/manifest/292保存・作業ソース/4出力を照合し、新たな拒否があれば証拠保全・凍結して同条件再試行しない。次はSSP02の完全同一論理時刻Band重複、または独立したSequenceの未達へ進む。menu Escape同入力原版比較、Cycle同拍別physical tick/全外観、Recording/Transition/全40/全8/Q2/Style015244は残しfullAcceptance=false。

一次入口：`work/analysis/ui-mdi-close-key/20261109T133909247Z/unit-record-v2.json`。goal active。

## 2026-11-09 現行再開点（131516 native100節目準備）

131516 native100登録節目を準備済み。50既知OS凍結/匿名Band1はPIDなし、49runnable/期待2785検査を実行し別集計する。driver172は一覧のみ/新実行0、30driver凍結保持。native入力・manifest・292saved/workingソース4outputsを照合し新規拒否は保全凍結/同条件再試行なし。前Cycle/メニュー状態診断の本体保存別復元は完了、全40/全8/fullAcceptance=false。

一次入口：`work/analysis/priority-regression-131516/20261109T135113300Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（131516 native100節目検証後）

131516同一候補のnative100登録節目は49合格2785検査。既知OS凍結50件と匿名Band1件はPIDなし、新規失敗0。入力30件（7directory）・292保存/作業ソース・4出力とmanifestを照合。driver172は既存一覧のみで今回実行0、30driver凍結は保持。component-nameの67検査後Names.pro Windows5とsegment-creation Mixer.sgpの既知拒否を再試行/迂回せず、旧候補の合格へ転用しない。

SSP03 Sequenceの最小操作/発音経路を独立に継続する。既存Sequence基本/初回試聴失敗と後続の対策・最新状態/CHM/画像13〜16・21〜24を先に照合し、初回発音の既知失敗を同条件で反復しない。専用TimeSig/Band/Tempo120文書からSequence追加/パート1開閉/選択/楽器/Insertを本体で扱い、必要な修正があれば保存build/関連回帰、native保存/通常終了/別復元、固定PCMによる試聴/発音中Stop/無音/再開と必要な原版比較を進める。完全同一論理時刻Band重複・同拍別physical tick・menu Escape原版比較・全外観/Recording/Transition/全40/全8/Q2/Style015244は未達、fullAcceptance=false。50native/30driver凍結と匿名Band同条件再試行なし。

一次入口：`work/analysis/priority-regression-131516/20261109T135113300Z/unit-record-v2.json`。goal active。

## 2026-11-09 現行再開点（131516 Sequence最小経路準備）

131516同一候補でSequence最小経路を準備。専用TimeSig/Band/Tempo120からSequence追加/part1開閉、InsertNewInstrument Piano1→Organ1、Insertで1note→Properties MIDI72/endBar6(duration15552)/UndoRedo、native保存/通常終了→別復元/再保存→新180s PCM cold/warm試聴・Play/6s発音中Stop/replay/quiet・負例6/clock4→通常終了・native全照合。現候補に初回試聴の後続修正は残るが旧音声合格を転用しない。source/build変更なし、関連は同候補native49/2785参照。50native/30driver凍結、全40/全8/fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-minimum/20261109T140607429Z/unit-record-v1.json`。goal active。

## 2026-11-09 現行再開点（131516 Sequence最小操作・短Stop時刻不合格）

131516同一候補でSequence追加/part開閉/Piano1→GS Cancel→Organ1/Band子editor/Insert/Properties MIDI72・endBar6/UndoRedoを本体で確認。初期402→Sequence486→Band1618→1note1638bytes。14保存段階でduration192↔15552、1note/velocity96/PCh0、Band1同GUID・他15inactive、既存TimeSig/Tempo/外側GUID/opaqueを保持。二control各17notes全bytes、ProjectはSegment1sizeとFILETIMEだけ。作者11720/復元7596/再検証2528は三通常exit0、別復元再保存/空UndoRedo/音声前後4files全bytes一致、292saved/working/4outputs一致。新buildなし、同候補native49/2785参照。

二180s PCMの音声開始/音高/静音は成立したが、固定6秒[5.5,6.5]のStop操作時刻が不合格。初回は時刻変数bindingの入力前エラー後に63.687s cold、Stop7.699/6.001s・最後129.697sでcoverageも不合格。再検証はbinding準備を先行したが、Play画像確認から次入力まで17.172sかかり初回音符が自然終了、再開Stopも7.122s。失敗証拠・監査コード/閾値を保全、6秒受入/負例clockは合格へ昇格しない。

短い6秒操作条件の反復を止め、同じ131516の独立したSequence activeStop単位へ進む。専用コピーでEndBar16(duration46272、約30.125s)を本体編集/UndoRedo/保存/通常終了/別復元し、録音前に固定する240s PCMでcold/warm試聴・25s演奏中Stop・再開/quiet・負例6/clock4を検証する。1s開始/音高/静音/clip閾値を維持し、操作時刻条件の変更を新単位に明示、旧6秒失敗は未達で残す。原版同入力/全外観/全40/全8/Q2/Style015244、完全同一論理時刻Band等は未達。50native/30driver凍結と匿名Band同条件再試行なし、fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-minimum/20261109T140607429Z/unit-record-v2.json`。goal active。

## 2026-11-09 現行再開点（131516 Sequence長音activeStop準備）

131516同一候補の独立Sequence activeStop単位を準備済み。専用3entryコピーでexisting noteのEndBar16→duration46272(30.125s)/UndoRedo/保存/通常終了/別復元、240s PCMでcold20/warm40/Play60・actual25s Stop/replay120・25s Stop/quiet・負例6/clock4。操作境界17sが観測されたため事前に長音/停止時刻を変更し、旧6秒2録音不合格は未達維持。1s発音/音高/RMS/静音/clip/clock閾値不変。native全byte差はduration1値だけ、292sources4outputs/現候補49・2785参照。50native30driver凍結/全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-active-stop/20261109T144604000Z/unit-record-v1.json`。goal active。

## 2026-11-10 現行再開点（131516 Sequence長音activeStop検証後）

131516同一候補の独立Sequence長音単位で既存1note終端を第16小節(duration46272、30.125s)へ変更。Undoは元1638bytes全一致/Redoは長音全一致、作者3352/復元16176は二通常exit0、別復元再保存/空UndoRedo。240s PCMは18/18、cold試聴0.825/warm0.520/通常Play0.485/replay0.465s、停止25.001/25.015s、音高72/発音/停止後無音/有限PCM・clipを確認。負例6とclock4全拒否。APIはprivate2/primary2の各Play/Stop/explicitstart、音響判定は別証拠。native9段階でdurationLONG1値だけ変更、二control17notes全bytes・3Project参照/GUID/entryID/FILETIME以外・音声前後/終了後4filesを保持。292saved/working/4outputs一致、新buildなし、同候補native49/2785参照。

旧Sequence最小単位の二180s/6s Stopは時刻不合格のまま。今回はtool/model境界17.172sを先に観測し、新単位で30.125s持続音と25s Stopを録音前固定した。旧6s受入へ転用しない。1s開始/音高/RMS/静音/clip/clock閾値不変。準備状態書込のcp932エラーは失敗bytes保存→直前backup exact復元→UTF8更新済み。runtime監査v1のstage-only既定0fieldsを72/96と仮定したassertは、source call record(stage)に従うv2へ訂正し、count/HRESULT/PCM/native閾値を変更していない。

正式原版の同入力Sequence比較を独立継続する。今回保存済み4track/Organ1/MIDI72長音の専用別コピーを原版で開き、正式EXE/登録先/PID/1280x800・96DPIを再照合。既知cold startup6〜7分は同一PIDで待ち重複起動/forcekillなし。Tree/Timeline/part/keyboard/楽器メニュー/Properties/endBar/Close・再open・保存native・通常終了を比較し、必要な実装修正だけ保存build/関連回帰/本体/保存別復元へ進む。全Sequence gestures/CC/複数part/全外観、Band完全同一論理時刻、全40/全8/Q2/Style015244は未達。50native30driver凍結と匿名Band同条件再試行なし、fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-active-stop/20261109T144604000Z/unit-record-v2.json`。goal active。

## 2026-11-10 現行再開点（131516 正式原版Sequence比較準備）

131516のSequence長音出力と完全同一bytesの専用別コピーで正式原版同入力比較を準備。43登録/正式EXEhashをread-only照合し、cold同一PID6〜7分を保持。初期Tree/primary、4tracks/part/Organ1/GS Cancel/Band子editor/既存noteProperties/endBar16、Close/reopen/native保存/通常終了を照合する。原版新PCMは未実施、25sPCMと旧6s失敗は別証拠。50native30driver凍結/全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-original/20261109T150614000Z/unit-record-v1.json`。goal active。

## 2026-11-10 現行再開点（原版1164通知待ち・独立Band別tick）

正式原版同入力Sequence比較はPID1164(正式EXE/43COM登録read-only一致/1280x800・96DPI)で起動中。registry更新失敗通知#32770はread-onlyで確認したがSky実window一覧に現れず、userへ今回のOKだけを依頼済み。前回15872とは別起動。架空target/Win32input/OS登録再変更/重複起動/forcekillなし。同一PIDを保持し、原版操作の再試行は回答後。並行して131516製品の同拍別physical tick Band Cycle単位を準備済み:logical0/768/1536、physical0/96/191とSolo3072、16inactive全native入力監査済み。本体C→A→B→C/Properties時刻/履歴不変/保存通常終了/別復元/全bytesと292sources4outputsを進める。source/build/PCM変更なし、関連は現候補49/2785参照。50native30driver凍結/全40全8/fullAcceptance=false。

原版一次入口：`work/analysis/ui-sequence-original/20261109T150614000Z/unit-record-v2.json`、製品一次入口：`work/analysis/ui-band-cycle-ticks/20261109T151300000Z/unit-record-v1.json`。goal active。

## 2026-11-10 現行再開点（131516 Band同拍別tick巡回検証後）

131516同一候補の追加Band入力で、同じ表示拍・異なるphysical0/96/191tickの3Bandを斜体1表示にまとめ、作者/復元それぞれC→A→B→Cへ一巡（計6calls/2wraps）。右clickで現在選択を保持、Properties A/B/CはStart1:1のtick0/96/191とBelongsTo1/2/3拍を照合。Solo/空白はCycle無効、dirtyなし/UndoRedo無効。作者13564/復元16048は二通常exit0、全3Segment bytesと4Band名前/GUID/時刻/16inactive・二control各17notesを保持。3ProjectはFILETIME以外同一、終了前後4filesも全一致、292saved/working/4outputs一致。source/build/PCM変更なし、現候補native49/2785参照。

今回の入力は旧Band Cycle固定出力の4physical LONGだけを独立変更したfixtureで、原版同入力動的比較・GUI作成の証拠ではない。旧単位の子editor所有先は別証拠、今回子editorは未実行。完全同一logical時刻の読込/移動/更新の拒否は残し、全重複対応とはしない。

正式原版のSequence比較はPID1164のunexposed registry更新通知でuserのOK操作回答待ち。同一PIDを保持し重複起動/forcekill/登録再変更/架空targetなし。独立してBand完全同一logical時刻のcore/API/選択・owner・履歴/opaque保護を一次契約と照合し、根拠ある範囲を実装・保存build・未凍結関連回帰・本体操作・保存通常終了・別復元まで進める。原版回答後はSequence同入力比較を再開。全Sequence gesture/CC/複数part/全外観・全40/全8/Q2/Style015244は未達、50native30driver凍結/匿名Band同条件再試行なし、fullAcceptance=false。

一次入口：`work/analysis/ui-band-cycle-ticks/20261109T151300000Z/unit-record-v2.json`。goal active。

## 2026-11-10 現行再開点（同一logical時刻Band対応準備）

同拍別tick Band巡回は検証済み。独立したBand同一logical時刻対応をbaseline131516/292sources4outputsから準備。readerは重複をnative順に保持、index更新/移動は同時刻も選択発生位置を返し、時刻だけの割当は複数一致なら全bytes不変で拒否。nativeidentityコピーは選択時owner全bytes一致の間だけindexを信頼し、外部変更後の曖昧targetは無効。physical一致をliveguardへ追加。未凍結sequence-crud等に新契約回帰、core/segment-creation等50凍結はPIDなしを保持。保存build/関連/本体Properties重複作成・Cycle・UndoRedo・保存通常終了・別復元・native全照合を進める。原版1164通知のOK回答待ちは別保持。50native30driver凍結/全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-band-logical-collision/20261109T153138000Z/unit-record-v1.json`。goal active。

## 2026-11-10 現行再開点（153816119 Band同一logical保存build後・原版比較再開）

153816119は同一logical時刻Bandのreader/indexed更新・stable occurrence選択/コピーidentity曖昧target無効・physical liveguardを実装し保存build成功。baseline131516と8source変更/284同一、292savedworking4outputs一致、syntax0。未凍結common-open/timeline-range/sequence-crud/sequence-audition-runtimeを新候補で実行し、本体Properties B所属拍2→1/名前だけ変更/C3→1・Cycle/owner/UndoRedo/保存通常終了・別復元を進める。入力4Bandはまだdistinctlogicalの独立コピー、50native凍結objectsとdriver一覧完全保持。正式原版1164はuserOK後に再開し、専用Project絶対path/4tracks/Sequence C6・96・end16/BelongsToBeatを確認中。原版のTempo1-32先頭/行順/Part縦zoomと音高scroll刻みは製品との差を観測、別単位で修正予定。全40/全8/fullAcceptance=false。

一次入口：`work/analysis/ui-band-logical-collision/20261109T153138000Z/unit-record-v2.json`、原版：`work/analysis/ui-sequence-original/20261109T150614000Z/`。goal active。

## 2026-11-10 現行再開点（正式原版Sequence比較・1164通常終了）

正式原版PID1164はuserの通知OK後に専用Project絶対path/43登録/正式EXE/1280x800・96DPIを照合し、Sequence C6・96・終端16/所属拍1:1/楽器GS Cancel/Band子owner/保存・close再open・通常exit0まで比較した。原版保存は1638→1972bytes、seqt/tetr/tims全bytes・外側/BandGUID/名前/二control17notesを保持する一方、Tempo group1→全32、track順、Band priority DWORD36、設計chunks/Project author・rfld/open/nodeを原版側で正規化。全runtime/opaque不変とはしない。

行順Tempo→TimeSig→Sequence→Bands、Tempo1-32/Bands表記、Part縦zoom/鍵盤1半音scroll、Sequence所属拍、Band子ownerタイトル/同pan-volume重なり、行高・ラベル幅・fontが製品との差。新候補153816119のBand同一logical本体/別復元・native監査を完了後、根拠ある外観操作修正を独立単位で進める。原版新PCM/別process原版復元/全外観/全40全8は未達。旧6s音声失敗を維持、50native30driver凍結/同条件再試行なし/fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-original/20261109T150614000Z/unit-record-v3.json`。goal active。

## 2026-11-10 現行再開点（153816119 同一logical Band検証後）

153816119はBand完全同一logical時刻のreader/index更新/安定した発生位置/曖昧な時刻割当拒否/コピーidentityの古いtarget無効/physical liveguardを実装。関連4モード508checks合格、50native凍結と172driver一覧完全保持。作者19116でB所属2→1・名前Logical B/C3→1、C子owner/UndoC保存B-name全bytes・Redo三重複全bytes、復元15324でC→A→B→C/所属拍全1・physical0/96/191/B子owner16inactive/空UndoRedo/再保存を確認し二通常exit0。native変更はB/C logical LONGとB UNAM・祖先sizeのみ、4BandGUID/16inactive/physical・二control17notes/他全bytes保持、3ProjectはFILETIMEとSegment1size4940→4944だけ。292savedworking4outputs/8changed284identical。

今回のnative/選択単位は非発音16inactive/noNotesで新PCMなし。重複Bandの演奏中適用/全clipboard/曖昧identityの全操作・全外観受入には転用しない。core/segment-creationの新契約定義は更新したが、既知OS凍結のため実行なし。旧6s停止時刻失敗は保持。全40全8/fullAcceptance=false。

正式原版比較（1164通常exit0）で確定したTimeline行順Tempo→TimeSig→Sequence→Bandsとラベルを次の独立単位で修正する。表示とnative順を分離、未知trackの位置/同種順/選択indexを保持し、group表示は実maskを圧縮表記する。原版がgroup1を全32へ正規化した事実は別記し既存native group1を勝手に変更しない。保存build/未凍結関連/本体hit所有先/保存通常終了/別復元/native不変まで進め、その後Part縦zoom/1半音scroll・所属拍・Band子title/graphへ継続。50native30driver凍結/同条件再試行なし/fullAcceptance=false。

一次入口：`work/analysis/ui-band-logical-collision/20261109T153138000Z/unit-record-v3.json`。goal active。

## 2026-11-10 現行再開点（162522992 Timeline行順・label保存build）

162522992は原版Sequence比較に基づき、Tempo→Time Sig→Sequence→Bandsの表示順/実group mask連続範囲表記/Time Sig・Bands名を1sourceで修正して保存build成功。native順/position/各kind index/他kindの位置/同kind順は保持。原版保存1972bytesのtrklだけTimeSig/Band/Tempo/Sequenceへ戻した独立入力（Tempo全32）を準備済み、3Projectrefs/二control17notes保持。本体各行Properties所有先/Sequence長音/保存通常終了/別復元/全native bytes保持と未凍結3関連を進める。group1 Tempoを原版が全32へ正規化するglobal semantics・Partzoom/1半音scroll/所属拍/Bandgraph/全外観は別未達。50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-timeline-order/20261109T162156000Z/unit-record-v2.json`。goal active。

## 2026-11-10 現行再開点（162522992 Timeline行順・label検証後）

162522992はTimelineの既知4種類を表示slot内でTempo→Time Sig→Sequence→Bandsへ安定配置し、Time Sig/Bands表記と実group maskの連続範囲表記を修正。1source変更/291同一/292savedworking4outputs、関連3モード478checks合格。原版保存由来1972bytesの独立入力はnative順TimeSig/Band/Tempo/Sequenceのまま、本体はTempo1-32先頭/各行Properties所有先/Organ1/C6・96・終端16を確認。作者17072/復元16084で二通常exit0、空UndoRedo/再保存/三Segment全bytes（全track順/headers/設計chunks/GUID/二control17notes）一致、ProjectはFILETIME以外全一致。

原版と同入力の比較結果は別単位で保持し、今回入力は原版保存1972bytesのtrack containersだけ並べ替えたfixture。原版がTempo group1を全32へ正規化するglobal semanticsは今回未修正、実mask表記のみ。初回Tempo文字クリックは既存hit範囲外で選択なし、時刻0位置選択で確認。未知kind/同kindの表示保持は実装方針であり全組合せの原版動的比較は未実行。全geometry/font/docking/Partzoom/所属拍/Bandgraph/全40全8は未達。新PCMなし/旧6s失敗保持/50native30driver凍結/fullAcceptance=false。

原版015/016とCHM tupiano/zoom仕様に基づき、Part鍵盤矢印1半音とPartごとの縦zoom・paint/hit/drag/CC境界を次単位で実装し、保存build/関連/本体/保存通常終了/別復元/native不変を進める。Sequence所属拍はmtTime+nOffsetの明示API/原版比較/音声を要する別単位、Band子ownerタイトル/graphとglobalTempoも残す。既知OS凍結の同条件再試行なし。fullAcceptance=false。

一次入口：`work/analysis/ui-timeline-order/20261109T162156000Z/unit-record-v3.json`。goal active。

## 2026-11-10 現行再開点（165309981 Part音域view保存build）

165309981はPart別pitch viewport・縦zoom虫眼鏡2個・native鍵盤scrollbar（矢印1半音/page/thumb）を実装して保存build成功。固定216px音域でCC位置を保持、pitch/velocity描画hit/dragを共通化しheader/CCからの不可視note hitを拒否。3source変更/289同一/292savedworking4outputs、syntax0。未凍結sequence-crudへ16契約/nativeUndo回帰追加、core/segment-creation等50native凍結/driver172一覧・30凍結を保持。本体1半音/zoom12→13→12/scrollpage-thumb/既存72右clickProperties/Part collapse保持/空UndoRedo・保存通常終了・別復元・全native不変と関連3モードを進める。新PCMなし/音響合格転用なし、全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-pitch-view/20261109T164346000Z/unit-record-v2.json`。goal active。

## 2026-11-10 現行再開点（165309981 Part音域view検証後）

165309981はPartごとのpitch viewport、縦zoom虫眼鏡2個、native鍵盤scrollbarを実装し、paint/hit/pitch・velocity drag/CC境界を共通化。3source変更289同一/292savedworking4outputs、関連3モード494checks（新16）合格。作者10256/復元17884で矢印1半音（pos57→56）、zoom12→13→12（page18→17→18）とCC/Bands位置保持、作者page55→37→55/つまみ55→45、zoom後既存C6・96・終端16の右clickProperties、折畳3部品hide/再展開音域zoom保持、空UndoRedo/保存通常終了/別復元再保存・二exit0を確認。三Segment全bytes・opaque設計情報/全Track順/GUID/二control17notes保持、ProjectはFILETIME以外一致。

MIDI両端/全zoom境界/Part独立/drag scale/native pitchUndoは既存未凍結sequence-crud内の新契約、実GUI全drag/keyboard端点/複数Part/全zoomは未実行。UIA scrollbar百分率をrawpositionとせずGetScrollInfo別証拠。default12px/固定216pxは原版全geometry/defaultscale合格ではない。表示のみで新PCMなし・旧音声合格転用なし・旧6s失敗保持。全40全8/50native30driver凍結/fullAcceptance=false。

正式原版029で確定したBand子editor title「Band: Segment1 - Band1」に所有Segment名を含める修正を次の独立単位で進める。Band payload不変の早期returnでもowner名変更を反映し、本体/保存通常終了/別復元/native不変まで検証する。Band graphのinactive rack対storedpan-volume配置とactive flagsは原版/CHM/SDKを別照合し、今回に混ぜない。Sequence所属拍・globalTempo・全外観は残す。既知OS凍結の同条件再試行なし/fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-pitch-view/20261109T164346000Z/unit-record-v3.json`。goal active。

## 2026-11-10 現行再開点（172245420 Band所有名title保存build）

172245420は正式原版029のBand子titleに所有Segment名を含め、Band payload不変の早期return前にもowner名を更新する修正を保存build成功。1source変更/291同一/292savedworking4outputs、syntax0・manifest全bytes保持。本体Band: Segment1 - Band1/一時owner名OwnerXYZ追従/previewディスク初期4件全一致/名前をUndoで原状復帰後だけ保存/通常終了/別復元title・native不変と関連3モードを進める。component-name既知OS凍結Names.proの変更名保存を迂回実行せず、titleの一時in-memory view証拠に限定。graph配置・所属拍・globalTempo/全40全8/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-band-owner-title/20261109T171721000Z/unit-record-v2.json`。goal active。

## 2026-11-10 現行再開点（172245420 Band所有名title検証後）

172245420はBand子titleに所有Segment名を含め、Band payload不変の早期return前もtitleを更新。1source変更291同一/292savedworking4outputs、関連3モード494checks合格。作者21232で既存Band子がOwnerXYZへ追従・16行/選択保持、previewディスク4件全一致、文書Undo1回でSegment1/cleanへ復帰してから保存。復元19020でBand: Segment1 - Band1/16行Organ1/選択/再保存、二通常exit0。三Segment全bytes（Band payload/owner UNAM/GUID/全track/opaque/二control17notes）保持、ProjectはFILETIME以外一致。

一時owner名は保存せず、OS凍結component-name Names.pro保存受入を代替しない。空owner名fallbackの原版同等性/全geometry/graph active flags/Sequence所属拍/globalTempoは未達。表示だけで新PCMなし・旧音声合格転用なし・旧6s停止失敗保持。全40全8/50native30driver凍結/fullAcceptance=false。

原版029の同入力Band graphはstored pan64/volume100で重なる一方、製品はinactiveを8列rackに分散する差がある。CHM/SDK/原版証拠を照合し、表示座標をstored値へ統一する独立単位を準備する。active flagsの意味やlive音声は別未達とし、native不変の本体/保存通常終了/別復元と新候補関連回帰まで進める。Sequence所属拍/globalTempo/全外観も継続、既知OS凍結の同条件再試行なし/fullAcceptance=false。

一次入口：`work/analysis/ui-band-owner-title/20261109T171721000Z/unit-record-v3.json`。goal active。

## 2026-11-10 現行再開点（174532050 Band保存座標graph保存build）

174532050はBand graphのinactive8列rackを除き、全PChannelをstoredpan-volumeから配置して保存build成功。2source変更290同一/292savedworking4outputs、syntax0・manifest全bytes保持。新7契約を既存未凍結sequence-crudへ追加。actual作者/別復元で16button同座標・表示16/選択readout/resize/保存通常終了/native不変と新候補関連3を進める。activeflags色/実GUI musical drag/livePCM/所属拍/globalTempo/全外観は未達。全40全8/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-band-graph-position/20261109T174400000Z/unit-record-v2.json`。goal active。

## 2026-11-10 現行再開点（174532050 Band保存座標graph検証後）

174532050はBand graphのinactive8列rackを除き、storedpan-volumeから全ボタン位置を計算。2source変更290同一/292savedworking4outputs、関連3モード501checks（新7）合格。作者/別復元PID[5788, 13916]で16buttonがstored64/100同座標に重なり16が上に表示、一覧row1/16選択とR1/100、resize後の再計算/再保存・二通常exit0を確認。三Segment全bytes（flags/値/Band/owner/GUID/全track/opaque/二control17notes）保持、ProjectはFILETIME以外一致。

inactive色判定/再生有効性を変更せず全同等性未達。drag開始位置も修正座標を使うため新7契約で端点/群移動/縮小量子化を確認したが、実GUI musical drag/livePCMは別未達。表示選択resizeのみで新PCMなし・旧音声合格転用なし・旧6s停止失敗保持。全40全8/50native30driver凍結/fullAcceptance=false。 作者の最初のresizeはclient内を掴み変化なし、境界再取得後の枠線dragで成功し別復元でも再現。次契約audit-v1の原版capture番号023は実在019へv2で訂正、一次結果変更なし。

Sequence Propertiesに未実装のBelongs to Beatを次単位で進める。CHMは物理発音位置と論理所属を区別、現parserはmtTime+nOffsetのみを公開。明示的な論理位置読出/変更API、物理onset・duration/PCh/status/pitch/velocity/opaque保持、signedSHORT境界、発生位置/Undo選択を先に設計し、保存build/関連/本体編集保存通常終了/別復元/必要PCMまで進める。原版の初期1:1比較は既存証拠、offset編集/variablemeter/loop全同等性は別照合。globalTempo/全外観も未達。既知OS凍結の同条件再試行なし/fullAcceptance=false。

一次入口：`work/analysis/ui-band-graph-position/20261109T174400000Z/unit-record-v3.json`。goal active。

## 2026-11-10 現行再開点（180733499 Sequence所属拍保存build）

180733499はSequence論理位置読出/発音位置不変のlogicalLONG・signedSHORT更新/Document原子的history/Properties所属小節・拍/論理位置を含むUndo選択を実装して保存build成功。9source変更283同一/292savedworking4outputs、syntax0・manifest全bytes保持。既存未凍結sequence-crudへ新35契約。本体Belongs1:1→1:2でもStart0/End16/duration46272/C6・96保持、UndoRedo/保存通常終了/別復元/固定240s・25sStop新PCMと関連4を進める。初期1972bytesの期待差はrawLONG0→768/SHORT0→-768の2bytesのみ。loop包含/原版offset正規化/variablemeter全同等性/globalTempo/全外観は別未達。全40全8/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-logical-time/20261109T180300000Z/unit-record-v2.json`。goal active。

## 2026-11-10 現行再開点（180733499 Sequence所属拍・新PCM検証後）

180733499はSequence論理位置読出/物理onset不変のlogicalLONG・signedSHORT変更/Document原子的history/Properties所属小節・拍/論理位置を含むUndo選択を実装。9source変更283同一/292savedworking4outputs、関連4モード566checks（新35）合格。作者19988で所属1:1→1:2でもC6・96/Start1:1:1:0/End16:1:2:0/Duration15:0:1:0を保持、Undo保存は初期全bytes、Redoは期待全bytes。復元7080で所属1:2/物理値/再保存、二通常exit0。native差は単一record logical0→768/offset0→-768（1972bytes中2bytes）のみ、他全native/control/track/opaque/GUIDとProjectFILETIME以外を保持。

新固定240s録音は18/18、負例6/clock4を全拒否し元WAV保持。初回/再試聴onset0.585/0.585s、先頭再生/再開0.475/0.815s、activeStop25.006/25.007s、C6/発音/停止後静音合格。作者試聴0/Play0、復元private2/primary2/explicitStarts4のAPI結果0を別監査。旧6秒停止不合格は保持し今回結果を転用しない。

原版019の初期所属1:1とCHM/SDK契約を根拠にし、原版offset編集正規化/variablemeter/loop包含/原版別process復元の全同等性は未達。ツールtype_textは空欄になり通常数字キーで確定、paste/IME/spin全操作合格とはしない。全geometry/globalTempo/全40全8/50native30driver凍結/fullAcceptance=false。

Tempoが全グループに適用される原版/CHM契約を次に監査する。正式原版はgroup1入力を1-32表示/0xffffffff保存へ正規化する一方、製品はselected_groupsでTempoを選別。native保持と意味上のglobal範囲、複数Tempo track/index/Timeline/CurrentTempo/runtimeの既存依存・未凍結回帰を照合して独立単位を設計・実装する。Sequence loop包含/原版offset動的比較も未達として保持し、同条件既知OS障害の再試行はしない。fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-sequence-logical-time/20261109T180300000Z/unit-record-v3.json`。goal active。

## 2026-11-10 現行再開点（184801667 CurrentTempo制御track保存build）

184801667はCurrentTempoの参照・変更をeditor-selectedTempoからglobalな先作成Tempoへ分離。4source変更288同一/292savedworking4outputs、syntax0・manifest全bytes保持。新primary APIは最低dwPosition/同値native順のsourcepolicyで決定し、rawtetrの対象DOUBLEだけ変更、別editor cache/選択/range/全headers/groups/orderを保持。既存group/index編集APIは維持。未凍結tempo-propertiesへ新28契約。専用2388bytes5tracks/primary132.25・later180/17C6（768間隔・last長音）入力で、later選択中toolbar主132.25→120/UndoRedo/保存通常終了/別復元と関連5、新固定240s PCM19項目・7負例4clockを進める。原版複数Tempo動的比較/位置tie/全global表示作成正規化/Length/loop/全40全8は未達、50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-current-tempo-primary-track/20261109T184300000Z/unit-record-v2.json`。goal active。

## 2026-11-10 現行再開点（184801667 CurrentTempo主track・新PCM検証後）

184801667はCurrentTempoをeditor-selectedTempoからglobalな先作成Tempoへ分離。4source変更288同一/292savedworking4outputs、関連5モード627checks（新28）合格。作者15528で後続Properties180を選択してもtoolbar132.25を表示、toolbar120確定は主Tempoのみ変更・後続180/選択保持。Undo保存は初期全bytes、Redo期待全bytes、復元14916で主120/後続180・再保存/二通常exit0。native差は主tetrのDOUBLE132.25→120（2388bytes中2bytes）のみ、後続Tempo全体/全header-group-position/17音/Band/opaque/GUID/control二17音を保持、ProjectはFILETIME以外一致。

新固定240s録音は19/19。先頭再生と再開の両方で17音/16間隔が0.5秒許容±0.03秒に一致し、後続180による間隔を採用していないことを確認。C6/発音開始1秒内/25秒activeStop/停止後静音も合格。負例6と追加wrong-tempo180の計7、clock4を全拒否し元WAV保持。作者試聴0/Play0、復元private2/primary2/explicitStarts4・API結果0を別監査。旧6秒停止不合格を保持。

dwPosition最小/同値native順はsourcepolicy。SDK/CHM契約と通常の先作成入力を根拠にし、原版複数Tempo同入力動的比較/輸入並替・tie/全global表示作成正規化/Length/loop/fullgeometryは未達。グループ別編集APIは維持。文字列入力は空欄、通常数字キーで確定できた観測を保存しpaste/IME同等性としない。全40全8/50native30driver凍結/fullAcceptance=false。

SegmentのAdd Tracksで新規Tempoを作る際の全32グループ契約を次に進める。現在add_tracksは全種類をselectedGroupsへ割当てるため、GUI新Tempoがgroup1になる差がある。既存importedheaderとgroup/index編集APIを保ち、GUIの明示的なglobalTempo作成経路を独立実装・未凍結関連/本体AddTempo/保存通常終了/別復元/native全照合まで進める。既存group1読み込み時の原版正規化/全globalview/Length/loopは別未達。既知OS凍結の同条件再試行なし、fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-current-tempo-primary-track/20261109T184300000Z/unit-record-v3.json`。goal active。

## 2026-11-10 現行再開点（192457714 GUI全groupTempo作成保存build）

192457714はGUIの新規Tempoを全32groupへ作成する明示APIを追加し、三つのGUI経路へ接続。既存add_tracks/importedheaderは保持。4source変更288同一/292savedworking4outputs、manifest全bytes保持。未凍結tempo-propertiesへmixed/atomic/history/global120/legacy/position境界契約を追加。独立2196bytes入力の既存3tracks/17C6/Band/GUID/opaqueは保持し、Tempo80bytes position4/groupsALL32だけappend。実GUI Cancel/追加120/132.25/二UndoRedo/保存通常終了/別復元/固定240s新PCM19項目・7負例4clockを進める。全40全8/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-tempo-global-creation/20261109T191400000Z/unit-record-v2.json`。goal active。

## 2026-11-10 現行再開点（193553936 GUI全groupTempo作成保存build）

193553936はGUIの新規Tempoを全32groupへ作成する明示APIを追加し、三つのGUI経路へ接続。既存add_tracks/importedheaderは保持。4source変更288同一/292savedworking4outputs、manifest全bytes保持。未凍結tempo-propertiesへmixed/atomic/history/global120/legacy/position境界契約を追加。独立2196bytes入力の既存3tracks/17C6/Band/GUID/opaqueは保持し、Tempo80bytes position4/groupsALL32だけappend。実GUI Cancel/追加120/132.25/二UndoRedo/保存通常終了/別復元/固定240s新PCM19項目・7負例4clockを進める。全40全8/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-tempo-global-creation/20261109T191400000Z/unit-record-v2b.json`。goal active。

## 2026-11-10 現行再開点（194909234 GUI全groupTempo作成保存build）

194909234はGUIの新規Tempoを全32groupへ作成する明示APIを追加し、四つのGUI経路へ接続。既存add_tracks/importedheaderは保持。5source変更287同一/292savedworking4outputs、manifest全bytes保持。未凍結tempo-propertiesへmixed/atomic/history/global120/legacy/position境界契約を追加。独立2196bytes入力の既存3tracks/17C6/Band/GUID/opaqueは保持し、Tempo80bytes position4/groupsALL32だけappend。実GUI Cancel/追加120/132.25/二UndoRedo/保存通常終了/別復元/固定240s新PCM19項目・7負例4clockを進める。全40全8/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-tempo-global-creation/20261109T191400000Z/unit-record-v2c.json`。goal active。

## 2026-11-10 現行再開点（194909234 GUI全groupTempo作成・新PCM検証後）

194909234はGUI新規Tempoを全32groupで作成する明示APIを追加、四つのGUI経路へ接続し既存add_tracks/importedheader保持。5source変更287同一/292savedworking4outputs、関連5モード648checks（新21）合格。作者19800でNoTempo空欄/無効→AddTracks Cancel→Tempo追加120/1-32→CurrentTempo132.25を確認。二段階UndoRedo保存は期待全bytes、復元12236で132.25/再保存/二通常exit0。native差はTempo80bytes position4/groupsALL32のappendだけ、既存3track/17音/Band/opaque/GUID/control二17音保持。ProjectはFILETIMEとSegment1size+80以外一致。

新固定240s録音19/19。二回の主再生で17音/16間隔が60/132.25秒の許容±0.03に一致。C6/発音開始1秒内/25秒activeStop/停止後静音を確認。PCM負例7/clock4全拒否、元WAV保持。作者試聴0/Play0、復元private2/primary2/explicitStarts4/API結果0。旧6秒停止不合格保持。

CHM全group契約と既存正式原版のgroup1→ALL32保存・1-32表示を根拠に新規作成を実装。原版新規作成同入力動的比較/別process復元、GUI四経路の個別操作、importedheader正規化/global表示/Length/loop/fullgeometryは別未達。全40全8/50native30driver凍結/fullAcceptance=false。

Segment Lengthがgroup1のTempoを使う依存を次に修正する。拍子のgroup1軸を保ち、既存CurrentTempoと同じ制御Tempoの読み取りmapを用いる。既存group別編集・全nativeheaderを保全し、未凍結tempo-propertiesで別group/後続Tempo/可変map/invalid/history契約、保存build/関連/本体Length/保存通常終了/別復元/必要新PCMを進める。一次設計はnext-contract-audit-v1.json。既知OS凍結の同条件再試行なし、全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-tempo-global-creation/20261109T191400000Z/unit-record-v3.json`。goal active。

## 2026-11-10 現行再開点（201457752 Length制御Tempo保存build）

201457752はSegment LengthのTempo積分をgroup1編集Tempoから既存の全体制御Tempoへ分離。拍子軸group1と全importedheader/group/index/selectionは維持。4source変更288同一/292savedworking4outputs、manifest全bytes保持。新primary_tempos読取copyと既存未凍結tempo-propertiesへ23契約を追加。専用2356bytes5tracks/主132.25group2・後続180group1/17C6入力で、20Bars36.295sプレビューCancel→24Bars43.554s→UndoRedo/保存通常終了/別復元/必要新PCM19項目・7負例4clockを進める。原版同入力Length動的比較/TimeSiggroup設定/trim/loop/fullgeometry/全40全8は未達、50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-length-primary-tempo/20261109T201400000Z/unit-record-v2.json`。goal active。

## 2026-11-10 現行再開点（201457752 Length制御Tempo・新PCM検証後）

201457752はLengthのTempo積分をgroup1編集Tempoから全体制御Tempoへ分離し、拍子軸group1/全importedheader/cache/selectionを保持。4source変更288同一/292savedworking4outputs、関連5モード671checks（新23）合格。作者18928で主132.25/後続180入力の20Bars36.295sプレビューCancel、24Bars43.554s確定/Undo20/Redo24保存を確認。復元20560で24Bars/43.554s/132.25・再保存/二通常exit0。native差は2356bytes中segh lengthDWORD61440→73728の2bytesだけ、全5tracks/Tempo masks2・1/17音/Band/opaque/GUID/control二17音保持、ProjectはFILETIME以外一致。

初回240s録音はStop25.782sが許容25.5sを超え18/19で不合格のまま保全。別process/newprefsへ通常再起動し、Play直後に相対Stop待機を即提出する手順へ変更、閾値不変の新固定240s録音19/19。二回の主再生で17音/16間隔が60/132.25秒の許容±0.03に一致。C6/発音開始1秒内/25秒activeStop/停止後静音合格。PCM負例7/clock4全拒否し元WAV保持。作者試聴0/Play0、復元private2/primary2/explicitStarts4/API結果0。旧6秒停止不合格保持。

最低dwPosition/同値native順は既存sourcepolicyで、原版同入力Length動的比較/別process復元/輸入並替tieの全同等性は未達。TimeSiggroup設定/trim/loop/globalimported表示正規化/fullgeometryも別未達。全40全8/50native30driver凍結/fullAcceptance=false。

Sequenceの所属拍とloop包含、または残るglobal Tempo editor表示を一次資料・既存回帰・実機依存から監査し、独立して安全に進められる次単位を実装・保存build/関連/本体保存通常終了/別復元/必要新PCMまで継続する。既知OS凍結の同条件再試行なし。全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-length-primary-tempo/20261109T201400000Z/unit-record-v3.json`。goal active。

## 2026-11-10 現行再開点（210902079 Sequence所属拍loop検証build）

210902079は既存Sequence所属拍のnative loop包含を検証する単位。製品runtimeは変更せず、既存未凍結sequence-crudへloopheader/論理値/物理不変/二UndoRedo/再生copy/試聴loop分離27契約を追加。1testsource変更291同一/292savedworking4outputs、manifest全bytes保持。独立1976bytes入力は120BPM/無限Loop3072..6912/物理0・2304のC6二音。所属loop外baselineは二発音、物理2304保持の所属2:1編集後は11発音を各新240s・二25sStopで比較、各19項目・7負例4clockを事前固定。GUI二編集/UndoRedo/native保存/通常終了/別復元/比較PCMを進める。Loopタブ/原版同入力動的比較/全40全8は未達、50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-loop-belongs/20261109T210700000Z/unit-record-v2.json`。goal active。

## 2026-11-10 現行再開点（210902079 所属拍loop時刻不合格）

210902079のSequence所属拍loop検証で、baseline新240sは19/19・7負例4clock合格、所属2:1の別復元新240sは18/19不合格。両再生11発音は確認したが、期待1.5+2.5n秒より約0.31〜0.39秒遅れる。閾値不変・不合格PCM保全。関連5/698、二編集/二UndoRedo/native2bytesのみ/別復元/作者7524・復元16908通常exit0は検証済み。runtime実測prepare300msが負offset500msより短いことを原因候補として、lookahead方針を次単位で修正・新build/関連/新PCM再検証する。同条件再録音はしない。全40全8・50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-loop-belongs/20261109T210700000Z/unit-record-v3.json`。goal active。

## 2026-11-10 現行再開点（214458343 Sequence負offset準備窓build）

214458343は210902079で保全した所属拍loop PCM18/19不合格の修正候補。negative Sequence offsetを全native tracksから検出し準備窓1000msを確保、短縮は単独所有時のみ・共有時は増加可、即時開始300ms/serialized bytesは維持。3source変更289同一/292savedworking4outputs、新lookahead12契約。既存baseline/memberの19項目・7負例4clock・240sを一切緩和せず、新候補で両方再採取する。関連7（既知凍結style-player-default除外）/本体二編集二UndoRedo/native保存通常終了/別復元/新PCMを進める。旧不合格は昇格せず、原版同入力/Loop tab/全40全8/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-sequence-logical-lookahead/20261109T214400000Z/unit-record-v2.json`。goal active。

## 2026-11-10 現行再開点（214458343 baseline採り直し）

214458343（negative Sequence offsetは全tracks検出・prepare1000ms、source3/289same/292/4）は保存buildと関連7/870合格。初回baselineはStop42.846sのため18/19不合格のまま保全、作者8000通常exit0/4files保持。新作者16800/window203950746/newprefsで第二baseline240s（second-baseline/audio、capture220550868）を採取し、二Stop25.009/25.012、末尾録音中。次は厳密判定→負例7/clock4→所属1:4→2:4→2:1/二UndoRedo保存→通常exit→別復元/member240s。old210902079 member18/19時刻不合格、旧6s/Length失敗、50native30driver凍結保持。全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-sequence-logical-lookahead/20261109T214400000Z/unit-record-v2b.json`。

## 2026-11-10 現行再開点（214458343 所属拍lookahead新PCM後）

20261109T214458343ZはSequence負offsetに必要なnative準備窓を全Sequence tracksから判定し1000msを確保、共有時は増加のみ・単独時短縮可、即時開始300ms/serialized sourceは維持。3source変更289同一/292savedworking4outputs、新lookahead12契約、関連7/870checks合格。作者16800で所属1:4→2:4→2:1/各保存/二UndoRedo、別復元3032で2:1・物理1:4〜2:2/C6/96/Tempo120と再保存、二通常exit0。nativeは1976bytes中論理LONG/offsetSHORTの2bytesだけ、Loop/header/全他tracks/Band/opaque/GUID/control二17音とProjectのFILETIME以外を保持。

本候補の初回baselineもStop42.846sによる18/19不合格を保全、作者8000通常exit0後に新作者16800/newprefsへ変更して採り直した。新候補でbaseline/memberを各新固定240s・二25sStop採取し、各19/19・7PCM負例4clock全拒否。baselineは二発音[0,1.5]、memberは11発音[0,1.5,4,6.5,9,11.5,14,16.5,19,21.5,24]の事前±0.03s条件に一致。private2/primary2を両processで確認、実測prepareはbaseline300/member1000/private300、API結果0・cold/warm/onset1s内・停止後無音も維持。旧210902079のmember18/19遅延録音は不合格のまま保全し、閾値変更・旧PCM昇格はなし。

即時UndoのUIA値遅延を検出し、既存の後続保存キャプチャで値照合、全即時証拠と補正理由を保全。1000msは既存full prepare方針でありCHMの約1秒を超えるoffsetや任意Tempo/modifierの全域保証ではない。Loop tab/StartTime UI/clock-time loop/原版同入力動的・別復元/全geometry/全40全8は未達。旧6s停止・Length初回Stop失敗と50native30driver凍結も保持しfullAcceptance=false。

SegmentのLoopタブを一次CHM画像・既存native segh保全・既存Length/Sequenceとの依存から監査し、music-time Repeat/Infinite/開始終了の次単位へ進む。独立実装・保存build/関連/本体保存通常終了/別復元/必要新PCMまで続行し、既知OS障害の同条件再試行はしない。全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-sequence-logical-lookahead/20261109T214400000Z/unit-record-v3.json`。goal active。

## 2026-11-10 現行再開点（223629869 Segment Loop music-time build）

223629869はSegment Loop music-time tabの修正候補。Repeat/Infinite/開始終了BarBeatGridTick/Resetをmodeless Propertiesへ追加、native既定end0保持・3DWORDだけの履歴・Clock/reference-length編集拒否、Clock controlsはread-only表示。7source変更285同一/292savedworking4outputs、manifest全bytes保持。新model42/finite runtime20契約を既存未凍結modeへ追加予定。入力1976bytesは全区間Infinite0/0、負offsetnote/120BPMを保持。新baseline240sは7発音、GUIcustom3072..6912/Repeat切替/ResetUndoRedo/保存exit/別復元の新member240sは11発音、各19項目7負例4clock。原版同入力動的/clocktime/全40全8/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T223000000Z/unit-record-v2.json`。goal active。

## 2026-11-10 現行再開点（223629869 Loop初期表示修正へ）

223629869は保存build/関連7・932（新model42/runtime20）合格、finiteRepeat0/2の実native発音数と自然終了も合格。ただし本体初期Loop画面に旧4Bars残像とRepeat/ClockStartのUIA誤ラベルを確認し候補未採用。入力変更・試聴・再生・PCMなし、通常exit0・初期4files完全保持。画像/GUIA拒否/回帰/旧snapshotを保全し、タブ再描画とnativeラベル順を修正して別savedbuildから本体/新PCMまで進める。clocktime/原版同入力動的/全40全8/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T223000000Z/failed-candidate-v1.json`。goal active。

## 2026-11-10 現行再開点（225340637 Loop UI再描画build）

225340637は初回Loop候補223629869で観測した旧4Bars残像と誤UIAラベルを修正した保存buildをコンパイル中。タブ切替の子全再描画、Repeat/ClockStart/LoopStart/LoopEndのnativeラベル順を修正、model/runtimeと62新契約は初回同一。初回候補は関連7/932合格だが本体表示不合格のまま保全・通常exit0・初期4files完全保持・Play/試聴/PCMなし。次は新build全292/4照合→新関連7→Loop表示/マウスとCtrlTab→新baseline7発音/GUI区間・Repeat・ResetUndoRedo保存/別復元/member11発音の各240s。全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T223000000Z/failed-candidate-v1.json`。goal active。

## 2026-11-10 現行再開点（225340637 Segment Loop music-time UI再描画build）

225340637は初回223629869の旧4Bars残像/UIAラベル不具合を修正。タブ変更時の全子再描画とラベル順序を変更し、model/runtimeは初回から同一。Segment Loop music-time tabの修正候補。Repeat/Infinite/開始終了BarBeatGridTick/Resetをmodeless Propertiesへ追加、native既定end0保持・3DWORDだけの履歴・Clock/reference-length編集拒否、Clock controlsはread-only表示。7source変更285同一/292savedworking4outputs、manifest全bytes保持。新model42/finite runtime20契約を既存未凍結modeへ追加予定。入力1976bytesは全区間Infinite0/0、負offsetnote/120BPMを保持。新baseline240sは7発音、GUIcustom3072..6912/Repeat切替/ResetUndoRedo/保存exit/別復元の新member240sは11発音、各19項目7負例4clock。原版同入力動的/clocktime/全40全8/50native30driver凍結/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T223000000Z/unit-record-v2b.json`。goal active。

## 2026-11-10 現行再開点（225340637 Loop背景消去修正へ）

225340637も新関連7/932合格・UIAラベルは修復したが、Loop上の旧4Bars背景残像が残り未採用。WS_CLIPCHILDREN親/WS_CLIPSIBLINGSタブと透明GroupBoxのため、子表示後の再描画では旧背景を消去できない構造をsourceで確認。試聴/Play/PCM/入力編集なし、通常exit0・4initialfiles完全保持。子を隠して背景を消去してから新ページを表示する修正を進め、別savedbuildで新回帰/本体/保存復元/新PCMまで続行。全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T223000000Z/failed-candidate-v2.json`。goal active。

## 2026-11-10 現行再開点（230916238 Loop背景消去build）

230916238は第二225340637でも残ったLoop背景残像を、子コントロールを隠す→親/タブ背景消去→新ページ表示の順に修正。focus-loss編集はupdating guardで抑止し、マウス/CtrlTab/タブfamily変更へ適用。Repeat等のラベル修正は第二候補から保持、model/runtime/62新契約は初回同一。旧二候補は各新関連7/932合格だが本体描画不合格として保全、作者18876/1912通常exit0・4initialfiles完全保持・試聴/Play/PCMなし。7source/285same/292/4の新保存build後、新関連7/本体表示/新baseline7発音/GUI区間とRepeat/ResetUndoRedo保存/別復元/member11発音の各240sまで続行。clocktime/全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T223000000Z/failed-candidate-v2.json`。goal active。

## 2026-11-10 現行再開点（230916238 Segment Loop背景消去build）

230916238は第二225340637でも残ったLoop背景残像を、子コントロールを隠す→親/タブ背景消去→新ページ表示の順に修正。focus-loss編集はupdating guardで抑止し、マウス/CtrlTab/タブfamily変更へ適用。Repeat等のラベル修正は第二候補から保持、model/runtime/62新契約は初回同一。旧二候補は各新関連7/932合格だが本体描画不合格として保全、作者18876/1912通常exit0・4initialfiles完全保持・試聴/Play/PCMなし。7source/285same/292/4の新保存build後、新関連7/本体表示/新baseline7発音/GUI区間とRepeat/ResetUndoRedo保存/別復元/member11発音の各240sまで続行。clocktime/全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T223000000Z/unit-record-v2c.json`。goal active。

## 2026-11-10 現行再開点（230916238 CtrlTab修正へ）

230916238は新関連7/932合格、Loop旧4Bars残像と全ラベルは本体で修復確認。しかしCtrlTabはMDI共通処理が別rootのPropertiesキーを先取りして不動となり未採用。編集/試聴/Play/PCMなし、通常exit0・4initialfiles完全保持。MDI加速を同一rootのframe/MDIに限定する変更を追加し、新savedbuild/関連/本体/保存復元/新baseline7発音/member11発音の各240sまで続行。全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T223000000Z/failed-candidate-v3.json`。goal active。

## 2026-11-10 現行再開点（232353178 Loop CtrlTab root修正build）

232353178は第三230916238で発見したCtrlTab先取りを、workspaceと同一rootのframe/MDIだけにTranslateMDISysAccelを限定して修正。Loop背景消去/ラベル/model/runtime62契約は第三から保持。先行三候補の各関連7/932合格と本体失敗は別保存、作者全て通常exit0・4initialfiles完全保持・編集/Play/試聴/PCMなし。8source変更284同一/292/4の新保存build後、新関連7/本体マウスとCtrlTab/新baseline7発音/各編集履歴保存/別復元/member11発音の各240sまで続行。全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T223000000Z/failed-candidate-v3.json`。goal active。

## 2026-11-10 現行再開点（232353178 Loop CtrlTab root修正build）

232353178は第三230916238で発見したCtrlTab先取りを、workspaceと同一rootのframe/MDIだけにTranslateMDISysAccelを限定して修正。Loop背景消去/ラベル/model/runtime62契約は第三から保持。先行三候補の各関連7/932合格と本体失敗は別保存、作者全て通常exit0・4initialfiles完全保持・編集/Play/試聴/PCMなし。8source変更284同一/292/4の新保存build後、新関連7/本体マウスとCtrlTab/新baseline7発音/各編集履歴保存/別復元/member11発音の各240sまで続行。全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T223000000Z/unit-record-v2d.json`。goal active。

## 2026-11-10 現行再開点（232353178 Loopスピン方向修正へ）

232353178は新関連7/932合格、マウス/CtrlTab背景とMDI両方向切替合格、新baseline240sは19/19・7PCM4clock全拒否、二7発音/Stop25.008・25.010。だがLoopスピン上下逆を実操作で確認し未採用。数値Enter/CtrlS・Repeat0→1→Infinite・Reset二UndoRedoはdraft native全一致、作者9184通常exit0/4files保持、別復元/member録音は未実行。符号修正のみ追加した別savedcandidateで全本体/保存復元と新baseline/member各240sを実施し、旧録音は流用しない。全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T223000000Z/unit-record-v2e.json`。goal active。

## 2026-11-10 現行再開点（235737154 Loopスピン方向修正build）

235737154は第四232353178で本体確認したLoopスピン上下逆をiDeltaの符号修正だけで解消する新候補。8source変更284同一/292/4、model42/runtime20契約・背景消去・ラベル・CtrlTab root制限を保持。前unit223000の四候補失敗・第四baseline19/7PCM4clock合格・draft native全履歴一致/作者9184通常exit0は保全し、合格録音の新候補への流用はなし。同じ初期1976bytesを新証拠領域へ完全copy、新build関連7/本体全スピン方向/区間RepeatReset二UndoRedo/保存通常終了/別新設定復元/新baseline7発音/member11発音の各240sへ進む。全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T235737154Z/unit-record-v1.json`。goal active。

## 2026-11-10 現行再開点（235737154 Loopスピン方向修正build）

235737154は第四232353178で本体確認したLoopスピン上下逆をiDeltaの符号修正だけで解消する新候補。8source変更284同一/292/4、model42/runtime20契約・背景消去・ラベル・CtrlTab root制限を保持。前unit223000の四候補失敗・第四baseline19/7PCM4clock合格・draft native全履歴一致/作者9184通常exit0は保全し、合格録音の新候補への流用はなし。同じ初期1976bytesを新証拠領域へ完全copy、新build関連7/本体StartBar/Repeatの上下スピン/区間RepeatReset二UndoRedo/保存通常終了/別新設定復元/新baseline7発音/member11発音の各240sへ進む。全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T235737154Z/unit-record-v2.json`。goal active。

## 2026-11-10 現行再開点（235737154新baseline後・本体継続中）

235737154の関連7/932・本体マウス/CtrlTab/MDI往復・StartBar/Repeat上増下減/全probe保存/初期1976bytes復帰が合格。作者18312/root83036946、96DPI1280x800、新prefs/trace、launcher3181は現在稼働。新baseline240sは19/19・7PCM負例4clock全拒否、二7発音[0,1.5,8,9.5,16,17.5,24]、Stop25.009/25.004、onset private.630/.610・main.495/.465。WAV2c1c48ab66e0fb187c7b932dd48e76585e573f2934844a69caae79c522712722。録音前後4filesと292source4outputs完全保持。次は空白Sequenceセルで試聴選択を解除（Escapeでは解除不可）→Tree/F11でSegment Loop→開始2:1/終了3:1→3:2/Repeat0→1/Infinite/Reset二UndoRedoと各保存→通常終了→別newprefs復元/新member240s11発音。現時点はauthor継続中、保存履歴/別復元/member未達、旧第四baselineは流用せず全40全8/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-loop-music/20261109T235737154Z/unit-record-v2a.json`。goal active。

## 2026-11-10 現行再開点（235737154 Loop保存復元新PCM後）

20261109T235737154ZはSegment PropertiesにLoopタブを追加し、Repeat/Infiniteとmusic-time開始終了BarBeatGridTick/Resetを操作可能にした。CHM原版画像の配置・ラベルに合わせ、先行候補で観測した旧Length背景残像を子非表示→背景消去→新ページ表示で修復し、CtrlTabのMDI先取りは同一root制限、上下スピンはiDelta符号で修復。8source変更284同一/292savedworking4outputs、新model42/有限runtime20契約、関連7/932checks合格。作者18312でマウス/CtrlTab/開始2:1/終了3:1→3:2/Repeat0→1/Infinite/Reset/二UndoRedoと各保存、別復元14116でInfiniteと区間2:1:1:0〜3:2:1:0を復元し再保存、二通常exit0。

nativeは1976bytesのsegh Repeat/loopStart/loopEndの3DWORDだけ編集可能で、実際の最終値はInfinite/3072..6912の2bytes差分。既定end0とReset0/0を保持し、length/start/flags/tail/全tracks/負offset論理物理時刻/Band/opaque/GUID/control二17音、ProjectのFILETIME以外を保持。新baseline/member各固定240sは各19/19、7PCM負例4clockを全拒否。whole0/0は二episode各7発音[0,1.5,8,9.5,16,17.5,24]、customは各11発音[0,1.5,4,6.5,9,11.5,14,16.5,19,21.5,24]、事前±0.03sに一致。private2/primary2、prepare main1000/private300/API結果0、二25±.5sStop・停止後無音も確認。有限Repeat0/2は別native runtimeで各2/4発音と[0,576]/[0,576,1344,2112]clocks、自然終了を確認。

別復元のcold Play直後にTree/timeline描画欠落、次Stopで復帰を観測し全rawを保全。途中frame未観測のため持続時間は不明。loop cursorもlinear elapsedのlength-1 clampのままで未受入。Clock Time controlsはread-only/disabledで、clock/reference-length入力のmusic-time編集は原子拒否。Info/Boundaryタブ、clock-time loop、StartTime UI、loop中caret追随、原版同入力動的・別復元、全geometry/全40全8は未達。Finite fallbackは同一session内だけ保持し、Infiniteの別復元で有限数値の永続化を主張しない。先行四候補の表示・操作不合格・旧録音不合格・50native30driver凍結を保全しfullAcceptance=false。

Loop設定の終了条件を保存した後、cold Play描画欠落とloop中再生位置表示、StartTimeの一次資料・native/runtime依存を監査し、独立する次単位へ進む。実装/保存build/関連/本体保存通常終了/別復元/必要新PCMまで続行し、既知OS障害の同条件再試行はしない。全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-segment-loop-music/20261109T235737154Z/unit-record-v3.json`。goal active。

## 2026-11-10 現行再開点（005605175 Loop cursor）

005605175はLoop再生位置表示の新候補。5source変更287同一/292saved-working4outputs。音源スケジュール/native bytesを変更せず再生時snapshotのlength/loop/seekとelapsedからfinite intro-repeat-outro/Infinite wrapを表示しBand live判定にも適用。34model契約と実native時刻基準/反復/末尾サンプルを追加。関連7、本体cold Play描画観測と複数wrap、保存通常終了、別設定復元再保存、新custom-loop240s PCMは未達。原版同入力動的/clock-time/描画修復全域は未受入。既知50native30driverと四Loop候補の失敗を保全、全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-segment-loop-cursor/20261110T005500000Z/unit-record-v2.json`。

## 2026-11-10 現行再開点（005605175 Loop cursor保存復元新PCM後）

005605175はmusic-time Loop中の再生位置を曲末clampから反復区間の折り返しへ修復。再生時snapshotのlength/loop/明示seekとelapsedを使用し、Repeat0 linear、有限intro→反復→outro、Infinite wrap、end0/巨大時刻/refusalを34model契約で確認。Band live判定も同じ位置を使う。5source変更287同一/292saved-working4outputs、関連7/988合格。実nativeではRepeat0/2の177/281samples、折り返し0/2と有限outro・自然終了、Infinite235samples/4wrap/elapsed3847と6発音を確認、GetStartTime基準は全て固定。

作者18116の本体で元8s曲長後もUIA4494→3432→6391 clocksと区間3072..6911内を移動。Play Cursor off二観測6742固定、on再開、通常停止3702。保存/通常exit0、別設定復元4812でInfiniteと2:1:1:0〜3:2:1:0を復元再保存、別起動も通常exit0。3SGP全byte、ProjectFILETIME以外を保持。新固定240sは19/19、7PCM/fourclock全拒否、二episode11発音[0,1.5,4,6.5,9,11.5,14,16.5,19,21.5,24]完全一致。private onset.630/.565、main.505/.455、Stop25.008/.007、停止後無音。WAV 762d0429aa9a4c3ddb9ff1f04bf9e7bf8ada55dcb60b3789883f486345521d2c。

動く間のUIAと画像は同時取得ではなく値差を保全。作者sample-threeでTree/toolbar一部欠落、別起動初期Tree白も観測、持続時間と原因は未確定。描画修復/原版同入力動的/clock-time cursor/任意seek全域/StartTime UI/InfoBoundary/全geometryは未受入。Clock/reference又は反復の不正bounds・seek>=loopEndは既存linearへfallbackしmusic-time合格範囲外。既知OS50native30driverと旧録音・Loop四候補失敗を保全。全40/全8は6working/2blocked/0passのままfullAcceptance=false。

カーソル限定範囲の保存を終え、再生中Tree/toolbar/timelineの描画欠落をsource・観測時刻から監査し、根拠を得た変更だけ新候補として実装/関連/GUI保存通常終了/別復元/必要新PCMへ続行。StartTime UIとPlayFromStartのnative既定開始点も次依存。既知OS障害の同条件再試行は避け、goal active。

一次入口：`work/analysis/ui-segment-loop-cursor/20261110T005500000Z/unit-record-v3.json`。

## 2026-11-10 現行再開点（012618577 toolbar steady refresh）

012618577はTransport steady refreshの新候補。toolbar1source変更291同一/292saved-working4outputs。100ms更新ではnative state/text/selectionを照合して同値の書込みを抑え、tempo等sibling配置をgeometry変更時へ制限。音源/native/loop cursor/runtime/check数は変更なし。関連7/988、本体Tempo入力保持Escape/AlternateTempo75→50/再生停止と複数frame/保存通常終了/別復元/新custom-loop240s PCMは未達。既存描画欠落との因果と全描画修復は未確定。全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-toolbar-steady-refresh/20261110T012600000Z/unit-record-v2.json`。

## 2026-11-10 現行再開点（012618577 toolbar steady保存復元新PCM後）

012618577は100ms Transport更新で同値のnative button state/text/selection書込を抑制し、tempo/spin/primary sibling配置をgeometry変更時と明示layoutへ制限した。frame_toolbar.cppの1source変更291同一/292saved-working4outputs、音源/native/loop cursor/runtimeとテスト契約は変更なし。新関連7/988合格。

作者21056で未確定132.25を保持し、Escape後も保持する既存原版仕様を確認。計画の「Escapeで120復元」という誤前提はprotocol-correction-v1に全旧資料/rawとともに保全し、product/source/audio基準は変えず、120再入力Returnの同値確定で復元。AlternateTempo50→75→50表示、三定常Play画像（11.716/25.211/41.141s）でTree/toolbar/timeline、disabled120/Stop/cursor区間内、PlayCursoroff二観測5332固定/on再開、Stop後enabled120とPlay復帰を確認。UIA modal80 set_value拒否はinputなしで記録し再試行せず、実画面の数値選択と通常キーで完了。作者通常exit0/別新設定14660復元再保存/通常exit0。3SGP全byte・ProjectFILETIME以外を保持。新240sは19/19、7PCM/fourclock全拒否、二episode11発音列[0,1.5,4,6.5,9,11.5,14,16.5,19,21.5,24]が事前±.03s内一致。private onset.660/.565、main.485/.465、Stop25.005/.015、停止後無音。WAV 8f9cb6fb1638d8b9476adc9a8b9b8dcd88cb1eb190c461292ab0a904ea8e19e1。

初期Tree白は次idle画像で復帰。三定常Play画像は描画できたが、cursor-on-movingではノート/Band label欠落、close-properties直後にはtimelineほぼ白を観測、全raw保持。開始/終了/原因の全時間計測はなく、全描画修復を主張しない。UIAと画像の動く値の取得時刻差も保持。原版同入力動的/全geometry/StartTime/clock-time/InfoBoundary/全40全8は未達。既知OS50native30driverと旧録音・四Loop候補失敗を保全。6working/2blocked/0passのままfullAcceptance=false。

定常toolbar更新単位の記録後、Segment paintが白地消去→部分描画を画面DCへ直接行う経路と欠落画像を一次API資料で照合し、描画をmemory DCで完了後に反映する限定修正へ進む。必要な関連/実画面複数frame/保存通常終了/別復元/新PCMまで続ける。StartTimeのnative/runtime既定開始点とUI依存も次queueに保つ。既知OSの同条件再試行はしない。全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-toolbar-steady-refresh/20261110T012600000Z/unit-record-v3.json`。

## 2026-11-10 現行再開点（020605949 Segment buffered paint）

020605949はSegment buffered paint新候補。timeline1source変更291同一/292saved-working4outputs。既存描画をcompatible memory DC/bitmapへ行い完了後BitBlt、単独background eraseを抑制、RAIIでbitmap/DCを復元解放、割当失敗は従来direct描画へfallback。音源/native/toolbar/loop意味論は変更なし。関連7/988、本体再生/複数frame/カーソルoff-on/Propertiesclose/実PID GDI増加<=8/保存通常終了/別復元/新240s PCMは未達。Tree初期白と全geometry/原版動的は範囲外未受入、全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-segment-buffered-paint/20261110T020500000Z/unit-record-v2.json`。

## 2026-11-10 現行再開点（020605949 Segment buffered保存復元新PCM後）

020605949はSegment描画をcompatible memory DC/bitmap内で完成後、BeginPaintのclipを保った画面DCへBitBltする限定修正。WM_ERASEBKGNDの単独消去を抑え、RAIIで元bitmapを復元してGDI資源を解放し、割当失敗は従来direct描画へfallbackする。timeline1source変更291同一/292saved-working4outputs、音源/native/toolbar/Loop意味論は変更なし。関連7/988合格。

作者10292でProperties close、cold Playと14.505/28.689/45.972秒後の三frame、Play Cursor off二観測6167固定/on再開/Stopを確認。観測したtimeline内ではruler/grid/二C6notes/Band1/cursorが保持された。同一control構成のGDIは90→91（事前上限+8内）。作者通常exit0、別新設定8120でInfiniteと2:1:1:0〜3:2:1:0/Tempo120を復元再保存し通常exit0。3SGP全byteとProjectFILETIME以外、292source/4outputsを保持。新固定240sは19/19、7PCM/fourclock全拒否、二episode各11発音[0,1.5,4,6.5,9,11.5,14,16.5,19,21.5,24]が事前±.03s内一致。private onset.575/.560・main.475/.475、Stop25.014/.004、停止後無音。WAV f20840f17a7059b65ee279bfe7940bb16e462727b2569dd1c2f6a9ca2394c65b。

先の録音二件は0actionsとfirst Stop34.501秒/4actionsで不合格のまま保全し、両起動10348/9312も通常exit0/入力完全保持。第三では短いUIA確認へ変更し、各Play直後三応答のStop欠落とhelper errorも保全。別read-only観測で実Stop173/181を確認後に別入力で停止し、基準は変更していない。作者Properties close直後にはtimeline clientより上の親frame灰色域へ白矩形が残り、次stable frameで復帰。初期Tree白も保全。全時間描画/親frame/全geometry/原版同入力動的/StartTime/clock-time/InfoBoundary/全40全8は未受入。GDIは限定増加試験であり普遍的なleakなしとはしない。既知OS50native30driverと旧録音・四Loop候補失敗を保全。6working/2blocked/0passのままfullAcceptance=false。

限定timeline描画単位の記録後、Properties非表示時の親frame白矩形とrefresh/layoutの再描画順を照合し、根拠を得た次候補の実装/関連/本体保存通常終了/別復元/必要新PCMへ続行。StartTime UIとPlayFromStart既定開始点のnative/runtime依存もqueueに保つ。既知OS障害の同条件再試行はしない。全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-segment-buffered-paint/20261110T020500000Z/unit-record-v3.json`。

## 2026-11-10 現行再開点（031819499 frame refresh paint）

031819499はframe refresh最終再描画を同期完了する候補。main1source変更291同一/292saved-working4outputs、関連check契約/音源/native/toolbar/loopは変更なし。既存layout_frameと同じRedrawWindow flagsでframeと子controlを更新する。親白矩形/Tree/timelineの本体frame、GDI増加<=8、関連7/988、保存通常終了/別設定復元/新240s PCMは未達。全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-frame-refresh-paint/20261110T031700000Z/unit-record-v2.json`。

## 2026-11-10 現行再開点（031819499 frame refresh checkpoint・PCM未達）

031819499はframe refresh末尾をInvalidateRect予約から既存layout_frameと同じ同期RedrawWindow(INVALIDATE|ERASE|ALLCHILDREN|UPDATENOW)へ変更し、旧control非表示後のframe/子control描画を完了させる候補。main1source変更291同一/292saved-working4outputs。音源/native/toolbar/Loop意味論は変更なし。関連7/988合格。初回選択名style-part-notesは未登録で6モードだけ実行されたため、正式style-playerのみ追加実行し先の6は繰り返していない。両runと訂正を保全。

作者16976の初期Tree、再生前保存とPropertiesclose、cold Playと13.850/24.011/36.011秒後の三frame、Cursoroff二観測5686固定/on再開/Stop4720、停止後保存とPropertiescloseを確認。別新設定2444でInfiniteと2:1:1:0〜3:2:1:0/Tempo120復元再保存、第三のPropertiescloseも確認。三保存close直後の親灰色域に白矩形は見られず、観測したtimeline内の二C6notes/Band1/grid/rulerを保持。GDI91→92（事前+8内）。二通常exit0、3SGP全byte/ProjectFILETIME以外と292source4outputs完全保持。

別起動の初期Treeは白く、Segment openで復帰したため全描画修復は未受入。新240s PCMは試聴二回/本再生一回/Stopの4actionsで、first Stop86.585秒となり不合格。各Play/Stop直後UIA応答欠落と別read-only復帰観測を保全。compact UIA/text/短い事前保存tool要求でも発生し、遅延原因は未確定。同条件の再録音は凍結し、基準緩和・旧WAV流用・欠落restart/finalStop補作なし。positive auditorはrequired actions不足でexit1、negative controlsは未実行。WAV 8c20dc6a3017a9f75114314b95396f2c277f4ff8d5472e5b8e821e8463519e5f。新PCM終了条件はfalse、単位boundedFinished=false。原版同入力動的/全geometry/全時間描画/StartTime/clock-time/InfoBoundary/全40全8も未達。既知OS50native30driverと全旧失敗を保全し6working/2blocked/0pass/fullAcceptance=false。

031819499の新PCMは実行条件が変わるまで未達として保持し、同条件再録音をしない。独立するSegment StartTimeの原版CHM/保存DWORD8/PlayFromStart明示0とnative既定開始点を監査し、model/native依存から実装・関連検証を続行する。後続GUI保存通常終了/別復元/必要新PCMも未達を明記し、音声条件を緩和しない。全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-frame-refresh-paint/20261110T031700000Z/unit-record-v3.json`。

## 2026-11-10 現行再開点（035502578 StartTime saved build）

035502578はSegment music StartTime候補。7source変更285同一/292saved-working4outputs。segh mtPlayStart DWORD8だけのatomic getter/setter、原版StartTime4入力とspin/Enter/focusloss/CtrlTab/CtrlA、primary TimeSig group1表示、PlayFromStart設定点とPlaycursorの独立、private start0を実装。clock/reference等は編集範囲外。model41契約とnative既定384/明示0/明示384/Infinite四caseを追加、関連7と全本体保存復元/新PCMは未達。親031819499の86.585秒停止PCM不合格と同条件再録音凍結を保持。native実測前に新PCM列を宣言しない。全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-segment-start-time/20261110T034900000Z/unit-record-v2.json`。

## 2026-11-10 現行再開点（035502578 StartTime label不合格）

GUI label layout failed before any edit/Play/audition/save/PCM. Related7/1069/native4cases preserved;author7824normalexit0/all4fileswholeexact. New explicit English8point StartTime font candidate required.

Use TimeSig of Track Group wraps and is clipped; Extend Primary TimeSig is clipped with locale-dependent DEFAULT_GUI_FONT in compact original layout. Use explicit MS Sans Serif8/ANSI logical font only for new StartTime controls, preserve original compact geometry; own/delete font after children destruction; new saved build/related/GUI before further input. 全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-segment-start-time/20261110T034900000Z/failed-candidate-v1.json`。

## 2026-11-10 現行再開点（041453112 StartTime font saved build）

041453112はStartTime英語font修正候補。035502578の関連7/1069/native4case合格とlabel表示不合格/未編集未再生/作者7824normalexit0/4files完全保持を保全。Properties2source変更290同一/292saved-working4outputs、feature base031819499からは7変更285同一。新StartTime controlsだけMS Sans Serif8point/ANSIを用い、NCDESTROYで解放。独立GDI実測ではstock MS UI Gothicの146/123+16pxは144/134枠を超え、MS Sans Serif135/111+16pxは収まる。既存model41/native4case・操作/native/audio意味論は保持。新関連/全GUI保存復元/GDIcycle/新PCMは未達。親PCM不合格同条件再録音凍結とfull40/8=falseを保持、goal active。

一次入口：`work/analysis/ui-segment-start-time/20261110T041200000Z/unit-record-v2.json`。

## 2026-11-10 現行再開点（041453112 StartTime保存復元・新baseline/member PCM後）

041453112はSegment music-time StartTimeの限定実装。segh mtPlayStart DWORD8だけをatomic変更し、原版CHMに沿うBar/Beat/Grid/Tick入力・spin・Enter/focus-loss/CtrlTab確定・CtrlA、PlayFromStart設定点と通常Playカーソルの独立を接続。clock/reference編集は対象外。feature base031819499から7source変更285同一、直前label不合格035502578からProperties2変更290同一、292saved-working/4outputs完全一致。新StartTimeだけMS Sans Serif8point/ANSIを所有し子破棄後に解放。関連7/1069、model41契約とnative既定384/明示0/明示384/Infinite四case合格。

作者7904で全4spin上下、Bar0/Tick192拒否、Enter/Tab/CtrlTab確定、最終1:3:3:0=1920、Tree Undo二回1920→1536→0とRedo二回→1536→1920を各保存。25段階のnative照合で開始DWORD以外の全Segment byte・他2SGP・Project FILETIME以外を保持。最終差は2byteのみ。設定開始点1920とeditor cursor3332を実primary要求で区別し、元8秒後もループ範囲内のカーソル、停止安定を確認。Properties三cycleのGDI90→90（事前上限+8内、普遍的leakなしとはしない）。作者7904、復元14640、座標診断17816、別復元21284は全て通常exit0。別新設定でStart1920/Infinite3072..6912/Tempo120を復元再保存。

新baseline0固定240sは19/19、7PCM/fourclock全拒否、二episode11発音[0,1.5,4,6.5,9,11.5,14,16.5,19,21.5,24]、WAV2373cee027982cfbd662c2d5c8d1b676b4b71562a6428c989124eddbce9f94e5。新member1920固定240sも19/19、7PCM/fourclock全拒否、二episode10発音[0,2.5,5,7.5,10,12.5,15,17.5,20,22.5]、既存±.03秒内。private onset.595/.580、main.715/.725、Stop25.013/.010、停止後無音。WAVefc59b56673972f8c4a09378f3fbbc9c5f0792e03b9b46ef94fe8038167c66cd。両phaseで新PIDの二つの無入力25秒試験を通過し、入力・生成物hash/UTC-QPCを照合。閾値緩和/旧WAV流用なし。

member初回はUIA再Play入力が本体要求へ到達せず5actions、required-action監査exit1として保全。Stop25.012自体は範囲内、再送や欠落finalStop補作なし。同じUIA手順を凍結。別PIDの実画像座標診断で二primary要求1920/二Stopとnative完全保持を実証して入力方法を変更し、更に新PIDと二応答試験を経た別録音だけが合格。正確なUIA入力不達原因は未確定。最初の制限付きshell PID確認失敗は録音前・recorder未起動で、実在を読み取り照合後に同じ可視範囲で録音。protocolのmember nativeExpected.playStart残存0は録音前に1920へ訂正し原文を別保存、発音列・閾値は不変。035502578 label不合格、親031819499の86.585秒Stop PCM不合格と凍結は保持し、遡って合格にしない。初期Tree白/動く画像とUIAの時差も保全。原版同入力動的・全geometry・任意TimeSig group・Extend・embedded AudioPath・clock/reference・Info/Boundary・全40全8は未受入。6working/2blocked/0pass/fullAcceptance=false。

StartTimeの限定結果を保存し、Segment Properties Info tabの原版CHM/静止図と既存UNFO/INFO保存モデルを照合する。未知chunk/header/文書GUID/履歴を保護し、根拠のあるInfo編集をmodel→保存build→関連→本体保存通常終了→別復元→必要音声/原版比較まで進める。既知50native/30driver凍結と旧失敗を条件変更なしで再試行しない。全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-segment-start-time/20261110T041200000Z/unit-record-v3.json`。

## 2026-11-10 現行再開点（054427509 Segment Info初回build中）

054427509はSegment Info文字情報/Version候補を保存build中。Subject USBJ/Author UART/Copyright UCOP/Information UCMTをUNAM/GUIDと独立して編集し、root vers最初8byteのみ変更、未知tail/odd padding/順序/選択/historyを保持。空欄同値no-opと曖昧/不正targetのatomic拒否を追加。3番目Info tab、8入力、Enter/focus-loss/CtrlTab/CtrlA、Information通常Enter改行/CtrlEnter確定、CtrlShiftTab逆巡回を接続。文字32767code-unit上限は実装上限、Version GUI0..9999/保存WORD全値を区別。Edit GUIDは参照更新依存のためdisabled、Boundary/原版動的/全40全8未達。親041453112の新baseline/member PCM合格と全旧失敗を保全。関連7/GUI保存通常終了/別復元/必要新PCMは未実行。fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-segment-info/20261110T054000000Z/unit-record-v1a.json`。

## 2026-11-10 現行再開点（055654512 Info検査fixture修正build中）

初回054427509関連は6pass/sequence-crud検査8失敗。文書load後の各field反復でfixtureの選択rangeを設定し直す2行だけを修正、保持assertと製品コードは変更なし。初回build/run/旧sourceとnative入力・GUI音声未実行を保全。055654512の新保存build/session18253を待ち、保存/作業ソースと4outputs照合→関連7→本体8入力/3tab/native保存通常終了→別新設定復元→必要新PCM10へ継続する。全40全8/6working2blocked0pass/fullAcceptance=false、goal active。既知OS50native30driverの同条件再試行なし。

一次入口：`work/analysis/ui-segment-info/20261110T054000000Z/unit-record-v1b.json`、初回失敗：`fixture-failure-v1.json`。作者PID未起動。

## 2026-11-10 現行再開点（063900475 Info Tab修正build中）

055654512は関連7/1125と新Info56modelに合格。作者8940でUnicode Subject、Author/Copyright、Information通常EnterのCRLF、全Version欄、空欄/負数拒否、4桁上限、空Subject復帰、対象Seed切替の元owner確定、二Undo/二Redoを実操作。18段階のnative全期待バイトとpeer全不変、ProjectはFILETIMEとSegment1サイズ2070だけを明示照合。作者通常exit0、Play/auditionなし。Copyright Tabが視覚順Versionを飛ばしてInformationへ進み、Information Tabも留まる観測を保全。原版動的Tab比較ではなく実装の操作性判断で作成順とplain Tab/ShiftTabを修正。063900475はProperties cpp一source変更、build81279中。旧0556はnavigation/別復元/newPCM未達のまま保全。新build/関連7/八欄両方向Tab/確定/3tabs/GDI/保存通常終了/別newprefs復元/二応答試験後の新240s member10 PCM19/7/4へ継続。旧WAV昇格なし、全40全8/6working2blocked0pass/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-segment-info-navigation/20261110T063900475Z/unit-record-v1.json`。前作者：`work/analysis/ui-segment-info/20261110T054000000Z/author-final-native-proof-v1.json`。

## 2026-11-10 現行再開点（063900475 Info保存復元後・clock不合格）

063900475はSegment Info文字/VersionとTab操作の候補。親055654512のUnicode/CRLF/4Version/拒否/4桁上限/空Subject/owner切替/二UndoRedo/18native/作者8940normal0を保全。新候補はProperties cpp一source変更291同一、feature base041453112から5変更287同一、292saved-working/4outputs一致、関連7/1125（Info56model/native4starts）。作者20808で全8欄を視覚順にTab/逆順、Informationから離脱、Subject Tab確定/Undo、Information Enter改行/CtrlEnter/Undo、CtrlTab/逆巡回を確認。末尾と先頭間にはnativeページTabが入り、事前手順の省略を旧原文/実観測付きで訂正、製品/音声閾値変更なし。GDI三作成90→91、7native段階全期待バイト、Project FILETIMEのみ。作者20808/別newprefs復元14356はnormalexit0。別復元でUnicode/CRLF/Version1.2.3.4/Start1920/Infinite3072..6912/Tempo120を保持して再保存。

新member固定240秒録音はrecorder0・六実入力・Stop25.005/25.011秒・停止5225安定。ただしUTC/QPC drift8.5643msが既定5msを超え、positive監査が時刻gateでexit1、PCM19項目/負例7/fourclockは未成立。WAV626394e11e690d77fcd94d0a761e0cb84aff2c9d0e683988b6659cb49e6d3391と全時刻/packet/入力/監査sourceを保全。negativeは実行せず、同条件再録音凍結/閾値不変/旧PCM昇格なし。二つの無入力25秒試験25.010/25.009合格だけでは240秒のclock整合を保証しなかった。音声受入newPcm=false/boundedFinished=falseで保持。実runtimeでは二primary要求1920/二private/二Stopを確認し、録音後/通常終了後の全4nativeファイルとsource/output完全保持。

Infoの新PCMはclock不合格のまま保全し、条件が変わらない再録音をしない。独立作業としてSegment Boundary一次auditを継続し、根拠のあるdefault-resolution保存モデル/GUI/関連/本体保存通常終了/別復元/必要音声へ進む。Boundary原版静止図/CHM/SDK/4native sample auditはwork/analysis/ui-segment-boundary/20261110T064400000Z/primary-audit-v1.json。原版動的/全geometry/全40全8は未達、6working2blocked0pass/fullAcceptance=false。既知OS50native30driverや正式原版起動を同条件再試行しない。

一次入口：`work/analysis/ui-segment-info-navigation/20261110T063900475Z/unit-record-v2-failed-clock.json`。

## 2026-11-10 現行再開点（073053576 Boundary基本設定build中）

Segment Boundaryの基本4境界Immediate/Grid/Beat/Barline、Start at next marker、Quick Response/After Prepare Timeを保存DWORD20の独立bit編集と4番目のtabへ接続。未知bit/全header/拡張tail/odd padding/Info/GUID/tracks/historyを保持し、Align/Queue/End/Default/競合boundaryは基本選択へ正規化せず拒否、marker/timingは独立保持編集。Align/Invalidation/追加境界はdisabled未受入。新規default自体は変更せず、原版new defaultの動的比較未達を残す。7source変更、build40115候補073053576中。model144予定、native4境界default readback/Stop bracket16予定はまだ未実測。次はsaved-working292/4outputs→関連7→本体4tab/選択/marker/timing/履歴/native保存通常終了/別newprefs復元。

親Info063900475は保存復元/2normalexitを保全し、新240秒PCMのclock drift8.5643ms>5ms gate exit1/newPcm=false/boundedFinished=false/同条件再録音凍結を維持。録音しない読み取り専用240秒時計照合probe24753を実行中。最初の試験が5ms超なら二回目へ進めず、二試験合格だけが新clock条件の根拠になる。過去WAVを合格にしない。正式原版/50native30driver等の同条件再試行なし。6working2blocked0pass/全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v1a.json`。

### 073053576 build中のclock条件照合

録音しない同じPrecise FILETIME/QPCの240秒診断は8.5674ms>既定5msで不合格。条件変化の根拠なしとして二試験目へ進まず、新録音も実行しない。OS時計設定/閾値/旧WAVは変更なし。Boundaryのsource/build/関連/本体保存通常終了/別復元を継続し、必要PCMは未達として残す。一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/clock-condition-observation-v1.json`。

### 073053576 relatedのnative観測不合格

関連6modeと新model144（sequence-crud697）は合格。sequence-audition-runtimeは新Immediate caseのnative default readback0に合格し、次のStop timing check2で不合格。StopEx return0ms、request1716→IsPlaying false1906、差190clocks。旧事前bracketを超えており、不合格とraw観測を保持。原因は未確定で、state pollを実際のStop scheduleとみなした仮定をSDK通知timeflags/reference time/valid musicTimeで切り分ける。閾値緩和/旧run遡及合格はしない。GUI/PCM未実行、入力全不変。時計条件も不変で新PCM凍結。一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v2-native-failed.json`。

## 2026-11-10 現行再開点（075007680 Boundary通知時刻観測build中）

旧073053576のnew model144/related6合格とImmediate default readback0合格、Stop state-poll timing check2失敗を保全。新候補075007680は通知のraw timeFlags/referenceTime/valid resolved musicTimeを値コピーへ追加し、既存clocks/identity/lifetime/再生scheduleは維持。time-validityは保存Microsoft SDKのMUSICTIME2/REFTIME1に従い、必要なら既存ReferenceToMusicTimeで変換、失敗時optionalなしで通知を捨てない。新native4はowned terminal通知の時刻を同じrequest bracket -32/+64へ照合し、IsPlaying終了pollの遅れは別diagnosticへ残す。StopEx100ms、PCM/clock閾値は不変、旧run遡及合格なし。直接親からConductor2とtestheader1変更、feature base0639から9source予定。build42954中。

次は292saved-working/4outputs→関連7/model144/native4通知→新PID本体4tab/4境界/marker/timing/履歴/Info保持/native保存通常終了/別newprefs復元。clock条件240秒診断8.5674ms>5msのため新録音なし/二試験目なし、Info0639旧WAV不合格を保全。全40全8/fullAcceptance=false、goal active。一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v2a.json`。

### 075007680 通知時刻と新Timeline OS拒否

新関連は5mode合格/new model144（sequence697）。Immediate default readback0合格、通知はowned id1/SEGABORT4/validflags19/raw=resolved1908で、current GetTime1731より177clocks先、IsPlaying終了1918。通知にも差があるためstate表示遅れだけで説明できず、native送出可能horizonと境界解決を次候補で読み取る。旧current-time bracket不合格と同じ32/64幅を保持し、再生scheduleは変えない。

Timeline回帰は67check後、TimelineProject/Range.sgpのatomic replaceがWindows error5で拒否、destinationを保全。書込実装riff.cppは前候補と同一。Range経路を同条件再実行/出力先変更/rename/代替testで迂回しない。次runは未阻害6modeのみとし、全7関連の終了条件はfalseを維持。clock8.5674ms不変で新PCMも凍結。GUI/PCM未実行、全入力不変。一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v3-native-failed.json`。

## 2026-11-10 現行再開点（080818423 Boundary native horizon観測build中）

Conductorの読み取り専用観測へGetTime/GetLatencyTime/GetQueueTimeと、latency時点のGrid/Beat/MeasureをGetResolvedTimeで解決したnative値を追加。既存StopEx(...,0,DEFAULT)/default値/再生scheduleは変更なし。新native caseでは前後のnative horizonとowned valid terminal通知を同じ-32/+64幅で照合し、旧current-time/IsPlaying比較を別diagnosticで残す。実測前に原因や合格を断定しない。候補080818423、build3954中、直接親0750からConductor2/test1変更、feature9/292予定。旧0730/0750のnative timing失敗を保全。

次回は未阻害6modeだけを実行。Timeline Range.sgp Windows5 after67は同条件再試行/relocate/rename/代替testなしで凍結し、全7関連の終了条件falseを保持する。独立した本体metadata/4tab/保存通常終了/別newprefs復元は継続でき、GUI境界再生はnative時刻観測合格後のみ。clock8.5674ms>5ms不変で新録音/二probeなし。Info0639旧WAV不合格も維持。全40全8/fullAcceptance=false、goal active。一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v3a.json`。

### 080818423 native horizon観測と独立GUI再開

未阻害5mode/new model144合格、Timeline経路は再実行なし。Immediateはnative latency/queue1856、current1678、IsPlaying終了1858を観測したが、その時点ではterminal通知未取得でcheck2不合格。時刻自体と通知到着を分け、後で最大500msの到着待ちをtest-onlyで追加する。event timestampの32/64幅やStopEx100ms/PCM/clock閾値は変えず、旧3runを遡及合格にしない。先に同じ保存済み0808本体で再生なしのmetadata GUI/4tab/選択/保存通常終了を独立して進める。runtime/全7関連/newPCMはfalse。Timeline OS5/clock8.5674ms不変の再試行なし、全40全8/fullAcceptance=false。一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v3d-horizon-failed.json`。


## 2026-11-10 現行再開点（083819483 Boundary配置・通知到着待ちbuild中）

0808本体18484は下段3controlがtab bottom204を超える表示不合格。未編集/未保存/未再生・全4native完全保持・通常exit0をlayout-failure-v1へ保全。新候補は同じ389×212/英語8pointのまま下段を内側へ配置し、test-onlyでIsPlaying終了後のowned terminal通知到着を最大500ms収集する。通知stamped-timeの-32/+64幅、StopEx100ms、product scheduling/default/PCM/clock閾値は不変。三つの旧native失敗を遡及合格にしない。build57531完了後292saved-working/4outputs→未阻害6→新author-v2 PID/表示/4tab/metadata/履歴/保存通常終了→別newprefs復元。Timeline Range.sgp Windows5 after67とclock8.5674ms>5msは条件不変なので再試行/録音/二probeなし。全7関連/newPCM/全40全8/fullAcceptance=false、goal active。

一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v4a.json`。


## 2026-11-10 現行再開点（083819483 Boundary metadata保存復元後・native未達）

作者12356で4境界/Marker on-off/AfterPrepare-Quick/3bit最終値0x01001400/Tree二Undo二Redoを実操作し、18保存全byteを照合。Segment1はDWORD20だけ、二peerは全byte、ProjectはFILETIME以外を保持。4tab両巡回/disabled入力skip、InfoΩ/Codex/2026/CRLF/Version1.2.3.4/Start1920/Infinite3072..6912/Tempo120を保持。3回Properties再作成でGDI91→91、下段実矩形はtab204内。作者12356と別newprefs復元15280は両通常exit0、別復元で全設定再確認/再保存3SGP全byte・ProjectFILETIMEのみ。初期Tree白は別復元にも残り、全描画は未受入。

新nativeはImmediate/Grid/Beat合格、BarlineはownedSEGABORT4=4688とSEGEND1=4689の両通知を得たがunique終端時刻条件でcheck14失敗。全raw/旧失敗を保全し遡及合格なし。次候補で全terminal通知を各々valid/同じ-32,+64幅へ照合し、無効時刻も拒否するtest-only修正を行う。product schedulingは不変。GUI Play/新PCMは未実行、clock8.5674ms>5msとTimeline Range.sgp OS5の条件不変再試行なし。全7関連/newPCM/originaldynamic/全40全8/fullAcceptance=false、goal active。一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v4f-metadata.json`。


### 090907740 Boundary全terminal観測test-only build中

0838metadata18保存/4tab/GDI/二通常exit0・別復元を保全した後、全owned終端通知を非空/全valid/各々同じ-32,+64幅へ照合するtest-only候補090907740を保存build3068中。無効時刻の通知もcountし、良い通知だけを選ばない。旧四native失敗は遡及合格なし。product scheduling/default/arrival500ms/StopEx100msは不変。新native4/継承runtime/新本体routing/保存通常終了/別復元を未達から継続。Timeline OS5とclock凍結/全7関連false/newPCMfalse/全40全8/fullAcceptance=false、goal active。一次入口：`work/analysis/ui-segment-boundary/20261110T064400000Z/unit-record-v5a.json`。


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

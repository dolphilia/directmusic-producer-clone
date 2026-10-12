# ツールバーとSegmentとSequenceの優先実装計画

表示環境の現行条件：ユーザーが主ディスプレイと解像度を変更した。4K/200%は以前の条件で、変更後のOS倍率は未測定。入力前に返却windowと最新画像で座標を再取得し、厳密比較では双方のDPI/font/theme/client寸法を記録する。

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
- 現在の4K/200%を維持する。OS/DPI、フォント、テーマ、原版/製品のDPI認識、クライアント寸法、zoom/scroll、選択・カーソル状態を記録する。切り抜き画像の物理ピクセルを無条件に論理座標へ転用しない。画像にない全画面配置やhoverカーソルの形は追加観測で補う。
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

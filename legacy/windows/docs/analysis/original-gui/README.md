# DirectMusic Producer 原版 GUI 調査計画

開始日 2026-11-11。調査成果は、原版の画面と操作を後から再現し、実装と照合するための参照資料として使う。Segment エディターを最優先とし、アプリ全体の画面・操作も段階的に調べる。新製品への採用は、各挙動の根拠と製品側の採用判断に基づいて決める。

現在の再開点は **SEG-14 の Pattern と Style**。専用 Project2 に [Chordmap・Chord・Signpost 入り素材](../../../work/original-gui/SEG-14-harmony/20261111T044759384015Z/inputs/harmony-segment/manifest.json) を複製して起動した。2026-11-11 の再確認では Producer は **DmusGuiPatternStyle.exe** の1プロセス。Computer Use がこの実行ファイルへの接続を承認段階で拒否している。アクセスが通ったら実行パスとウィンドウを照合し、[Pattern と Style の手順](seg-14-pattern-style.md#compose-chords-の確認順序) の1から同じ作業コピーで再開する。 [実行パスとプロセス記録](../../../work/original-gui/UI13-14-transport-aux/20261111T083323468666Z/installed-and-processes.json)

- [UI01 View とツールバー表示](ui-01-view-toolbars.md) は表示前後、項目ごとの切替、再起動後の保持を掲載。
- [初回観測](pilot.md) は New Files の3状態と過去の Segment 画面を掲載。
- [SEG-01 作成とトラック追加](seg-01-creation.md) は Length の切替・確定・取消し、17種類のトラック一覧、範囲選択、追加結果を掲載。
- [SEG-02・03 表示範囲と文書切替](seg-02-03-viewport.md) は Sequence 展開、最大化、横ズーム、スクロール、2文書の切替を掲載。
- [SEG-02 子ウィンドウ操作](seg-02-mdi.md) は上下・左右・重ね整列、移動、サイズ変更、最小化・復元・最大化を掲載。
- [SEG-04 トラックヘッダー](seg-04-track-headers.md) は選択色、Sequence の折りたたみ・展開、境界と並べ替えの試行を掲載。
- [SEG-05 選択とノート入力](seg-05-selection.md) はルーラーと空セルの違い、時間範囲、矩形・単独・全選択、混在値の表示を掲載。
- [SEG-05 貼付けと履歴](seg-05-clipboard-history.md) は Merge、Cut、Overwrite、フォーカスと Undo/Redo、文書別の履歴と保存値を掲載。
- [SEG-06 Sequence Part と楽器選択](seg-06-sequence-parts.md) は鍵盤、縦ズーム、PChannel 2・10、打楽器名、Band0 と楽器メニューを掲載。
- [SEG-07 ノートのポインターと Snap](seg-07-note-gestures.md) は中央と端の形状、ドラッグの試行、Grid・None の挿入と保存値を掲載。
- [SEG-08 CC レーンと曲線](seg-08-cc-curves.md) は RPN の取消、PBend と Expression の描画・開閉・選択、伸縮、再起動時の復元を掲載。
- [SEG-09 Tempo と Time Sig](seg-09-tempo-timesig.md) は同一拍の異なる Tick、Cycle Tempos、6/8 とグリッド、表示範囲外のラベルと保存値を掲載。
- [SEG-09 Bands と Band 子エディター](seg-09-band-editor.md) は時刻と楽器欄、子ウィンドウ、パン・音量、Organ 1・Room、DLS の取消、再読込を掲載。
- [SEG-10 Segment Properties](seg-10-properties.md) は4タブ、入力欄の有効条件、Length と GUID の取消し、選択に応じた対象切替を掲載。
- [SEG-11 再生・停止・位置](seg-11-transport-position.md) は2つの再生入口、現在テンポ、横追従、停止・再開、Play Cursor を掲載。
- [SEG-12 ループ・端点・境界停止](seg-12-loop-boundary.md) は有限・Infinite・Clock Time の折り返し、端点の入力と保存、Barline 停止、別起動後の復元を掲載。
- [SEG-13 保存と別プロセスでの再読込](seg-13-reopen.md) は Project の自動再オープン、Segment の表示と値の復元、再保存の照合を掲載。
- [SEG-14 Groove と Lyric](seg-14-groove-lyric.md) は4トラックの追加、Groove Level と繰り返し、歌詞の時刻と同じ拍の切り替え、日本語表示と保存値を掲載。
- [SEG-14 Marker と Mute](seg-14-marker-mute.md) は Marker と Enter Switch Point の上下段、Mark Range メニュー、PChannel の拍・行操作、再割り当て、折りたたみと再読込を掲載。
- [SEG-14 Chord の拍セルと鍵盤](seg-14-chord.md) は拍単位の挿入、Chord/Scale・Inversion、Hide と Undo、トラックの Key と再読込を掲載。
- [SEG-14 Chordmap 参照と Signpost](seg-14-chordmap-signpost.md) は参照が空の状態、候補の選択と履歴、ChordMap Designer の入口、Cadence、Activity Level、Compose Chords の依存不足を掲載。
- [SEG-14 Pattern と Style の素材と入口](seg-14-pattern-style.md) は原版サンプルの保存値、画面の入口、Compose Chords の依存条件と再開手順を掲載。
- [SEG-14 Parameter Control、Segment Trigger、Script の入口と試験素材](seg-14-control-triggers-script.md) は原版ヘルプの専用画面、参照候補を用意するための保存値、実機で確認する状態を掲載。
- [SEG-14 Wave Track と Wave Properties](seg-14-wave-track.md) は原版ヘルプの Part・Layer、Wave 項目と文書の設定、FarmGame の6組の参照・PCM・時刻を掲載。
- [UI04 Style Designer と Motif](ui-04-style-motif.md) は過去の原版撮影から Style の作成・読込、Pattern と Motif の一覧、accordion の右クリックと3つのプロパティタブを掲載。
- [UI07 DLS Collection、Instrument、Wave エディター](ui-07-dls-wave-instrument.md) は空の Collection、Wave 挿入、Instrument 作成の依存条件、波形の初期表示、保存とエラーを掲載。
- [UI08 AudioPath Designer](ui-08-audiopath-designer.md) は原版ヘルプのペイン、バスとバッファ、エフェクトの追加と順序、Send の宛先を掲載。
- [UI10 Script Designer・Message Window](ui-10-script-designer.md) は原版ヘルプの3ペイン、Properties、エラー表示と、旧セッションの到達条件を掲載。
- [UI11 Container](ui-11-container.md) は原版ヘルプの参照と埋め込み、Add/Remove Files、FarmGame の設計用・実行用データを掲載。
- [UI12 ToolGraph Designer](ui-12-toolgraph-designer.md) は原版ヘルプの3ペイン、Tool 割り当て、現行ホストの登録と未登録クラスの素材を掲載。
- [UI13 Transport と Wave・MIDI 書き出し](ui-13-transport-export.md) は原版で撮影した空の Secondary と出力名、ヘルプのメーター、設定、書き出し条件を掲載。
- [UI14 Help メニューと HTML Help](ui-14-help.md) はインストール版の Contents、独立ウィンドウ、Help を閉じた後の本体とヘルプ索引を掲載。
- [UI14・ROLE01 補助アプリと導入の入口](ui-14-companion-apps.md) は Farm Demo と StylePlayer の配置、試聴手順、インストール済み版の登録を掲載。

## 成果物と保存場所

| 成果物 | 保存場所と使い方 |
| --- | --- |
| 調査計画と進捗 | このページ。各回で観測済みのケース、保留理由、次の操作を更新する |
| 画面別・操作別の Markdown | このディレクトリにケース ID ごとに追加する。画像を本文に埋め込み、入口、操作、状態の変化、終了時の状態を説明する |
| Markdown 掲載画像 | [figures/](figures/) に保持する。画像は出典・撮影条件を記録し、相対リンクで埋め込む |
| 撮影原本・操作記録・試験入出力 | `legacy/windows/work/original-gui/<ケースID>/<実施日時>/` に保存する。撮影画像、観測ログ、入力・出力ファイル、ハッシュをひも付ける |
| 既存の互換性台帳 | [UI 互換性台帳](../ui-compatibility.json) の該当ケースに資料を結ぶ。原版の観測結果と候補側の比較結果をそれぞれの証拠で更新する |

既存の [外観と操作互換の契約](../ui-compatibility.md)、[画像1〜47の要件索引](../priority-segment-sequence.json)、[優先実装計画](../priority-segment-sequence-2026-10-10.md) を画面一覧の出発点にする。各画面の入口は原版のメニュー、ツリー、右クリック、ヘルプで洗い出し、資料が見つかったケースはその取得条件を確認する。既存画像のうち今回の資料に必要なものは、元のパス・観測日・バイナリと画像のハッシュを記録して引用する。

## 実施順序

| 段階 | 対象 | 次へ進む条件 | 現在 |
| --- | --- | --- | --- |
| 0 | 起動環境、証拠の保存方法、New Files の予備観測 | 参照バイナリを特定し、操作前後の画像を Markdown から参照できる | 調査用コピーと専用 Project2 で確認済み。Windows 11、1280×800、96 DPI |
| 1 | Segment の作成、開閉、プロパティ、Timeline 共通操作 | SEG-01〜05、SEG-10、SEG-13 の基本ケースを同一入力から再実行できる | 作成、横ズーム、子ウィンドウ操作、Properties、別起動、選択、貼付け、Undo/Redo を観測。ヘッダーの境界操作、長さやサイズの上下限、Save As と閉じる分岐を継続 |
| 2 | Sequence・Part・CC、Band、再生中の Timeline | SEG-06〜09 と SEG-11・12 の基本ケースと主要な境界を撮影する | 複数 Part、Snap、CC、Tempo・Time Sig、埋め込み Band、再生の追従、有限・Infinite・Clock Time ループ、Barline 停止を観測。個別の追試条件は各資料に記録 |
| 3 | その他の Segment トラック、参照文書と複合操作 | SEG-14・15 の全入口を確認し、個別動作と文書間の状態を記録する | Groove・Lyric・Marker・Mute・Chord・Chordmap・Signpost は追加から別起動まで観測。Pattern・Style・Parameter Control・Segment Trigger・Script・Wave は資料と素材を用意し、実機操作を待つ |
| 4 | 本体共通、各専用エディター、Transport と補助画面 | アプリ全体の調査表に観測・保留の根拠と次の手順がそろう | 新規作成、プロジェクトツリー、Window、選択対象と Transport の連動を一部観測。Style・Motif、DLS・Wave、Help の過去の原版撮影を照合。AudioPath・Script・Container・ToolGraph・Transport 補助機能はヘルプ、Farm Demo と StylePlayer はリソースを採取。各専用画面の操作は追試待ち |
| 5 | 主要ケースの撮り直しと相互照合 | 別の作業コピーで追試し、状態・操作・画像の記述とリンクを確認する | 未着手 |

1回の作業は同じ入力を使う1〜3ケース程度を目安とする。まず通常経路を一通り確定し、同じ画面の境界操作を続けて調べる。Segment の観測を優先し、調査中に通過する本体共通画面も同時に記録する。撮影枚数は、Segment でおおむね150〜250枚、全体では数百枚を想定する。遷移の意味が異なる状態を揃えることを優先し、枚数は観測した状態数に応じて見直す。

## 実行対象と比較条件

- 原版 5.3.0.900 のインストール済み版、起動修正済みインストール版、作業用コピーは、実行パスと SHA-256 で特定する。[原版の履歴](../original-reference-2026-10-10.md) と [起動修正の記録](../../../README.md#この環境のインストール済み版) を参照する。原版の機能を変える差分がある場合は、影響する画面・操作をケースに記録する。起動時の登録に関係する操作には未改造時の証拠も確認する。
- このローカルマシンで本計画を実施する間の Computer Use は、2026-11-11 にユーザーから継続承認を受けている。計画内の画面操作・撮影・調査用ファイルの編集や通常終了は、その都度ユーザーに確認せず進める。
- 原版または作業用コピーを起動する前に、開いている Producer のウィンドウとプロセスを実行パスで照合する。使う実行ファイルと専用 Project がすでに開いていれば、そのウィンドウを再利用する。不要な Producer は通常終了し、未保存の変更があれば保存先と内容を確認してから閉じる。
- 別の実行ファイルまたは別プロセスでの検証が必要なときは、既存の Producer の終了を確認してから、対象の実行ファイルと作業ディレクトリを指定して起動する。起動直後に PID・実行パス・ウィンドウを再照合し、目的の1プロセスで操作する。
- 新規作成・編集・再読込は、調査用の INI と作業用プロジェクトだけが開く状態で実行する。コピー元の入力を保全し、試験ごとに `inputs/` から `session/` へ複製する。保存・再保存先と元データへの変更有無をハッシュで確認する。既存プロジェクトが開いている場合は、作成や保存を始める前に専用セッションを確保する。
- 基準環境は最初のセッションで OS、表示言語、テーマ、フォント、ディスプレイ解像度、DPI、作業領域、ウィンドウ全体とクライアント寸法を採取して固定する。旧資料の 1280×800・96 DPI や 4K・200% は別条件として記録する。標準サイズに加え、狭いウィンドウ、最大化、高 DPI の代表例を調べる。
- 通常の Segment 検証では、調査用コピーの `View` で `Project Tree`、`Standard Toolbar`、`Transport Controls`、`Status Bar` を表示する。観測に必要なツールバーをその都度表示し、撮影時の表示項目を記録する。切替と復元は [UI01 の手順](ui-01-view-toolbars.md) を参照する。
- 撮影前に表示中の文書、フォーカス、ツリー展開、ウィンドウの配置、トラック表示順、ズーム、横・縦スクロール、再生位置と入力デバイスの条件を記録する。音源、AudioPath、MIDI 入力などが操作可能状態に関係するケースでは接続条件も記録する。

## Segment エディターの調査表

| ケース | 画面と入口 | 確認する操作・境界 | 主な撮影点 |
| --- | --- | --- | --- |
| SEG-01 作成・追加 | File New、Standard New、Ctrl+N、Project ツリー、New Files、Length、Add New Tracks | 既定/指定名、単独/複数選択、選択解除、無選択、OK/Cancel/Esc、長さの単位と最小/最大、初期トラック、空選択での確定可否 | 初期、Segment 選択、Length の各単位、Add New Tracks 初期/選択中/無選択、確定後、取消後 |
| SEG-02 ウィンドウと画面構成 | ツリーから開く、MDI 子、Window メニュー、Properties | 最大化/復元/最小化、移動、四辺と四隅のリサイズ、最小寸法、重なり、Tile/Cascade、複数文書切替、子ウィンドウを閉じた後のフォーカス | 初期/狭幅/最大化、ドラッグ前後、フォーカス有無、切替前後、閉鎖後 |
| SEG-03 時間軸と表示範囲 | 小節・拍・グリッド・時刻ルーラー、横縦スクロールバー、Zoom | ホイール・矢印・ページ送り・つまみ、ズーム中心、最小/最大、全体表示、端でのクランプ、長い Segment、拍子変更、スクロール位置の保持 | 通常/拡大/縮小、開始端/中間/終端、狭幅、目盛り切替、グリッドの変化 |
| SEG-04 ヘッダーとトラック配置 | トラック名、グループ番号、上下境界、表示メニュー | 選択/非選択、グループ表記と順序、表示名省略、開閉、個別高さと境界リサイズ、ヘッダー幅、並べ替え可否、アクティブ行、スクロール中の固定部 | 折りたたみ/展開、短/長ラベル、複数グループ、境界ホバー、ドラッグ前/中/後 |
| SEG-05 選択と共通編集 | ルーラー、トラック名、空セル、イベント、右クリック、Edit | 単一/複数/範囲、Shift/Ctrl/キーボード、解除、Select All、メニューの有効状態、挿入/削除、コピー/切り取り/貼付け、取り消しとやり直し、別トラック・別文書 | 選択の段階、右クリック位置ごとのメニュー、空/選択あり、貼付け前後、Undo/Redo 後 |
| SEG-06 Sequence・Part | Sequence ヘッダー、Part、ピアノロール、鍵盤、楽器選択 | 開閉、パート追加・切替、グループと PChannel、縦ズーム、半音単位の移動、鍵盤試聴、複数パート、ドラム表示、Instrument メニューと Cancel | 折りたたみ/展開、通常/狭幅、縦ズーム前後、鍵盤移動、Part 切替、楽器階層 |
| SEG-07 ノートとプロパティ | ノート中央/左右端、空グリッド、Note Properties | 挿入、選択、矩形、移動・伸縮、ピッチ変更、Snap、複製、無効範囲、オートスクロール、Velocity、Start/End/Duration、Belongs to Beat、複数選択時の混在値 | ポインター形状、ドラッグ前/中/後、Snap の結果、単一/複数のプロパティ、取消/Undo |
| SEG-08 CC と曲線 | CC レーン、CC の追加/削除、Curve Properties | レーン開閉・高さ、CC 種別、点/線選択、値と時刻の移動、端点と曲線形状、複数選択、ノート選択との連動、再読込 | レーンなし/あり、点選択、ハンドルホバー/ドラッグ、表示切替、プロパティ |
| SEG-09 Tempo・Time Sig・Bands | トラック名、空セル、イベント、Band 子エディター | テンポ変更、拍子とグリッド、同じ拍の複数イベント、先頭/直前値、Bands の参照/所有・ダブルクリック、音色/PChannel・Pan・Volume、閉鎖後の選択 | 基本表示、スクロール境界、グループ差、選択/入力/エラー、Band 子の開閉と親の変化 |
| SEG-10 Segment Properties | ツリー/エディターの Properties、F11、各ページ | Info、長さ、Start Time、Loop、Boundary の表示と既定値、スピン方向、Tab 順、相互依存・無効化、適用/取消、範囲外、再表示/保存復元 | 全ページ、コントロール別のフォーカス/無効/入力値、ページ切替、適用直後、再表示 |
| SEG-11 再生と位置 | Transport、再生位置、現在テンポ、ステータスバー | 停止中の位置変更、再生・停止・再開、Play Cursor の追従有無、横端での追従、表示位置と再生位置の分離、終了時の位置 | 停止中、開始直後、端の手前/後、停止直後、自然終了、再開直後 |
| SEG-12 ループ・境界 | Start/Loop、境界設定、ループマーカー、Transport | 開始位置とループ区間、反復、端点の移動、music/reference time、拍子・テンポ切替、ループ折返し、境界停止/遷移、無効な範囲 | 端点ホバー/移動、開始前後、折返し前後、停止要求時/確定時、エラー |
| SEG-13 保存と閉鎖 | File Save/Save As、閉じる、文書切替、別起動 | 編集済み表示、参照の差し替え、保存取消し、破棄/継続、元入力の保持、Undo 履歴と選択・ズーム・スクロールの再読込 | 編集前/後、確認の各分岐、保存後、閉鎖後、別起動で復元した直後 |
| SEG-14 その他のトラック | Add New Tracks、各ストリップと Properties | Chord、ChordMap Reference、SignPost、Command、Lyric、Marker、Mute、Parameter Control、Wave、Segment Trigger、Script、Style/Pattern など、一覧で確認できた種類を個別に調査する | 空行、追加後、選択、専用メニュー/ページ、移動/取消/境界、依存不足 |
| SEG-15 複合・文書間 | 複数トラック、複数 Segment、参照先のエディター | 同じ小節での種類別動作、グループの違い、複数文書の Undo・Clipboard・再生の対象、参照更新と独立性 | 操作対象と非対象、フォーカス切替、前後、別起動 |

SEG-01〜05 を先に追試し、同じ Segment の状態を SEG-06〜12 へ引き継ぐ。追加したトラックは、共通操作が同じかを一つずつ照合する。グループや音源に依存する差は操作名と発生条件を記録する。

## アプリ全体の調査表

| 既存台帳 | 画面・機能 | 収集する主な状態 |
| --- | --- | --- |
| UI01 | 起動、メインフレーム、Project ツリー、File/Edit/View/Add-Ins/Window/Help、Standard・Bookmarks、共通ダイアログ | 何も開いていない状態、作成/読込後、ツリーの展開・折りたたみ、名前変更、分割幅、表示/非表示、ツールバー移動・ドッキング、メニュー項目の無効/有効、キーボード移動 |
| UI02・03・09 | Timeline、Segment と時刻付きトラック | 上の Segment 調査表を使用する |
| UI04 | Style、Pattern、Motif | パターン一覧、Part・Variation、試聴、共有/所有表示、子画面、選択・移動・複製 |
| UI05・06 | Band、Chord、ChordMap、SignPost | 鍵盤・音色、グラフ・節点、リンクの作成/解除、複数選択、プロパティ・移動、Undo/Redo |
| UI07 | DLS Collection、Instrument、Wave、Region、Envelope、PanVol | ツリー/波形/鍵盤の連動、分割位置、鍵域・速度域・ループ端のハンドル、重なり、エラー、試聴 |
| UI08 | AudioPath、バス、送信、効果 | 経路と順序、値の直接入力とスピン、挿入・並べ替え、宛先切替、プロパティ・取消 |
| UI10〜12 | Script、Container、ToolGraph | 参照/埋込/変数、追加と切替、選択/移動、グラフ接続、読込不足と実行エラー |
| UI13 | Transport、Synth Status、録音/書き出し | 停止/再生/録音、Primary/Secondary、速度・メトロノーム・MIDI、音源切替、出力前後とエラー |
| UI14・ROLE01 | 補助アプリ、Help、導入・解除関連 | 実用上の画面と入口、操作可能状態、参照専用の画面。機能の入口を確認できない場合は理由を台帳に記録する |

## 撮影と操作の記録

1. **操作前を固定する。** 原版の実行パスとハッシュ、ケース ID、入力ハッシュ、開始状態、対象ウィンドウと表示条件を記録する。メニューや独立したプロパティ・ダイアログは本体画面と対象だけの画像を取得する。
2. **実際の入力と結果を結ぶ。** 操作対象を名前・階層・位置で特定し、クリック回数・ボタン、押したキーと修飾キー、ドラッグの始点/終点、ホイールの位置と方向、操作順を記録する。入力の直後に画像と必要な UIA を採り、選択、フォーカス、スクロール、値、利用可能な操作を記述する。UIA と画像が食い違う場合は採取順と再観測結果を残す。
3. **形状と時間を観測する。** ポインターは平常時、ヒット領域へ入った直後、押下中、移動中、ドロップ可能/不可、解放後を区別する。対象はヘッダーの境界、スプリッター、スクロールバー、ノート端、ルーラー、ループマーカー、ウィンドウ枠。マウスポインターと再生カーソルを個別に記録する。ホバーや押下中を取得できない場合は観測待ちにし、手動の保持・撮影など再現できる方法を試す。自動操作ツールの青い強調表示はツールの表示として注記する。再生中は開始時点からの経過時刻と音楽時刻を併記し、連続フレームで折返し・追従・一時表示を記録する。
4. **外観を記述する。** 画面全体、対象の拡大、前後比較を使う。部品名、整列、寸法、余白、字体・サイズ・色、アイコン、切り詰め、選択・非アクティブ・無効状態を記録する。寸法はウィンドウ相対座標と物理ピクセル、DPI を明記する。OS の透かし、描画遅延、ツールの強調表示があれば撮影条件に記す。
5. **終了状態を確かめる。** Cancel/Esc、範囲外、エラー、Undo/Redo、フォーカス移動、閉じて再表示した状態も必要に応じて撮る。文書の保存や復元が関係する場合は試験コピーを別プロセスで開き、値と表示状態を読み戻す。再生表示と音声の実出力はそれぞれ確認内容を記録する。
6. **画像を照合する。** 原本は撮影時の形式で保存し、ファイル名・寸法・SHA-256 を記録する。切り抜きや注釈を使う場合は元画像と範囲を示す。Markdown の各画像に代替テキストと「何を操作して何が変わったか」の短い説明を添える。既存プロジェクト名、個人パス、他アプリを含むフレームは掲載前に見直す。

### 1ケースの Markdown 構成

| 欄 | 内容 |
| --- | --- |
| 対象と入口 | ケース ID、台帳 ID、画面名、メニュー/キー/クリックからの手順 |
| 条件 | 原版の版・SHA-256、実行パス、OS/DPI/寸法、入力の来歴とハッシュ、選択・再生状態 |
| 操作列と画像 | 番号順に前状態、入力、観測結果、画像 ID、カーソル/フォーカス、確認できた終了状態 |
| 例外と範囲 | 無効/取消/範囲外、時間依存、既知の再現条件、未観測項目、元資料へのリンク |
| 実装時の照合 | 操作と見た目をどう比較できるか、対応する保存値やログ、候補側の比較への入口 |

各画像は、例として `seg-07-03-note-left-edge-hover.png` のようにケース、順番、状態が分かる名前にする。同名の取り直しは新しい実施記録に保存し、根拠を更新した日付を記す。

## 試験素材の準備

| 素材 | 用途 |
| --- | --- |
| 空の作業用 Project と新規 Segment | 作成、既定値、取消、追加可能なトラック一覧、初期の選択/Transport |
| 20小節程度の複数トラック Segment | 一覧と順序、グループ、プロパティ、編集、Undo、保存復元。Sequence の短い音と長い音、CC、Tempo/Time Sig/Band を入れる |
| 長尺・複数拍子の Segment | 目盛り、省略、横縦スクロール、端のズーム、追従、ループ折返し。開始・中間・終端を同一入力で再現する |
| 複数 Part、複数グループ、ドラムを含む Segment | 鍵盤・パート・グループ間の差、PChannel、CC 種別、選択と複製 |
| 参照先を含む Project | Style、Band、DLS、Wave、AudioPath、Script の接続と独立した編集画面。既存 [Q1DlsOracle](../../../Q1DlsOracle/) は来歴を調べて作業コピーに利用する |
| 原版サンプルの Project | [QuickStart](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/inputs/quickstart/manifest.json) の既存 Style 参照、[FarmGame](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/inputs/farmgame/manifest.json) の内蔵 Pattern と Wave 参照。保存値を照合済みの素材から作業コピーを作る |
| Wave の参照・取込素材 | [6組の照合記録](../../../work/original-gui/SEG-14-wave-audiopath/20261111T081723992826Z/farmgame-wave-audit.json) と [元 WAV](../../../work/original-gui/SEG-14-wave-audiopath/20261111T081723992826Z/inputs/wave-import/manifest.json)。既存項目から Wave 文書へ移る操作と、別 Project への取込・追加を確認する |
| Script と AudioPath の参照素材 | [FarmGame の保存値](../../../work/original-gui/SEG-14-control-triggers-script/20261111T081051527595Z/farmgame-script-and-audiopath.json) に Script の12ルーチンと10件の Segment 参照、AudioPath の Echo を記録。Script Track、Script Designer、Parameter Control の候補を出すために使う |
| Container と ToolGraph の依存素材 | [FarmGame 実行用ファイル](../../../work/original-gui/UI11-12-container-toolgraph/20261111T082559380067Z/inputs/farmgame-runtime/manifest.json) は外部参照と GUID の差を保持。[未登録の Tool クラスを含む Graph](../../../work/original-gui/UI11-12-container-toolgraph/20261111T082559380067Z/inputs/missing-tool/manifest.json) は読込時の表示とエラー確認に使う |
| DLS の PCM 素材 | [Wave エディターの Project](../../../work/original-gui/UI07-14-archive/20261111T080241498238Z/inputs/wave-editor/manifest.json) は Wave を開く操作、[Instrument 入り Project](../../../work/original-gui/UI07-14-archive/20261111T080241498238Z/inputs/instrument-saved/manifest.json) は Region と楽器の読込、[Tone.wav](../../../work/original-gui/UI07-14-archive/20261111T080241498238Z/inputs/pcm-import/manifest.json) は空の Collection への挿入に使う |
| 2文書・無効値・欠損参照の専用コピー | 対象文書の切替、Undo の対象、取消やエラー時の回復、確認ダイアログ |

## 完了条件と継続手順

- ケースは「未観測」「観測中」「原版確認済み」「追試待ち」「障害あり」「非該当」で記録する。原版確認済みには、入力と環境、画像付きの手順、前後状態、該当する取消し・境界の結果が必要。画面の見た目と操作の挙動をそれぞれ再現できる記述にする。
- 段階1〜3の完了時に、Segment のトラック一覧と Properties の全ページを照合し、未確認の入口、描画、カーソル、ドロップ、境界、文書間差を明記する。段階4では各 UI 群で到達可能な画面を照合し、見つからない入口は確認した操作と依存条件を記録する。
- 1回の作業を終えるたび、画像のリンク・表示、サイズ、撮影時の状態とキャプションの一致、同じ入力からの再現、試験コピーへの保存先を確認する。次回の入口、入力、狙う状態をこのページに追記する。ツールや素材に障害が出た場合は、起きた箇所と影響範囲を記録し、独立して進められるケースを続ける。

### 原版での再調査が残る項目

[Add New Tracks の17種類](seg-01-creation.md#3-add-new-tracks-の選択と取り消し) のうち、11種類は追加または編集した画面がある。Band、Chord、Chordmap、Groove、Lyric、Marker、Mute、Sequence、Signpost、Tempo、TimeSig。残る6種類は一覧での存在確認とヘルプ・素材の準備まで。[Segment Properties](seg-10-properties.md) の4ページは撮影済みで、値の上下限と操作順の追試が残る。

| 順 | 原版で確認すること | 再開に使う資料 |
| --- | --- | --- |
| 1 | Pattern・Style の空状態、参照設定、Compose Chords と Undo。続いて Parameter Control、Script、Segment Trigger、Wave の追加・編集・保存 | [Pattern と Style](seg-14-pattern-style.md) · [Control・Trigger・Script](seg-14-control-triggers-script.md) · [Wave](seg-14-wave-track.md) |
| 2 | ノート・CC の押下中と移動、ヘッダー境界と複数グループ、ループ端点、画面幅・DPI の差、Save As と閉じる分岐、参照変更と複数文書の再生・履歴・音声 | [ノート](seg-07-note-gestures.md) · [CC](seg-08-cc-curves.md) · [ヘッダー](seg-04-track-headers.md) · [ループ](seg-12-loop-boundary.md) · [再読込](seg-13-reopen.md) |
| 3 | 本体のメニューとドッキング、専用エディターの選択・移動・取消、Transport 補助画面と出力・実音声、補助アプリ、導入・解除画面の入口 | [アプリ全体の調査表](#アプリ全体の調査表) · [Transport](ui-13-transport-export.md) · [補助アプリ](ui-14-companion-apps.md) |
| 4 | 別の作業コピーで主要ケースの画面、操作、保存・音声を照合して、各ケースの状態を更新 | [実施順序](#実施順序) の段階5 |

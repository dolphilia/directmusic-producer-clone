# Band PChannelミキサー（画像25〜28）

2026-10-11（日本時間）。SSP05は作業中。190330限定成立：268保存/作業ソース・4出力一致、関連13モード873検査合格。先頭0 clocksからの191秒ノートで、音量低下/回復・左右パン・発音中Stop・停止後無音・再開・音階が固定基準を満たした。作者9256/別復元19596の保存・二通常exit0と、1 binsのみ変更/他30 leaf保持を確認。初回発音1.125秒は1秒基準に不合格。連続ドラッグ/取消し/UndoRedoライブ反映、原版/厳密外観/全操作は未達。fullAcceptance=false。

## 実装と契約

画像25〜28と正式原版CHMの Band Editor / Volume and Pan / To change the pan and volume を根拠に、楽器割当の有効状態、ミキサー設定の有効状態、選択状態を分けた。選択は赤、未有効番号は灰色、編集後の非選択番号は濃く表示する。上下で音量0〜127、左右でパン0〜127を変更し、L63〜R64を表示する。画像27のL2/102は保存値61/102として確認した。初期未有効時の数値や中心座標の完全一致は原版比較未確認。

複数選択をまとめて動かし、対象チャンネルだけのflags・pan・volumeを更新する。16を超えるDWORD PChannelを許可し、4予約値を拒否する既存契約を保護した。埋込Bandの変更は所有Segmentの一つのUndoとして保存し、Sequence・別Band・patch/bank/DLS参照・未知chunk/paddingを保持する。

## これまでの候補と実行結果

- 175912（20261010T175912382Z）：268保存ソース/4出力一致。実本体で選択、単独ドラッグ、Undo/Redo、L2/102、選択解除、2チャンネル矩形選択・同時移動・一回Undo、保存、通常終了、別process復元、通常終了を実行。作者22876/復元4380ともexit0。独立RIFF解析では最終状態はPCh1の対象データだけ、途中の複数編集はPCh1〜3だけが変化し、Sequenceと別Bandが保持された。
- 同175912：左リストの楽器欄・番号欄をクリックすると選択が消える不具合を保存。関連13モードのうちsegment-creationが29番の検査で失敗。失敗の原因は、新Projectへの所属追加でProjectがdirtyになる既存仕様を、Segment自身のdirtyと混同した試験期待値だった。Segmentのclean確認とProject所属dirty確認を別々に修正した。
- 181030（20261010T181030821Z）：268保存ソース/作業ソース/4出力一致。関連13モード855検査が合格し、segment-creationは190検査。DLS descriptorの未知データを含む保持も追加確認。この候補で新GUI・音声は未実行。
- 182114（20261010T182114440Z）：選択通知時に、内容が同じBandのListViewを再作成しない修正を保存。コードレビューでドラッグ取り消し後の表示位置の復元も必要と判断したため、後続ソースで補正。この候補を最終確認済みとは扱わない。

- 182751（20261010T182751249Z）：選択保持と取消し表示位置を修正した268ソース/4出力一致。関連13/855合格（segment-creation190）。実本体で楽器欄・番号欄・右ボタンの同期、Ctrl複数選択、L2/102、Undo/Redo、選択解除、矩形複数選択/移動/一回Undo、保存復元を改めて実行。作者4304/復元15876はともに通常exit0。独立監査では最終1 bins、途中3 binsだけ変化し、Sequence・別Band・他15チャンネルを保持した。

## 証拠の入口

開始記録と版付きソース・候補同一性・関連結果は work/analysis/ui-band-mixer/20261010T175036931Z/ に保存。first-candidate-gui-v1.json、gui-native-audit-v1.json、intermediate-regression-v2.json を区別する。screens/15-list-selection-loss と17-number-column-selection-lossは不具合の証拠、10-project-first-save-errorはSegmentより先にProjectを保存した手順上のエラーとして保持する。後続ではSave File→Save Projectの順で成功した。原本screenshotは変更していない。

## 182751時点の次の確認と残件（履歴）

過去候補182751の入口は unit-record-v2.json、source-changes-v5.json、candidate-identity-v4.json、working-state-v1.json、gui-save-reload-proof-v2.json、selection-fix/gui-native-audit-v2.json。関連一次runは work/acceptance/regression/20261010T183430244Z/run.json。選択保持の修正は新候補で再確認済み。ドラッグ中Escape/capture取消しと全境界は未観測であり、ソース補正だけで操作合格にはしない。最初の復元撮影は前面の原版に遮られ、クローンだけを前面化して撮り直した。原版への入力は行っていない。

ドラッグ確定時のライブ反映を190330で接続した。連続ドラッグ中の反映、取消し時の再生値復元、Undo/Redoの再生値反映は未実装。複数Band trackやloop中の対象判定も未確認。PChannel Properties/F11、重なった番号ボタンの操作性、各境界・取消し・厳密配置/字体/色/アイコンの原版比較も残る。

限定採用0735は変更しない。162022の全98native/170driver一巡、46native/28driver OS凍結、既知匿名Band失敗保留、Style015244、SSP01〜04の発音/比較/全操作残件、Q2、全40/全8を保持する。全体受入は6作業中/2障害/0合格。

## 190330のライブ反映と音声失敗

Win32 DMUS_MIDI_PMSGのABIを固定し、所有する再生セッションのAudioPathでPChannelを変換してCC7/10を送る。別の同じローカルPChannelへの漏れ、無効batch、停止後の送信を専用実行で確認した。複数メッセージ送信の途中失敗は原子的に戻せず、文書の編集値を保持して次のPlayへ反映する。文書や選択の変更だけで別セッションを操作しない。

185456の専用実行は、主Segment停止後のsecondaryにTempoがない状態で失敗した。この失敗を残し、secondaryだけはtempoAvailable=falseを返して再生状態を保持する修正後、190330の関連13/873が合格した。segment-creation195、sequence-audition-runtime29。全98native/170driver一巡ではない。

47秒入力では作者13688/復元25344が通常exit0。最終R64/99を復元し、nativeは1 binsだけ変更、他30 leaf不変。180秒PCMで音量低下比0.1034、左右優勢約18倍、発音中Stop/無音/再開/音階を限定確認した。しかし初回発音1.100秒は1秒基準を超え、回復時は既にノートが終了し、追加Playも必要だった。固定手順全体は失敗。

検証用コピーだけのSegment/ノート長を192/191秒へ延長した。最初の300秒回は回復前にノートが終了したため手順中断として保存。次の300秒回ではPlay→音量40→100→左→右→Stop→再開→Stopの連続操作と固定窓を満たしたが、PCMは全窓で無音相当（RMS約0.000055、発音しきい値0.001未満）だった。初回/再開とも発音を検出できず、当時は原因未確定だった。後続の同一入力・先頭再生との比較を下記に記録する。

同じ作者19044から保存/通常exit0、別復元9892でR64/100をcleanのまま表示し通常exit0。nativeは対象1 binsだけ変化し、他30 leaf不変。原版への入力は行っていない。入口はgui-save-reload-proof-v3.json、source-changes-v8.json、candidate-identity-v6.json、working-state-v2.json、live-commit/audio-proof-v1.json、live-commit-extended-repeat/audio-proof-v1.jsonとnative-proof-v1.json。失敗を上書きせず、必要音声はfalseを維持する。

## 同一191秒入力の先頭再生による切り分け

現在位置からのPlayは1536 clocksから開始し、入力の唯一のNoteOn0を通過していた。同じ初期sgp（SHA256 a825753800d10500f45b6d35bf6fd0b4e861d3c93cec4cf1333a0f73459e0d19）でPlay From Startを使い、状態表示が0 clocksとなった回では発音を確認した。長いノート自体が無音の原因ではない。最初の先頭再生回は操作間隔が185秒空いて回復前にノートが終了したため、手順失敗として保存した。

再実行は作者9256、別復元19596。300秒固定PCM・較正時計・同じ数値基準で、音量低下比0.15890、回復比6.295、左右優勢約17.9倍、発音中Stop、停止後無音、再開0.440秒、C4音階、連続手順が成立した。15判定のうち初回発音だけが1.125秒で1秒基準を超え、音声総合はfalseを維持する。しきい値を変更していない。

Save File→Save Project→通常exit0→別process復元→通常exit0を実行。R64/100をcleanで復元し、対象1 binsだけ変更、他30 leaf不変。原版への入力は行っていない。72モジュールの観測は点観測であり、原版40hash監査や全機能依存解消を意味しない。

現行入口はunit-record-v4.jsonとgui-save-reload-proof-v4.json。live-commit-from-start-repeatのaudio-proof-v1.json（総合false）/native-proof-v1.json（限定true）/actions.jsonを使用する。旧短音声、現在位置からの無音回、先頭再生の手順中断を上書きせず保持する。ソースと生成物は190330のままであり、関連13/873の再実行はしていない。


## 201607の連続preview接続とUndo/Redo実行

201607限定成立：268保存/作業ソース・4出力一致、関連13モード878検査合格。再生中の音量・パンのUndo/Redoを含む固定PCM18/19項目が成立し、初回発音1.15秒のみ1秒基準に不合格。作者3356/別復元21780の保存・二通常exit0、1 binsのみ変更/他30 leaf保持を確認。明示PAN/VOLUMEの連続previewと取消し復元はソース接続済みだが、ドラッグ途中/取消しの本体・PCM確認、継承値、原版/厳密外観/全操作は未達。

明示PAN/VOLUMEを持つ選択PChannelに対し、開始時のPlaybackId・所有Segment・再生スナップショット・active Bandを固定するpreview経路を接続した。previewは文書/historyを変更せず、取消し・capture喪失・終了では開始mixに戻す。確定は所有文書の一回Undoを維持する。BandコンテキストのUndo/Redoは、同じ楽器/参照の明示mix差分だけを同じ再生経路へ送る。継承値・無効化へのUndo・patch/DLS変更は現在の限定経路の対象外で、未達として保持する。

保存ソース268件と生成物4件が作業木と一致。関連13モード878検査（segment-creation200、audition29を含む）が合格した。最初の20:24:32Z収録はpan-redo後の処理中断で未完となり、入力・録音・操作時刻・中断記録を保持した。再試験は同じ191秒C4 Organ1入力・固定300秒・同じ数値基準で先頭0 clocksから実行した。

| 検証 | 201607の結果と限界 |
| --- | --- |
| Undo/Redo音声 | 音量比0.1590、Undo回復比6.2948、Redo低下、左/中央/左/右の左右差が成立。停止まで一度の再生を保持 |
| Stop/再開 | 発音中Stop、停止後無音、再開0.425秒、右パン保持、最終無音とC4音高が成立 |
| 初回発音 | 1.15秒。固定1秒基準に不合格。18/19合格でもaudio=falseを維持 |
| native保存/復元 | 作者3356・別復元21780とも通常exit0。Pan R64・Volume100復元。31leaf中bins1件のmixのみ変更、他30leaf不変 |
| 途中preview/取消し | ソース接続済み。現行Computer Useのdragは一括入力で途中保持/取消しを観測できず、本体/PCM未確認 |
| 依存/比較 | 78moduleの点一覧のみ。原版40hash監査・原版動的/厳密外観比較・全機能依存解消を意味しない |

長いSegmentをFitした際の密な縦グリッドと、有効1と無効7のボタン重なりは画像で確認した残差として保持する。新しい細部で今回の検証範囲を広げず、後続queueへ置く。原始98/170一巡は162022の記録を保持し、201607の全一巡は未実行。

一次記録: `work/analysis/ui-band-mixer/20261010T175036931Z/live-gesture-followup/start-v1.json`、`source-changes-v1.json`、`candidate-identity-v1.json`、`working-state-v1.json`、関連 `work/acceptance/regression/20261010T202229448Z/run.json`、`work/analysis/ui-band-mixer/20261010T175036931Z/live-gesture-followup/main-repeat/actions.json`、`audio-proof-v1.json`、`native-proof-v1.json`、`work/analysis/ui-band-mixer/20261010T175036931Z/gui-save-reload-proof-v5.json`、`unit-record-v5.json`。

SSP05の初回発音1秒基準、途中preview/取消し、継承値、原版/厳密外観、全98/170一巡を未達で保持する。同群の細分化を区切り、独立したSSP06画像29〜37のCC/PBend/Expressionを、CHM・既存Sequenceモデルとの照合→開始記録→実装→新候補関連/本体/native保存・別process復元/固定PCMへ進める。SSP01〜04・独立Style015244・46native/28driver OS凍結を保持する。

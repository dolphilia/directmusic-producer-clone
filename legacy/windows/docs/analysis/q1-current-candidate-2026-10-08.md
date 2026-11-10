# 現候補 Q1 の代表経路検証（2026-10-08）

現候補 20261008T030413240Z で、新規 native Project、五形式の編集・Undo/Redo・保存、別プロセス復元、Style と Transport の音声までを結合して限定合格とした。全40責務と全体8受入は未完了で、fullAcceptance=false を維持する。

[完了証拠](../../work/analysis/q1-current-030413/20261008T053450054Z/completion-20261008T071418524Z/proof.json) SHA-256: a527df80e8c7fe528cb51296c6ac6f9226c732bca7e0cda0f27f58c8ba499419。
[単位記録](../../work/analysis/q1-current-030413/20261008T053450054Z/unit-record.json)と[統合判定](../../work/analysis/q1-current-030413/20261008T053450054Z/scenario-proof.json)から一次証拠へ辿れる。統合判定 SHA-256: 0703cc50e881ffba327fcd6a55acbc8dd8c1257980aff0272671ef31a1d329b3。

## 固定候補と入力

[build-summary](../../work/build/product-snapshot/20261008T030413240Z/build-summary.json)の218保存ソース・作業ソース、4生成物の一致を判定器が再照合した。configure/build/install は各0。build-summary SHA-256 は 8abb10f27016c53ef35c04bb1351b106915a320499b7b913f4b9bf65e2dee8fa、install Producer.exe は f3ec7a1c54d87c39783462698a2cb2fdb9fad0055715964b4a9ed65ff1b67d8d。製品ソース・期待値・閾値は今回変更していない。

[開始契約](../../work/analysis/q1-current-030413/20261008T053450054Z/unit-start.json)は不変。旧候補の12入力を入力材料として保存し、実行成功を転用していない。Control clone は root GUID の16 bytesだけを分離し、それ以外の bytes を保持した。

本体で作った Fresh.pro は DMST 1、DMSG 4、DMBD 1、DMAP 2、DLS 1、DMSC 1 の10所有文書を持つ。全所有GUIDは一意で、Project catalogのGUID/sizeと文書bytesが一致する。録音にはこれらとProject、外部waveを合わせた最終12入力をhashで固定した。Fresh.pro SHA-256 は 3bdb1f4fb14ff59376034133a27f9449fd426e5da09e2d67350895c72112b1c1。

## 操作・保存・正常終了

| 実プロセス | 成立した範囲 |
| --- | --- |
| 初期author 1056 | 新規ProjectとInitial Note、Sequence、Style、Band、DLS、AudioPathの編集/Undo/Redo/native保存、通常exit0 |
| 修正author 23928 | AudioPath名称を文字通り Q1 Current Conflict に修正し、Undo/Redo・AP/Project保存、通常exit0 |
| 最終reload 436 | 修正後の全保存値を別プロセスで復元、両録音を順次実行、通常exit0 |

初回保存名称は空白を欠いていたため、判定器の期待値を変えず実GUIで修正した。初回12入力・native監査・終了記録は first-author-native に保全した。全authoringが一つのPIDで行われたとは扱わない。最終PIDは開始時刻 2026-10-08T06:44:13.2657759Z、通常終了は 2026-10-08T07:08:27.7286205Z。read-only Watch-ProductGuiExit の実ハンドル/EXE/hash/exit0とGUI Closeを結合した。

最終reloadはSequence 0 clocks=5 BPM / 3072 clocks=7.5 BPM、Style109、Band volume101、DLS Key low1、AudioPath名称、Route PChannel0 count16、VBScriptと「本体 日本 Ω 🎵」を復元した。最終12入力はauthor保存後・各録音後も不変だった。独立RIFF監査は原版相互編集の証明とは区別する。

## 実 WASAPI 音声

| 固定録音 | 合格結果 |
| --- | --- |
| Style 90秒 | MIDI60を10発、全9間隔2秒、音高一致、baseline/停止後RMS0、packet整合、peak約0.09304 |
| Transport 300秒 | 競合defaultのControl RMS0、embedded Route優先、発音中Stop、停止中RMS0、全8音再開、間隔12/12/12/12/8/8/8秒、音高・DLS持続・packet整合、末尾RMS0 |

Style109の設定は復元を確認し、発音間隔はSegment120 BPMの契約で判定した。Styleのembedded Bandは初期volume100のprivate copyであり、後のowned Band101と同一bytesにはならない。immutable unit-start入力への参照を明記し、録音runの原本も保持した。

Transportの中断前は3発を観測し、Stop直前RMS約0.06526だった。停止後の保持区間はRMS0、再開は正しい8音を完遂した。Styleの自然終了とSequenceの発音中Stopを別々に評価した。

Style判定器CLI PID12768 exit0、6 controls CLI16020 exit0、Transport判定器20792 exit0、11 controls18036 exit0、統合判定4028 exit0。変更なしのPCMを採択し、無音化・欠落音・誤音高/テンポ・停止後混入・再開欠落・持続不足等の明示的欠陥を拒否した。

画面記録補助のエラーと、持続REPLの古い記録先を参照した状態は保全した。録音用操作ログは実際の一次GUI actionのbefore/after時刻とwindowから結び直し、推測時刻・再操作・PCM修正は使っていない。誤記録と空の旧記録も保存した。

## 正本と再現

状態更新CLI PID10096 exit0。[状態更新証拠](../../work/analysis/q1-current-030413/20261008T053450054Z/completion-20261008T071418524Z/state-update-proof.json)は全体8項目のstatus/scope/remaining、責務キー、登録native89/driver152と原始driver一巡の不変を検証した。6作業中/2障害/0全体合格を維持する。driver152の原始24合格/23障害/105未実行と、Send/Q1の同候補補足を分けた。

保存された判定器CLIのrun.jsonにNode/driver/wrapper hash、引数、PID、stdout/stderr、正常exitがある。再解析はscripts/Inspect-ProductGuiProjectStyleDlsAudio.mjsにStyle録音directoryと --style-tempo 109、scripts/Inspect-ProductGuiTransportDlsPriorityAudio.mjsにTransport録音directoryと --slow --require-lifecycle を渡す。統合はscripts/Inspect-Q1CurrentScenario.mjsへ単位directory、固定build-summary、両録音directoryを順に渡す。再録音は同じnative入力を新しい本体プロセスで開き、Capture-ProductGuiAudio.ps1 -SilentKeepAlive と実GUI操作を新規証拠directoryへ記録する。

原版approval timeout、各Windows5保存拒否、Q2独立Windows未用意は凍結を維持する。Farmのdescriptor/class/graph/KEEP、Wave loop/end、Send外部宛先寿命、既定Reverb、legacy ABI、Timeline/OLE、ライブラリ・配布などの残責務は保持した。次はFarmのname/category/class参照解決を契約・既存実装から照合し、実装不足を減らす。

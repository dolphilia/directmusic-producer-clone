# Q3D 継承DLSのruntime保存

候補 `20261007T060019216Z`、207保存ソースで、WaveのWSMPを継承するDLSのruntime保存を修正した。nativeのRegion WSMP不在と編集履歴を保ち、OSへ渡すruntime出力だけにWaveの設定全体を明示する。standalone・Project一括・設定済み出力に共通の境界を適用した。全40責務・全体8受入は未完、`fullAcceptance=false`。

公開SDKのWSMPL、保全した原版help、以前のOS再生での継承/明示設定の差が契約根拠である。runtime出力にも有効設定を持たせる判断はこの境界からの適用であり、原版Producerの動的保存結果を観測した主張ではない。開始時契約は既記録hashと一致するbytesを保全し、追記した現行契約と区別した。

44専用チェックで拡張WSMPヘッダ・ループレコード・末尾、独立SMPL、未知chunkのodd padding、GUID/PCMを保持した。明示ゼロループを継承へ変えない。不正Unity Noteのstandalone・一括・設定済み出力は公開前に拒否し、既存出力・native bytes・dirty・保留Redoを維持した。Sequence26、DLS policy28、runtime Save As25、folder15、AudioPath export7も同候補で合格した。

本体作者PID22628で「Waveの設定を使用」、Undo、Redo、native DLS/Project保存を操作し通常exit0。別PID15048でnative Projectと保存済み継承を復元し、runtime DLS/Segmentを保存して通常exit0。さらに別PID10324ではruntime Segmentだけを開き、同じ出力ディレクトリのDLSを参照して再生し通常exit0。空の起動文書は保存した。独立RIFF監査でGUI変更はRegion WSMPの除去だけ、runtime DLS変更は全Wave WSMPの複製と既契約のauthoring dmpr除去だけで、Band/Segment/native DLS bytesは不変。Projectの差分は2文書のruntimeディレクトリ設定だけで、3文書のGUID・サイズ・native名が一致した。一括/設定済みの拡張carrierも製品パーサーを使わない期待木で照合した。

nativeとruntime各90秒を対応済みWASAPI録音器のSilentKeepAliveで取得した。同じendpoint `{0.0.0.00000000}.{e57d7fbb-8f4a-4c47-9f28-60cc4f93d911}`、同じ一秒C4の所有DLS、12 BPMの2音を2回再生した。各発音は一秒を越えて持続し、発音間隔10秒、発音中Stop、停止から再開まで無音、再開後の同じ音高を確認した。停止後最大RMSは双方0、packet最大gap2 frames、timestamp error0。4音のnative/runtime RMS差は1%未満で、固定正弦音に別音を重ねた混入も判定する。テンポ値と発音間隔の確認であり、テンポ変更全経路の完了ではない。

native/runtime各19対照は無変更を合格、持続欠落・再開欠落・誤音高・発音中/停止後混入・Stop欠落/遅延・PID/window誤指定・同作者PID・非正常/強制終了・build/input hash・packet gap/error・ループ/テンポの不整合18負例を拒否した。初回のwrong-tempo負例はASCII探索がトラック識別子を変更して実tempoを変えなかったため試験を失敗にし、RIFF構造探索へ修正した。両失敗ディレクトリを保持した。最初のnative録音はStopが遅く発音中停止を証明できず、判定器exit1と録音を保全した。時刻指定の一操作ずつの実行へ変更した後の録音のみ合格。

一次証拠は [単位記録](../../work/analysis/q3-dls-runtime/20261007T055248Z/unit-record.json)、[独立単位証明](../../work/analysis/q3-dls-runtime/20261007T055248Z/unit-proof.json)、[native PCM](../../work/analysis/q3-dls-runtime/20261007T055248Z/native-timed-session/audio-20261007T063435649Z/dls-runtime-audio-proof.json)、[runtime PCM](../../work/analysis/q3-dls-runtime/20261007T055248Z/runtime-session/audio-20261007T064555101Z/dls-runtime-audio-proof.json)、[native対照](../../work/analysis/q3-dls-runtime/20261007T055248Z/native-controls-corrected/negative-tests.json)、[runtime対照](../../work/analysis/q3-dls-runtime/20261007T055248Z/runtime-controls-corrected/negative-tests.json)、[失敗履歴](../../work/analysis/q3-dls-runtime/20261007T055248Z/gui-and-auditor-failures.json)。build summary hashは `2992755b07140b240badd4c1fc57eec4abcf12be2b4db6e73bdb274d015ea3f9`、Producer.exeは `5a34390c1c339e5aef03dfb8b3141880c8ed3b5c1b9d7d385000aa0afc45f4ca`。

再判定コマンド（リポジトリroot）:

```powershell
node scripts/Inspect-DlsRuntimeAudio.mjs work/analysis/q3-dls-runtime/20261007T055248Z/native-timed-session/audio-20261007T063435649Z native
node scripts/Inspect-DlsRuntimeAudio.mjs work/analysis/q3-dls-runtime/20261007T055248Z/runtime-session/audio-20261007T064555101Z runtime
node scripts/Inspect-DlsRuntimeUnit.mjs work/analysis/q3-dls-runtime/20261007T055248Z work/build/product-snapshot/20261007T060019216Z/build-summary.json work/analysis/q3-dls-runtime/20261007T055248Z/native-timed-session/audio-20261007T063435649Z work/analysis/q3-dls-runtime/20261007T055248Z/runtime-session/audio-20261007T064555101Z
```

新規実行では同候補installと44/28チェックが生成した新carrierを使う。Computer Useでnative継承編集/UndoRedoを行い、native保存・通常終了・別PID復元・runtime保存・通常終了・さらに別PIDのruntime再生を結ぶ。各再生を開始して14秒でStop、3秒以上無音を挟んで再開し、再度14秒でStopする。対応録音器は `work/build/audio-capture/20261006T211007927Z/build-summary.json`、90秒SilentKeepAlive。クリック前後UTC、QPC/UTC校正、入力・録音器・endpoint・packets・PCM・通常exitをhashで保存し、新しい対照ディレクトリへ上の負例を実行する。既存証拠を上書きしない。

native81一巡は原始49合格/32障害。後発wrapperのRouteBand.bnp atomic replace Windows error5を保持し現行48合格/33障害。driver原始119は21合格/1失敗/21障害/76未実行で、後発OS拒否を別扱いにした基準は21合格/22障害/76未実行。今回追加した3判定器driverを同候補補足として登録・実行し、inventory122は24合格/22障害/76未実行。原始一巡を122件実行済みと書き換えない。

PID14984のCLI起動はGUIが観測できずtimeout/exit nullのまま保持した。OS拒否や製品故障とは断定していない。所定のComputer Use復旧で得たPID15048だけをnative復元へ結合した。runtimeセッションのProjectを入力より狭いディレクトリに保存した試行は製品の相対参照検証エラーで、両入力を含む親へ保存先を訂正した。原版DLSDesigner不在、StylePlayer approval timeout、Q2独立Windows未用意、core/Script/Container/RouteBandの別々のWindows5を解除していない。DLS ADSR/release、WaveTrack loop/end、全対象の外部ABI・編集互換・原版比較は残責務。次は独立したWaveTrackループ責務へ進む。

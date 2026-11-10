# Segmentトラックの保存順とランタイム順

対象は本体の新規Sequence・Band作成。全体受入は未完了。

SDK固定参照 `work/analysis/sources/dmusicf.h` の `DMUS_IO_TRACK_HEADER` は、GUIDの直後のDWORD（データoffset16）を `dwPosition`、次のDWORD（offset20）をgroup bitsと定義する。DX8のflags/priorityは別のextras chunkに属する。ヘッダーをgroupや位置として取り違えない。

build031059939Zで入力の六つのposition DWORDだけを変更した比較を実施した。原入力は旧GUIで保存した `resaved-after-restart.sgp`（SHA d398c0053c7db8d0cbf6c70e5b58bfc6902319c3fd2071832735c197131d9526）。旧GUI成功を現行GUI成功へ転用する試験ではない。

| 同一型のRIFF順group | position | Sequence GetTrackGroup順 | Band GetTrackGroup順 |
| --- | --- | --- | --- |
| 1,2,6 | 0,0,0（既存観測） | 1,2,6 | 6,2,1 |
| 1,2,6 | 1,2,3 | 1,2,6 | 1,2,6 |
| 1,2,6 | 3,2,1 | 6,2,1 | 6,2,1 |
| 1,2,6 | 2,3,1 | 6,1,2 | 6,1,2 |

各変更入力は23 group queries、3 Tempo parameters、入力全bytesのランタイム保持、Play/Stopを実測し、独立RIFF監査で変更対象六DWORD以外の一致を確認した。各実行の55モジュールを別々に監査し、原版40 PEのhash一致は0。証拠は `work/acceptance/track-position/20261003-position/position-proof.json` と、そのresultsが結合するrun/proof/module provenance。位置の昇順が観測された範囲を示すものであり、同順位の一般アルゴリズムやBandイベントの同一性を証明していない。

本体の契約：新規Sequence・Bandを追加する際は、全既存DMTKの最大position+1を新しいヘッダーへ書く。既存ヘッダー、extended bytes、opaque chunks、group bitsは変更しない。既存最大がUINT32_MAXなら追加を拒否し、文書bytes・cache・dirty・Undo履歴を保持する。Sequence/Bandを追加した後のUndo/Redoは位置も含む文書snapshotを復元する。再生用のテスト文書もこの経路を使う。

この契約は保存順と実行順の無条件な対応付けを導入しない。読み込んだ文書にはpositionの重複やRIFF順と異なる位置が残り得る。編集UIのindexは引き続きRIFF順のgroup/type filter内index、ConductorのindexはWindows runtime内indexである。対応が確認できない入力で両indexを流用する変更は行わない。新規Tempo/TimeSigや読み込んだ既存トラックの位置変更は今回の対象外。

残る検証：原版Producerによる新規・並べ替え・保存時のposition/extras観測、同group・同positionの複数Bandのイベントidentity、extras flagsとparameter/playbackの関係、保存後の現行GUI別process再読込・実音声。この証拠を得るまでは一般的なeditor/runtime mappingを完成扱いにしない。

再現：`node scripts/Inspect-TrackPosition.mjs prepare <directory> <input>` で所有する入力コピーを作り、各コピーを `scripts/Test-GroupRuntime.ps1 -BuildSummaryPath <summary> -Segment <copy>` で一度ずつ実行する。各runを `node scripts/Inspect-GroupPlayback.mjs <run.json>` と `scripts/Inspect-ProductModules.ps1 -RunPath <run.json> -CaseName group-playback-api` で監査し、`node scripts/Inspect-TrackPosition.mjs audit <directory> <ascending-run> <descending-run> <permuted-run>` で比較を結合する。スクリプト初稿のBand CLSID誤記によるprepare assertion失敗もunit recordに残す。SDK固定参照のCLSIDに訂正後、生成した入力だけが今回の成功対象。


2026-10-03 同版GUI追加検証：build034701101Z/EXE bc2f9bed44446a2f138ee0606bcb2313c5525635038bdff9573d84ae45375097。GUI035336791Zでgroup1/group2のSequence・Bandを新規作成、piano.bnp/source.sgp/project.dmpj保存、正常閉鎖、別process起動・project再読込・group選択を再適用、Play/Stop表示、Segment別名再保存とproject参照更新。70 captures、二窓は別ID（1901946/4458694）、閉鎖後不在を確認、exit code未測定。起動時未保存starter文書とprojectを両方保存してからtarget projectを開き、discardはNo。source/resaved.sgpは808bytes全一致、Tempo/Sequence/Band/Sequence/Bandのposition0..4・group1/1/1/2/2、各note0/384/60/96/pchannel0、各Band時刻0/0・embedded piano.bnp全一致を独立RIFF監査。project metadata保持・相対参照source→resavedのみ変更。GUI129 modulesと保存入力runtime55 modulesは原版40hash一致0。runtime041231132Zは15group/3Tempo・全bytes snapshot・PlayStop・保存順一致。監査 `scripts/Inspect-AppendedTrackPositionGui.mjs <GUIdir>`、`scripts/Test-GroupRuntime.ps1 -BuildSummaryPath <034701101Z-summary> -Segment <GUIdir>/resaved.sgp`、`scripts/Inspect-GroupPlayback.mjs <run.json>`、`scripts/Inspect-ProductModules.ps1 -RunPath <run.json> -CaseName group-playback-api`。証拠GUI/appended-position-gui-proof.jsonとruntime/group-playback/group-playback-proof.json。現行音声・原版動的比較・一般Band identity・全40全八は未完了。過去native/gui/audio成功の転用なし。


2026-10-03 原版position GUI観測の障害：`work/analysis/reference-track-position/20261003T041600Z` に原版42filesをハッシュ一致で複製し、既存許可に従ってComputer Useのlaunch_appで一度起動した。helperはtargetable window無しを返し、list_windows再観測でも原版窓なし。所有PID5400は存在・Responding true・MainWindowHandle0、module数はprocess-diagnostic.jsonの実測値を正とする。UI原版観測は未実行。OS拒否を示す証拠はなく、原因未特定。観測用PIDのEXE絶対pathを照合後、そのPIDの停止を試みたがWindowsがアクセス拒否。停止失敗・プロセス残存を記録（正常終了とは扱わない）。同条件再起動・COM登録変更・security変更なし。製品側のビルド/GUI/runtime成功を否定しない。次は原版startup/profile/登録依存をread-onlyで切り分け、GUIが成立するまで依存しない本体機能を進める。

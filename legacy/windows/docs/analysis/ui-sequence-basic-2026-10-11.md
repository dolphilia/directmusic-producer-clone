# Sequence基本と楽器割当の本体統合

2026-10-11（日本時間）。指定画像13〜16・21〜24に対応するSSP03は作業中。単位は `work/analysis/ui-sequence-basic/20261010T144602091Z`。限定採用0735、独立未完Style015244、全8受入6作業中/2障害/0合格と `fullAcceptance=false` を維持する。

## 実装と根拠

物理Sequenceトラックごとの開閉式ピアノロール、鍵盤、淡黄active/淡白inactive、半透明入力位置、矢印キーと赤い時刻バー、Insert/右メニュー入力、青/赤のノート選択を既存Segmentモデルへ接続した。各physical track/PChannel/groupを区別し、原版CHMの複数パート契約を保持する。インポートした空トラックのpart metadataはまだ完全解読しておらず、channel0 fallbackを未解決として残す。

楽器アイコンからInsert New Instrument→Piano 1、GM/GS階層からOrgan→Organ 1へ進む。対象時刻までのBandとPChannelを更新し、一つのSegment履歴として保存する。GM/GS名は旧コピーのBandEditor.ocxを静的解析して得た235項目（GM128、GS98、drum9）。旧コピーの実行・build依存は追加していない。GSのpacked bank/program解釈と原版メニュー件数との差は比較未確認。

試聴はTransportから独立したPerformanceへ、現在の楽器・pan/volume・DLS参照を含む一音の非保存スナップショットを渡す。元文書を試聴用に書き換えない。

## 候補別の一次結果

| 候補 | 結果 |
|---|---|
| 150200 (`20261010T150200342Z`) | 263ソース/4出力一致、関連7合格590検査、2既知拒否は起動なし。本体で2ノートとOrgan 1保存、author23324通常exit0。展開クリックが入力時刻を変える問題を発見。 |
| 152201 (`20261010T152201724Z`) | 展開/ヘッダー選択の修正。263ソース/4出力一致、関連7合格590検査、2既知拒否は起動なし。author11080で追加2ノートを保存、通常exit0。別process25324で4ノート/Organ 1を復元し通常exit0。 |
| 154512 (`20261010T154512990Z`) | 初回Performance初期化を画面入口へ移動。263ソース/4出力一致、関連7合格590検査/2既知拒否は起動なし。作者12276が6ノート/Organ 1を保存・通常exit0、別process21804で復元/入力不変・通常exit0。固定300秒PCMは初回1.19秒で不合格。 |
| 162022 (`20261010T162022402Z`) | 無音の試聴AudioPath/楽器準備を実装。264ソース/4出力一致、関連13合格755検査。作者17360が8ノート/Organ1保存・通常exit0、別process1668が復元/入力不変・通常exit0。固定300秒PCMは初回1.15秒で不合格。native98/driver170一巡の合格・障害・未実行は下記へ分記。 |

152201の独立RIFF監査は時刻1536/1728/3264/4608、音高60/61/62/58、duration192、velocity96、PChannel0の4ノートを確認。Organ 1はprogram16/flags0x1103で、pan/volumeの有効bitは増やしていない。初回2ノート保存からの差はSequenceのseqtだけ、他49leaf payloadは同一。再読込後の入力ファイルも不変だった。入力位置だけの操作は文書をdirtyにせず、復元直後のUndo/Redoは無効。

150200で追加実行したstyle-player-bandsは49検査後「Later anonymous Band source cannot be resolved」で失敗した。過去の同一stderr SHAと一致する既存未解決であり、152201では同条件を再試行していない。これを関連全体の合格へ含めない。

## 音声の失敗と次の修正

152201/author11080の300秒WASAPI録音に、Organ C5/D5とPiano C5の3クリックを時刻付きで記録した。固定判定は「クリックから1秒以内の発音」で失敗。診断では初回1.30秒、後続0.49/0.50秒。PCM、packet、UTC/QPC校正、操作、判定器、失敗記録を保存し、閾値を変更していない。音程/音色/無音の受入全体は未合格。

154512の3クリックPCMは初回1.19秒、後続0.485/0.485秒で不合格だった。162022でAudioPath/楽器Downloadも無音準備・再利用したが、別起動17360の3クリックは初回1.15秒、後続0.505/0.430秒。固定1秒の閾値を維持し、音高/音色/無音を含む全受入は未合格。native API準備16ms/Play0msはPCM開始遅延の合格根拠にしない。原因内訳は未測定のため推論と一次失敗を区別する。

## 証拠と未達条件

`unit-record-v1/v2.json`、`working-state-v1/v2.json`、`source-changes-v1/v2/v3/v4.json`、`build-identity-v1/v2/v3.json`、`gui-native-audit-v2/v3/v4.json`、`gui-save-reload-proof-v1/v2.json`、`native-save-diff-v1/v2/v3.json`、`gui/actions.json`、`audition-pcm-failure-v1/v2.json`、`audition-onset-diagnosis-v1/v2.json`、`related-failure-classification-v1.json`に候補・hash・入力・PID・通常終了を保存した。v3監査のsaved.path誤記はv2 GUI証拠のerratumへ明示し、旧記録は保持。v4はprepared-saved-nativeの正確なパスと6ノート/Organ1・49leaf不変/seqtのみ変化・二終了0・入力hash不変を確認した。

原版同入力の動的比較、厳密外観、Transport同時再生中の試聴、他velocityのPCMは未達。次はノート端/bodyドラッグ・Edit Velocity・全Snap（17〜20）のSSP04へ進む。Band pan/volume（25〜28）、CC/PBend/Expression（29〜37）、新part/同時編集/drum/DLS（38〜43）、連続本体シナリオを後続段階へ残す。

162022の最初の録音は3回目のクリックが録音終了後になったため、試行手順の不成立としてaudio-protocol-failure-v3.jsonへ保存。製品の音声失敗とは区別する。再録音は同一生成物・同一入力・同一300秒/閾値で、別起動17360の3操作を録音内に収めた。native試験のAPI所要時間をPCM発音遅延の成功に転用しない。

162022の現行証拠は `unit-record-v3.json`、`working-state-v3.json`、`source-changes-v7.json`、`build-identity-v4.json`、`gui-native-audit-v5.json`、`gui-save-reload-proof-v3.json`、`native-save-diff-v4.json`、`audition-pcm-failure-v3.json`、`audition-onset-diagnosis-v3.json`。8ノートの最後は時刻7680/pitch67、9216/pitch69。保存差分はseqtだけ、他49leaf不変。二終了の直後window一覧には旧windowが一時的に残ったため、通常終了はlaunch.jsonのexited/exitCode0で確認する。

原始native98一巡は51合格2746検査/40既知OS凍結/新6OS拒否/既知匿名Band失敗1保留。関連段階の新3拒否と合わせて46nativeがOS障害（旧37+新9）。新9はcanonical又はGetFinalPathNameByHandleWのAccess deniedで、stderr・対象パス・PID・解除条件を個別保存し同条件再試行を凍結した。原始driver170は20合格/28凍結/122未実行。`registered-native-os-classification-v1.json`と`registered-driver-summary-v1.json`で分類し、障害/未実行を合格に含めない。

準備前倒しによる二段階の修正後も初回発音基準を満たさないため、この未達を保ったまま独立SSP04へ移る。発音調整のみの反復を区切り、指定された未対応編集操作を減らす。全体完成、SSP03完了、原版比較成功への昇格は行わない。

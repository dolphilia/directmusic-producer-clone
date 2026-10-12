# Band音声再検証（画像25〜28、2026-10-11）

候補080641（20261011T080641928Z）を追加ソース変更なしで再検証した。既存と同じ300秒・19条件は **18/19で不合格**。初回発音0.470秒、再開0.480秒は1秒以内に改善したが、最初のStop後の静音条件が不成立だった。Band全体・原版外観操作の合格にはしない。

## 適用条件と結果

[一次単位](../../work/analysis/ui-band-revalidation/20261011T093249087Z/unit-record-v1.json)、[固定プロトコル](../../work/analysis/ui-band-revalidation/20261011T093249087Z/audio-protocol-v1.json)、[PCM判定](../../work/analysis/ui-band-revalidation/20261011T093249087Z/audio-proof-v1.json)、[録音](../../work/acceptance/product-project-gui/20261011T093303670Z/audio-20261011T093543762Z/run.json)を参照。旧191秒C4・Organ1ノート/192秒Segmentを新しい検証コピーで使用し、先頭から再生した。旧判定器とbyte一致、発音1秒・停止後settle1秒・静音RMS0.0001/peak0.0005等の数値条件を変更していない。

| 検証 | 結果 |
| --- | --- |
| 初回/再開発音 | 0.470秒/0.480秒、成立 |
| 音量低下/回復 | RMS比0.1589/6.2948、成立 |
| 左右パン、再生中の音量・パンUndo/Redo | 成立 |
| 発音中Stop | 191秒ノートの開始約158.474秒後に入力、成立 |
| firstStop後の静音 | **不成立**。固定窓181.096〜200.628秒の右RMS0.013423/peak0.136961 |
| 最終Stop後の静音 | 成立。219.683〜299.700秒、RMS約0.0000610/peak0.0000610 |
| C4音高、packet integrity、安定窓、連続手順 | 成立 |

12操作は約197.181秒を要し、追加した「前半150秒以内」の作業目安は達成しなかった。既存の19条件へ追加・削除した基準ではなく、目安不達と音声不合格の両方を保持する。初期/最終静音は数値基準内の背景レベルであり、完全なゼロではない。

[停止診断](../../work/analysis/ui-band-revalidation/20261011T093249087Z/stop-diagnosis-v1.json)では10ms窓で、firstStop入力完了から約1.78秒、finalStopから約0.645秒まで基準超過を観測した。この診断で固定判定を上書きしない。runtime traceは4096イベントで上限となり、最初のStopより前に記録が途切れた。StopExの戻り値・IsPlaying待機・AudioPath deactivateの順序と実時間はこの記録だけでは確定できず、原因を断定しない。

## 保存・再読込・候補同一性

作者PID18540と別process PID20920はいずれも通常終了exit0。別processでPCh1/Organ1/Pan R64/Volume100と1ノートを確認した。[保存監査](../../work/analysis/ui-band-revalidation/20261011T093249087Z/main/native-proof-v1.json)は意図したBand binsのpan/volume/flagsだけの差分、他30 leafの不変を確認した。

Projectはbyte不変ではない。[Project監査v2](../../work/analysis/ui-band-revalidation/20261011T093249087Z/project-save-proof-v2.json)で既存filhのFILETIMEとファイルサイズ欄だけの更新を確認した。初期Projectの記録サイズ2894が実Segmentサイズ1622へ修正された。FILETIMEだけを許容した最初の監査は失敗したため、[失敗記録](../../work/analysis/ui-band-revalidation/20261011T093249087Z/project-save-audit-failure-v1.json)と診断を残した。音楽payload差分と管理メタデータ更新を分ける。

288保存/作業ソースと4生成物は080641と一致。実ロード80モジュールは74x86/6WoW64ホスト、正式原版インストール先からのロード0件を確認した。[証拠整合監査](../../work/analysis/ui-band-revalidation/20261011T093249087Z/artifact-audit-v1.json)は25/25成立したが、これは証拠の整合性でありPCM不合格を合格へ変更しない。正式原版EXE・保護した原版3入力・ユーザー画像47枚はhash不変。

製品ソース・生成物に追加変更がないため、同一候補の直前一巡native100（53合格2965検査/46OS凍結/既知Band1保留）、driver172（20合格/28OS凍結/124未実行）と専用3runを参照し、今回再実行していない。専用runを原始一巡の合格数へ加算しない。録音器の存在しないbuildを指定した準備失敗は実録音開始前であり、記録を保持して有効な既存録音器へ修正した。

## 引き継ぐ不足と次の作業

画像25〜28のBand操作に関する補足結果であり、旧201607の初回1.15秒失敗を消去・置換しない。ドラッグ途中preview/取消しの本体・PCM、継承値、画像27のL2/102の今回再確認、原版の厳密外観・全操作比較は未確認。Sequenceベロシティ300秒12/12は別単位として保持する。

次はStop時の重要イベントが欠落しないログを用意し、同じ長いノートと再生中の音量・パンUndoRedo後のfirstStopを再現する。原因を確認した所有再生ライフサイクルだけ修正する。製品変更時は新候補のidentity・関連試験・native100/driver172を確認した上で、同じ300秒・19条件・settle1秒で再検証する。音源変更や窓の後送りで今回失敗を合格にしない。

Count-In15/16（操作窓80.906秒>80秒）、Echo停止中/実機/Advanced/録音、SSP04全gestureと原版比較、独立Style015244/Q2/OS凍結は保持。限定採用0735、全体8受入6作業中/2障害/0合格、fullAcceptance=false、goal activeを維持する。

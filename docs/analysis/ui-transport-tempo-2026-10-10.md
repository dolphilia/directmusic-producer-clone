# Transport Controlsの速度変更と限定統合

2026-10-10（日本時間）。ユーザー画像2とSSP01に対応する試聴速度の経路を実装し、候補074753で実行した。[単位記録](../../work/analysis/ui-transport-tempo/20261010T072440521Z/unit-record-v1.json)が一次証拠の入口。SSP01全体・原版互換・全体完成は未達。限定採用0735と独立Style015244の未達条件を維持する。

## 実装した範囲

Conductorにmaster-tempo倍率の設定/実値読取を追加した。許可範囲は1〜10000%、初期の別速度は50%、非選択時は1倍。有限値/範囲を先に検査し、不正値は再生所有権・キャッシュ・文書を変更しない。初期化前の設定も実Performance初期化へ適用する。

本体にPlay、Stop Immediately、速度修飾ボタンと読取専用BPMの行を接続した。速度ボタンの右クリックでAlternate Tempoを開き、数値/スピン/OK/Cancelを提供する。ViewからStandardと独立して表示切替できる。4K/200%を維持した。BPMの編集、現在位置/先頭の別再生、境界停止、録音等を完成済みにしていない。

## 同じ候補で確認したこと

242保存/作業ソースと4出力のhashが一致し、configure/build/installは各0。build-summary SHA256はcff6191bc1ec20616cdadd4bc2b071029b939c8ea559d8baeaac0a68926232f5、Producer.exeは47aea41076108d2e5ec4c4253c8ae9da3e21593c3c2f16e307edddf83f6ce30c。関連timeline-range/sequence-crud/tempo-propertiesの3件は合格した。

| 倍率 | 実Performance clock/秒 | 期待値 | WASAPIの音間隔 |
| --- | ---: | ---: | ---: |
| 1 | 1534.759 | 1536 | 0.5秒 |
| 0.5 | 767.679 | 768 | 1秒 |
| 1へ戻す | 1536.338 | 1536 | 0.5秒 |
| 2 | 3077.460 | 3072 | 約0.25秒 |

実Set/GetGlobalParam、境界値1%/10000%、NaN/範囲外の不変、停止/再開を確認した。[32秒PCMと独立解析](../../work/acceptance/transport-tempo/20261010T075421154Z/transport-tempo-audio-proof.json)では音高維持、停止後の無音、再開の発音も合格。WAVだけ無音化した負例は解析exit1/不合格となり、音のない記録を合格にしないことも確認した。これはCLI専用経路の音声で、本体操作中の音声と取り違えない。

本体では125%を入力してCancel→50%保持、再表示→スピン51%→OK→ラベル51%、有効化、Play/Stopの状態切替、再生中解除、Viewの非表示/再表示を画面で確認した。操作支援のUIA値が50のまま残るため、更新値の判定は画面による。数値欄へのUnicode文字注入は空欄となり、通常の数字キー1/2/5とスピンは動作した。

検証用Segment/Projectを保存し、作者PID20792と別復元PID9592は両方通常exit0。復元後も120BPM、別速度は初期50%となった。Segment/Styleはbyte一致、Projectは既存filhのFILETIME欄だけ変化した。この本体入力にはSequenceノートがなく、GUIからの発音速度は未検証。

実ロード56モジュールはx86/出所解決済み、原版40hash一致0。ただし観測したCLI経路だけの点監査であり、GUI全経路・独立WindowsのQ2・全40の原版依存解消の証明ではない。

## 失敗と比較の限界

072949は不足headerでcompile失敗、073527は検証fixtureの2番目Tempo180が残ってclock判定失敗。bulk選択ではChangeが成立しないことを独立RIFFで確認し、明示選択と戻り値検査でfixtureを修正した。失敗build/録音は保持し、旧候補の成功を074753へ転用していない。

正式原版では別速度の既定50%、右クリックのダイアログ配置とCancelを観測した。文書/設定は確定していない。同じ入力での再生比較、全Transportの外観・操作比較は未達。元のユーザー原版プロセスは終了していない。

## 次の実装と残差

SSP01は継続中。現在位置からの再生とFrom Start、境界停止/所有権、Transport List/Transition、MIDI録音、編集可能Current Tempo、Transport Options、MIDI/Performance Options、glyph/配置/ドッキング/設定保持を残す。次は既存ConductorとCHMからseek/From Start/境界停止の契約を固定し、限定した一経路を実装・本体・音声まで閉じる。SSP02のCreate New File(s)等は指定順で続ける。

現登録native96/driver166、074753での全登録一巡は未実行。旧Style015244の固定300秒PCM/96-164一巡、Q2環境、全40/全8、凍結条件は保持する。今回の機能部分は成立、originalComparison=falseのためboundedFinished/completed/fullAcceptanceはfalse。

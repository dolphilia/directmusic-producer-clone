# Play Cursor の限定統合と原版の部分比較

2026-10-11。ユーザー指定画像3の Transport Options にある Play Cursor を、Segment の編集カーソルへ接続した。最新試験候補は `20261011T040727214Z`。本報告は SSP01/UI13.priority-03 の不足を一部減らした記録であり、SSP01全体や全体受入の完了ではない。限定採用候補0735、独立Style015244、Count-In031359の音声失敗は維持する。

## 契約と変更

抽出済み原版CHMの `htm/toggleplaycursorbutton.htm` は、既定で再生に合わせて編集ペインのカーソルが動き、ボタンを解除すると固定されると説明する。再生カーソルは挿入位置を示す細い縦線である。[開始記録](../../work/analysis/ui-play-cursor/20261011T040133085Z/start-v1.json)に根拠hash、変更範囲、終了条件を保存した。

`main.cpp`、`frame_toolbar.h/.cpp`、`segment_timeline_window.h/.cpp`の5ファイルを変更した。Play Cursor を有効なチェックボタンとして既定Onにし、タイマーで再生中Segmentに対応するTimelineの表示カーソルを更新する。Offでは表示位置を固定して手動の挿入位置変更を許し、Onへ戻すと実際の再生位置へ戻る。Count-In中と停止後には進めない。停止で解放されたPlayback IDを照会しないガードを加えた。Conductor、文書シリアライズ、音声生成は変更していない。

初回候補040305は、静的レビューで解放済みIDの照会経路を発見したため実行せず保存した。修正後040727をビルドし直した。これは実際にクラッシュを再現したという記録ではない。[修正記録](../../work/analysis/ui-play-cursor/20261011T040133085Z/static-review-correction-v1.json)を保持する。

## 現候補の実行結果

| 段階 | 結果と範囲 |
| --- | --- |
| 構成・build・install | 各exit0。284保存ソースと作業木、4生成物が一致 |
| 関連回帰 | Timeline/Transport/Meter/Sequence audition/Metronome/Count-Inの6モード、300検査が合格 |
| 登録native原始一巡 | 100件：53合格2965検査、46既知OS条件で凍結、既知匿名Band1検証待ち |
| 登録driver原始一巡 | 170件：20合格、28既知OS条件で凍結、122未実行 |
| Main | Onで進行、Offで手動位置114432 clocksを32.202秒固定、Onで337360 clocksへ再追従。明示Stop後のOn/Off切替も安全 |
| Count-In | 32小節のCount-In中、84237 clocksを35.034秒固定。Count-In中Stop後も固定 |
| 保存・別process | author18184/reload8328、同じEXE/build、両方通常exit0。別processは保存済み3入力を開き、表示カーソルは初期位置0へ戻る |
| 独立監査 | [監査結果](../../work/analysis/ui-play-cursor/20261011T040133085Z/artifact-audit-v1.json)の18条件が成立。全体受入への昇格なし |

[build](../../work/build/product-snapshot/20261011T040727214Z/build-summary.json)、[native原始run](../../work/acceptance/regression/20261011T041635158Z/run.json)、[driver原始run](../../work/acceptance/registered-drivers/20261011T041941131Z/run.json)を一次証拠とする。旧候補の成功を流用していない。音声生成に変更がない表示単位のため新規PCMは非適用とし、031359の15/16・操作窓超過による総合不合格をそのまま残す。

## native保存差分

「3ファイルのbytesが全部不変」という開始時の厳しい条件は**不成立**。SegmentとDLSは完全に不変だったが、Project Save→Segment Save→Project再Saveにより、Projectの既存Segment参照 `LIST:file/filh` 内のFILETIMEが更新された。

独立RIFF読取ではProjectの長さ512 bytesと全チャンク構造が同一、差分は絶対offset212〜216だけで、44-byte `filh` のFILETIME欄（payload offset16〜23）に収まる。保存値134361667026274810はSegmentの実際の更新日時と一致し、記録サイズ1702も実ファイルと一致する。参照名・GUID・他のpayloadに差はない。この結果を「音楽データと参照契約の保持」として分け、厳しい全3ファイル不変を合格に書き換えない。

最初の保存直後は3ファイル不変だったが、Projectが再びdirtyになったため通常終了時の確認をNoで取り消し、Projectを再保存してから終了した。未保存文書は破棄していない。[保存記録の訂正](../../work/analysis/ui-play-cursor/20261011T040133085Z/main/save-exit-progress-v2.json)と旧v1を両方保持する。

## 正式原版で確認した範囲

手動起動済みの正式インストール版5.3.0.900を使用し、同じ初期3入力の専用コピーを開いた。Onで赤いカーソルとルーラーが進み、Offで両方が固定、Onで後の再生位置へ戻ることを確認した。OffではElapsed/Offset表示も23.483秒で固定されたが、Voices=1とStop有効の状態が続き、Onで73.390秒へ戻った。Stop後はVoices=0、Play表示へ戻り、原版は開いたまま停止状態にしている。

[原版比較記録](../../work/analysis/ui-play-cursor/20261011T040133085Z/original/comparison-v1.json)に操作・撮影・入力hashを保存した。原版3入力とインストールEXEは不変。AudioPathや速度表示などの条件は揃っておらず、音声・実時間の同等性、厳密な外観、Count-In再生の比較は未確認である。

## 残る差と次の作業

- 製品の長いSegmentを全体表示するとグリッドが密集する。拡大すると開始位置が画面外になる場合があり、カーソル更新だけでは横スクロールが追従しない。原版は数小節を表示して再生位置へ追従した。次はSSP03/UI02の表示単位で、読める目盛り・クリップ・再生中の追従を修正する。
- 原版のElapsed/Offset固定、細いカーソルの外観、アイコン、配置、厳密なDPI比較は未達。複数文書の再生位置分離はソース上の対応のみで、実GUI検証は未実施。
- GUI状態取得は一操作遅れる場合があった。数値判定には落ち着いた観測を用いた。自然終端付近の最初のtoolbar Stopは曖昧な操作として残し、明示Stop後の観測で停止を確認した。
- runtime traceは4096イベントでoverflowしており、完全な再生ログの証拠にはしない。初期UIAのカーソル未取得と監査のinstall出力パス誤りを修正した旧監査も保存する。
- MIDI/Echo/録音/Download、Count-In音声と同条件原版比較、全SSP経路の厳密外観、独立Style、個別OS凍結、Q2は引き続き未完。

[単位記録](../../work/analysis/ui-play-cursor/20261011T040133085Z/unit-record-v1.json)。全40責務と全体8受入は保持し、6作業中/2障害/0合格、`fullAcceptance=false`。

# Segment の表示範囲と再生中の横追従

2026-10-11。ユーザー指定画像13〜19・40に対応するSSP03/UI02の表示単位として、長いSegmentの密集した目盛り、トラック名欄への描画、画面外へ進む再生カーソルを修正した。候補は `20261011T045647838Z`。指定操作全体、厳密な外観、全体受入の完成ではない。

## 変更と根拠

抽出済み原版CHMのTimeline、Zoom、Play Cursor、Sequence拡大の説明と、040727単位で確認した原版の数小節表示・再生追従を根拠にした。[開始契約](../../work/analysis/ui-segment-viewport/20261011T045417593Z/start-v1.json)に根拠hash、閉じる不足、試験、終了条件を保存した。

変更は `src/producer/segment_timeline_window.cpp` のみ。初期表示を読める時間幅にし、全体表示では密集した細線と小節ラベルを間引く。グリッドと折りたたみ時のノートを時間領域へクリップする。Play Cursorが表示範囲を越えたら横スクロールし、Offでは固定する。拡大時は見えているカーソルの相対位置を保ち、Sequenceの開閉で倍率を初期化しない。

細線の6px、ラベル間隔の48pxはGDI/client座標による製品側の可読性方針であり、原版の仕様値やスクリーンショットの物理ピクセル数ではない。OSの200%設定は変更していない。文書モデル、履歴、Conductor、音声生成は変更していない。

## 実行結果

| 段階 | 結果 |
| --- | --- |
| 構成・build・install | 各exit0。保存/作業284ソースと4生成物のhash一致 |
| 関連回帰 | Timeline、Transport、Meter、Sequence audition、Metronome、Count-Inの6モード300検査が合格 |
| native原始一巡 | 登録100：53合格2965検査、46既知OS凍結、既知匿名Band1検証待ち |
| driver原始一巡 | 登録170：20合格、28既知OS凍結、122未実行 |
| Main表示 | 120小節の初期表示が約7小節幅。Fitでも線が面状に密集せず、Zoom/Sequence開閉後も先頭を保持 |
| Main再生 | 初期ページを越えて横追従。Offの89449 clocks/横位置23が固定。Onへ戻すと134807 clocks/横位置35へ追従。明示Stopを確認 |
| 保存・別process | 作者6640/再読込22056、同じEXE/build、両方通常exit0。再読込で表示カーソル/横位置が0、初期ルーラー1〜8へ戻る |
| 独立監査 | 表示状態、hash、保存差分、二終了を含む18条件が成立 |

一次証拠は[build](../../work/build/product-snapshot/20261011T045647838Z/build-summary.json)、[native run](../../work/acceptance/regression/20261011T050420473Z/run.json)、[driver run](../../work/acceptance/registered-drivers/20261011T050722719Z/run.json)、[独立監査](../../work/analysis/ui-segment-viewport/20261011T045417593Z/artifact-audit-v1.json)。旧候補の成功を新候補へ転用していない。viewのみの変更につき新規PCMは非適用とし、Count-In031359の固定PCM15/16・総合不合格は保持する。

最初の再生は観測中に自然終端へ達した。Off→Onの復帰と明示Stopには、固定位置89449 clocksから始めた2回目の短い再生を使用した。GUIの状態取得は一操作遅れる場合があるため、数値判定は落ち着いた観測を使用している。最初の再読込画像は原版ウィンドウに覆われていたため保存し、検証用本体を前面にした画像を別記録した。

## 保存データ

SegmentとDLSのbytesは完全に不変。Projectは512 bytesと全RIFF構造が同一で、既存Segment参照 `filh` のFILETIMEだけが変更された。絶対offset212〜216、payload offset16〜20の差分に限られる。記録値134361695677627600とサイズ1702は実際のSegmentと一致する。参照名、GUID、その他payloadは不変。

全3ファイルの厳密なbytes不変はfalseとして別記録した。保存順はSegment→Projectで、終了時に未保存文書を破棄していない。独立RIFF監査は製品のparserを使用していない。

## 原版と残差

手動起動済み正式インストール版を停止状態で読み取り撮影した。ルーラー42〜47、細い青の拍線、黒い小節線、時間線に上書きされないトラック名欄を確認した。[原版観測](../../work/analysis/ui-segment-viewport/20261011T045417593Z/original/viewport-observation-v1.json)は静的な部分比較であり、この候補と同じ操作列での動的比較ではない。原版の入力3ファイルとインストールEXEは不変。

残る差分は右端小節ラベルの省略、字体・行高・アイコン・配置、初期の展開鍵盤範囲外にあるpitch72ノート、複数文書の分離、同条件の動的原版比較。runtime traceは4096イベントでoverflowしており、完全な再生ログとはしない。新しい細部をこの単位へ無制限に追加せずqueueへ残す。

次はSSP01/UI13のMIDI Input・Echo設定の無効表示を調べ、入力port、設定保存、PChannel経路の未対応責務を減らす。MIDI/録音/Download、Count-In音声、独立Style015244、Q2、個別OS凍結、全40責務・全8受入を維持する。

[単位記録](../../work/analysis/ui-segment-viewport/20261011T045417593Z/unit-record-v1.json)。画像47枚のhashは不変、`screenshot/`はGit対象外、45番は誤撮影として除外する。全体は6作業中/2障害/0合格、`fullAcceptance=false`。

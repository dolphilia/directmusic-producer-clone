# Transportの現在位置・先頭再生・境界停止

2026-10-10（日本時間）。画像2とSSP01のTransport Controlsの不足を、候補083342で限定実装・検証した。[一次単位](../../work/analysis/ui-transport-position/20261010T082910362Z/unit-record-v1.json)が証拠の入口。SSP01全体・原版互換・全体受入は未達で、限定採用0735を維持する。

## 実装した範囲

Segment Timelineに文書編集と独立したカーソルを設け、クリック位置をGridへ合わせて赤線と時刻を表示する。Play/Spaceは開いているSegment editorのカーソル、Play From Start/Alt+<は0から再生する。editorがなければ0を使用する。これは後続のSequenceピアノロールのノート挿入カーソル全体を完成させるものではない。

停止中はFrom StartとPlay、再生中はStop ImmediatelyとStop at Boundaryの2ボタンへ切り替える。Space/Alt+<は即時停止、Alt+>はSegment既定境界での非同期停止。通常は所有する全再生、Ctrl指定はprimaryのみを対象とする経路を接続した。停止待ちでは資源を保持してmonitor終了を待つ。開始位置の範囲外は実APIや再生所有権を変える前に拒否し、private runtime Segmentだけへ開始位置を設定する。

## 同候補の確認

242保存/作業ソース・4出力のhash一致、configure/build/install各0。Producer.exe SHA256は07170d6e612f78ec247390bdab69cfe715204a97e7e15582fe3286eff3cb1879。関連timeline-range/transport-audiopath/sequence-crud/playback-monitor/tempo-propertiesの5件は合格した。

専用runtimeでは6144 clocksからの開始値読取、不正開始値での不変、primaryのみ/全所有再生の停止、境界待ちへの即時停止割込みを確認した。小節境界停止は要求5038 clocks、期待7799に対し7804で終了し、呼出しは約0.043msで戻った。[40秒WASAPI](../../work/acceptance/transport-position/20261010T083912042Z/transport-position-audio-proof.json)は開始位置のG4、先頭のC4、約1.8秒の境界待ち、停止後無音と再開を独立RIFF/clock/PCMで確認した。

本体ではカーソル6336 clocks、現在位置/先頭の各ボタン、2つの停止表示、境界待ち→終了、Space/Alt+>/Alt+<を観測した。[主画面180秒収録](../../work/analysis/ui-transport-position/20261010T082910362Z/main-audio-v2/transport-position-gui-audio-proof.json)は現在位置G4と先頭C4を区別し、発音中の即時停止・境界停止後の無音を合格とした。QPC/UTC driftは約5μs、最大packet gapは2 frames。WAVの音だけを0にした対照はexit1/不合格となった。Ctrl指定の主画面操作は未実測であり、専用runtimeの所有権検証と区別する。物理スピーカー出力や原版同入力比較の証明ではない。

参照を整えた検証用Project/Segmentを本体から保存し、別processで170ノート/120BPMを復元した。Segment/Styleはbyte一致、Project差分は既存filhの更新日時/ファイルサイズ欄だけ。再読込時のカーソルは0、6336への変更でUndo/Redoは増えず、文書も変化しない。作者/復元の通常exit0に加え、失敗入力を使った最初の本体も通常exit0。ユーザーが起動した正式原版PID25548・開始時刻03:11:40.455588Zは保持し、今回の製品/録音プロセスは終了した。

## 失敗と適用範囲

最初の100秒GUI収録では先頭再生が末尾、即時停止が収録外だったため不合格として保持した。新たに180秒protocolを指定して収録し直した。解析器作成時の改行構文エラーも修正後に実行した。

最初の検証入力では、新しく生成したSegment GUIDと既存JAZP参照GUIDが不一致で、Save Projectが識別子保護検査により拒否された。3入力のbytesは不変で、製品の検査は緩めていない。失敗入力/画面を保持し、別の検証用コピーの最上位GUIDだけを参照と整合させて保存・復元をやり直した。GUI音声は前者の入力、成功した保存/再読込は後者の入力としてhashを分ける。音楽データは同じだが、保存成功を前者へ転用しない。

UIAの値が直前状態のまま残ることや一部indexが負の座標へ解決されることがあった。画面を再取得し観測した座標で回復した。これらをOS拒否や製品の音声失敗へ分類していない。

## 残差と次の一手

正式原版/CHMは仕様根拠と静止参照であり、同じ入力の動的比較は未実行。現在の2つのPlay/Stop glyphはそれぞれ同形で、原版の見分け・全配置・字体・ドッキング・保持は未完。境界終了の状態文言も汎用segment-endedのまま。MIDI録音、Transport List/Transition、編集可能Current Tempo、Transport Options/MIDI-Performance Options、他境界/Motif統合を残す。

SSP01: Transport Options / MIDI-Performance Optionsの画像3/4・CHM・既存runtime設定を照合し、実設定へつながる一経路を実装する。Transport List/Transition/MIDI録音/編集BPM/外観と原版同入力比較は未完として保持し、SSP02へ接続する。

現登録native96/driver169、083342の全登録一巡は未実行。独立Style015244の固定300秒PCM/96-164一巡、全40/全8の6作業中・2障害・0合格、凍結条件とQ2未用意を保持する。機能部分と保存/復元/PCMは限定成立、originalComparison=falseのためboundedFinished/completed/fullAcceptance=false。

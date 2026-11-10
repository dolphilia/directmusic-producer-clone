# MIDI Inputの選択と設定保存の限定統合

2026-10-11。ユーザー指定画像 **(3) Transport Options、(4) MIDI/Performance Options** の入力欄を対象とした。054337 MIDI Inputを固定無効表示から実WinMM列挙/None選択・所有受信キューへ接続。286ソース/4出力一致、関連6/300、native100=53合格2965検査/46OS凍結/既知Band1保留、driver171=20合格/28OS凍結/123未実行。専用MIDI8条件と既存音声Options回帰、Main Cancel保持/None保存・別復元/二通常exit0、独立監査24条件成立。列挙機器0件のため実受信/Echo/録音は未確認。Segment/DLS不変、Project既存FILETIMEのみ更新。原版はNone表示/Open/Cancelの部分比較、厳密外観/実機同条件比較は未達。全体8受入は6作業中/2障害/0合格、fullAcceptance=false。

## 変更と根拠

固定された無効なNone欄を、実際のWinMM入力機器とNoneを選ぶ欄へ変更した。受信handleを所有し、短いMIDIメッセージと入力開始からの時刻を有界キューに保持する。callback内では音源・COM・文書・UIへ触れず、本体timerへ渡す。キュー欠落とdriver異常を成功扱いにしない。終了に失敗したdriverのcallback記憶域を解放して使用後解放を起こさないが、そのケースの正常終了は主張しない。

保存は一時的な機器番号ではなくメーカー/製品/driver版/名称による識別を独立sidecarへ記録する。同一識別の重複、未接続、Open/Start失敗を拒否し、既存の所有入力を保持する。Cancelでは変更せず、OKの後段が失敗した場合の復元処理を用意した。原版と同一の保存形式や入力APIとは主張しない。Echo/PChannel転送、SysEx、Step Time/Realtime録音は未実装のまま。

原版CHMの [MIDI Options](../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/midiperformanceoptionsdialogbox.htm) と [MIDI capture](../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/capturingmididata.htm) を契約にした。受信APIはMicrosoftの [midiInOpen](https://learn.microsoft.com/en-us/previous-versions/dd798458(v=vs.85))、[MIM_DATA](https://learn.microsoft.com/en-us/windows/win32/multimedia/mim-data)、[callback制約](https://learn.microsoft.com/en-us/previous-versions/dd798460(v=vs.85))、[midiInClose](https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-midiinclose)、[機器情報](https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-midiingetdevcapsw) に基づく。

## 検証と限界

- 候補`20261011T054337555Z`はconfigure/build/install各0、286保存/作業ソース・4生成物一致。[build](../../work/build/product-snapshot/20261011T054337555Z/build-summary.json)、[独立監査](../../work/analysis/ui-midi-input/20261011T053540713Z/artifact-audit-v1.json)。
- 原始native100は53合格2965検査/46OS凍結/既知匿名Band1保留。原始driver171は20合格/28OS凍結/123未実行。専用MIDIとPerformance Optionsの明示実行は原始一巡と分け、未実行を合格へ混入しない。
- 専用MIDIは8条件成立、実機列挙0。未接続の設定fixtureは設定復元用であり、実MIDI機器や実受信の模擬ではない。実機のOpen/受信/欠落/切断/音声/Echo/録音は未確認。
- 本体PID10252でNone選択→Cancel後のsidecar SHA不変、再表示で未接続fixture復元を確認。None→OK後はMINP v1の28 bytes、再表示と別process PID2008でNoneを復元。既存音声設定fileは生成されず、音声Options回帰は別の専用processで成立。二本体processは通常exit0。
- Segment/DLS bytesは不変。Project512 bytesの既存Segment filh FILETIME欄だけ更新し、実ファイルのmtime/1702 bytesと独立RIFF読取で一致。厳しい全3ファイルbyte不変は不成立であり、音楽payload保持と区別する。traceは20/14 events、overflow=false。
- 手動起動済み正式原版でInput None、64 voices、22kHz、55msと欄の配置を観測しCancelした。原版EXE/検証用入力は不変。原版ではEcho/AdvancedとDownload Statusが有効で、再構成版はまだ無効。字体/余白/高さ・アイコンも残差。実機同条件比較と厳密pixel一致は未達。
- 所有modalのUIA cached index不在とgeometry不在を現行window/画像で復旧した。これをOS拒否や製品不具合の証拠にしない。

入力Noneで本単位の出力変更がないため新PCMは非適用。既存Count-In031359のPCM15/16総合不合格、失敗録音と閾値は保持する。限定採用0735、独立Style015244、全40/Q2/個別Windows5の凍結条件も変更しない。

## 次の作業

次はSSP01/UI13のEcho MIDI Input・16 PChannel出力範囲の契約と現行Conductorの即時MIDI経路を照合し、独立した入力所有権/ルーティング/Cancel・失敗時復元の開始契約→実装→新候補build/関連/本体設定・保存復元へ進む。実入力機器0件を模擬実機成功にせず、実受信/発音比較/録音/Advanced残差を保持する。Count-In031359の失敗PCM、Viewport045647/その他SSP/Style015244/Q2/OS凍結を保持。

[開始契約](../../work/analysis/ui-midi-input/20261011T053540713Z/start-v1.json)・[単位記録](../../work/analysis/ui-midi-input/20261011T053540713Z/unit-record-v1.json)に候補、hash、終了条件と未達条件を保存した。

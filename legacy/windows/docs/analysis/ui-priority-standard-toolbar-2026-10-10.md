# SSP01 Standard Toolbarの基本統合

2026-10-10（日本時間）。画像1のStandard Toolbarについて、新規・Open Project・Insert Files・Save Project・Save File・Project Tree・水平/垂直整列の8ボタンを既存コマンドへ接続した。ViewからStandardを表示/非表示にできる。SSP01全体とStandardの外観再現は未完、全体fullAcceptance=false。

## 実装と修正

[src/producer/frame_toolbar.cpp](../../src/producer/frame_toolbar.cpp) とヘッダーを追加し、main、Project Tree、MDI workspace、CMakeへ接続。OS共通コントロールのstock bitmapとソースで描くglyphを使用し、原版PEからUI資産を取得しない。Save Fileは文書状態、Treeは表示状態、整列は子ウィンドウの有無に連動する。

本体検証で見つかったcaptionの表示残り、子ウィンドウなしでも整列が有効になる問題、Standard再表示後のTree描画欠けを修正した。caption抑制ではTBSTYLE_LISTとmixed buttonsの組合せ、および最大テキスト行0を設定した。[Microsoftのtoolbar仕様](https://learn.microsoft.com/en-us/windows/win32/controls/toolbar-extended-styles)を参照した。Treeの修正は子コントロールを含めた再描画。

Newは既存5種の一時popupである。画像5のCreate New File(s)と3つの入口の統一はSSP02へ残す。

## 保存候補と検証

最終試験候補は **20261010T064441833Z**、Win32/Release、宣言SDK10.0.26100.0。configure/build/installは各exit0。保存ソース242件、作業ソース242件、出力4件が検証後も一致した。

- [build-summary](../../work/build/product-snapshot/20261010T064441833Z/build-summary.json)：SHA256 7a541cbfabb0a91428e09bf4fae2e70601f81f46b2869d025afd0bd5efff840c
- Producer.exe：SHA256 ed87e4dedc95631c108f19ba5c36ebc3f907275836518f3c4b087676ce56a4f9
- [単位記録](../../work/analysis/ui-priority-toolbars/20261010T055934298Z/unit-record-v1.json)：SHA256 3174aa775780a141123f37e9513f9323d523abb6a9198252dd8f82a3a6395310
- [関連回帰](../../work/acceptance/regression/20261010T065113524Z/run.json)：timeline-range117、sequence-crud26、component-name70 check、3件合格/exit0。登録群の原始一巡ではない。

同じ候補を4K/200%で操作し、Viewチェックと表示切替、30論理pixelの配置変更、TreeボタンとShift+F11、Segmentを開く/閉じる際の整列状態、水平/垂直整列、Open/InsertのfilterとCancel、New popupとEscapeを確認した。整列は1子ウィンドウで確認し、複数子の原版比較は残る。

検証用コピーをSave File/Save Projectで保存し、作者PID24604は通常exit0。[別processの再読込](../../work/acceptance/product-project-gui/20261010T070151377Z/launch.json)はPID3292、別開始時刻、同EXE hash。Project Tree/120 BPM/Segment表示を確認して通常exit0。PIDは履歴であり、次回の操作対象へ固定しない。

[独立RIFF差分](../../work/analysis/ui-priority-toolbars/20261010T055934298Z/project-save-diff-v4.json)ではSegment/Styleのbytes一致、Project492bytes/12leaf維持。変化はleaf9のfile header内FILETIMEだけで、保存したSegmentの更新日時に対応した。ユーザーの入力原本には触れていない。

![再表示後のStandardとProject Tree](../../work/analysis/ui-priority-toolbars/20261010T055934298Z/product-v4-toolbar-restored.jpg)

## 未達と次の作業

原版のViewメニュー、MIDI/Performance Optionsの表示とCancelを限定観測した。画像1とのglyph/幅/間隔/ドッキング/表示設定保持や、全操作の同条件動的比較は未達。Transport Controls/Options/MIDI設定は未実装であり、既存Play/Stopを完成したTransportと扱わない。

[Transport契約](../../work/analysis/ui-priority-toolbars/20261010T055934298Z/transport-contract-v1.json)は原版CHM14話題へhashで結ぶ。録音はMIDI入力、試聴倍率は文書Tempoを書き換えない、再生中のPlay2ボタンはStop2種へ変わる等を区別した。次は既存Conductorを活用してSSP01の残りへ進む。seek/master tempo/MIDI入力/performance設定の不足を具体化し、必要WASAPI PCMと原版比較を同候補へ結ぶ。

今回の変更はファイル/表示/ウィンドウコマンドに限り、音声engine・開始/停止・録音・performance設定を変更していないため、この限定Standard統合に音声録音は非適用。Transport段階の音声受入は必要であり、全体音声の合格へ転用しない。

中間候補060257はsource不一致の部分build、060752/062023/063023はそれぞれcaption/状態/再描画修正前として証拠を保存。各候補の限定GUI・関連回帰を最終EXEの実行結果へ転用していない。V3のmodule inventoryはdmime.dll読取不足を残し、原版依存解消の証明にはしない。

限定採用0735とそのlatestUnit/latestReport、015244 Styleの固定PCM/同候補一巡、凍結37/個別拒否、Q2未用意、全40責務、全8の6作業中/2障害/0合格を維持する。

## 再現

PowerShell7、repoをcwdにする。既存保存候補を使う際もsource/output/inputを再照合する。

~~~powershell
.\scripts\Test-ProductProjectGui.ps1 -BuildSummaryPath '.\work\build\product-snapshot\20261010T064441833Z\build-summary.json' -Project '.\work\analysis\ui-priority-toolbars\20261010T055934298Z\gui-input\AudioChord\AudioChord.pro' -InputPaths @('.\work\analysis\ui-priority-toolbars\20261010T055934298Z\gui-input\AudioChord\AudioChord.sgp','.\work\analysis\ui-priority-toolbars\20261010T055934298Z\gui-input\AudioChord\Pulse.stp')
~~~

返された新window/EXE/PID/開始時刻へComputer Useを結び、通常終了を記録する。work/とscreenshot/はGit対象外なので、別checkout/ホストへ移送する場合は別途保全する。

# 共通 Edit clipboard と Tempo 複数選択（2026-10-09）

0019候補では Ctrl+A、複数選択の削除、Undo/Redo、メニューからの全選択と削除、既存パスへの保存、別プロセスでの復元が限定成立した。Ctrl+C は `SetClipboardData` で `Clipboard data could not be copied (Windows error 5)` となった。単位は未完、`fullAcceptance=false`。限定採用0735と全40責務・全8受入を維持する。

契約と全文証拠は [単位記録](../../work/analysis/ui-edit-clipboard/20261008T235623019Z/unit-record.json)、[開始契約](../../work/analysis/ui-edit-clipboard/20261008T235623019Z/unit-start.json)、[操作手順](../../work/analysis/ui-edit-clipboard/20261008T235623019Z/gui-protocol-v1.json)、[診断手順](../../work/analysis/ui-edit-clipboard/20261008T235623019Z/diagnostic-protocol-v1.json)から辿れる。原版helpに基づく Cut/Copy/Paste/Delete/Select All とキーの期待は実際の原版操作観測とは分ける。200%表示設定を変更していない。

## 変更と生成物

Segment用の複数選択リストと検証済みindex群を受け取るcore選択APIを追加し、全選択対象へDelete/Cut/Copyを接続。Cutはコピー成功後だけ削除する。Editメニューと Ctrl+X/C/V/A・Del を編集中の対象へ接続し、文字入力・Tree・文書切替・所有ダイアログへのclipboard/deleteキーの横取りを除外する。Styleの既存singlePattern Copy/Pasteは保持し、Bandや未対応Style操作は無効。Clipboard形式とcommand有効状態を更新する。

失敗candidate2358のUINT/enum混在、0002のtestnamespace誤りを保存して修正。0006はbuild/install・関連回帰とShift/Ctrl+Aの本体選択が成立したが、初回Copy失敗。18入力不変のまま二プロセスを通常終了した。診断のため EmptyClipboard/SetClipboardData のエラー観測だけを分離し、0019を新規buildした。旧成功を新生成物へ転用していない。

[0019 build](../../work/build/product-snapshot/20261009T001952298Z/build-summary.json)はSDK10.0.26100.0固定、configure/build/install各0。229保存/作業ソース、4生成物が一致。Producer SHA256 `7e6960400ab17e4b49afd739809eaa5ee671101ff5b2d82f74554ee634b1cebb`。関連13は [新規run](../../work/acceptance/regression/20261009T002525386Z/run.json)で12合格・jazp-save1凍結。timeline-range107、Sequence26、component-name44を含む。原始2137のnative92/driver164と現在候補の限定補足を分けて保持。

## 本体と保存の成立範囲

[観測](../../work/analysis/ui-edit-clipboard/20261008T235623019Z/observations.json)23〜72は0019の実画面/操作。EmptyのEdit7無効、Source全選択、Deleteで二件一括削除、Ctrl+Z/Y、別StyleをTreeで選択したままactiveSourceへのメニュー全選択/Delete/Undoを確認。最後は3072clocks/7.5BPMだけを削除し、0clocks/9BPMと8ノート・名前を保持して Save SourceSequence と Save Project を実行した。

別起動でSource9/ノート8/名前、Style117/Pattern1、Bandpatch1/PChannel0/pan64/volume101を確認。StylePattern選択でCopyだけが有効、Band楽器選択後はEdit7無効。Empty PID16928、author PID10132、reload PID24780は同じEXE/hash・異なる開始時刻で全て通常exit0。終了後のwindow capture不可は終了判定に使わず、独立した読み取り監視とlauncherの結果を結んだ。強制終了なし。

[固定した独立native監査](../../work/analysis/ui-edit-clipboard/20261008T235623019Z/audit-native-v1.mjs)は [6チェック合格](../../work/analysis/ui-edit-clipboard/20261008T235623019Z/native-proof.json)。18baselineのうち変更はSourceSequence.sgpとFresh.proだけ。第二Tempoの16byte record除去以外はSequence/名前/未知chunk/paddingがexact。ProjectはそのSourceのfilh保存metadataだけが変化し、他文書・参照・identityはexact。native6は実Clipboard/Cut/Paste・音声・原版比較の合格を意味しない。

## 未達と解除条件

[一度の診断](../../work/analysis/ui-edit-clipboard/20261008T235623019Z/clipboard-setdata-refusal-v1.json)でOpenClipboardとEmptyClipboardは成功し、SetClipboardDataがWindows5を返した。下位原因は未確定。同条件Copy/Cut/Paste、同APIのStyleCopy、準備済みbusy fixtureは凍結。Cut失敗時の本体保全、成功copy/pasteのpayload、local text/modal/modeless isolationは未検証で残す。設定/COM/別UI手段を変更して拒否を迂回していない。アクセス条件変更の一次証拠、または契約根拠のある具体的実装不整合が解除条件。

0006のCtrl+Spaceは期待した個別toggleと異なる表示になったため未確認として保持。Ctrl/Shift mouseclickは公開Computer Use APIでmodifier保持ができず未観測。右click context、他strip/OLE/drag、Style複数Pattern、Toolbar/MDI/他editorもqueueに保持。required現候補WASAPIは既存filename cache障害、原版はapprovaltimeout障害で未実行/比較未確認。旧音声・原版結果を転用しない。

次は既存 `style-player-bands` 不合格（49チェック後 Later anonymous Band source cannot be resolved）の契約・ソースを照合し、複数Band責務を扱う。共通UIの同じ障害の細分化だけを続けない。

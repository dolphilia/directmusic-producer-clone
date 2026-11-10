# Frame menuとkeyboard経路 — 2026-11-09

100901はFile/Edit/View/Add-Insの先頭順序とView内AudioPathを実現し、Alt/F10をnative frameへ渡す修正を検証。関連4件238検査、空16236/作者11488/復元20764の三通常exit0。AltF→N/NewCancel、F10→Down、AltV→A→Enter、マウスNewCancel、MDI/Edit focusからのFile保存が成立。既存2再生キーは無ノートSegmentでLoad/Play/Stop各2回成功・停止後IsPlaying false。native初期/作者/復元のSegment全bytesと3Project FILETIME以外、292ソース/4出力一致。新PCMなし、音声受入への転用なし。

091437の変更前baseline11184はAltFでFile popupが開かず、先頭Transport AudioPathも原版の順序と異なった。095232は先頭配置/明示&File/動的submenuを直したがAltFは未達。関連4/238・空2612通常exit0と失敗画面をunit-record-v2へ保全し、native作者/復元は未実行。100901は既存transport key処理の後、frame/childのWM_SYSKEY群と無修飾F10をTranslateMessage/Dispatchへ渡してIsDialogMessageより先に処理する。独立Properties窓の翻訳条件は変えていない。

既存AltShift comma/periodのbodyは095232と同一。syntax-v2の一時的な優先順は実行前レビューで修正しsyntax-v3後に100901をbuild。無ノートの本体操作で2つの再生キーがロード/Play/Stopへ届くことを確認した。PCM・音高・active-note Stopの新しい音声合格ではない。開始位置はUI/sourceの要求とSet/Get成功による検証であり、trace.musicTime0を開始位置実測値と誤読しない。

正式原版15872は手動OK後に本体へ進み、専用TimeSig-onlyのopen/close/reopenを観測して通常exit0。出力は入力に使わない固定フォルダーへ保存。原版の先頭4メニューを比較したが、AltF/F10の新しい原版同時操作や全menu/Help/Recording/Pattern/font/docking一致とはしない。原版のprimary-NoTempo空欄は候補の120表示と異なり、次の単位で修正する。

一次入口：`work/analysis/ui-frame-menu/20261109T095015925Z/unit-record-v4.json`、`gui-proof-v1.json`、`native-proof-v1.json`、`keyboard-runtime-proof-v1.json`。

SSP01 CurrentTempo表示を継続。原版15872の固定比較ではprimaryなしはdisabled120、TimeSig-only primaryありはdisabled空欄、最後の編集窓Close後もprimary/Play/空欄保持。091437の全noTempo120fallbackを、primaryなしだけ120へ狭める独立修正へ進み、numeric BPMとの切替/保存/通常終了/別復元を検証する。初期primary登録/最後Closeのgray/TimeSig-only再Open promptは別残件。全40/全8/fullAppearance/fullAcceptance=false、49native/30driver凍結と匿名Band同条件再試行なし。原版15872は通常exit0、通知手動対応待ちは解消済み。 goal active。

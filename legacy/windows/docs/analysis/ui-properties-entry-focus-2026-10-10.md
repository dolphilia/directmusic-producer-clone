# Lyric / Marker 共通 Properties の入口と選択追従

2026-10-10（日本時間）。候補1915（`20261009T191511828Z`）の限定検証。原版比較と子controlへの直接F11送達は未確認であり、単位完成・全体受入には昇格しない。

## 製品の修正と成立範囲

候補1825ではmain-windowのF11条件がTempo contextだけを参照していた。LyricまたはMarkerを選択してmainへ戻った際に、既存の共有Propertiesを呼び出せない不足を確認した。`src/producer/main.cpp` の条件にLyric/Marker contextを含めた。既存のeditor側F11/context handler、保存・再生処理は変更していない。

1915は236保存/作業ソースと4出力が一致し、configure/build/installがexit0。関連8群（common-open、timeline-range、lyric-runtime、lyric-document、marker-document、sequence-crud、component-name、tempo-properties）が合格。本体の操作19項目、native監査6項目が合格し、画像を直接確認した5項目を別記録にした。

- Lyric/Markerのchild欄をクリックした後、rootをtargetとするF11で正しい共有Propertiesを開いた。未選択main F11はevent Propertiesを作らない。
- 両editorの選択欄を右クリックしてnative popupのPropertiesを選び、対応する値を表示した。旧Lyric contextのmain activationによるdismissalだけを根拠に未実装とした判断は、現在候補の実操作で更新した。
- Lyricの先頭文字列を「Earlier Lyric」から「Focused Lyric」へ変更し、Tabでcommit。Undoは元文字列とSavedへ戻し、Redo後は変更文字列を保持。第二Lyricを選ぶと「Lyric UI Ω」、Start 2/2/24、Belongs 4/1へ追従した。
- MarkerのEnter SwitchPointをTick13へ一時変更し、6144→6157→Undo6144→Redo6157→Undo6144を確認した。最後のRedo/UndoはUIAが空であり、画像の確認を自動UIA判定とは区別した。後続のcontext値・保存bytes・再読込でも6144を確認した。
- 両editorを開いたまま選択を変えると共有Propertiesが切り替わる。focusを移しただけではMarker表示が残り、Lyricを選択して切り替わった。selection通知とfocusのみを区別して記録した。

## 保存・別起動

検証専用のWaves三ファイルを使用した。変更後Song.sgpのSHA256は `fdf832fe9dd73738d2f202928f361de71886a0790b1346743f761b90b22f5a52`。事前に独立parserで作成した期待ファイルと全文bytesが一致し、先頭Lyricの26-byte UTF16置換だけが差分。第二Lyric、Marker、Tempo、meter、8notesはすべて不変。Effect.aupは全文不変。ProjectはSongのfile write journalだけが変更され、参照・他entryは不変だった。

author PID11248と別起動reload PID19652は、同じ1915 EXE（SHA256 `81742a4bb41a970422160490f4d0d0924fa5f5938980366b79e3df1e0ecb75d1`）で通常exit0。reloadでは両Lyric・両Markerの値、Saved、履歴空を確認した。最終三入力は保存コピーと再読込コピーが一致する。

reloadの最初のAltF4はPropertiesだけを閉じ、mainは継続した。その時点で走らせたnative監査v1は終了条件で不合格。主画面close後のexit0を確認し、同じ6assertionのv2で合格した。失敗v1・訂正記録は保持する。記録helperの誤ったactions filenameによるENOENT、set_valueのcache index unavailable、空/重複UIAも製品/OS拒否と断定しない。

## 未達と次の一手

Computer Useはtargetのmain rootを自動activateするため、今回のF11成功はroot fallbackの実証であり、childへ直接keyboard messageが届く経路の実証ではない。原版のapp approval timeoutも条件不変で凍結し、配置・字体・アイコン・メニュー・手順の動的比較は未完。OS4K/200%を変更していない。録音は入口・選択・文字列保存だけの責務に非適用とし、旧再生成功を新候補へ転用していない。

次は共通Timelineの原版基準・既実装・残差を照合し、複数stripの選択と共有Propertiesを結ぶ不足を減らす。全40画面/操作inventory、Q2、同じ最終構成の全8は未完。0735限定採用/Q1 latestUnit/latestReport、2137原始一巡、native94/driver164、Wave/Send/Farm等の未達を維持する。

一次証拠の入口は [単位記録](../../work/analysis/ui-properties-entry-focus/20261009T190827635Z/unit-record-v1.json)、[GUI監査](../../work/analysis/ui-properties-entry-focus/20261009T190827635Z/gui-proof-v1.json)、[native監査](../../work/analysis/ui-properties-entry-focus/20261009T190827635Z/native-proof-v2.json)、[画像確認](../../work/analysis/ui-properties-entry-focus/20261009T190827635Z/visual-review-v1.json)、[関連回帰](../../work/analysis/ui-properties-entry-focus/20261009T190827635Z/related-regression-v2.json)。

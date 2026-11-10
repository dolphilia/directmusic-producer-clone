# Current Tempo再開とツールバー入力対象の修正 — 2026-11-09

全体未完、`fullAcceptance=false`。004853候補の本体未検証から再開し、専用Projectで120→132.25をEnter確定・保存、文書Undo120・Redo132.25、Project保存、PID11092の通常終了exit0を確認した。独立RIFF監査では4保存状態の先頭テンポ1件と17ノートのSequence payload保持が一致した。最初のUndo後Ctrl+Sは保存bytesを変更しなかったため、その失敗snapshotも保持し、後続のStandard Save Fileの結果と区別する。

再開照合は004853保存/作業291ソース・4生成物・baseline/candidate入力4件・参照記録11件が全一致。主画面は1280×800、96 DPI（100%）。古い原版PID25548と旧未保存候補6640は現存しないが、終了結果や未保存内容の回収を確認した意味にはしない。今回のアプリ起動は最初にwindow未取得を返し、再列挙で実windowを得た。途中の手動入力検出・一時停止と再開を保持する。

Current Tempo編集欄とspin、AudioPath comboはフレーム直下のtoolbarと同階層にあり、toolbarより背後へ配置されていた。現行本体のWin32読み取り観測で各欄の中心座標のhit対象がtoolbarであることを確認した。132.25でspin上をクリックしても変化しない。キーボード到達の成功をマウス操作の成功にはしない。

`src/producer/frame_toolbar.cpp`でlayout/refresh時に埋込欄を`HWND_TOP`へ置き、親フレームへの通知経路を保つ。保存候補20261109T041303743Zはconfigure/build/install0、関連5件496検査合格。本体でマウスfocus→132.25入力→Enter→Ctrl+Sが成立した。その後、spin上が132.25→131→130、下が130→131となる方向逆転を実観測した。元の`UDN_DELTAPOS`の差分を減算していたため加算へ修正。041303の失敗画像と保存131を保持し、新候補20261109T042716200Zをビルド中。新候補の回帰・本体全操作・保存復元・PCMへ旧候補の結果を転用しない。

正式原版は5.3.0.900、EXE hash fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011でPID20376として起動した。専用Projectは開けたがSeed.sgpのダブルクリックはWindowsのアプリ選択へ進み、Segment editorとTransportを得られなかった。関連付けは変更せずEscapeで閉じた。32bit COM43件が削除済みの2026-10-02 visualization配下を指していた。ユーザーの明示承認「登録先を復旧する」を得て変更前全値を保存し、43件の既存default値だけを正式インストール先へ復旧。全43件のpath/hash・他値不変を照合した。インストール直下PEは変更していない。原版20376は通常exit0。再起動16440ではConductor/Segment/Style等11の正式インストールmoduleがロードされたが、mainWindow=0でtargetable windowを得られていない。プロセスを重複起動・強制終了せず、読み取り待機連鎖を保存した。Escape/focus-loss/不正値など追加原版契約は未確認、従来の132.25/UndoRedo/spin証拠は当時の環境限定として保持する。

その後、同じ原版PID16440の起動が完了し、専用SeedのSegment/Transportを操作できた。重複起動はしていない。Escapeは未確定132.25を残し、フォーカス離脱で120→132.25を確定した。数値0は1、1000.01は1000に正規化し、空欄Enterは直前1000へ戻す。上spinは132.25→133、下は132.25→131。6保存状態のテンポ1件と17ノート/seqt不変を独立監査し、最終132.25で保存・通常exit0を確認した。旧説明の「原版window未取得」は起動途中の観測として保持する。

Escapeと数値UI正規化を一次証拠に合わせて修正し、新候補044017をビルド中。文書APIの範囲外/NaN等拒否は変更しない。計画の不正入力時不変は空欄/不正構文と直接APIに適用し、原版で有効値へ正規化される数値UI入力は別の契約として明記した。042716の関連5件496検査は合格だが、新しいEscape/正規化修正の実行証拠にはしない。Style/再生中/負数/非数値/3桁小数の追加細部は別queueへ保持し、この単位を無制限に拡大しない。

044017は291保存/作業ソース・4出力一致、関連5件496検査合格。本体で132.25入力/Enter/Ctrl+S、文書Undo120/Redo132.25、上133/下131と各Undo、Escape保留134.5→focus-loss確定、0→1/1000.01→1000、空欄で132.25保持を確認した。各native保存状態と画像を保全。欄・spin・AudioPathのhit対象も各control自身に改善した。修正後hit診断の初回は自動承認レビューの期限切れで未実行、ツールが許可した一度の再試行で読み取りが成功した。

Peer.sgpをInsert Filesで追加したところ、active文書/イベントはPeer120なのに、フォーカスが戻ったTempo欄にはSeed132.25が残る不具合を確認した。数値欄から離れるとPeer120へ更新され、Peerの保存bytesは保全した。二文書Project保存と044017作者17720の通常exit0を確認。この不具合を残したまま別復元/PCMを合格にはしていない。フレームは表示対象の文書identity/index/cursorを追跡し、対象変更時には旧入力bindingを外して新しい値を表示し、欄にfocusがある場合は新文書へbindingし直す。新候補050149をビルド中。これは本体で実観測した不足の修正であり、独立した細部を単位へ無制限に追加したものではない。

一次入口：`work/analysis/ui-current-tempo-resume/20261109T035725Z/`の`unit-record-v2.json`、`resume-identity-v1.json`、`actions.jsonl`、`control-stacking-final-v2.json`、`owner-display-defect-v1.json`、`old-candidate-native-v1.json`、`author-exit-v1.json`、`original-native-proof-v1.json`、`original-restored-exit-v1.json`、`contract-amendment-v1.json`、`approved-com-backup-v1.json`、`com-restoration-verification-v1.json`。次は050149のbuild/関連回帰→専用owner-fixed入力で全Current Tempo操作とPeer追加後の表示/所有権→保存/別復元/二通常終了→132.25/120の固定WASAPI。全40責務・全8受入、厳密外観、Q2、既知49native/30driver凍結等を維持する。

## 050149で確認した結果

050149同一候補で停止中SegmentのCurrent Tempo入力/Enter/文書UndoRedo/spin/原版Escape保留とfocus-loss確定/1..1000正規化/空欄/Peer追加直後の表示と編集所有権を確認。関連5件496検査、native14状態+Peer5状態、作者14160/別12000の二通常exit0、別起動保存復元成立。132.25/120 BPMの実PCMは各17発音・音程・発音中Stop・無音・replay成立、間隔中央値0.455/0.500秒、負例5件拒否。全登録native100は50合格2830検査/49既知OS凍結/匿名Band1保留、新実行失敗なし。driver172は一覧参照のみで新一巡ではない。

保存/作業291ソースと4出力一致。Project再保存はfile entryのFILETIME以外が一致し、Seed/PeerはBPM以外の全bytesと17ノートを保持した。音声対照後のSeed132.25も作者保存と全bytes一致。原版6状態の独立監査・追加操作比較・通常exit0は別証拠として保持する。

PCM監査v2は減衰中の約0.003 RMSの揺れを誤発音として数え、不合格を保存。発音検出の上昇閾値を0.01へ厳しくし、全cycle発音数を数えるv3/v4へ改訂した。テンポ/音程/無音/停止等の受入閾値は変更していない。未測定120対照、誤テンポ/誤音程/再生欠落/一音欠落/無音区間への雑音の負例5件で検証。120初回は2回目停止36秒で事前25秒上限を超えたため不合格のまま保持。入力終了から14秒へ待機を固定した取り直しで合格した。録音完了前に監査を呼んだ2件のENOENTは未実行扱いで、完了後に新しいproofを作成した。集計初回のtrkh tag位置誤認も、RIFF監査を変えずBPM値位置と全bytes照合で修正した。

現行入口は `work/analysis/ui-current-tempo-resume/20261109T035725Z/unit-record-v3.json`、`owner-fixed-native-proof-v2.json`、`audio-summary-v1.json`、`native-milestone-summary-v1.json`。機能の限定確認成立と、Transport全体/厳密外観の未達を区別する。

SSP01を継続。現環境の原版/CHMからTransport Listの対象と停止中/編集対象切替/再生の契約を限定観測し、同じ選択・配置・Current Tempo連動を実装→保存build/関連回帰→本体/保存/二通常終了/別復元/必要PCMへ進む。Style/live/secondary・録音/Transition・厳密外観・全40/全8/Q2・既知OS凍結を保持。 全体fullAcceptance=false、goal active。

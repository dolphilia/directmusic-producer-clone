

## 2026-10-04 一次/secondary個別所有・同時再生・個別Stop

全体未完了。前回GUI163814810Zの成果を保持し、新製品work/build/product-snapshot/20261003T170530848Zへ成功を転用しない。Conductorの共通Performance/COM/Graphと、再生ごとのLoader/SegmentState/Style/DLS backing bytes/ダウンロードを分離した。再生IDを単調増加しposition(id)/stop(id)/playback_idsを追加、既存Stopは全個別再生を止める。secondary開始は既存一次をStopせず保持する。停止対象は具体的なSegmentStateで、Stop後のIsPlaying確認・Unload・依存物解放をそのインスタンスに限定する。所有文書の不正Motif選択は既存再生を変更する前に拒否する。

通知はIUnknown canonical identityを現在/保持中/停止済み識別子と比較してplaybackIdへ帰属。停止後に届く通知にも対応するため、停止済みIUnknownのみをshutdownまで保持、Loader/bytes/downloadは停止時解放。通知identityの長期回収・長時間多数回再生の負荷検証は未完了。新一次再生は旧一次を開始前にStopする既存置換方針を保持しており、将来時刻の一次置換で旧一次をその境界まで継続する仕様は未実装。新再生失敗時はそのインスタンスを整理して保持中の他再生へ戻すが、旧一次置換済み状態の復元を保証するものではない。

本体PatternへStop Most Recent Playbackを追加、従来Stopボタンは全停止。最新Motif自然終了時はそのIDだけをStopし他所有があればtimerを継続する。この新GUIメニュー/保持再生へのtimer引継ぎは実操作未検証。一次/secondaryを任意に選ぶ再生一覧UIも未実装。

原版既存Motif設定観測162000Zは保持。今回の同時再生動的原版比較は未実行。仕様の一次資料はMicrosoft Learn https://learn.microsoft.com/nb-no/previous-versions/ms809719(v=msdn.10) のprimary置換とSegmentState返却、保存SDK dmusici.hのSECONDARY。原版と完全同等とは判定しない。

初版work/build/product-snapshot/20261003T170200380Z構成/compile/install0、EXE a478fd99ff46fd99efe6ae5172f9dd7d5f2b536e9e88464ae4d791eb8d487566、work/acceptance/motif-concurrent/20261003T170336505Z同時再生/個別Stop内部チェックexit0/noteCount11。停止後通知option4がID0だったため修正。初版にC4457/C4459/C4456 shadow warnings3件、修正版では解消。初版schedule/hostも各成功だが修正版へ転用しない。

現行work/build/product-snapshot/20261003T170530848Z保存59sources・構成0/compile0/install0、EXE 1da22f95a19d771d8bc4c6f8de375db34081379fb6e9bad0dcf26869423311b2、core cb7af287effc857b5852aba96014324d2b5fdae264d5dea9fb75526a3dc3fa56。build.log warnings0。core suiteは未実行。work/acceptance/motif-concurrent/20261003T170726655Z新規PID18284/exit0。入力Style35caea149f46d8971be73328130ec2161b2cda22a8a45ca10683223fbac72de7のprimary用コピーのみmtfs.repeats1→8、secondary全bytes一致を独立raw監査。Style/DLS所有のFramework解決を使用し、元保存ファイルを変更しない。今回は並列observerの実runtime DLS bytesを出力しておらず、完全bytes比較は未実行。

再生ID1/2の両IsPlaying true、primaryStart1628/secondaryStart3041、無効Motif要求後も両true。secondary2を個別Stopして100ms後もprimary1 true/所有1件。secondary3再開時も両true、primary1を個別Stopして100ms後もsecondary3 true/所有1件、最後に3をStopして所有0件。取得12音/overflowなし/forwarding failureなし。生成音のインスタンス別帰属/実音声継続は未検証。通知start1/2/3、停止後option4のID2/1/3帰属を取得。Inspect-MotifConcurrent.mjsがsources59/保存source/EXE/driver/input/PID/run/result/並列checkpoint/停止結果/通知/primary repeatだけの差分を独立照合passed。初回監査はrepeat0仮定で失敗、入力は実repeat1と確認し検査修正、同実行結果を再解析してpassed。製品再実行や結果書換えで埋めていない。

work/acceptance/motif-scheduling/20261003T170753398Z同版新規3072clock遅延primary flags0/submit96/request3168/actual3168、secondary flags17536/submit121/request3193/actual3193、両exit0/6音/natural end。work/acceptance/product-host/20261003T170728591Zhost exit0。並列module/host module点監査passed原版40hash一致0。Windows DirectMusic/DirectSound/GM.DLSは残る。無人録音/新GUI実操作/一次置換境界/実Tempo/Clipboard/JAZP/全40/全八は未完成。前版録音164008086ZとGUI164547643Zを現行版合格には転用しない。

再現：Build-ProductSnapshot.ps1、Test-MotifConcurrent.ps1 -BuildSummaryPath <同summary> -Style work/analysis/motif-dls-gui/20261003T161000Z/Heartlnd.stp。Node scripts/Inspect-MotifConcurrent.mjs <新run dir>。Inspect-ProductModules.ps1 -RunPath <run.json> -CaseName motif-concurrent。Test-MotifScheduling.ps1同summary/同Style、Test-ProductHost.ps1同summary/Inspect-ProductModules。通常承認済みWindows/OS DirectMusicを使用。人による音確認は要求しない。

次の具体的な一手：異なる音高/PChannelを持つ一次とsecondary入力で、開始・並列・個別Stop・再開・全停止のWASAPI無人録音を行い、APIがplayingでも音が失われる可能性を検査する。並列GUI表示と個別選択Stop、一次置換の予定境界/通知identity回収、保存境界/実Tempo、Clipboard/JAZP/全40/全八も継続。範囲・全体条件を縮小しない。凍結work/analysis/motif-concurrent/20261003T171100Z/unit-record.json。


## 2026-10-04 一次/secondary個別停止後の音声継続を無人録音

全体未完了。前回171100ZのConductor個別所有/API結果を保持。今回は本体CLI --motif-concurrent-audio <出力dir> <primaryStyle> <secondaryStyle> <Motif名>を追加し、Framework所有Style/collection解決から一次再生、secondary追加、secondary個別Stop、secondary再開、primary個別Stop、全Stopを実行。QPC操作時刻、生成音、通知、実source/runtime Style/DLS bytesを出力し録音と結合する。GUI操作の成功ではない。

Create-MotifConcurrentAudioFixture.mjsは保存Styleから異なるGUID・MIDI60/PChannel4(primary)とMIDI67/PChannel5(secondary)、repeat15の有限Motifを作る。両者の所有Motif Bandはpatch777(bank3/program9)、同PCM音源。元保存Style/DLSを変更せず別試験入力を作る。初版work/analysis/motif-concurrent-audio/fixture-20261003T171500Zは元DLSのRegion範囲へ60/67を含め忘れ、work/acceptance/audio-concurrent/20261003T171755585Zのcapture/player各exit0/API41音に対し独立sample監査がeligible Region0件で失敗。失敗WAV/ログ/入力を保持し成功扱いしない。

修正fixturework/analysis/motif-concurrent-audio/fixture-20261003T171900Zは選択bank3/program9の単一Regionを60..72へ明示変更、元DLSをsource.dlsとして保持しWave/WSMPを再利用。同製品EXEで新規録音work/acceptance/audio-concurrent/20261003T171910126Z。本体work/build/product-snapshot/20261003T171458542Z保存59sources、構成0/compile0/install0/warnings0、EXE 0b0486ab4a7fd35d006c01488896dd25d06e1aff8f193cc5fb48bf591c7f15d7、core f6a53d28a82270ceaa21788c5d73a09e9a6d1e0357581c82682efdc62012ac60。core suiteと旧CLI全suiteは今回未実行。録音器はソース製102354462Z EXE cf06449043e1b6cbea6e19b5ac8875e63f2f99d65e1614f167311cee6c855727、保存source2/summary/EXEを照合。同版を再ビルドしたとはしない。

新規24秒WASAPI default-render 48kHz/2ch/float32、playerPID19328/capturePID1100、両exit0、timestamp errors0/max packet gap2frames/peak0.07882051169872284。sourceStyleと入力一致、source/runtime DLS/入力owned.dls全bytes一致。独立保存Band patch/PChannel/DMRF GUID/Part channel/音高/velocity96/duration384/repeat15、生成音を検査、41音/overflowなし/forwarding failureなし。単一Region/cue0/mono PCM16/zero tuningに限りPCM dominant552.5Hz・unity85から期待周波数130.37263943915892/195.33824830278377Hzを算出。多楽器/多Wave/articulation一般の音声受入ではない。

録音基準時刻：primary ready3.2469362秒、both ready6.1903、secondary Stop return8.28814、both restarted11.2850221、primary Stop return13.4010083、all Stop return15.5359245。primary単独/両音/secondary停止後primaryのみ/再開後両音/primary停止後secondaryのみ/全停止後無音を独立intervalで検査。primary残存最大成分0.005521239621203593、secondary残存0.00798514350286892、baseline/final rms0。実音声継続の無人デジタル受入passed。物理スピーカー/人の聴取/GUI/原版音声比較は未確認。

初回解析の不在tone絶対閾値0.00008はPCM sideband/transientの0.000110..0.000145成分も拒否しpitch false。元proof/auditorを*-absolute-thresholdへ保持。停止側成分が残存toneの5%未満、両音各成分が最大の8%超かつ0.0003超、全無音rms0.0001未満とする相対判定へ変更。これは弱い残留音の完全不存在を証明せず、区間内の意図した音高成分の有無を限定判定する。録音は再実行せず同WAVを再解析。初回driver解析失敗exit1を最終driver成功へ書換えていない。最終Inspect-MotifConcurrentAudio exit0と独立proof passedを記録。

work/acceptance/audio-auditor-controls/20261003T172200Z-concurrentは未変更copy合格、全無音/secondary Stop後のprimary欠落/primary Stop後のsecondary欠落/both区間をprimary単音へ置換/開始前背景音の5派生を拒否。すべてAPI成功データを保持して音検査だけが拒否する。派生WAVで新製品録音ではない。新script/driver/source/EXE/input/packet/native/WAV/hashを証拠結合。work/acceptance/product-host/20261003T172249996Z同版host exit0、host/並列録音module監査passed原版40hash一致0の点観測。Windows DirectMusic/DirectSound/GM.DLS依存は残る。

再現：Create-MotifConcurrentAudioFixture.mjs work/analysis/motif-dls-gui/20261003T161000Z <新fixturedir>、Build-ProductSnapshot.ps1。Test-MotifConcurrentAudio.ps1 -BuildSummaryPath <同summary> -FixtureDirectory <新fixturedir> -Node <実Nodepath>、既存録音器summary102354462Z。Node scripts/Inspect-MotifConcurrentAudio.mjs <録音dir>、Test-MotifConcurrentAudioAuditor.mjs <録音dir> <新controldir>。Test-ProductHost/Inspect-ProductModules、録音は -CaseName audio-concurrent。人の応答や追加Windows設定変更は不要、通常の対話Windows/OS DirectMusicを使用。

次の具体的な一手：本体の並列再生を表示する一覧と、一次/secondaryを選択してStopするUIを実装し実操作する。一次置換の予定境界まで旧一次を継続、停止通知identityの長期回収も未実装。実保存境界/Tempo、Clipboard/JAZP/原版比較/全40/全八を継続。全体条件を縮小しない。証拠凍結work/analysis/motif-concurrent-audio/20261003T172500Z/unit-record.json。


## 2026-10-04 並列再生一覧と任意選択Stopの本体GUI

全体未完了。Conductorへ再生ID/表示名/primary-secondary種別/positionの値コピー一覧を追加し、本体Pattern > Playback Sessionsへmodeless一覧を接続。200ms更新・IDによる選択保持、Stop Selectedは選択IDだけを停止、Stop Allは全停止。空一覧で両ボタン無効。画面を閉じても再生を止めず、本体終了で所有windowを解放する構成。今回windowを閉じる時点では全停止済みで、再生中のwindow close/reopenは未試験。

本体work/build/product-snapshot/20261003T172842472Z保存61sources、構成/compile/install各0、EXE 137fdf030259df50eb532e6acbfc137576bfe9b2cebcc64e9db724d207b4ad5a。core生成物は未実行、旧成功を転用しない。work/acceptance/product-project-gui/20261003T173031057Z PID20220/通常終了0。同一project/primary.stp/secondary.stp/owned.dlsをhash結合。fixtureは既存保存Styleから異なるGUID/MIDI60/PChannel4とMIDI67/PChannel5・repeat127の有限Motifを作成、DLS key range60..72・patch777を保持。projectはfixture builderで生成した入力でありGUI新規作成の証明ではない。

Computer Useで一次/secondary各Saved boundary・prepare checked・delay0を再生。一覧にSecondary secondary.stp/Primary primary.stpの両Playingを確認。最新以外のPrimary行を選びStop SelectedするとPrimaryが消え、Secondary Playingのみ残る。Stop All後は一覧0件・両button disabled。選択行/secondary checkbox checkedは保存画像を直接確認し、UIAにselection/checked状態がないため独立auditorがそれらを機械分類したとはしない。停止直後UIAは旧行を返したため別ready観測を採用。Inspect-PlaybackSessionsGuiが保存sources/workspace/EXE/driver/入力/通常終了/PID/module/画像hash/状態時系列を監査passed。

初回再生は一覧観測までに終了して空になった。成功に数えず全状態を保持。main timerは最新だけの開始履歴を追跡し、既に終了した保持再生へ戻るとPlayback did not start within five secondsを表示した。一覧Stop All後の本体statusもStopped(segment ended)で、ユーザー停止との区別が欠ける。任意選択停止自体は確認済みだが、各IDの開始/終了履歴とmain表示連携は次に修正する。

同版work/acceptance/audio-concurrent/20261003T174149244Zは別CLI入力repeat15の新規24秒WASAPI録音、playerPID5396/capturePID14576、両exit0、解析passed。期待DLS成分130.37263943915892/195.33824830278377Hzについて単独/両音/secondary停止後primary継続/secondary再開/primary停止後secondary継続/全停止無音を確認。保存Style、runtime Style/DLS、生成音、WAV/packet/QPCを結合。同一監査アルゴリズムの前回6派生対照172200Zは別unitの証拠として保持し、今回の新WAV対照を再実行したとはしない。GUI音声録音/物理speaker/原版同時GUI比較は未確認。

work/acceptance/product-host/20261003T174216353Z同版本体host exit0。host/audio/GUI75modules由来点監査passed、原版40hash一致0。Windows DirectMusic/DirectSound/GM.DLS依存は残り、全40責務/全八受入は未完成。

再現：Build-ProductSnapshot、Create-MotifConcurrentAudioFixture.mjs <保存Style dir> <新dir> 127とproject作成、Test-ProductProjectGuiへ同summary/project/Style/DLS、上記GUI操作・状態画像・module取得・通常終了。Inspect-PlaybackSessionsGui.mjs <GUI dir> <同版host run>、Inspect-ProductGuiModules。音はTest-MotifConcurrentAudio.ps1同summary/既存repeat15 fixtureとソース製録音器102354462Z。人の聴取や追加OS設定は不要。

次の一手：再生ごとの開始履歴/予定clockに基づくtimer監視と一覧停止のmain表示連携を実装・検証。その後一次置換予定境界まで旧一次継続、停止通知identity回収、実保存境界/Tempo、Clipboard/JAZP/原版比較/全40/全八を継続。全体条件は縮小しない。凍結work/analysis/playback-sessions-gui/20261003T174500Z/unit-record.json。

# AudioPath Send の限定本体受入（全体未完）

現候補 `20261008T030413240Z` は、録音用Waves DMOを最後の共有再生のStop後にFlushする修正を採用。218保存/作業ソースと4生成物を照合し、configure/build/install各0、関連8試験合格。原始native89は52合格/37個別凍結/失敗0、driver152は24合格/23障害/105未実行。原始一巡と補足を分け、旧候補の失敗を保持する。

[限定終了proof](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T030413240Z/completion-20261008T0525/proof.json)は、承認済みnative Project保存、author22692通常exit0、別PID3472のexact復元/通常exit0、full native監査/controls1正例9反例、4条件の本体・音声・通常終了を結合する。保存でProjectのSegment FILETIMEだけが変化し、7文書のbytesは不変。過去の二条件checkpointと失敗試行を保持した。

[実4条件の全結合](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T030413240Z/main-bindings-proof.json)はCLI PID9708通常exit0。同じ218保存/作業ソース、4生成物、native入力、GUI File Open、7操作、各300秒WASAPI/3 FileOutput、通常終了を照合した。採用processはauthor22692、dry-zero24264、dry-minus60013632、Waves前10916、Waves後16660の5件。手順失敗の3472/15272を含む実通常終了は7件で、採用5件と区別する。

二Playともdry-zeroのSend比1.0、dry-minus600約0.5066、Waves前約0.5066、Waves後約0.08711。効果前後差は約0.828で、別routeの変化は最大約0.0000037。全4条件の各二Stop後、endpointと3 FileOutputはRMS0。発音中Stop、停止保持、再開、音程/混入/ピーク、終了後無音、packet/時計が固定基準で合格した。runtimeテンポと物理スピーカーは測定していない。

[独立数値CLI](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T030413240Z/completion-20261008T0525/numeric-all-proof.json)はPID23640通常exit0。数値controls4/18、結合controls7/25、GUI ProjectOpen controls1/9も現入力/契約で合格。対照driverの旧固定観測名によるENOENT（PID23928 exit1）を保存し、任意のhash付き実観測indexを受け取る変更後にPID18700 exit0。製品ソース・閾値・受入assertは変更していない。

次は030413候補でQ1のnative五形式/10文書、編集/UndoRedo、通常終了/別PID復元、embedded/default優先とテンポ音声を統合する。外部Other/共有寿命/既定Environmental Reverb/原版比較/Q2/全40/全8を保持し、`fullAcceptance=false`。以下の二条件段階・失敗・旧候補は履歴であり、現在の未実行条件ではない。

## 同候補の二条件段階と手順失敗履歴

[Waves前の実PCM数値結果](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T030413240Z/cases/send-before-waves-minus600-r1/numeric-proof.json)はPID10916の通常exit0、300秒WASAPIの通常exit0、実7操作と3 FileOutputに結合。2回の発音/発音中Stop/再開、音程・混入・gain、packet/時計校正を固定条件で合格した。停止後endpointと3本の出力は二窓ともRMS0、Send音量比は約0.5066。数値CLI PID20060はexit0。runtimeテンポの測定・原版比較・全4条件の本体結合合格は主張しない。

[dry-zero再測定](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T030413240Z/cases/dry-zero-r1/numeric-proof.json)は本体PID24264、数値CLI PID22108とも通常exit0。二Playは26.893秒/16.044秒で固定4〜40秒内、Send比は両Playとも1.0。endpoint/3 FileOutputの二停止窓、baseline/停止保持/終了後はRMS0、packet/clockと音程・混入・ピークも合格。現候補で二条件合格、残dry-minus600/Waves後と全結合は未実行。actual通常exit0はauthor・失敗手順を含め5件で、最終受入の五プロセス成立とは数えない。

[初回dry-zeroの手順失敗](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T030413240Z/cases/dry-zero/procedure-failure.json)は本体PID15272通常exit0、数値CLI PID23952 exit1。最初のPlay42.271秒が上限40秒を超え、Close後の録音も3秒未満だった。原始操作・録音・不合格を保存し、新しいPID/同じbytes/同じ閾値で再測定した。

前回の同候補録音は再開の42.229秒が上限40秒を超え、初回Playing観測のaccessibility textも欠けたため合格採用しない。録音・操作・数値失敗・残音診断RMS0を保持し、今回の新しい入力コピー/別processで再録音した。数値閾値と判定器を変更していない。

この段階では残条件の別PID録音と四条件全結合を次作業として記録した。現在は冒頭の限定終了proofに従う。原版比較、外部Other/共有寿命/既定Environmental Reverb、Q2、全40/全8は未達で、`fullAcceptance=false`。

## 旧候補000858585Zの採用・失敗履歴
候補 `20261008T000858585Z` に実Send class、4バイト減衰、private runtimeの宛先先行順、global mix-in取得、挿入/音量UIを採用した。本体で追加/−600/UndoRedo、AudioPath 2ファイルと埋込Segmentのnative保存を確認した。承認済みnative Project保存、author通常exit0、別PID exact復元と通常exit0を確認した。dry-zeroの実PCMは限定合格、dry-minus600録音はpacket欠落で失敗保存。残2条件/録り直し/四条件全結合は未達で、`fullAcceptance=false`。

[単位記録](../../work/analysis/q3-audiopath-send/20261007T094855774Z/unit-record.json) が現在の入口。[10月7日の報告](q3-audiopath-send-2026-10-07.md) は初期ソース作業の履歴であり、現在の実行判定ではない。

## 候補と原始回帰

[build-summary](../../work/build/product-snapshot/20261008T000858585Z/build-summary.json) の218保存/作業ソースと4生成物を照合。configure/build/installは各0。SHA-256 `36ec194c6061563e758d8095d535338c1291ef9723318c648444c5d6db78fc66`、install Producerは `9814bbea954262c83cd4a825bce50a0a45412c090a8d210ad97f3b51ac5ce2f7`。

[原始native89一巡](../../work/acceptance/regression/20261008T001333190Z/run.json) は52合格/36凍結/1新規保存拒否。Send authoringはPID23132・43チェック、runtimeはPID13384・49チェック、いずれも通常exit0。Source.proのreplace Windows error5を[独立ゲート](../../work/send-repair-20261007T2250/adoption-20261008T0010/audiopath-source-project-save-refusal.json)として保存し、原始失敗1を削除していない。製品assert失敗と同一視しない。

[原始driver144一巡](../../work/acceptance/registered-drivers/20261008T001648453Z/run.json) は24合格/23障害/97前提不足未実行。独立保存監査2、音声の数値判定器/controls2、本体との結合監査/controls2を追加して現登録はnative89/driver152（native入力準備とGUI ProjectOpen controlsを追加）。補足を原始一巡の件数へ加算しない。

## 実本体と保存文書

[本体進捗proof](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T000858585Z/checkpoint-20261008T004936501Z/main-author-progress.json) のPID20668は、上記installと開始時刻 `2026-10-08T00:18:38.3310237Z` に結合。原入力5ファイルをbefore/authorへコピーし、原入力とbeforeをhash付きで保持した。

Computer Useで、PChannelのないstereo mix-in Buffer3、FileOutput、送信元FileOutputの後段の実Sendを追加した。0の保存後に−600を設定し、Undoで0、Redoで−600を実表示で確認。Segmentへコピーして保存した。

[独立保存文書監査](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T000858585Z/documents-audit-20261008T004621455Z/evidence/native-proof.json) はNode PID18888・通常exit0。0と−600のAudioPathは減衰4バイトだけが異なり、保存順/宛先GUID/既存FileOutput/port/PChannel/未知データを保持する。Segmentの埋込DMAPは−600のAudioPathとbyte一致し、非AP chunk、2音符、60BPMを保持する。2音符のduration122880 clocksはこの入力の160秒。これはruntimeテンポのPCM測定ではない。

[監査controls](../../work/analysis/q3-audiopath-send/20261007T094855774Z/native-controls-20261008T004726462Z/evidence/controls.json) はPID5032・通常exit0。正例1/反例7で減衰、class、宛先、元buffer、音符、埋込AP欠落、chunk overrunを検査。初回監査はseqt内のevtlを探索しない監査コードの誤りで失敗し、[初回run](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T000858585Z/documents-audit-20261008T004511938Z/run.json) とコードを保持した。公開SDKのコンテナーに従って修正し、製品/入力/閾値は変更していない。

## 実測前の音声判定器

[固定protocol](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T000858585Z/main-audio-protocol-v1.json) と[数値監査範囲](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T000858585Z/audio-auditor-preparation-20261008T010226993Z/scope.json) に従い、2回の発音中Stop/無音/再開、3つのFileOutputの排他的音程と同期、0と−600の送信音量比、Wavesの前後に置く効果順序を判定する層を用意した。減衰の期待値と許容幅は元protocolのまま。WASAPIはUTC/QPC/packet校正を使用し、FileOutputは各WAVのフレーム原点と発音間隔を照合する。FileOutputの絶対UTC開始時刻や、この持続音入力のruntimeテンポを測定したとはしない。

[準備proof](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T000858585Z/audio-auditor-preparation-20261008T010226993Z/preparation-proof.json) はcontrols PID15400、数値CLI PID17620、いずれも通常exit0。人工PCMの正例4/反例18で、音量・音程・混入・停止後発音・早期終了・同期ずれ・clipping・40秒期限・効果順序・control変動・packet欠落/時刻エラー・時計drift・WAVE越境を検査した。人工入力/PCM/packet/action/codeも保存した。人工controlsは実製品受入と別。実製品ではdry-zeroの300秒WASAPI/3 FileOutputと二度の発音中Stop/無音/再開を数値確認。dry-minus600の300秒録音は非startup discontinuityでCLI exit1となり失敗保存した。四条件全結合は未実行。

[本体結合監査の準備proof](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T000858585Z/binding-auditor-preparation-20261008T011518896Z/preparation-proof.json) はPID4548・通常exit0、正例7/反例26。独立native RIFFのSend class/order/4B減衰/宛先、元APから許容差分だけの復元、長い音符・Band/DLS・Projectの4文書GUID/sizeを検査する。PID/start/image/hash、通常Close/exit0、window/状態/新鮮な観測/画像寸法/依存hashの反例も確認した。実際の全結合CLIは未実行で、派生native fixtureと人工のlifecycle記録を通常GUI終了の証拠に数えない。

初回PID16560は保存画像名の指定誤り、2回目PID4860は既存 `.png` ファイルの実データがJPEGだったため失敗した。両方のコード/run/logを保持し、実データ形式によるJPEG/PNGの寸法照合へ修正。scope初版のcatalog tree想定も、既存の実本体観測に基づき選択中Segmentコンボと独立native catalog監査へ修正し、3版を保持した。製品ソース、入力、数値閾値は変更していない。

## 保存確認と残条件

誤ったProject名はname/folder制約で書込前に拒否。新規parent-folder保存も既存のnative JAZP runtime-path relocation未対応で拒否した。両方の画面/時刻/対象を保持し、SDK拒否やOS5と混同しない。

検証用 `author/MultiCapture/MultiCapture.pro` の上書きをユーザーが明示承認。実GUI保存後のSHA-256は `ab35945c1585fb5d990aa0e3ec96f45d1bc4e0780ebb5688b7b19b84ddf2b9fe`。author PID20668は通常exit0。full native監査（PID23924 exit0）とfull controls正例1/反例9（PID14268 exit0）でProject catalog/metadata、AP2、埋込Segment、元入力不変を確認した。

別PID14192で正確なauthor ProjectをGUI File Openから復元し、二音符/60 BPMを確認。同じprocessでdry-zero派生入力を復元・録音し通常exit0。最初のCLI起動はprocessが応答してもComputer Useにwindowが出ず、未操作の履歴として保持。skyの復旧手順で新規processを選び、架空の起動引数は記録していない。scope-v4は実File Openの入力時刻/path/画像を監査し、ProjectOpen controls正例1/反例9（PID20720 exit0）を実行した。旧v3 controls7/26は旧コードの履歴で、現v4全controlsと四条件結合は検証待ち。

[承認後の進捗](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T000858585Z/checkpoint-after-approval-20261008T023422561Z/main-progress.json) に保存承認、full native、exact復元、通常終了、実録音と判定器のhashを結合。[dry-zero数値結果](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T000858585Z/actual-case-preparation-20261008T014524065Z/dry-zero-numeric-diagnostic.json) はunity音量比、排他的音程、Stop後無音/再開、packet/時計を限定合格。[dry-minus600失敗](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T000858585Z/actual-case-preparation-20261008T014524065Z/dry-minus600-packet-failure.json) は非startup discontinuityを保持。原因は未特定で、閾値を変えず新規入力ディレクトリ/別processで録り直す。

現v4の本体結合controlsは実CLI PID10596・通常exit0、正例7/反例25で限定合格。旧v3の追加反例「actual author lifecycle未完」は、承認後のauthor通常終了によって現在は該当しない。旧結果とコードは保持し、四条件の実本体受入とは区別する。

3条件目 `send-before-waves-minus600` は実本体PID16604通常exit0、300秒WASAPIと3 FileOutputを保存。packet/時計は合格したが、固定のStop後+.7〜1.7秒のendpoint RMSは1回目 `0.0002445485`、再開後 `0.0002515655` で上限 `0.0001` を超え不合格。3 FileOutputは同じ停止窓でRMS0。[数値不合格](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T000858585Z/actual-case-preparation-20261008T014524065Z/send-before-waves-minus600-numeric-diagnostic.json) と [出力別診断](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T000858585Z/actual-case-preparation-20261008T014524065Z/send-before-waves-minus600-stop-failure-diagnostic.json) を保持した。初回録音器の270文字ready pathは未生成で、通常終了のexit codeは取得不能。失敗を保持し、短いディレクトリへのlaunch完全同bytesコピーによる次の録音は正常完了した。

[Stop修正開始契約](../../work/analysis/q3-audiopath-send/20261007T094855774Z/main-20261008T000858585Z/stop-residue-repair-20261008T030038152Z/repair-start.json) に従い、録音中に保持するWaves DMOの内部データを最後の共有再生のStop後にFlushする変更を準備。DMOパラメータ、native bytes、継続中の別再生、FileOutputの継続は保護する。SDK拒否による制限付きbuild失敗 `20261008T030329138Z` を保持し、既存承認の通常ホストbuild `20261008T030413240Z` を実行中。これはbuild開始時点の履歴で、現在の判定は冒頭の030413候補と一次進捗に従う。

当時の次作業としてStop修正版build/installと関連回帰を記録した。現在は冒頭の現候補・四条件限定終了/Q1統合へ更新。固定閾値を維持し、旧候補の音声成功を新生成物へ転用しない。旧承認待ち監査は解決履歴に移した。

外部Other宛先の共有寿命、既定Environmental Reverb、原版papd grouping/orderと同入力動的比較は残責務。各OS5、原版approval timeout、独立Windows未用意/Q2を別障害として保持する。全40責務と全8受入の同一最終構成合格は未達。

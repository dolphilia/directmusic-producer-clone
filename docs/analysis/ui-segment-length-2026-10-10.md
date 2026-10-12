# Segment PropertiesからのLength再編集（限定統合）

2026-10-10（日本時間）。画像44→46→47のSegmentツリーProperties→Lengthを接続した。最終試験候補は20261010T140539591Z。既存Segmentの20小節/40秒を24小節/48秒へ変更し、Cancel、未変更OK、Undo/Redo、native保存、通常終了、別processでの復元と再通常終了を確認した。限定採用0735を更新するものではなく、原版との同入力比較・厳密外観比較は未確認。completed=false、fullAcceptance=falseを維持する。

[一次単位](../../work/analysis/ui-segment-length/20261010T134932507Z/unit-record-v1.json)、[build](../../work/build/product-snapshot/20261010T140539591Z/build-summary.json)、[258ソース/4出力同一性](../../work/analysis/ui-segment-length/20261010T134932507Z/closing-build-identity-v1.json)、[変更12ファイル](../../work/analysis/ui-segment-length/20261010T134932507Z/source-changes-v3.json)、[作業状態](../../work/analysis/ui-segment-length/20261010T134932507Z/working-state-v2.json)を照合する。保存ソースと作業木、生成物はすべてhash一致。configure/build/installは各exit0。Producer.exe SHA256は82085e0982b5524e9e0490f24d0fc28ad0d2d3c65d7d3061b8a1c28964c11140。

既存文書の拍子・テンポを使って小節/clocktimeを換算し、異なる拍子・テンポを跨ぐ位置も計算する。計算用の拍子/テンポはgroup1を使い、選択中の編集groupから独立する。新規作成の4/4・120既定値とは区別する。プレビューとCancel/未変更OKは文書・dirty・履歴を変更しない。長さ変更はSegment全体の一つのUndo/Redoへ結び付け、既存nativeヘッダー、未知データ、track bytesを保持する。ツリーでファイル自身を選んだ場合にもPropertiesを開けるよう修正した。

[関連回帰](../../work/acceptance/regression/20261010T141302941Z/run.json)は7モード553確認が合格した。timeline-range117、mute-document99、sequence-crud26、component-name70、tempo-properties19、meter-properties65、segment-creation157。既知のStyle/Band障害2モードはprocessを起動せず障害を保持した。[実行時manifest](../../work/analysis/ui-segment-length/20261010T134932507Z/manifest-executed-v1.json)を固定し、コマンド・判定器・PID・exitを一次runへ結んだ。登録native97/driver170の同候補全一巡は未実行であり、旧候補の結果を転用しない。

[本体証拠](../../work/analysis/ui-segment-length/20261010T134932507Z/gui-save-reload-proof-v1.json)に41観測と操作を保存した。作者PID19824と別復元PID24440は両方通常exit0。保存はSegment→Projectの順で実施した。先行するProject保存は未保存Segmentのエラーで止まり、終了確認ではNoを選んで保存後に通常終了した。未保存データの破棄は行っていない。復元後のPropertiesは24 Bars、Lengthは24小節/48秒で、読込後に保存ファイルhashの変化はない。

[復元後Length表示](../../work/analysis/ui-segment-length/20261010T134932507Z/gui/039-reload-length-24-48-1.png)。[独立RIFF監査](../../work/analysis/ui-segment-length/20261010T134932507Z/gui-native-audit-v2.json)ではSeed.sgpの2734bytesのうちsegh.mtLengthのみ61440→73728へ変化し、他の全bytes、4track、未知領域とflagsは一致した。元Band検証fixtureと正式インストール原版には変更していない。

135324はbuild成功後にファイルPropertiesが無効と判明した。140343はfollow修正が不足しており実行していない。最終140539で両方を修正し、保存/作業ソース一致を再確認した。UIA要素167がcached stateから消えた一回のクリック失敗は保存し、新規観測後のCancelで復帰した。これらをOS拒否や原版不具合とは判定しない。

短縮は既知TimeSig/Tempo/Bandの末尾とStart/Loopが範囲内で、他種別が既知の空trackの場合に限定して許可する。内容のあるSequence/未知trackや、末尾削除・切詰めが必要なケースは保守的に拒否する。[CHMのSegment tab](../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/segmenttab.htm)で示される範囲外データの削除は未実装であり、この拒否を原版同等の完成としない。ヘッダー上のmusic/ref timeの扱いは[MicrosoftのSegment Header資料](https://learn.microsoft.com/da-dk/previous-versions/ms808136(v=msdn.10))も参照した。

残件は、各trackの末尾削除/切詰め、Extension/pickup、可変TimeSig group、Segment PropertiesのStart/Loop/Info/Boundary等、clocktime/loop再生の実効性、原版同入力/200%表示での外観・操作比較。音声は今回の長さ編集の範囲外であり未検証とする。SSP01のMIDI/録音等、Band音色・pan/volume、独立Style015244の固定300秒PCMと同候補一巡、全40/全8、独立Windows/Q2と凍結拒否は未完のまま保持する。

次はSSP03画像13〜16・21〜24のSequence開閉/鍵盤/挿入カーソル/ノート/楽器。CHMの[契約準備](../../work/analysis/ui-segment-length/20261010T134932507Z/next-sequence-contract-read-v2.json)は、一つの物理Sequence trackが一つのpart/PChannel/groupを持ち、同groupのtrackを一緒に表示することを確認した。複数partを一物理trackへ独自に詰め込まない。次単位の開始記録を作成してからソース変更し、青/赤ノート、Insert/右メニュー、試聴とBand割当、native保存/別process復元へ接続する。
